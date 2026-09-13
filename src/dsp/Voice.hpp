#pragma once
#include <rack.hpp>
#include "Oscillator.hpp"
#include "LadderFilter.hpp"
#include "Envelope.hpp"
#include "Lfo.hpp"
#include "Glide.hpp"

namespace m32 {

using namespace rack;

/** The analog signal path: VCO and noise into the mixer, then the ladder filter, then the VCA.
Callers set the control and patch fields each sample, call process(), then read the outputs. */
struct Voice {
	/** Volts of cutoff modulation an accented step adds, and the VCA gain it forces. */
	static constexpr float ACCENT_CUTOFF_VOLTS = 1.5f;
	static constexpr float ACCENT_VCA_GAIN = 1.4f;
	static constexpr float FREQUENCY_KNOB_OCTAVES = 2.f;
	static constexpr float NOISE_LEVEL = 5.f;

	// Panel controls
	float frequencyKnob = 0.5f;
	bool vcoWaveIsPulse = false;
	float pulseWidthKnob = 0.5f;
	float mixKnob = 0.f;
	float cutoffKnob = 1.f;
	float resonanceKnob = 0.f;
	bool vcaModeIsOn = false;
	float volumeKnob = 1.f;
	float glideKnob = 0.f;
	bool vcoModSourceIsLfo = true;
	float vcoModAmount = 0.f;
	bool vcoModDestIsPulseWidth = false;
	bool vcfModeIsHighPass = false;
	bool vcfModSourceIsLfo = false;
	float vcfModAmount = 0.f;
	bool vcfModPolarityIsPositive = true;
	float lfoRateKnob = 0.5f;
	bool lfoWaveIsSquare = false;
	float attackKnob = 0.f;
	bool sustainIsOn = false;
	float decayKnob = 0.f;

	// Patchbay inputs
	float extAudio = 0.f;
	bool extAudioConnected = false;
	float mixCv = 0.f;
	float vcaCv = 0.f;
	bool vcaCvConnected = false;
	float cutoffCv = 0.f;
	float resonanceCv = 0.f;
	float pitchCv = 0.f;
	float linearFm = 0.f;
	float vcoModIn = 0.f;
	bool vcoModInConnected = false;
	float lfoRateCv = 0.f;

	// Note and step state
	float notePitch = 0.f;
	bool gate = false;
	bool glideEnabled = false;
	bool accent = false;

	// Outputs
	float vcaOut = 0.f;
	float noiseOut = 0.f;
	float vcfOut = 0.f;
	float sawOut = 0.f;
	float pulseOut = 0.f;
	float lfoTriOut = 0.f;
	float lfoSqOut = 0.f;
	float egOut = 0.f;

	void process(float dt, float sampleRate) {
		processLfo(dt);
		processEnvelope(dt);

		const float modulator = vcoModulator();
		processOscillator(dt, modulator);

		const float mixed = mixSources();
		processFilter(mixed, sampleRate);
		processAmplifier();
	}

	void reset() {
		oscillator.reset();
		filter.reset();
		envelope.reset();
		lfo.reset();
	}

	void onNoteOn(float pitch, bool glide) {
		if (!glide)
			glideSlew.jumpTo(pitch);
		notePitch = pitch;
		glideEnabled = glide;
	}

	void retriggerEnvelope() {
		envelope.retrigger();
	}

private:
	Oscillator oscillator;
	LadderFilter filter;
	Envelope envelope;
	Lfo lfo;
	Glide glideSlew;

	void processLfo(float dt) {
		const float knobFreq = Lfo::MIN_FREQ * std::pow(Lfo::KNOB_MAX_FREQ / Lfo::MIN_FREQ, lfoRateKnob);
		lfo.freq = clamp(knobFreq * std::pow(2.f, lfoRateCv), Lfo::MIN_FREQ, Lfo::CV_MAX_FREQ);
		lfo.process(dt);
		lfoTriOut = 5.f * lfo.triangle;
		lfoSqOut = 5.f * lfo.square;
	}

	void processEnvelope(float dt) {
		envelope.attackTime = Envelope::MIN_TIME * std::pow(Envelope::MAX_ATTACK / Envelope::MIN_TIME, attackKnob);
		envelope.decayTime = Envelope::MIN_TIME * std::pow(Envelope::MAX_DECAY / Envelope::MIN_TIME, decayKnob);
		envelope.sustainEnabled = sustainIsOn;
		envelope.setGate(gate);
		envelope.process(dt);
		egOut = 10.f * envelope.value;
	}

	/** The VCO mod source switch selects the LFO, or the envelope with the VCO MOD jack overriding it. */
	float vcoModulator() const {
		if (vcoModSourceIsLfo)
			return lfoWaveIsSquare ? lfo.square : lfo.triangle;
		if (vcoModInConnected)
			return vcoModIn / 5.f;
		return envelope.value;
	}

	void processOscillator(float dt, float modulator) {
		const float knobOffset = (frequencyKnob * 2.f - 1.f) * FREQUENCY_KNOB_OCTAVES;
		const float pitchMod = vcoModDestIsPulseWidth ? 0.f : modulator * vcoModAmount * FREQUENCY_KNOB_OCTAVES;
		glideSlew.time = glideEnabled ? glideKnob * Glide::MAX_TIME : 0.f;
		const float glided = glideSlew.process(dt, notePitch);

		const float volts = glided + pitchCv + knobOffset + pitchMod;
		const float baseFreq = dsp::FREQ_C4 * std::pow(2.f, clamp(volts, -8.f, 8.f));
		oscillator.freq = clamp(baseFreq + linearFm * LINEAR_FM_HZ_PER_VOLT, 0.f, 20000.f);

		const float pwMod = vcoModDestIsPulseWidth ? modulator * vcoModAmount : 0.f;
		oscillator.pulseWidth = clamp(0.02f + 0.96f * pulseWidthKnob + pwMod, 0.f, 1.f);

		oscillator.process(dt);
		sawOut = 5.f * oscillator.saw;
		pulseOut = 5.f * oscillator.pulse;
	}

	/** Noise is normalled to the external audio input, so patching there replaces it. */
	float mixSources() {
		noiseOut = NOISE_LEVEL * (random::uniform() * 2.f - 1.f);
		const float secondary = extAudioConnected ? extAudio : noiseOut;
		const float primary = vcoWaveIsPulse ? pulseOut : sawOut;
		const float blend = clamp(mixKnob + mixCv / 5.f, 0.f, 1.f);
		return crossfade(primary, secondary, blend) / 5.f;
	}

	void processFilter(float in, float sampleRate) {
		const float envMod = vcfModSourceIsLfo ? (lfoWaveIsSquare ? lfo.square : lfo.triangle) : envelope.value;
		const float polarity = vcfModPolarityIsPositive ? 1.f : -1.f;
		const float modVolts = envMod * vcfModAmount * polarity * CUTOFF_KNOB_OCTAVES;
		const float accentVolts = accent ? ACCENT_CUTOFF_VOLTS : 0.f;

		const float volts = cutoffKnob * CUTOFF_KNOB_OCTAVES + cutoffCv + modVolts + accentVolts;
		filter.setCutoff(LadderFilter::MIN_CUTOFF * std::pow(2.f, volts), sampleRate);

		const float resonance = clamp(resonanceKnob + resonanceCv / 5.f, 0.f, 1.f);
		filter.setResonance(vcfModeIsHighPass ? 0.f : resonance * MAX_RESONANCE_K);

		filter.process(in);
		vcfOut = 5.f * (vcfModeIsHighPass ? filter.highPass : filter.lowPass);
	}

	void processAmplifier() {
		const float envGain = vcaModeIsOn ? 1.f : envelope.value;
		const float cvGain = vcaCvConnected ? clamp(vcaCv / 5.f, 0.f, 1.f) : 1.f;
		const float accentGain = accent ? ACCENT_VCA_GAIN : 1.f;
		vcaOut = vcfOut * envGain * cvGain * accentGain * volumeKnob;
	}

	/** 20 Hz to 20 kHz is just under ten octaves of cutoff travel. */
	static constexpr float CUTOFF_KNOB_OCTAVES = 9.966f;
	/** Self-oscillation lands near 3 o'clock on the resonance knob. */
	static constexpr float MAX_RESONANCE_K = 4.5f;
	static constexpr float LINEAR_FM_HZ_PER_VOLT = 200.f;
};

} // namespace m32
