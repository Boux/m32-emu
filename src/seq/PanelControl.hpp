#pragma once
#include <rack.hpp>
#include "Pattern.hpp"
#include "Sequencer.hpp"

namespace m32 {

using namespace rack;

static constexpr int WHITE_KEYS = 8;
static constexpr int BLACK_KEYS = 5;
static constexpr int KEYBOARD_KEYS = WHITE_KEYS + BLACK_KEYS;
static constexpr int PAGES = 4;
static constexpr int OCTAVE_LEDS = 8;
/** The fifth black key is SET END rather than a page selector. */
static constexpr int SET_END_KEY = WHITE_KEYS + 4;

static constexpr int MIN_OCTAVE = 1;
static constexpr int MAX_OCTAVE = 8;
/** Low C at this octave is the transposition reference, so pressing it changes nothing. */
static constexpr int DEFAULT_OCTAVE = 4;
/** Octave 5 puts low C at 0 V, which the hardware calibrates to middle C. */
static constexpr int OCTAVE_AT_ZERO_VOLTS = 5;

int keySemitone(int key);

enum class SeqMode { KB, STEP };

/** Turning certain knobs lights the octave row with a readout for a moment (p25, p27, p20, p54). */
enum class Readout { NONE, GATE_LENGTH, RATCHET, CLOCK_DIVISION, SWING_INTERVAL };

/** A value that only starts following its knob again once the knob reaches it.
The hardware needs this because GLIDE and TEMPO each drive several values, so releasing a
modifier leaves the knob pointing somewhere that has nothing to do with what it now controls. */
struct CatchUpKnob {
	float value = 0.f;

	void takeOver(float knob) {
		engaged = true;
		lastKnob = knob;
		value = knob;
	}

	/** Call while the knob is assigned to something else, so it reconnects by crossing. */
	void release(float knob) {
		engaged = false;
		lastKnob = knob;
	}

	float process(float knob) {
		if (!engaged && crossed(knob))
			engaged = true;
		if (engaged)
			value = knob;
		lastKnob = knob;
		return value;
	}

private:
	bool engaged = true;
	float lastKnob = 0.f;

	bool crossed(float knob) const {
		if (std::fabs(knob - value) < 1e-4f)
			return true;
		return (lastKnob < value) != (knob < value);
	}
};

struct PanelInput {
	bool shift = false;
	bool patternButton = false;
	bool runStop = false;
	bool resetAccent = false;
	bool holdRest = false;
	bool kb = false;
	bool step = false;
	bool keys[KEYBOARD_KEYS] = {};
	float glideKnob = 0.f;
	float tempoKnob = 0.5f;
	float dt = 0.f;
};

struct PanelTarget {
	Pattern* pattern = nullptr;
	PatternMemory* memory = nullptr;
	Sequencer* sequencer = nullptr;
	int* bank = nullptr;
	int* patternIndex = nullptr;
	int* octave = nullptr;
};

struct PanelOutput {
	/** Key acting as a note right now: live play, transposition, or a step's pitch. */
	int noteKey = -1;
	/** Sound the selected step's stored pitch once, for monitoring while stopped. */
	bool audition = false;
	float auditionPitch = 0.f;

	bool liveAccent = false;
	bool liveMute = false;
	int liveRatchet = 0;
	bool hold = false;

	float glideTime = 0.f;
	float bpm = 120.f;
};

/** Turns held buttons and knob positions into pattern edits, exactly as the panel does.
Nothing here knows about mice; the widget is what decides when a button counts as held. */
struct PanelControl {
	SeqMode mode = SeqMode::KB;
	bool recording = false;
	/** Step selected for editing, or -1. */
	int editStep = -1;
	/** True while record mode advances the cursor on each note entered. */
	bool stepWrite = false;
	int page = 0;
	bool pageChasing = true;
	bool setEndArmed = false;
	bool armedForExternalClock = false;

	bool saving = false;
	int saveBank = 0;
	int saveIndex = 0;

	/** Latched modifier state, so the panel lights can read it without touching params. */
	bool shiftHeld = false;
	bool patternHeld = false;
	bool kbHeld = false;
	bool stepHeld = false;

	/** What the octave row is temporarily showing, and the value it is showing. */
	Readout readout = Readout::NONE;
	int readoutValue = 0;

	/** Counts down a save, cancel or initialize animation on the octave row. */
	enum class Animation { NONE, SAVED, CANCELLED, INITIALIZED };
	Animation animation = Animation::NONE;
	float animationTime = 0.f;

	/** 0 at the start of an animation, 1 at its end. */
	float animationProgress() const {
		return 1.f - animationTime / ANIMATION_SECONDS;
	}

	PanelOutput out;

	void process(const PanelInput& in, const PanelTarget& target);

	json_t* toJson() const;
	void fromJson(json_t* j);

	/** Holding SHIFT + RUN/STOP this long starts the save process instead of toggling record. */
	static constexpr float SAVE_HOLD_SECONDS = 1.f;
	/** How long a knob readout stays lit after the knob stops moving. */
	static constexpr float READOUT_SECONDS = 0.8f;
	static constexpr float ANIMATION_SECONDS = 0.6f;

private:

	PanelInput previous;
	float runStopHeldFor = 0.f;
	bool runStopConsumed = false;
	bool kbConsumed = false;
	bool stepConsumed = false;

	float readoutTimer = 0.f;

	CatchUpKnob glide;
	CatchUpKnob tempo;
	/** Live ratchet persists for as long as SHIFT is held, not just while the knob moves. */
	int heldRatchet = 0;
	/** Step-write advances on the next entry, not on the current one, so the step just played
	stays selected and its gate length, accent and rest can still be set. */
	bool pendingAdvance = false;

	bool pressed(bool now, bool before) const {
		return now && !before;
	}

	void processKnobs(const PanelInput& in, const PanelTarget& t);
	void processGlideKnob(const PanelInput& in, const PanelTarget& t);
	void processTempoKnob(const PanelInput& in, const PanelTarget& t);
	void processLiveHolds(const PanelInput& in);
	void processRunStop(const PanelInput& in, const PanelTarget& t);
	void processResetAccent(const PanelInput& in, const PanelTarget& t);
	void processHoldRest(const PanelInput& in, const PanelTarget& t);
	void processArrows(const PanelInput& in, const PanelTarget& t);
	void processKeys(const PanelInput& in, const PanelTarget& t);

	bool handleWhiteKey(int index, const PanelInput& in, const PanelTarget& t);
	bool handleBlackKey(int index, const PanelInput& in, const PanelTarget& t);
	bool keysAreNotes(const PanelInput& in) const;
	void soloArrow(int direction, const PanelInput& in, const PanelTarget& t);
	void advanceWriteCursor(const PanelTarget& t);
	void takePendingAdvance(const PanelTarget& t);

	void showReadout(Readout which, int value);
	void advanceTimers(float dt);
	void startAnimation(Animation which);
	void selectPage(int newPage);
	void selectStepForEdit(int step, const PanelTarget& t);
	void writeNote(int key, const PanelInput& in, const PanelTarget& t);
	void loadPattern(const PanelTarget& t);
	void commitSave(const PanelTarget& t);
	void rotatePattern(const PanelTarget& t, int direction);

	int absoluteStep(int index) const {
		return page * WHITE_KEYS + index;
	}
};

} // namespace m32
