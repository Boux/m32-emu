#pragma once
#include <rack.hpp>
#include "Pattern.hpp"

namespace m32 {

using namespace rack;

/** Musical timebase for the sequencer.

Swing redistributes real time between the two halves of each swing-interval pair without changing
how long the pair takes. Musical time therefore runs fast in the compressed half and slow in the
stretched half, which is what makes a step that straddles the boundary finish at the other half's
rate, as the manual describes. */
struct Clock {
	static constexpr float MIN_BPM = 20.f;
	static constexpr float MAX_BPM = 300.f;
	/** Keeps a fully collapsed swing phase from dividing by zero. */
	static constexpr float MIN_PHASE_FRACTION = 1e-4f;

	float bpm = 120.f;
	Timing timing;

	/** True for the one sample on which a new step begins. */
	bool stepAdvanced = false;
	/** Position within the current step, 0 to 1, in musical time. */
	float stepPhase = 0.f;
	/** Counts steps since reset; the sequencer maps this onto pattern positions. */
	int64_t stepCount = 0;

	void processInternal(float dt) {
		const float rate = bpm / 60.f * swingRateMultiplier();
		advance(double(dt) * double(rate));
	}

	/** Advances by one external clock pulse worth of musical time. */
	void advanceByPulse(float beatsPerPulse) {
		advance(double(beatsPerPulse));
	}

	void reset() {
		beats = 0.0;
		stepCount = 0;
		stepPhase = 0.f;
		stepAdvanced = true;
	}

	float stepLengthBeats() const {
		return noteLengthBeats(timing.clockDivision, timing.clockForm);
	}

	/** Where the playhead sits in the swing cycle, for driving the clock output. */
	bool onSwingBeat() const {
		const float interval = noteLengthBeats(timing.swingInterval, timing.swingForm);
		const double pair = 2.0 * double(interval);
		return std::fmod(beats, pair) < double(interval);
	}

private:
	double beats = 0.0;

	float swingRateMultiplier() const {
		const float onBeatFraction = clamp((timing.swingAmount + 1.f) / 2.f, MIN_PHASE_FRACTION, 1.f - MIN_PHASE_FRACTION);
		const float fraction = onSwingBeat() ? onBeatFraction : 1.f - onBeatFraction;
		return 0.5f / fraction;
	}

	void advance(double deltaBeats) {
		const double stepLength = double(stepLengthBeats());
		const int64_t before = int64_t(std::floor(beats / stepLength));

		beats += deltaBeats;

		const double position = beats / stepLength;
		const int64_t after = int64_t(std::floor(position));

		stepAdvanced = after != before;
		stepCount += after - before;
		stepPhase = float(position - std::floor(position));
	}
};

} // namespace m32
