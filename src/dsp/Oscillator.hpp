#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Bandlimited saw and pulse sharing one phase, as on the hardware's single VCO. */
struct Oscillator {
	float phase = 0.f;
	float freq = dsp::FREQ_C4;
	float pulseWidth = 0.5f;

	float saw = 0.f;
	float pulse = 0.f;

	dsp::MinBlepGenerator<16, 16, float> sawBlep;
	dsp::MinBlepGenerator<16, 16, float> pulseBlep;

	void process(float dt) {
		const float deltaPhase = clamp(freq * dt, 0.f, 0.35f);
		const float pw = clamp(pulseWidth, 0.f, 1.f);
		const float oldPhase = phase;
		phase += deltaPhase;

		insertPulseEdge(oldPhase, deltaPhase, pw);

		if (phase >= 1.f) {
			phase -= 1.f;
			const float p = -phase / deltaPhase;
			sawBlep.insertDiscontinuity(p, -2.f);
			if (pw > 0.f)
				pulseBlep.insertDiscontinuity(p, 2.f);
		}

		saw = 2.f * phase - 1.f + sawBlep.process();
		pulse = (phase < pw ? 1.f : -1.f) + pulseBlep.process();
	}

	void reset() {
		phase = 0.f;
	}

private:
	/** Falling edge at the pulse-width crossing. Skipped at 0% and 100% so the output goes silent. */
	void insertPulseEdge(float oldPhase, float deltaPhase, float pw) {
		if (pw <= 0.f || pw >= 1.f)
			return;
		if (oldPhase >= pw || oldPhase + deltaPhase < pw)
			return;
		const float p = (pw - (oldPhase + deltaPhase)) / deltaPhase;
		pulseBlep.insertDiscontinuity(p, -2.f);
	}
};

} // namespace m32
