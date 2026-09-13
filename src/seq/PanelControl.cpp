#include "PanelControl.hpp"

namespace m32 {

/** White keys are C D E F G A B C, black keys C# D# F# G# A#. */
static const int WHITE_SEMITONES[WHITE_KEYS] = {0, 2, 4, 5, 7, 9, 11, 12};
static const int BLACK_SEMITONES[BLACK_KEYS] = {1, 3, 6, 8, 10};

int keySemitone(int key) {
	if (key < WHITE_KEYS)
		return WHITE_SEMITONES[key];
	return BLACK_SEMITONES[key - WHITE_KEYS];
}

static float bpmFromKnob(float knob) {
	return rescale(knob, 0.f, 1.f, Clock::MIN_BPM, Clock::MAX_BPM);
}

/** KB gives dotted values, STEP triplet, both together straight (p20, p54). */
static NoteForm formForArrows(bool kb, bool step) {
	if (kb && step)
		return NoteForm::STRAIGHT;
	return kb ? NoteForm::DOTTED : NoteForm::TRIPLET;
}

/** Splits a knob's travel into n equal detents. */
static int detent(float knob, int count) {
	return clamp(int(knob * float(count)), 0, count - 1);
}

void PanelControl::process(const PanelInput& in, const PanelTarget& t) {
	out.noteKey = -1;
	out.audition = false;
	out.liveRatchet = 0;
	shiftHeld = in.shift;
	patternHeld = in.patternButton;
	kbHeld = in.kb;
	stepHeld = in.step;
	advanceTimers(in.dt);

	processKnobs(in, t);
	processLiveHolds(in);
	processRunStop(in, t);
	processResetAccent(in, t);
	processHoldRest(in, t);
	processKeys(in, t);
	processArrows(in, t);

	if (pageChasing && t.sequencer->running && editStep < 0)
		page = t.sequencer->currentStep / WHITE_KEYS;

	previous = in;
}

void PanelControl::processKnobs(const PanelInput& in, const PanelTarget& t) {
	processGlideKnob(in, t);
	processTempoKnob(in, t);
}

/** GLIDE means swing interval under the arrows, ratchet under SHIFT, and glide otherwise. */
void PanelControl::processGlideKnob(const PanelInput& in, const PanelTarget& t) {
	const float knob = in.glideKnob;
	const bool moved = knob != previous.glideKnob;

	if (in.kb || in.step) {
		glide.release(knob);
		if (!moved)
			return;
		t.pattern->timing.swingInterval = NoteLength(detent(knob, 8));
		t.pattern->timing.swingForm = formForArrows(in.kb, in.step);
		showReadout(Readout::SWING_INTERVAL, detent(knob, 8));
		kbConsumed = kbConsumed || in.kb;
		stepConsumed = stepConsumed || in.step;
		return;
	}

	if (in.shift) {
		glide.release(knob);
		if (moved) {
			heldRatchet = 1 + detent(knob, MAX_RATCHET);
			showReadout(Readout::RATCHET, heldRatchet);
		}
		if (editStep >= 0 && moved)
			t.pattern->steps[editStep].ratchet = heldRatchet;
		if (editStep < 0)
			out.liveRatchet = heldRatchet;
		return;
	}

	heldRatchet = 0;

	out.glideTime = glide.process(knob);
	// Glide rate is never per-step. Turning the knob also sets the selected step's glide flag,
	// but merely selecting a step must not overwrite what it already holds.
	if (editStep >= 0 && moved)
		t.pattern->steps[editStep].glide = out.glideTime > 0.01f;
}

/** TEMPO sets the clock division under the arrows, swing amount under SHIFT, gate length while
editing a step, and tempo otherwise (p54). */
void PanelControl::processTempoKnob(const PanelInput& in, const PanelTarget& t) {
	const float knob = in.tempoKnob;

	if (in.kb || in.step) {
		tempo.release(knob);
		if (knob != previous.tempoKnob) {
			t.pattern->timing.clockDivision = NoteLength(detent(knob, 8));
			t.pattern->timing.clockForm = formForArrows(in.kb, in.step);
			showReadout(Readout::CLOCK_DIVISION, detent(knob, 8));
			kbConsumed = kbConsumed || in.kb;
			stepConsumed = stepConsumed || in.step;
		}
		out.bpm = bpmFromKnob(tempo.value);
		return;
	}

	if (in.shift) {
		tempo.release(knob);
		if (knob != previous.tempoKnob)
			t.pattern->timing.swingAmount = knob * 2.f - 1.f;
		out.bpm = bpmFromKnob(tempo.value);
		return;
	}

	if (editStep >= 0) {
		tempo.release(knob);
		if (knob != previous.tempoKnob) {
			t.pattern->steps[editStep].gateLength = 1 + detent(knob, TIE_GATE_LENGTH);
			showReadout(Readout::GATE_LENGTH, t.pattern->steps[editStep].gateLength);
		}
		out.bpm = bpmFromKnob(tempo.value);
		return;
	}

	out.bpm = bpmFromKnob(tempo.process(knob));
}

void PanelControl::processLiveHolds(const PanelInput& in) {
	const bool editing = editStep >= 0;
	out.liveAccent = in.shift && in.resetAccent && !editing;
	out.liveMute = in.shift && in.holdRest && !editing;
	out.hold = in.holdRest && !in.shift && !editing;
}

void PanelControl::processRunStop(const PanelInput& in, const PanelTarget& t) {
	if (pressed(in.runStop, previous.runStop))
		runStopConsumed = false;

	if (in.runStop) {
		if (!in.shift)
			return;
		runStopHeldFor += in.dt;
		if (saving || runStopHeldFor < SAVE_HOLD_SECONDS)
			return;
		saving = true;
		saveBank = *t.bank;
		saveIndex = *t.patternIndex;
		runStopConsumed = true;
		return;
	}

	const bool released = previous.runStop;
	runStopHeldFor = 0.f;
	if (!released || runStopConsumed)
		return;

	if (saving) {
		if (in.shift)
			commitSave(t);
		else {
			saving = false;
			startAnimation(Animation::CANCELLED);
		}
		return;
	}

	if (in.kb && in.step) {
		armedForExternalClock = true;
		kbConsumed = true;
		stepConsumed = true;
		return;
	}

	if (in.shift) {
		if (mode != SeqMode::KB)
			return;
		recording = !recording;
		stepWrite = recording;
		editStep = recording ? 0 : -1;
		pendingAdvance = false;
		page = recording ? 0 : page;
		if (recording)
			t.sequencer->stop();
		return;
	}

	// RUN/STOP on its own leaves record mode rather than starting the transport (p27).
	if (recording) {
		recording = false;
		stepWrite = false;
		editStep = -1;
		return;
	}

	t.sequencer->running ? t.sequencer->stop() : t.sequencer->start();
}

void PanelControl::processResetAccent(const PanelInput& in, const PanelTarget& t) {
	if (!pressed(in.resetAccent, previous.resetAccent))
		return;

	if (editStep >= 0) {
		Step& step = t.pattern->steps[editStep];
		step.accent = !step.accent;
		return;
	}
	if (in.shift && in.patternButton) {
		t.pattern->initialize();
		startAnimation(Animation::INITIALIZED);
		return;
	}
	if (in.patternButton) {
		loadPattern(t);
		return;
	}
	if (in.shift)
		return;

	t.sequencer->reset();
}

void PanelControl::processHoldRest(const PanelInput& in, const PanelTarget& t) {
	if (!pressed(in.holdRest, previous.holdRest))
		return;
	if (editStep < 0)
		return;

	if (stepWrite)
		takePendingAdvance(t);

	Step& step = t.pattern->steps[editStep];
	step.rest = !step.rest;
	step.written = true;
	pendingAdvance = stepWrite;
}

/** Keys are notes in KB mode and while editing a step; otherwise they are panel commands. */
bool PanelControl::keysAreNotes(const PanelInput& in) const {
	if (in.shift || in.patternButton || setEndArmed || saving)
		return false;
	if (in.kb && in.step)
		return false;
	if (editStep >= 0)
		return true;
	return mode == SeqMode::KB;
}

void PanelControl::processKeys(const PanelInput& in, const PanelTarget& t) {
	const bool notes = keysAreNotes(in);

	for (int i = 0; i < KEYBOARD_KEYS; i++) {
		if (!pressed(in.keys[i], previous.keys[i]))
			continue;
		if (notes)
			writeNote(i, in, t);
		else if (i < WHITE_KEYS)
			handleWhiteKey(i, in, t);
		else
			handleBlackKey(i - WHITE_KEYS, in, t);
	}

	if (!notes)
		return;
	for (int i = KEYBOARD_KEYS - 1; i >= 0; i--) {
		if (in.keys[i]) {
			out.noteKey = i;
			return;
		}
	}
}

bool PanelControl::handleWhiteKey(int index, const PanelInput& in, const PanelTarget& t) {
	if (in.kb && in.step) {
		if (index < 4)
			t.sequencer->order = PlaybackOrder(index);
		kbConsumed = true;
		stepConsumed = true;
		return true;
	}
	if (saving && in.patternButton) {
		saveBank = index;
		return true;
	}
	if (saving) {
		saveIndex = index;
		return true;
	}
	if (setEndArmed) {
		t.pattern->endStep = clamp(absoluteStep(index), 0, MAX_STEPS - 1);
		setEndArmed = false;
		return true;
	}
	if (in.shift && in.patternButton) {
		*t.bank = index;
		loadPattern(t);
		return true;
	}
	if (in.patternButton) {
		*t.patternIndex = index;
		loadPattern(t);
		return true;
	}
	if (in.shift) {
		selectStepForEdit(absoluteStep(index), t);
		return true;
	}
	if (mode == SeqMode::STEP) {
		Step& step = t.pattern->steps[absoluteStep(index)];
		step.rest = !step.rest;
		return true;
	}
	return false;
}

bool PanelControl::handleBlackKey(int index, const PanelInput& in, const PanelTarget& t) {
	// KB mode reaches the page selectors through SHIFT; STEP mode uses them directly.
	if (mode == SeqMode::KB && !in.shift)
		return false;

	if (index == SET_END_KEY - WHITE_KEYS) {
		setEndArmed = true;
		return true;
	}
	selectPage(index);
	return true;
}

void PanelControl::processArrows(const PanelInput& in, const PanelTarget& t) {
	if (pressed(in.kb, previous.kb))
		kbConsumed = false;
	if (pressed(in.step, previous.step))
		stepConsumed = false;

	if (in.shift) {
		if (pressed(in.kb, previous.kb)) {
			mode = SeqMode::KB;
			kbConsumed = true;
		}
		if (pressed(in.step, previous.step)) {
			mode = SeqMode::STEP;
			recording = false;
			stepWrite = false;
			stepConsumed = true;
		}
		return;
	}

	if (!in.kb && previous.kb && !kbConsumed)
		soloArrow(-1, in, t);
	if (!in.step && previous.step && !stepConsumed)
		soloArrow(1, in, t);
}

/** A tapped arrow moves the octave, rotates the pattern, or steps through save locations. */
void PanelControl::soloArrow(int direction, const PanelInput& in, const PanelTarget& t) {
	if (saving && in.patternButton) {
		saveBank = clamp(saveBank + direction, 0, BANKS - 1);
		return;
	}
	if (saving) {
		saveIndex = clamp(saveIndex + direction, 0, PATTERNS_PER_BANK - 1);
		return;
	}
	if (in.patternButton) {
		*t.patternIndex = clamp(*t.patternIndex + direction, 0, PATTERNS_PER_BANK - 1);
		loadPattern(t);
		return;
	}
	if (mode == SeqMode::STEP) {
		rotatePattern(t, direction);
		return;
	}
	*t.octave = clamp(*t.octave + direction, MIN_OCTAVE, MAX_OCTAVE);
}

void PanelControl::showReadout(Readout which, int value) {
	readout = which;
	readoutValue = value;
	readoutTimer = READOUT_SECONDS;
}

void PanelControl::startAnimation(Animation which) {
	animation = which;
	animationTime = ANIMATION_SECONDS;
}

void PanelControl::advanceTimers(float dt) {
	readoutTimer = std::max(readoutTimer - dt, 0.f);
	if (readoutTimer <= 0.f)
		readout = Readout::NONE;

	animationTime = std::max(animationTime - dt, 0.f);
	if (animationTime <= 0.f)
		animation = Animation::NONE;
}

void PanelControl::selectPage(int newPage) {
	if (page == newPage && !pageChasing) {
		pageChasing = true;
		return;
	}
	page = clamp(newPage, 0, PAGES - 1);
	pageChasing = false;
}

void PanelControl::selectStepForEdit(int step, const PanelTarget& t) {
	if (editStep == step) {
		// Record mode always keeps a step selected, so leaving step edit resumes step-write
		// from there rather than clearing the cursor and silently dropping notes.
		stepWrite = recording;
		pendingAdvance = recording;
		if (!recording)
			editStep = -1;
		return;
	}

	editStep = clamp(step, 0, MAX_STEPS - 1);
	stepWrite = false;
	if (t.sequencer->running)
		return;

	out.audition = true;
	out.auditionPitch = t.pattern->steps[editStep].pitch;
}

void PanelControl::writeNote(int key, const PanelInput& in, const PanelTarget& t) {
	if (editStep < 0)
		return;

	takePendingAdvance(t);

	Step& step = t.pattern->steps[editStep];
	step.pitch = float(*t.octave - OCTAVE_AT_ZERO_VOLTS) + keySemitone(key) / 12.f;
	step.rest = false;
	step.written = true;

	pendingAdvance = stepWrite;
}

/** Moves to the next step only once another note or rest arrives. */
void PanelControl::takePendingAdvance(const PanelTarget& t) {
	if (!stepWrite || !pendingAdvance)
		return;
	pendingAdvance = false;
	advanceWriteCursor(t);
}

/** Step-write carries the gate length forward, so a run of steps shares one setting. */
void PanelControl::advanceWriteCursor(const PanelTarget& t) {
	const int next = std::min(editStep + 1, MAX_STEPS - 1);
	t.pattern->steps[next].gateLength = t.pattern->steps[editStep].gateLength;
	editStep = next;
	page = editStep / WHITE_KEYS;
}

void PanelControl::loadPattern(const PanelTarget& t) {
	*t.pattern = t.memory->at(*t.bank, *t.patternIndex);
	editStep = -1;
	stepWrite = recording;
}

void PanelControl::commitSave(const PanelTarget& t) {
	startAnimation(Animation::SAVED);
	t.memory->at(saveBank, saveIndex) = *t.pattern;
	*t.bank = saveBank;
	*t.patternIndex = saveIndex;
	saving = false;
}

void PanelControl::rotatePattern(const PanelTarget& t, int direction) {
	const int length = t.pattern->length();
	if (length <= 1)
		return;

	Step rotated[MAX_STEPS];
	for (int i = 0; i < length; i++) {
		const int source = ((i - direction) % length + length) % length;
		rotated[i] = t.pattern->steps[source];
	}
	for (int i = 0; i < length; i++)
		t.pattern->steps[i] = rotated[i];
}

json_t* PanelControl::toJson() const {
	json_t* j = json_object();
	json_object_set_new(j, "mode", json_integer(int(mode)));
	json_object_set_new(j, "recording", json_boolean(recording));
	json_object_set_new(j, "editStep", json_integer(editStep));
	json_object_set_new(j, "page", json_integer(page));
	json_object_set_new(j, "pageChasing", json_boolean(pageChasing));
	return j;
}

void PanelControl::fromJson(json_t* j) {
	if (!j)
		return;

	json_t* modeJ = json_object_get(j, "mode");
	if (modeJ)
		mode = SeqMode(clamp(int(json_integer_value(modeJ)), 0, 1));

	json_t* recordingJ = json_object_get(j, "recording");
	if (recordingJ)
		recording = json_boolean_value(recordingJ);

	json_t* editJ = json_object_get(j, "editStep");
	if (editJ)
		editStep = clamp(int(json_integer_value(editJ)), -1, MAX_STEPS - 1);

	json_t* pageJ = json_object_get(j, "page");
	if (pageJ)
		page = clamp(int(json_integer_value(pageJ)), 0, PAGES - 1);

	json_t* chasingJ = json_object_get(j, "pageChasing");
	if (chasingJ)
		pageChasing = json_boolean_value(chasingJ);

	stepWrite = recording && editStep >= 0;
}

} // namespace m32
