#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Four-pole transistor ladder, zero-delay-feedback form with a saturating input stage.
Low pass is the fourth stage output. High pass is the binomial sum of the stage outputs,
which is how the hardware derives 24 dB/oct high pass from the same ladder. */
struct LadderFilter {
	static constexpr float MIN_CUTOFF = 20.f;
	static constexpr float MAX_CUTOFF = 20000.f;
	/** Feedback at which the ladder self-oscillates. */
	static constexpr float SELF_OSC_K = 4.f;

	float lowPass = 0.f;
	float highPass = 0.f;
	/** How hard the ladder clips. Lower values stay linear longer, so the self-oscillation
	limit cycle settles at a larger amplitude before the saturation catches it. */
	float drive = 0.22f;

	void setCutoff(float hz, float sampleRate) {
		const float nyquistLimit = 0.49f * sampleRate;
		const float fc = clamp(hz, MIN_CUTOFF, std::min(MAX_CUTOFF, nyquistLimit));
		const float g = std::tan(float(M_PI) * fc / sampleRate);
		G = g / (1.f + g);
	}

	void setResonance(float k) {
		this->k = std::max(k, 0.f);
	}

	void process(float in) {
		const float S1 = (1.f - G) * s1;
		const float S2 = (1.f - G) * s2;
		const float S3 = (1.f - G) * s3;
		const float S4 = (1.f - G) * s4;

		const float G2 = G * G;
		const float G4 = G2 * G2;
		const float sigma = G2 * G * S1 + G2 * S2 + G * S3 + S4;

		// The ladder's first transistor pair sees input minus feedback, so the saturation belongs
		// there rather than on the input alone. Putting it inside the loop is also what keeps the
		// filter bounded above self-oscillation instead of diverging.
		const float estimate = (G4 * in + sigma) / (1.f + k * G4);
		const float u = std::tanh((in - k * estimate) * drive) / drive;

		const float y1 = G * u + S1;
		const float y2 = G * y1 + S2;
		const float y3 = G * y2 + S3;
		const float y4 = G * y3 + S4;

		s1 = 2.f * y1 - s1;
		s2 = 2.f * y2 - s2;
		s3 = 2.f * y3 - s3;
		s4 = 2.f * y4 - s4;

		lowPass = y4;
		highPass = u - 4.f * y1 + 6.f * y2 - 4.f * y3 + y4;
	}

	void reset() {
		s1 = s2 = s3 = s4 = 0.f;
		lowPass = highPass = 0.f;
	}

private:
	float G = 0.f;
	float k = 0.f;
	float s1 = 0.f, s2 = 0.f, s3 = 0.f, s4 = 0.f;
};

} // namespace m32
