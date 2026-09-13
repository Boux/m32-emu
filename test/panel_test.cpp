#include <rack.hpp>
#include <cstdio>
#include <vector>
#include "../src/seq/PanelControl.hpp"

using namespace rack;
using namespace m32;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail) {
	printf("%s %-52s %s\n", ok ? "  ok " : "FAIL", name, detail);
	if (!ok)
		failures++;
}

static void checkEq(const char* name, int actual, int expected) {
	char detail[96];
	snprintf(detail, sizeof(detail), "got %d, expected %d", actual, expected);
	check(name, actual == expected, detail);
}

/** Drives PanelControl the way the widget does: one frame at a time with buttons held or not. */
struct Rig {
	Pattern pattern;
	PatternMemory memory;
	Sequencer sequencer;
	PanelControl panel;
	PanelInput in;
	int bank = 0;
	int index = 0;
	int octave = 4;

	Rig() {
		in.dt = 1.f / 60.f;
		pattern.endStep = 7;
	}

	PanelTarget target() {
		PanelTarget t;
		t.pattern = &pattern;
		t.memory = &memory;
		t.sequencer = &sequencer;
		t.bank = &bank;
		t.patternIndex = &index;
		t.octave = &octave;
		return t;
	}

	void frames(int count) {
		for (int i = 0; i < count; i++)
			panel.process(in, target());
	}

	void tap(bool& button) {
		button = true;
		frames(1);
		button = false;
		frames(1);
	}

	void tapKey(int key) {
		in.keys[key] = true;
		frames(1);
		in.keys[key] = false;
		frames(1);
	}

	void turnGlide(float value) {
		in.glideKnob = value;
		frames(2);
	}

	void turnTempo(float value) {
		in.tempoKnob = value;
		frames(2);
	}

	/** Enters KB record mode the way the panel does: latch SHIFT, tap REC, release SHIFT. */
	void startRecording() {
		in.shift = true;
		frames(1);
		tap(in.runStop);
		in.shift = false;
		frames(1);
	}
};

static void testModeSwitching() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tap(rig.in.step);
	check("SHIFT + STEP enters step mode", rig.panel.mode == SeqMode::STEP, "step mode");

	rig.tap(rig.in.kb);
	check("SHIFT + KB returns to keyboard mode", rig.panel.mode == SeqMode::KB, "kb mode");
}

static void testArrowTapsChangeOctave() {
	Rig rig;
	rig.tap(rig.in.step);
	checkEq("tapping STEP raises the octave", rig.octave, 5);
	rig.tap(rig.in.kb);
	rig.tap(rig.in.kb);
	checkEq("tapping KB lowers the octave", rig.octave, 3);
}

/** Holding an arrow to reach a combo must not also fire its own action. */
static void testHeldArrowDoesNotChangeOctave() {
	Rig rig;
	rig.in.kb = true;
	rig.frames(2);
	rig.turnGlide(0.8f);
	rig.in.kb = false;
	rig.frames(2);

	checkEq("holding KB for swing interval leaves the octave alone", rig.octave, 4);
	check("KB + GLIDE picks a dotted swing interval",
		rig.pattern.timing.swingForm == NoteForm::DOTTED, "dotted");
}

/** The arrows plus TEMPO set the clock division; the arrows plus GLIDE set the swing interval. */
static void testClockDivisionAndSwingInterval() {
	Rig rig;
	rig.in.kb = true;
	rig.frames(2);
	rig.turnTempo(0.7f);
	rig.in.kb = false;
	rig.frames(2);
	check("KB + TEMPO picks a dotted clock division",
		rig.pattern.timing.clockForm == NoteForm::DOTTED, "dotted");
	checkEq("and does not move the octave", rig.octave, 4);

	const float bpmBefore = rig.panel.out.bpm;
	rig.in.step = true;
	rig.frames(2);
	rig.turnTempo(0.3f);
	rig.in.step = false;
	rig.frames(2);
	check("STEP + TEMPO picks a triplet clock division",
		rig.pattern.timing.clockForm == NoteForm::TRIPLET, "triplet");
	check("setting the clock division does not change the tempo",
		std::fabs(rig.panel.out.bpm - bpmBefore) < 0.01f, "tempo held");

	rig.in.kb = true;
	rig.in.step = true;
	rig.frames(2);
	rig.turnTempo(0.9f);
	rig.in.kb = false;
	rig.in.step = false;
	rig.frames(2);
	check("KB + STEP + TEMPO picks a straight clock division",
		rig.pattern.timing.clockForm == NoteForm::STRAIGHT, "straight");
}

static void testRecordStepWrite() {
	Rig rig;
	rig.startRecording();
	check("SHIFT + REC starts recording", rig.panel.recording, "recording");
	checkEq("recording starts on step 1", rig.panel.editStep, 0);

	rig.tapKey(0);
	checkEq("the step just entered stays selected for tweaking", rig.panel.editStep, 0);

	rig.tap(rig.in.resetAccent);
	check("so its accent can still be set", rig.pattern.steps[0].accent, "accented");

	rig.tapKey(4);
	checkEq("the next note moves on to step 2", rig.panel.editStep, 1);
	rig.tapKey(7);
	checkEq("three notes fill steps 1 to 3", rig.panel.editStep, 2);

	check("step 1 stored low C of octave 4", std::fabs(rig.pattern.steps[0].pitch - (-1.f)) < 1e-5f, "-1 V");
	check("step 2 stored G", std::fabs(rig.pattern.steps[1].pitch - (-1.f + 7.f / 12.f)) < 1e-5f, "G");
	check("step 3 stored upper C", std::fabs(rig.pattern.steps[2].pitch - 0.f) < 1e-5f, "upper C");
}

static void testRunStopLeavesRecordMode() {
	Rig rig;
	rig.startRecording();
	rig.tap(rig.in.runStop);
	check("RUN/STOP alone exits record mode", !rig.panel.recording, "exited");
	check("and does not start the transport", !rig.sequencer.running, "still stopped");
	checkEq("the edit cursor clears", rig.panel.editStep, -1);

	rig.tap(rig.in.runStop);
	check("the next RUN/STOP starts playback", rig.sequencer.running, "running");
}

static void testGateLengthCarriesForward() {
	Rig rig;
	rig.startRecording();
	rig.turnTempo(0.1f);
	rig.tapKey(0);
	rig.tapKey(2);

	checkEq("gate length carries to the next step", rig.pattern.steps[1].gateLength, rig.pattern.steps[0].gateLength);
}

static void testStepSelectAndEdit() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(2);
	checkEq("SHIFT + step 3 selects it for editing", rig.panel.editStep, 2);

	rig.in.shift = false;
	rig.frames(1);
	rig.tap(rig.in.resetAccent);
	check("ACCENT toggles accent on the edited step", rig.pattern.steps[2].accent, "accented");

	rig.tap(rig.in.holdRest);
	check("REST toggles rest on the edited step", rig.pattern.steps[2].rest, "rested");

	rig.turnTempo(1.f);
	checkEq("TEMPO sets gate length while editing", rig.pattern.steps[2].gateLength, TIE_GATE_LENGTH);

	rig.turnGlide(0.6f);
	check("GLIDE turns glide on for the edited step", rig.pattern.steps[2].glide, "gliding");

	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(2);
	checkEq("SHIFT + the same step exits editing", rig.panel.editStep, -1);
}

/** Changing an already-recorded note: select the step, play a key, leave step edit. */
static void testChangeNoteDuringRecord() {
	Rig rig;
	rig.startRecording();
	rig.tapKey(0);
	rig.tapKey(2);

	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(0);
	rig.in.shift = false;
	rig.frames(1);
	checkEq("SHIFT + step selects an earlier step while recording", rig.panel.editStep, 0);

	rig.tapKey(4);
	check("playing a key overwrites that step",
		std::fabs(rig.pattern.steps[0].pitch - (-1.f + 7.f / 12.f)) < 1e-5f, "now G");
	checkEq("overwriting does not advance", rig.panel.editStep, 0);

	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(0);
	rig.in.shift = false;
	rig.frames(1);

	// Key 6 is B. After leaving step edit, recording must continue and store it somewhere.
	const float b = -1.f + 11.f / 12.f;
	rig.tapKey(6);
	bool landed = false;
	for (int i = 0; i < MAX_STEPS; i++)
		landed = landed || std::fabs(rig.pattern.steps[i].pitch - b) < 1e-5f;
	check("step-write resumes after leaving step edit", landed, "note recorded");
	check("the resumed note did not overwrite the edited step",
		std::fabs(rig.pattern.steps[0].pitch - (-1.f + 7.f / 12.f)) < 1e-5f, "step 1 intact");
}

static void testRatchetAndSwing() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);

	rig.turnGlide(0.9f);
	checkEq("SHIFT + GLIDE sets a live ratchet", rig.panel.out.liveRatchet, 4);

	rig.turnTempo(0.75f);
	check("SHIFT + TEMPO sets swing amount",
		std::fabs(rig.pattern.timing.swingAmount - 0.5f) < 1e-4f, "+50%");
}

/** After SHIFT borrows TEMPO for swing, tempo must not jump when SHIFT is released. */
static void testTempoCatchUp() {
	Rig rig;
	rig.in.tempoKnob = 0.5f;
	rig.frames(2);
	const float before = rig.panel.out.bpm;

	rig.in.shift = true;
	rig.frames(1);
	rig.turnTempo(0.9f);
	rig.in.shift = false;
	rig.frames(2);

	check("tempo holds its value after SHIFT releases the knob",
		std::fabs(rig.panel.out.bpm - before) < 0.01f, "no jump");

	rig.turnTempo(0.5f);
	check("tempo reconnects once the knob crosses back",
		std::fabs(rig.panel.out.bpm - before) < 0.01f, "caught up");

	rig.turnTempo(0.2f);
	check("tempo follows the knob again", rig.panel.out.bpm < before - 10.f, "following");
}

static void testLiveHolds() {
	Rig rig;
	rig.in.shift = true;
	rig.in.resetAccent = true;
	rig.frames(1);
	check("SHIFT + ACCENT accents every step while held", rig.panel.out.liveAccent, "live accent");

	rig.in.resetAccent = false;
	rig.in.holdRest = true;
	rig.frames(1);
	check("SHIFT + REST mutes while held", rig.panel.out.liveMute, "live mute");

	rig.in.shift = false;
	rig.frames(1);
	check("REST alone holds the step instead", rig.panel.out.hold, "hold");
	check("holding does not mute", !rig.panel.out.liveMute, "not muted");
}

static void testPlaybackOrder() {
	Rig rig;
	rig.in.kb = true;
	rig.in.step = true;
	rig.frames(1);
	rig.tapKey(2);
	rig.in.kb = false;
	rig.in.step = false;
	rig.frames(2);

	check("KB + STEP + key 3 selects pendulum", rig.sequencer.order == PlaybackOrder::PENDULUM, "pendulum");
	checkEq("the combo leaves the octave alone", rig.octave, 4);
}

static void testPatternAndBankSelect() {
	Rig rig;
	rig.memory.at(0, 3).endStep = 11;
	rig.memory.at(5, 3).endStep = 24;

	rig.in.patternButton = true;
	rig.frames(1);
	rig.tapKey(3);
	checkEq("PATTERN + key selects the pattern", rig.index, 3);
	checkEq("selecting a pattern loads it", rig.pattern.endStep, 11);

	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(5);
	checkEq("SHIFT + PATTERN + key selects the bank", rig.bank, 5);
	checkEq("selecting a bank loads that pattern", rig.pattern.endStep, 24);
}

static void testInitializeAndRestore() {
	Rig rig;
	rig.pattern.steps[0].accent = true;
	rig.pattern.endStep = 20;
	rig.memory.at(0, 0) = rig.pattern;

	rig.pattern.steps[0].accent = false;
	rig.pattern.endStep = 3;

	rig.in.patternButton = true;
	rig.frames(1);
	rig.tap(rig.in.resetAccent);
	check("PATTERN + RESET restores from memory", rig.pattern.steps[0].accent, "restored");
	checkEq("restore brings back the length", rig.pattern.endStep, 20);

	rig.in.shift = true;
	rig.frames(1);
	rig.tap(rig.in.resetAccent);
	check("SHIFT + PATTERN + RESET initializes", !rig.pattern.steps[0].accent, "initialized");
	checkEq("an initialized pattern is 16 steps", rig.pattern.endStep, 15);
}

static void testSaveFlow() {
	Rig rig;
	rig.pattern.endStep = 9;

	rig.in.shift = true;
	rig.in.runStop = true;
	rig.frames(70); // longer than the one second hold
	check("holding SHIFT + RUN/STOP starts saving", rig.panel.saving, "saving");

	rig.in.runStop = false;
	rig.frames(2);
	check("the hold itself does not toggle the transport", !rig.sequencer.running, "not running");

	rig.tapKey(6);
	checkEq("a key picks the save location", rig.panel.saveIndex, 6);

	rig.tap(rig.in.runStop);
	check("SHIFT + RUN/STOP commits the save", !rig.panel.saving, "saved");
	checkEq("the pattern lands in the chosen slot", rig.memory.at(0, 6).endStep, 9);
	checkEq("the current location follows the save", rig.index, 6);
}

static void testSaveCancel() {
	Rig rig;
	rig.in.shift = true;
	rig.in.runStop = true;
	rig.frames(70);
	rig.in.runStop = false;
	rig.frames(2);

	rig.in.shift = false;
	rig.frames(1);
	rig.tap(rig.in.runStop);
	check("RUN/STOP alone cancels the save", !rig.panel.saving, "cancelled");
	checkEq("nothing was written", rig.memory.at(0, 0).endStep, 15);
}

static void testStepModeRestToggleAndRotate() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tap(rig.in.step);
	rig.in.shift = false;
	rig.frames(1);

	rig.tapKey(3);
	check("a step button toggles rest in step mode", rig.pattern.steps[3].rest, "rested");
	rig.tapKey(3);
	check("pressing it again clears the rest", !rig.pattern.steps[3].rest, "cleared");

	rig.pattern.steps[0].accent = true;
	rig.tap(rig.in.step);
	check("STEP rotates the pattern right", rig.pattern.steps[1].accent, "rotated");
	rig.tap(rig.in.kb);
	check("KB rotates it back", rig.pattern.steps[0].accent, "rotated back");
}

static void testSetEndStep() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(WHITE_KEYS + 4); // SET END
	check("SHIFT + SET END arms end-step selection", rig.panel.setEndArmed, "armed");

	rig.tapKey(5);
	checkEq("the next step button becomes the end step", rig.pattern.endStep, 5);
	check("the arm clears after use", !rig.panel.setEndArmed, "cleared");
}

/** Odd pattern lengths have to actually loop at the end step. */
static void testSevenStepPattern() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(WHITE_KEYS + 4); // SET END
	rig.tapKey(6);              // step 7
	rig.in.shift = false;
	rig.frames(1);

	checkEq("SET END then step 7 gives a seven step pattern", rig.pattern.length(), 7);
	checkEq("the end step is step 7", rig.pattern.endStep + 1, 7);

	rig.sequencer.clock.bpm = 300.f;
	rig.sequencer.start();
	rig.sequencer.reset();

	std::vector<int> visited;
	const float dt = 1.f / 48000.f;
	for (int i = 0; i < 48000 * 4 && int(visited.size()) < 9; i++) {
		rig.sequencer.process(rig.pattern, dt);
		if (rig.sequencer.clock.stepAdvanced)
			visited.push_back(rig.sequencer.currentStep);
	}

	const std::vector<int> expected = {1, 2, 3, 4, 5, 6, 0, 1, 2};
	check("it plays steps 1 to 7 then wraps to step 1", visited == expected, "wrapped after seven");
}

/** White keys address steps; all thirteen keys are available as notes for whichever step is selected. */
static void testBlackKeyNoteOnLastStep() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(WHITE_KEYS + 4); // SET END
	rig.tapKey(6);              // last step is step 7

	rig.tapKey(6);              // SHIFT + step 7 selects it for editing
	rig.in.shift = false;
	rig.frames(1);
	checkEq("the last step can be selected for editing", rig.panel.editStep, 6);

	rig.tapKey(WHITE_KEYS + 0); // C#
	check("a black key sets a sharp on the last step",
		std::fabs(rig.pattern.steps[6].pitch - (-1.f + 1.f / 12.f)) < 1e-5f, "C sharp");

	rig.tapKey(WHITE_KEYS + 4); // A#
	check("every black key works as a note",
		std::fabs(rig.pattern.steps[6].pitch - (-1.f + 10.f / 12.f)) < 1e-5f, "A sharp");
	checkEq("entering a note there does not change the length", rig.pattern.length(), 7);
}

static void testPageSelect() {
	Rig rig;
	rig.in.shift = true;
	rig.frames(1);
	rig.tapKey(WHITE_KEYS + 2); // 17-24
	checkEq("SHIFT + page button selects page 3", rig.panel.page, 2);
	check("selecting a page stops it chasing the playhead", !rig.panel.pageChasing, "paused");

	rig.tapKey(WHITE_KEYS + 2);
	check("pressing the same page resumes chasing", rig.panel.pageChasing, "chasing");
}

static void testKeysPlayNotesInKbMode() {
	Rig rig;
	rig.in.keys[5] = true;
	rig.frames(1);
	checkEq("a key with no modifier is a note", rig.panel.out.noteKey, 5);

	rig.in.keys[5] = false;
	rig.in.shift = true;
	rig.in.keys[5] = true;
	rig.frames(1);
	checkEq("a key under SHIFT is a command, not a note", rig.panel.out.noteKey, -1);
}

int main() {
	random::init();

	printf("\nM32 panel control checks\n\n");
	testModeSwitching();
	testArrowTapsChangeOctave();
	testHeldArrowDoesNotChangeOctave();
	testClockDivisionAndSwingInterval();
	testRecordStepWrite();
	testRunStopLeavesRecordMode();
	testGateLengthCarriesForward();
	testStepSelectAndEdit();
	testChangeNoteDuringRecord();
	testRatchetAndSwing();
	testTempoCatchUp();
	testLiveHolds();
	testPlaybackOrder();
	testPatternAndBankSelect();
	testInitializeAndRestore();
	testSaveFlow();
	testSaveCancel();
	testStepModeRestToggleAndRotate();
	testSetEndStep();
	testSevenStepPattern();
	testBlackKeyNoteOnLastStep();
	testPageSelect();
	testKeysPlayNotesInKbMode();

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
