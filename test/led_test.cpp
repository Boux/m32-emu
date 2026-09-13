#include <rack.hpp>
#include <cstdio>
#include "../src/seq/PanelLeds.hpp"

using namespace rack;
using namespace m32;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail) {
	printf("%s %-54s %s\n", ok ? "  ok " : "FAIL", name, detail);
	if (!ok)
		failures++;
}

/** Names a colour the way the manual does, so failures read like the manual reads. */
static const char* describe(const LedColour& c) {
	const bool r = c.red > 0.5f;
	const bool g = c.green > 0.5f;
	if (r && g)
		return "yellow";
	if (r)
		return "red";
	if (g)
		return "green";
	return "off";
}

static void checkColour(const char* name, const LedColour& actual, const char* expected) {
	check(name, std::string(describe(actual)) == expected, describe(actual));
}

struct Rig {
	PanelControl panel;
	Pattern pattern;
	Sequencer sequencer;
	LedState leds;
	int octave = DEFAULT_OCTAVE;
	int bank = 0;
	int patternIndex = 0;
	float blinkPhase = 0.f;
	bool halfRate = true;

	Rig() {
		pattern.endStep = 7;
	}

	void compute() {
		LedInputs in;
		in.panel = &panel;
		in.pattern = &pattern;
		in.sequencer = &sequencer;
		in.octave = octave;
		in.bank = bank;
		in.patternIndex = patternIndex;
		in.blinkPhase = blinkPhase;
		in.halfRate = halfRate;
		computeLeds(in, leds);
	}
};

/** p17: eight octave LEDs lit red show the keyboard octave. */
static void testKeyboardModeShowsOctaveInRed() {
	Rig rig;
	rig.octave = 6;
	rig.compute();

	checkColour("keyboard mode lights the octave red", rig.leds.octave[5], "red");
	checkColour("and leaves the other octave LEDs off", rig.leds.octave[2], "off");
}

/** p32: entering step mode lights LED 1 yellow and removes the red octave LED. */
static void testStepModeDropsTheOctaveLed() {
	Rig rig;
	rig.panel.mode = SeqMode::STEP;
	rig.compute();

	checkColour("step mode shows the page in yellow", rig.leds.octave[0], "yellow");
	for (int i = 1; i < OCTAVE_LEDS; i++) {
		if (rig.leds.octave[i].red > 0.5f)
			check("step mode shows no red octave LED", false, "red LED present");
	}
	check("step mode shows no red octave LED", true, "none");

	// p36: selecting a step brings the red octave LED back, since the keys now set the note.
	rig.panel.editStep = 0;
	rig.compute();
	bool anyRed = false;
	for (int i = 0; i < OCTAVE_LEDS; i++)
		anyRed = anyRed || (rig.leds.octave[i].red > 0.5f && rig.leds.octave[i].green < 0.5f);
	check("editing a step in step mode brings the octave LED back", anyRed, "red returns");
}

/** p24: LEDs 5 to 8 show glide, ratchet, accent and rest for the step being edited. */
static void testStepFlagLeds() {
	Rig rig;
	rig.panel.editStep = 2;
	rig.pattern.steps[2].glide = true;
	rig.pattern.steps[2].ratchet = 3;
	rig.pattern.steps[2].accent = true;
	rig.pattern.steps[2].rest = true;
	rig.compute();

	checkColour("LED 5 is green for glide in keyboard mode", rig.leds.octave[4], "green");
	checkColour("LED 6 is green for a ratchet above one", rig.leds.octave[5], "green");
	checkColour("LED 7 is green for accent", rig.leds.octave[6], "green");
	checkColour("LED 8 is green for rest", rig.leds.octave[7], "green");

	// p36: the same flags are yellow in step mode.
	rig.panel.mode = SeqMode::STEP;
	rig.compute();
	checkColour("LED 5 is yellow for glide in step mode", rig.leds.octave[4], "yellow");
	checkColour("LED 7 is yellow for accent in step mode", rig.leds.octave[6], "yellow");
}

/** p25: the octave row temporarily shows gate length in red, all eight meaning a tie. */
static void testGateLengthReadout() {
	Rig rig;
	rig.panel.readout = Readout::GATE_LENGTH;
	rig.panel.readoutValue = 3;
	rig.compute();

	checkColour("three red LEDs for a gate length of 3/8", rig.leds.octave[2], "red");
	checkColour("and the fourth stays off", rig.leds.octave[3], "off");

	rig.panel.readoutValue = TIE_GATE_LENGTH;
	rig.compute();
	checkColour("all eight red for a tie", rig.leds.octave[7], "red");
}

/** p27: ratchet count shows on LEDs 1 to 4 in yellow. */
static void testRatchetReadout() {
	Rig rig;
	rig.panel.readout = Readout::RATCHET;
	rig.panel.readoutValue = 2;
	rig.compute();

	checkColour("two yellow LEDs for two ratchets", rig.leds.octave[1], "yellow");
	checkColour("and the third stays off", rig.leds.octave[2], "off");
}

/** p37: bank numbers are yellow so they can be told apart from green pattern numbers. */
static void testBankAndPatternColours() {
	Rig rig;
	rig.patternIndex = 4;
	rig.panel.patternHeld = true;
	rig.compute();
	checkColour("holding PATTERN shows the pattern in green", rig.leds.octave[4], "green");

	rig.bank = 2;
	rig.panel.shiftHeld = true;
	rig.compute();
	checkColour("adding SHIFT shows the bank in yellow", rig.leds.octave[2], "yellow");
}

/** p31: the save destination blinks green. */
static void testSaveDestinationBlinks() {
	Rig rig;
	rig.panel.saving = true;
	rig.panel.saveIndex = 5;

	rig.blinkPhase = 0.1f;
	rig.compute();
	checkColour("the save destination lights green", rig.leds.octave[5], "green");

	rig.blinkPhase = 0.7f;
	rig.compute();
	checkColour("and blinks off again", rig.leds.octave[5], "off");
}

/** p27: an octave LED that coincides with the page LED alternates between red and green. */
static void testSharedOctaveAndPageLed() {
	Rig rig;
	rig.panel.recording = true;
	rig.panel.page = 0;
	rig.octave = 1;

	rig.blinkPhase = 0.1f;
	rig.compute();
	checkColour("a shared LED shows red on one half of the blink", rig.leds.octave[0], "red");

	rig.blinkPhase = 0.7f;
	rig.compute();
	checkColour("and green on the other", rig.leds.octave[0], "green");
}

static void testStepRow() {
	Rig rig;
	rig.pattern.endStep = 5;
	rig.pattern.steps[1].written = true;
	rig.pattern.steps[3].rest = true;
	rig.sequencer.running = true;
	rig.sequencer.currentStep = 2;
	rig.blinkPhase = 0.1f;
	rig.compute();

	check("the playing step is fully lit", rig.leds.step[2] > 0.9f, "lit");
	check("a step holding data is dimly lit", rig.leds.step[1] > 0.f && rig.leds.step[1] < 0.9f, "dim");
	check("a step past the end step is off", rig.leds.step[6] == 0.f, "off");
	check("the end step blinks", rig.leds.step[5] > 0.9f, "blinking");

	// p36: step mode lights every step that is not a rest.
	rig.panel.mode = SeqMode::STEP;
	rig.sequencer.running = false;
	rig.compute();
	check("step mode lights a non-rest step", rig.leds.step[0] > 0.f, "lit");
	check("step mode leaves a rest unlit", rig.leds.step[3] == 0.f, "off");

	// p44: editing in step mode turns all the other step LEDs off.
	rig.panel.editStep = 4;
	rig.compute();
	check("editing in step mode blanks the other steps", rig.leds.step[0] == 0.f, "blanked");
	check("and blinks the step being edited", rig.leds.step[4] > 0.9f, "blinking");
}

/** p27, p33: yellow while recording, alternating yellow and red while editing, red while running. */
static void testTempoLed() {
	Rig rig;
	rig.blinkPhase = 0.1f;

	rig.panel.recording = true;
	rig.compute();
	checkColour("recording holds the tempo LED yellow", rig.leds.tempo, "yellow");

	rig.panel.recording = false;
	rig.panel.editStep = 1;
	rig.compute();
	checkColour("editing a step alternates yellow", rig.leds.tempo, "yellow");
	rig.blinkPhase = 0.7f;
	rig.compute();
	checkColour("and red", rig.leds.tempo, "red");

	rig.panel.editStep = -1;
	rig.sequencer.running = true;
	rig.blinkPhase = 0.1f;
	rig.compute();
	checkColour("running blinks it red", rig.leds.tempo, "red");
	rig.blinkPhase = 0.7f;
	rig.compute();
	checkColour("with an off half", rig.leds.tempo, "off");
}

/** p38, p39: saving sweeps green, cancelling sweeps red, initializing sweeps right to left. */
static void testAnimations() {
	Rig rig;
	rig.panel.animation = PanelControl::Animation::SAVED;
	rig.panel.animationTime = PanelControl::ANIMATION_SECONDS;
	rig.compute();
	checkColour("a save sweep starts at the outer LEDs in green", rig.leds.octave[0], "green");
	checkColour("and has not reached the middle yet", rig.leds.octave[3], "off");

	rig.panel.animation = PanelControl::Animation::CANCELLED;
	rig.compute();
	checkColour("a cancelled save sweeps red instead", rig.leds.octave[0], "red");

	rig.panel.animation = PanelControl::Animation::INITIALIZED;
	rig.compute();
	checkColour("initializing sweeps from the right", rig.leds.octave[7], "red");
	checkColour("and has not reached the left yet", rig.leds.octave[0], "off");
}

int main() {
	random::init();

	printf("\nM32 panel LED language checks\n\n");
	testKeyboardModeShowsOctaveInRed();
	testStepModeDropsTheOctaveLed();
	testStepFlagLeds();
	testGateLengthReadout();
	testRatchetReadout();
	testBankAndPatternColours();
	testSaveDestinationBlinks();
	testSharedOctaveAndPageLed();
	testStepRow();
	testTempoLed();
	testAnimations();

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
