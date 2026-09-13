#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

static constexpr int MAX_STEPS = 32;
static constexpr int BANKS = 8;
static constexpr int PATTERNS_PER_BANK = 8;
static constexpr int PATTERN_SLOTS = BANKS * PATTERNS_PER_BANK;

/** Gate length runs 1/8 to 8/8 of a step. The longest value ties the note into the next step. */
static constexpr int MIN_GATE_LENGTH = 1;
static constexpr int TIE_GATE_LENGTH = 8;

static constexpr int MIN_RATCHET = 1;
static constexpr int MAX_RATCHET = 4;

/** Defaults match an initialized hardware pattern: -1 V and a 50% gate. */
struct Step {
	float pitch = -1.f;
	int gateLength = 4;
	int ratchet = MIN_RATCHET;
	bool accent = false;
	bool rest = false;
	bool glide = false;
	bool written = false;

	bool isTied() const {
		return gateLength >= TIE_GATE_LENGTH;
	}

	json_t* toJson() const {
		json_t* j = json_object();
		json_object_set_new(j, "pitch", json_real(pitch));
		json_object_set_new(j, "gateLength", json_integer(gateLength));
		json_object_set_new(j, "ratchet", json_integer(ratchet));
		json_object_set_new(j, "accent", json_boolean(accent));
		json_object_set_new(j, "rest", json_boolean(rest));
		json_object_set_new(j, "glide", json_boolean(glide));
		json_object_set_new(j, "written", json_boolean(written));
		return j;
	}

	void fromJson(json_t* j) {
		if (!j)
			return;
		readReal(j, "pitch", pitch);
		readInt(j, "gateLength", gateLength, MIN_GATE_LENGTH, TIE_GATE_LENGTH);
		readInt(j, "ratchet", ratchet, MIN_RATCHET, MAX_RATCHET);
		readBool(j, "accent", accent);
		readBool(j, "rest", rest);
		readBool(j, "glide", glide);
		readBool(j, "written", written);
	}

private:
	static void readReal(json_t* j, const char* key, float& out) {
		json_t* v = json_object_get(j, key);
		if (v)
			out = float(json_number_value(v));
	}
	static void readInt(json_t* j, const char* key, int& out, int lo, int hi) {
		json_t* v = json_object_get(j, key);
		if (v)
			out = clamp(int(json_integer_value(v)), lo, hi);
	}
	static void readBool(json_t* j, const char* key, bool& out) {
		json_t* v = json_object_get(j, key);
		if (v)
			out = json_boolean_value(v);
	}
};

/** Note lengths available to the clock division and the swing interval, longest first. */
enum class NoteLength { TWO_WHOLE, WHOLE, HALF, QUARTER, EIGHTH, SIXTEENTH, THIRTY_SECOND, SIXTY_FOURTH };
enum class NoteForm { STRAIGHT, DOTTED, TRIPLET };

/** Length in quarter notes. */
float noteLengthBeats(NoteLength length, NoteForm form);

struct Timing {
	NoteLength clockDivision = NoteLength::SIXTEENTH;
	NoteForm clockForm = NoteForm::STRAIGHT;
	NoteLength swingInterval = NoteLength::EIGHTH;
	NoteForm swingForm = NoteForm::STRAIGHT;
	/** -1 pushes off-beats early, +1 pushes on-beats late, 0 is even. */
	float swingAmount = 0.f;

	json_t* toJson() const;
	void fromJson(json_t* j);
};

struct Pattern {
	Step steps[MAX_STEPS];
	int endStep = 15;
	Timing timing;

	void initialize() {
		*this = Pattern();
	}

	int length() const {
		return endStep + 1;
	}

	json_t* toJson() const;
	void fromJson(json_t* j);
};

/** The 64 saved slots, arranged as 8 banks of 8. */
struct PatternMemory {
	Pattern slots[PATTERN_SLOTS];

	Pattern& at(int bank, int index) {
		return slots[clamp(bank, 0, BANKS - 1) * PATTERNS_PER_BANK + clamp(index, 0, PATTERNS_PER_BANK - 1)];
	}

	json_t* toJson() const;
	void fromJson(json_t* j);
};

} // namespace m32
