#include <rack.hpp>
#include <cstdio>
#include <cmath>
#include <vector>
#include "../src/dsp/Voice.hpp"

using namespace rack;
using namespace m32;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail) {
	printf("%s %-42s %s\n", ok ? "  ok " : "FAIL", name, detail);
	if (!ok)
		failures++;
}

static void checkNear(const char* name, float actual, float expected, float tolerance) {
	char detail[128];
	snprintf(detail, sizeof(detail), "got %.4f, expected %.4f +/- %.4f", actual, expected, tolerance);
	check(name, std::fabs(actual - expected) <= tolerance, detail);
}

static const float SR = 48000.f;
static const float DT = 1.f / SR;

/** Counts rising zero crossings to recover a waveform's frequency. */
static float measureFrequency(Oscillator& osc, float seconds) {
	int crossings = 0;
	float previous = osc.saw;
	const int frames = int(seconds * SR);
	for (int i = 0; i < frames; i++) {
		osc.process(DT);
		if (previous < 0.f && osc.saw >= 0.f)
			crossings++;
		previous = osc.saw;
	}
	return crossings / seconds;
}

static float rms(const std::vector<float>& xs) {
	double sum = 0.0;
	for (float x : xs)
		sum += double(x) * double(x);
	return float(std::sqrt(sum / xs.size()));
}

static void testOscillatorPitch() {
	Oscillator osc;
	osc.freq = 440.f;
	checkNear("oscillator saw tracks 440 Hz", measureFrequency(osc, 1.f), 440.f, 1.f);

	osc.freq = 55.f;
	checkNear("oscillator saw tracks 55 Hz", measureFrequency(osc, 1.f), 55.f, 1.f);
}

static void testPulseWidthExtremes() {
	Oscillator osc;
	osc.freq = 220.f;
	osc.pulseWidth = 0.f;

	float minimum = 10.f, maximum = -10.f;
	for (int i = 0; i < 4800; i++) {
		osc.process(DT);
		minimum = std::min(minimum, osc.pulse);
		maximum = std::max(maximum, osc.pulse);
	}
	check("pulse at 0% width is silent", maximum - minimum < 0.01f, "no transitions");

	osc.pulseWidth = 0.5f;
	double sum = 0.0;
	for (int i = 0; i < 48000; i++) {
		osc.process(DT);
		sum += osc.pulse;
	}
	checkNear("pulse at 50% width is DC free", float(sum / 48000.0), 0.f, 0.02f);
}

/** Small enough that the saturating input stage stays linear. */
static const float SMALL_SIGNAL = 0.05f;
static const float SMALL_SIGNAL_RMS = SMALL_SIGNAL / 1.41421f;

/** A 4-pole low pass should lose about 24 dB per octave above cutoff. */
static void testLadderRolloff() {
	auto attenuationAt = [](float cutoff, float toneHz) {
		LadderFilter filter;
		filter.setCutoff(cutoff, SR);
		filter.setResonance(0.f);
		std::vector<float> out;
		const int frames = int(SR * 0.5f);
		for (int i = 0; i < frames; i++) {
			const float in = SMALL_SIGNAL * std::sin(2.f * float(M_PI) * toneHz * i * DT);
			filter.process(in);
			if (i > frames / 2)
				out.push_back(filter.lowPass);
		}
		return rms(out);
	};

	const float atCutoff = attenuationAt(500.f, 500.f);
	const float oneOctaveUp = attenuationAt(500.f, 1000.f);
	const float twoOctavesUp = attenuationAt(500.f, 2000.f);

	const float dbPerOctave1 = 20.f * std::log10(oneOctaveUp / atCutoff);
	const float dbPerOctave2 = 20.f * std::log10(twoOctavesUp / oneOctaveUp);

	char detail[128];
	snprintf(detail, sizeof(detail), "%.1f dB then %.1f dB per octave", dbPerOctave1, dbPerOctave2);
	check("ladder low pass rolls off near -24 dB/oct", dbPerOctave2 < -21.f && dbPerOctave2 > -27.f, detail);
}

/** High pass mode must reject DC and pass content above cutoff. */
static void testHighPassMode() {
	LadderFilter filter;
	filter.setCutoff(1000.f, SR);
	filter.setResonance(0.f);

	std::vector<float> dcOut, toneOut;
	for (int i = 0; i < 24000; i++) {
		filter.process(SMALL_SIGNAL);
		if (i > 12000)
			dcOut.push_back(filter.highPass);
	}
	filter.reset();
	for (int i = 0; i < 24000; i++) {
		filter.process(SMALL_SIGNAL * std::sin(2.f * float(M_PI) * 8000.f * i * DT));
		if (i > 12000)
			toneOut.push_back(filter.highPass);
	}

	check("high pass rejects DC", rms(dcOut) < 0.001f, "DC blocked");
	checkNear("high pass is flat at 8x cutoff", rms(toneOut) / SMALL_SIGNAL_RMS, 1.f, 0.1f);
}

/** The input stage should pass small signals untouched and compress large ones. */
static void testInputSaturation() {
	auto gainAt = [](float amplitude) {
		LadderFilter filter;
		filter.setCutoff(18000.f, SR);
		filter.setResonance(0.f);
		std::vector<float> out;
		for (int i = 0; i < 24000; i++) {
			filter.process(amplitude * std::sin(2.f * float(M_PI) * 100.f * i * DT));
			if (i > 12000)
				out.push_back(filter.lowPass);
		}
		return rms(out) / (amplitude / 1.41421f);
	};

	checkNear("small signals pass at unity gain", gainAt(0.05f), 1.f, 0.05f);
	check("gain falls as level rises", gainAt(3.f) < gainAt(1.f) && gainAt(1.f) < gainAt(0.05f), "compressive");
	char detail[96];
	snprintf(detail, sizeof(detail), "x1 %.2f, x3 %.2f, x8 %.2f", gainAt(1.f), gainAt(3.f), gainAt(8.f));
	check("very loud signals compress hard", gainAt(8.f) < 0.8f, detail);
}

static void testSelfOscillation() {
	LadderFilter filter;
	filter.setCutoff(440.f, SR);
	filter.setResonance(LadderFilter::SELF_OSC_K + 0.3f);

	for (int i = 0; i < 2000; i++)
		filter.process(i < 10 ? 0.5f : 0.f);

	std::vector<float> out;
	float peak = 0.f;
	for (int i = 0; i < 48000; i++) {
		filter.process(0.f);
		out.push_back(filter.lowPass);
		peak = std::max(peak, std::fabs(filter.lowPass));
	}
	char detail[96];
	snprintf(detail, sizeof(detail), "rms %.3f, peak %.3f", rms(out), peak);
	// The manual calls a self-oscillating filter a sound source, and puts the VCF jack near +/-5 V.
	check("self-oscillation reaches a usable level", rms(out) > 0.3f && peak < 1.2f, detail);
}

static void testEnvelopeTiming() {
	Envelope env;
	env.attackTime = 0.1f;
	env.decayTime = 0.2f;
	env.sustainEnabled = true;
	env.setGate(true);

	int frames = 0;
	while (env.value < 0.99f && frames < int(SR)) {
		env.process(DT);
		frames++;
	}
	checkNear("attack reaches full in the stated time", frames * DT, 0.1f, 0.02f);

	for (int i = 0; i < 4800; i++)
		env.process(DT);
	checkNear("sustain holds at full while gate is high", env.value, 1.f, 0.001f);

	env.setGate(false);
	frames = 0;
	while (env.value > 0.01f && frames < int(SR)) {
		env.process(DT);
		frames++;
	}
	checkNear("decay falls to 1% in the stated time", frames * DT, 0.2f, 0.03f);
}

static void testSustainOffRetriggers() {
	Envelope env;
	env.attackTime = 0.001f;
	env.decayTime = 0.05f;
	env.sustainEnabled = false;
	env.setGate(true);

	for (int i = 0; i < 480; i++)
		env.process(DT);
	check("sustain off falls into decay while gate is held", env.value < 0.999f, "decaying");

	const float beforeRetrigger = env.value;
	env.setGate(false);
	env.setGate(true);
	for (int i = 0; i < 96; i++)
		env.process(DT);
	check("a new gate retriggers when sustain is off", env.value > beforeRetrigger, "rose again");
}

static void testVoiceProducesAudio() {
	Voice voice;
	voice.cutoffKnob = 1.f;
	voice.volumeKnob = 1.f;
	voice.vcaModeIsOn = true;
	voice.notePitch = 0.f;

	std::vector<float> out;
	for (int i = 0; i < 24000; i++) {
		voice.process(DT, SR);
		if (i > 12000)
			out.push_back(voice.vcaOut);
	}
	const float level = rms(out);
	char detail[128];
	snprintf(detail, sizeof(detail), "rms %.3f V", level);
	check("voice outputs audio with VCA on", level > 0.5f && level < 12.f, detail);
}

static void testVoicePitchTracking() {
	auto pitchOf = [](float volts) {
		Voice voice;
		voice.notePitch = volts;
		voice.vcaModeIsOn = true;
		for (int i = 0; i < 480; i++)
			voice.process(DT, SR);

		int crossings = 0;
		float previous = voice.sawOut;
		for (int i = 0; i < int(SR); i++) {
			voice.process(DT, SR);
			if (previous < 0.f && voice.sawOut >= 0.f)
				crossings++;
			previous = voice.sawOut;
		}
		return float(crossings);
	};

	checkNear("0 V plays middle C", pitchOf(0.f), 261.6f, 2.f);
	checkNear("1 V plays one octave up", pitchOf(1.f), 523.3f, 3.f);
	checkNear("-1 V plays one octave down", pitchOf(-1.f), 130.8f, 2.f);
}

/** p47-50: with a knob centred, -5 V to +5 V must sweep that control end to end.
So a centred knob plus 5 V has to land in the same place as the knob turned fully up. */
static void testCvReachesTheSameAsTheKnob() {
	auto filterLevel = [](float resonanceKnob, float resonanceCv) {
		Voice voice;
		voice.vcaModeIsOn = true;
		voice.cutoffKnob = 0.55f;
		voice.resonanceKnob = resonanceKnob;
		voice.resonanceCv = resonanceCv;
		std::vector<float> out;
		for (int i = 0; i < int(SR * 0.5f); i++) {
			voice.process(DT, SR);
			if (i > int(SR * 0.25f))
				out.push_back(voice.vcfOut);
		}
		return rms(out);
	};

	const float knobFull = filterLevel(1.f, 0.f);
	const float centrePlusFive = filterLevel(0.5f, 5.f);
	const float knobZero = filterLevel(0.f, 0.f);
	const float centreMinusFive = filterLevel(0.5f, -5.f);

	checkNear("centred resonance plus 5 V equals resonance fully up", centrePlusFive, knobFull, knobFull * 0.02f);
	checkNear("centred resonance minus 5 V equals resonance fully down", centreMinusFive, knobZero, knobZero * 0.02f);

	auto mixLevel = [](float mixKnob, float mixCv) {
		Voice voice;
		voice.vcaModeIsOn = true;
		voice.cutoffKnob = 1.f;
		voice.mixKnob = mixKnob;
		voice.mixCv = mixCv;
		voice.extAudioConnected = true;
		voice.extAudio = 5.f; // steady DC, so the ext share shows up as a DC offset
		double sum = 0.0;
		const int frames = int(SR * 0.3f);
		for (int i = 0; i < frames; i++) {
			voice.process(DT, SR);
			sum += voice.vcfOut;
		}
		return float(sum / frames);
	};

	checkNear("centred mix plus 5 V equals mix fully up", mixLevel(0.5f, 5.f), mixLevel(1.f, 0.f), 0.05f);
	checkNear("centred mix minus 5 V equals mix fully down", mixLevel(0.5f, -5.f), mixLevel(0.f, 0.f), 0.05f);
	check("and those two ends are actually different",
		std::fabs(mixLevel(1.f, 0.f) - mixLevel(0.f, 0.f)) > 1.f, "mix does something");
}

/** p47: the VCA CV jack sums with the ON/EG switch, so patching 0 V must change nothing. */
static void testVcaCvSums() {
	auto level = [](bool connected, float cv) {
		Voice voice;
		voice.vcaModeIsOn = true;
		voice.volumeKnob = 1.f;
		voice.vcaCvConnected = connected;
		voice.vcaCv = cv;
		std::vector<float> out;
		for (int i = 0; i < int(SR * 0.3f); i++) {
			voice.process(DT, SR);
			if (i > int(SR * 0.15f))
				out.push_back(voice.vcaOut);
		}
		return rms(out);
	};

	const float unpatched = level(false, 0.f);
	checkNear("a patched 0 V leaves the level alone", level(true, 0.f), unpatched, unpatched * 0.02f);
	check("negative CV closes the VCA in ON mode", level(true, -5.f) < unpatched * 0.02f, "silenced");
}

/** p53: the EG jack swings 0 to +7.5 V. */
/** Resonance at maximum must stay bounded rather than running away. */
static void testFilterStaysBounded() {
	Voice voice;
	voice.vcaModeIsOn = true;
	voice.resonanceKnob = 1.f;
	voice.volumeKnob = 1.f;

	float peak = 0.f;
	for (int sweep = 0; sweep < 20; sweep++) {
		voice.cutoffKnob = sweep / 19.f;
		for (int i = 0; i < int(SR * 0.1f); i++) {
			voice.process(DT, SR);
			peak = std::max(peak, std::fabs(voice.vcaOut));
		}
	}

	char detail[96];
	snprintf(detail, sizeof(detail), "peak %.2f V across a full cutoff sweep", peak);
	check("full resonance stays bounded at every cutoff", std::isfinite(peak) && peak < 30.f, detail);
}

static void testEnvelopeOutputRange() {
	Voice voice;
	voice.sustainIsOn = true;
	voice.attackKnob = 0.f;
	voice.gate = true;
	for (int i = 0; i < int(SR * 0.2f); i++)
		voice.process(DT, SR);

	checkNear("the EG output tops out at 7.5 V", voice.egOut, 7.5f, 0.05f);
}

int main() {
	random::init();

	printf("\nM32 DSP checks at %.0f Hz\n\n", SR);
	testOscillatorPitch();
	testPulseWidthExtremes();
	testLadderRolloff();
	testHighPassMode();
	testInputSaturation();
	testSelfOscillation();
	testEnvelopeTiming();
	testSustainOffRetriggers();
	testVoiceProducesAudio();
	testVoicePitchTracking();
	testCvReachesTheSameAsTheKnob();
	testVcaCvSums();
	testFilterStaysBounded();
	testEnvelopeOutputRange();

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
