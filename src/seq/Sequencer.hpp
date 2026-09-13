#pragma once
#include <rack.hpp>
#include "Pattern.hpp"
#include "Clock.hpp"

namespace m32 {

using namespace rack;

enum class PlaybackOrder { FORWARD, REVERSE, PENDULUM, RANDOM };

struct SequencerOutput {
	float pitch = 0.f;
	bool gate = false;
	bool accent = false;
	bool glide = false;
	/** High for the one sample a new note begins. Ties and rests do not retrigger. */
	bool retrigger = false;
};

/** Plays a pattern. Live overrides come from the panel and are never written back to the pattern. */
struct Sequencer {
	Clock clock;
	PlaybackOrder order = PlaybackOrder::FORWARD;
	bool running = false;

	bool liveAccent = false;
	bool liveMute = false;
	/** Zero means use each step's stored ratchet count. */
	int liveRatchet = 0;
	bool holdHeld = false;
	/** While the RESET jack is high the pattern sits on step 1 and repeats it (p56). */
	bool resetHeld = false;
	float transpose = 0.f;

	int currentStep = 0;

	void process(const Pattern& pattern, float dt) {
		out.retrigger = false;
		clock.timing = pattern.timing;

		if (!running)
			return;

		if (pendingStart) {
			pendingStart = false;
			beginStep(pattern, false);
		}

		clock.processInternal(dt);
		if (clock.stepAdvanced)
			beginStep(pattern, !holdHeld);

		updateGate(pattern);
	}

	void start() {
		running = true;
		pendingStart = true;
	}

	/** Pausing leaves the playhead where it is; the hardware resumes on the next step. */
	void stop() {
		running = false;
		out.gate = false;
	}

	void reset() {
		currentStep = 0;
		pendulumDirection = 1;
		ratchetSlot = 0;
		clock.reset();
		pendingStart = true;
	}

	const SequencerOutput& output() const {
		return out;
	}

private:
	SequencerOutput out;
	int pendulumDirection = 1;
	int ratchetSlot = 0;
	bool pendingStart = false;

	/** Starts sounding a step. The tie that matters belongs to the step being left, not the one arriving. */
	void beginStep(const Pattern& pattern, bool advance) {
		const Step& leaving = pattern.steps[currentStep];
		const bool tiedIntoNext = advance && leaving.isTied() && !leaving.rest;

		if (resetHeld) {
			currentStep = 0;
			pendulumDirection = 1;
		}
		else if (advance) {
			currentStep = nextStep(pattern);
		}

		ratchetSlot = 0;

		const Step& arriving = pattern.steps[currentStep];
		if (!arriving.rest)
			out.pitch = arriving.pitch + transpose;
		out.glide = arriving.glide;
		out.retrigger = !tiedIntoNext && !arriving.rest;
	}

	int nextStep(const Pattern& pattern) {
		const int last = pattern.endStep;
		if (order == PlaybackOrder::FORWARD)
			return currentStep >= last ? 0 : currentStep + 1;
		if (order == PlaybackOrder::REVERSE)
			return currentStep <= 0 ? last : currentStep - 1;
		if (order == PlaybackOrder::RANDOM)
			return clamp(int(random::uniform() * (last + 1)), 0, last);
		return pendulumStep(last);
	}

	/** Reflects at both ends without repeating the endpoint: 0 1 2 3 2 1 0 1 ... */
	int pendulumStep(int last) {
		if (last == 0)
			return 0;
		int candidate = currentStep + pendulumDirection;
		if (candidate > last) {
			pendulumDirection = -1;
			candidate = last - 1;
		}
		else if (candidate < 0) {
			pendulumDirection = 1;
			candidate = 1;
		}
		return candidate;
	}

	int ratchetCount(const Step& step) const {
		return liveRatchet > 0 ? liveRatchet : clamp(step.ratchet, MIN_RATCHET, MAX_RATCHET);
	}

	/** Gate length is a fraction of each ratchet slot; a tie holds the gate through the whole step. */
	void updateGate(const Pattern& pattern) {
		const Step& step = pattern.steps[currentStep];
		const int ratchets = ratchetCount(step);
		const int slot = std::min(int(clock.stepPhase * ratchets), ratchets - 1);
		const float slotPhase = clock.stepPhase * ratchets - float(slot);

		if (slot != ratchetSlot) {
			ratchetSlot = slot;
			out.retrigger = !step.rest;
		}

		const float openFraction = float(step.gateLength) / float(TIE_GATE_LENGTH);
		const bool open = step.isTied() || slotPhase < openFraction;

		out.gate = open && !step.rest && !liveMute;
		out.accent = (step.accent || liveAccent) && !liveMute;
	}
};

} // namespace m32
