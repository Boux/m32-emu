#include <rack.hpp>
#include <cstdio>
#include <cmath>
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
	check("loud signals compress", gainAt(3.f) < 0.6f, "saturated");
}

static void testSelfOscillation() {
	LadderFilter filter;
	filter.setCutoff(440.f, SR);
	filter.setResonance(LadderFilter::SELF_OSC_K + 0.3f);

	for (int i = 0; i < 2000; i++)
		filter.process(i < 10 ? 0.5f : 0.f);

	std::vector<float> out;
	for (int i = 0; i < 48000; i++) {
		filter.process(0.f);
		out.push_back(filter.lowPass);
	}
	check("ladder self-oscillates past k=4", rms(out) > 0.05f, "sustained output");
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

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
