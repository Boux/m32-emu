#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Exponential portamento on the pitch CV. Rate comes from the panel knob, never from step data. */
struct Glide {
	static constexpr float MAX_TIME = 4.f;

	float value = 0.f;
	float time = 0.f;

	float process(float dt, float target) {
		if (time <= 0.f) {
			value = target;
			return value;
		}
		const float tau = time / 4.6052f; // ln(100), within 1% of target in `time` seconds
		value += (target - value) * (1.f - std::exp(-dt / tau));
		return value;
	}

	void jumpTo(float target) {
		value = target;
	}
};

} // namespace m32
