#pragma once
#include <rack.hpp>
#include "PanelControl.hpp"

namespace m32 {

using namespace rack;

/** The octave row is red, green or yellow; yellow is both channels lit. */
struct LedColour {
	float red;
	float green;

	LedColour(float red = 0.f, float green = 0.f) : red(red), green(green) {}

	static LedColour off() { return LedColour(); }
	static LedColour redOn(float level = 1.f) { return LedColour(level, 0.f); }
	static LedColour greenOn(float level = 1.f) { return LedColour(0.f, level); }
	static LedColour yellow(float level = 1.f) { return LedColour(level, level); }
};

struct LedState {
	LedColour octave[OCTAVE_LEDS];
	float step[WHITE_KEYS] = {};
	LedColour tempo;
};

struct LedInputs {
	const PanelControl* panel = nullptr;
	const Pattern* pattern = nullptr;
	const Sequencer* sequencer = nullptr;
	int octave = DEFAULT_OCTAVE;
	int bank = 0;
	int patternIndex = 0;
	/** Position within the current step, 0 to 1. */
	float blinkPhase = 0.f;
	/** Alternates every step, for things the manual says blink at half the clock rate. */
	bool halfRate = false;
};

/** Reproduces the panel's LED language, which is the only state readout the hardware has. */
void computeLeds(const LedInputs& in, LedState& out);

} // namespace m32
