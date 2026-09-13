#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Triangle and square modulation oscillator. Reaches audio rate, as the hardware's does. */
struct Lfo {
	static constexpr float MIN_FREQ = 0.1f;
	static constexpr float KNOB_MAX_FREQ = 350.f;
	static constexpr float CV_MAX_FREQ = 600.f;

	float freq = 1.f;

	float triangle = 0.f;
	float square = 0.f;

	void process(float dt) {
		phase += clamp(freq * dt, 0.f, 0.5f);
		if (phase >= 1.f)
			phase -= 1.f;

		triangle = 4.f * std::fabs(phase - 0.5f) - 1.f;
		square = phase < 0.5f ? 1.f : -1.f;
	}

	void reset() {
		phase = 0.f;
	}

private:
	float phase = 0.f;
};

} // namespace m32
