#include <rack.hpp>
#include <cstdio>
#include <vector>
#include "../src/seq/Sequencer.hpp"

using namespace rack;
using namespace m32;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail) {
	printf("%s %-46s %s\n", ok ? "  ok " : "FAIL", name, detail);
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

static Pattern makePattern(int lengthSteps) {
	Pattern pattern;
	pattern.endStep = lengthSteps - 1;
	for (int i = 0; i < MAX_STEPS; i++)
		pattern.steps[i].pitch = i / 12.f;
	return pattern;
}

static void testStepRate() {
	Pattern pattern = makePattern(16);
	Sequencer seq;
	seq.clock.bpm = 120.f;
	seq.start();
	seq.reset();

	// Time eight steps between boundaries rather than counting inside a fixed window,
	// which would be sensitive to where the window edges land.
	int advances = 0;
	float firstBoundary = 0.f, lastBoundary = 0.f, elapsed = 0.f;
	for (int i = 0; i < int(SR * 3); i++) {
		seq.process(pattern, DT);
		elapsed += DT;
		if (!seq.clock.stepAdvanced)
			continue;
		advances++;
		if (advances == 2)
			firstBoundary = elapsed;
		if (advances == 10)
			lastBoundary = elapsed;
	}
	// 120 BPM is 2 beats per second; sixteenth notes are 4 per beat, so 8 steps take one second.
	checkNear("120 BPM with 1/16 steps runs 8 steps per second", lastBoundary - firstBoundary, 1.f, 0.002f);
}

/** Swing must move the boundary between the two halves without changing the pair's total time. */
static void testSwingRedistributesWithinPair() {
	auto measurePair = [](float swingAmount) {
		Pattern pattern = makePattern(16);
		pattern.timing.swingAmount = swingAmount;
		pattern.timing.swingInterval = NoteLength::SIXTEENTH;

		Sequencer seq;
		seq.clock.bpm = 120.f;
		seq.start();
		seq.reset();

		std::vector<float> boundaries;
		float elapsed = 0.f;
		for (int i = 0; i < int(SR * 2); i++) {
			seq.process(pattern, DT);
			elapsed += DT;
			if (seq.clock.stepAdvanced && boundaries.size() < 3)
				boundaries.push_back(elapsed);
		}
		return boundaries;
	};

	const std::vector<float> even = measurePair(0.f);
	const std::vector<float> swung = measurePair(0.5f);

	const float evenPair = even[2] - even[0];
	const float swungPair = swung[2] - swung[0];
	checkNear("swing keeps the pair duration unchanged", swungPair, evenPair, 0.002f);

	// Boundaries land at beats 0.25, 0.5 and 0.75, so the first gap is the off-beat
	// and the second is the on-beat.
	const float offBeat = swung[1] - swung[0];
	const float onBeat = swung[2] - swung[1];
	char detail[96];
	snprintf(detail, sizeof(detail), "on-beat %.4f s vs off-beat %.4f s", onBeat, offBeat);
	check("positive swing lengthens the on-beat", onBeat > offBeat * 1.5f, detail);
}

static void testPlaybackOrders() {
	auto collect = [](PlaybackOrder order, int count) {
		Pattern pattern = makePattern(4);
		Sequencer seq;
		seq.clock.bpm = 240.f;
		seq.order = order;
		seq.start();
		seq.reset();

		std::vector<int> visited;
		for (int i = 0; i < int(SR * 4) && int(visited.size()) < count; i++) {
			seq.process(pattern, DT);
			if (seq.clock.stepAdvanced)
				visited.push_back(seq.currentStep);
		}
		return visited;
	};

	const std::vector<int> forward = collect(PlaybackOrder::FORWARD, 6);
	check("forward wraps 0 1 2 3 0 1", forward == std::vector<int>({1, 2, 3, 0, 1, 2}), "wrapped");

	const std::vector<int> reverse = collect(PlaybackOrder::REVERSE, 6);
	check("reverse wraps 3 2 1 0 3 2", reverse == std::vector<int>({3, 2, 1, 0, 3, 2}), "wrapped");

	const std::vector<int> pendulum = collect(PlaybackOrder::PENDULUM, 8);
	check("pendulum reflects without repeating ends", pendulum == std::vector<int>({1, 2, 3, 2, 1, 0, 1, 2}), "bounced");

	const std::vector<int> shuffled = collect(PlaybackOrder::RANDOM, 40);
	bool inRange = true;
	for (int step : shuffled)
		inRange = inRange && step >= 0 && step <= 3;
	check("random stays inside the pattern", inRange, "all within 0..3");
}

/** Gate length is a fraction of the step; a tie holds the gate across the boundary. */
static void testGateLengthAndTies() {
	auto dutyCycle = [](int gateLength) {
		Pattern pattern = makePattern(4);
		for (int i = 0; i < MAX_STEPS; i++)
			pattern.steps[i].gateLength = gateLength;

		Sequencer seq;
		seq.clock.bpm = 120.f;
		seq.start();
		seq.reset();

		int high = 0, total = 0;
		for (int i = 0; i < int(SR); i++) {
			seq.process(pattern, DT);
			total++;
			if (seq.output().gate)
				high++;
		}
		return float(high) / float(total);
	};

	checkNear("gate length 4/8 is half the step", dutyCycle(4), 0.5f, 0.02f);
	checkNear("gate length 2/8 is a quarter of the step", dutyCycle(2), 0.25f, 0.02f);
	checkNear("gate length 8/8 holds the gate open", dutyCycle(8), 1.f, 0.001f);
}

static void testTieSuppressesRetrigger() {
	Pattern pattern = makePattern(4);
	pattern.steps[0].gateLength = TIE_GATE_LENGTH;
	pattern.steps[1].gateLength = 4;
	pattern.steps[2].gateLength = 4;
	pattern.steps[3].gateLength = 4;

	Sequencer seq;
	seq.clock.bpm = 120.f;
	seq.start();
	seq.reset();

	std::vector<int> retriggeredOn;
	for (int i = 0; i < int(SR); i++) {
		seq.process(pattern, DT);
		if (seq.output().retrigger)
			retriggeredOn.push_back(seq.currentStep);
	}

	bool step1Retriggered = false;
	for (int step : retriggeredOn)
		step1Retriggered = step1Retriggered || step == 1;
	check("a tied step does not retrigger the next note", !step1Retriggered, "step 1 played legato");
	check("untied steps still retrigger", retriggeredOn.size() > 2, "other steps retriggered");
}

static void testRatchets() {
	Pattern pattern = makePattern(1);
	pattern.steps[0].ratchet = 3;
	pattern.steps[0].gateLength = 2;

	Sequencer seq;
	seq.clock.bpm = 120.f;
	seq.start();
	seq.reset();

	// Measure one whole step, from its opening boundary to the next.
	int retriggers = 0;
	int gateRises = 0;
	int boundaries = 0;
	bool wasHigh = false;
	for (int i = 0; i < int(SR); i++) {
		seq.process(pattern, DT);
		if (seq.clock.stepAdvanced)
			boundaries++;
		if (boundaries < 1)
			continue;
		if (boundaries >= 2)
			break;
		if (seq.output().retrigger)
			retriggers++;
		if (seq.output().gate && !wasHigh)
			gateRises++;
		wasHigh = seq.output().gate;
	}
	char detail[96];
	snprintf(detail, sizeof(detail), "%d retriggers, %d gate rises", retriggers, gateRises);
	check("ratchet 3 fires three times in one step", retriggers == 3 && gateRises == 3, detail);
}

static void testRestsAndLiveOverrides() {
	Pattern pattern = makePattern(2);
	pattern.steps[1].rest = true;

	Sequencer seq;
	seq.clock.bpm = 120.f;
	seq.start();
	seq.reset();

	bool gateOnRest = false;
	for (int i = 0; i < int(SR); i++) {
		seq.process(pattern, DT);
		if (seq.currentStep == 1 && seq.output().gate)
			gateOnRest = true;
	}
	check("a rest produces no gate", !gateOnRest, "silent");

	seq.liveMute = true;
	bool anyGate = false;
	for (int i = 0; i < int(SR); i++) {
		seq.process(pattern, DT);
		anyGate = anyGate || seq.output().gate;
	}
	check("live mute silences the gate while advancing", !anyGate, "muted");

	seq.liveMute = false;
	seq.liveAccent = true;
	bool sawAccent = false;
	for (int i = 0; i < int(SR); i++) {
		seq.process(pattern, DT);
		sawAccent = sawAccent || seq.output().accent;
	}
	check("live accent accents unaccented steps", sawAccent, "accented");
}

static void testHoldRepeatsStep() {
	Pattern pattern = makePattern(8);
	Sequencer seq;
	seq.clock.bpm = 120.f;
	seq.start();
	seq.reset();

	for (int i = 0; i < int(SR * 0.3f); i++)
		seq.process(pattern, DT);

	const int held = seq.currentStep;
	seq.holdHeld = true;
	int retriggers = 0;
	for (int i = 0; i < int(SR * 0.5f); i++) {
		seq.process(pattern, DT);
		if (seq.output().retrigger)
			retriggers++;
	}
	check("hold freezes the step", seq.currentStep == held, "same step");
	check("hold keeps retriggering it", retriggers >= 3, "repeated");
}

static void testPatternJsonRoundTrip() {
	Pattern pattern = makePattern(9);
	pattern.steps[3].accent = true;
	pattern.steps[3].ratchet = 4;
	pattern.steps[5].rest = true;
	pattern.steps[7].glide = true;
	pattern.steps[7].gateLength = TIE_GATE_LENGTH;
	pattern.timing.swingAmount = -0.4f;
	pattern.timing.swingInterval = NoteLength::THIRTY_SECOND;

	json_t* j = pattern.toJson();
	Pattern restored;
	restored.fromJson(j);
	json_decref(j);

	const bool same = restored.endStep == pattern.endStep
		&& restored.steps[3].accent && restored.steps[3].ratchet == 4
		&& restored.steps[5].rest && restored.steps[7].glide
		&& restored.steps[7].isTied()
		&& std::fabs(restored.timing.swingAmount - (-0.4f)) < 1e-5f
		&& restored.timing.swingInterval == NoteLength::THIRTY_SECOND;
	check("pattern survives a JSON round trip", same, "restored intact");
}

int main() {
	random::init();

	printf("\nM32 sequencer checks at %.0f Hz\n\n", SR);
	testStepRate();
	testSwingRedistributesWithinPair();
	testPlaybackOrders();
	testGateLengthAndTies();
	testTieSuppressesRetrigger();
	testRatchets();
	testRestsAndLiveOverrides();
	testHoldRepeatsStep();
	testPatternJsonRoundTrip();

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
