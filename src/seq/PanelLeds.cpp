#include "PanelLeds.hpp"

namespace m32 {

static constexpr int OCTAVE_LEDS_HALF = OCTAVE_LEDS / 2;

/** Save and cancel sweep inward from both ends; initialize sweeps right to left (p38, p39). */
static bool fillAnimation(const PanelControl& panel, LedColour* octave) {
	if (panel.animation == PanelControl::Animation::NONE)
		return false;

	const float progress = clamp(panel.animationProgress(), 0.f, 1.f);
	const bool initializing = panel.animation == PanelControl::Animation::INITIALIZED;
	const LedColour lit = panel.animation == PanelControl::Animation::SAVED
		? LedColour::greenOn() : LedColour::redOn();

	for (int i = 0; i < OCTAVE_LEDS; i++) {
		const float reached = initializing
			? float(OCTAVE_LEDS - 1 - i) / float(OCTAVE_LEDS - 1)
			: float(std::min(i, OCTAVE_LEDS - 1 - i)) / float(OCTAVE_LEDS_HALF - 1);
		octave[i] = progress >= reached ? lit : LedColour::off();
	}
	return true;
}

/** Gate length and ratchet count as a bar; clock division and swing interval as one lit LED. */
static bool fillReadout(const PanelControl& panel, LedColour* octave) {
	if (panel.readout == Readout::NONE)
		return false;

	const int value = panel.readoutValue;
	for (int i = 0; i < OCTAVE_LEDS; i++) {
		if (panel.readout == Readout::GATE_LENGTH)
			octave[i] = i < value ? LedColour::redOn() : LedColour::off();
		else if (panel.readout == Readout::RATCHET)
			octave[i] = i < value ? LedColour::yellow() : LedColour::off();
		else if (panel.readout == Readout::CLOCK_DIVISION)
			octave[i] = i == value ? LedColour::yellow() : LedColour::off();
		else
			octave[i] = i == value ? LedColour::greenOn() : LedColour::off();
	}
	return true;
}

/** Saving blinks the destination green, or shows the destination bank in yellow (p31, p38). */
static bool fillSaving(const LedInputs& in, LedColour* octave) {
	const PanelControl& panel = *in.panel;
	if (!panel.saving)
		return false;

	const bool showBank = panel.patternHeld;
	const int index = showBank ? panel.saveBank : panel.saveIndex;
	const bool on = in.blinkPhase < 0.5f;
	octave[index] = showBank ? LedColour::yellow() : (on ? LedColour::greenOn() : LedColour::off());
	return true;
}

/** Bank numbers are yellow, pattern numbers green, so the two can be told apart (p37). */
static bool fillPatternSelect(const LedInputs& in, LedColour* octave) {
	const PanelControl& panel = *in.panel;
	if (!panel.patternHeld)
		return false;

	const bool showBank = panel.shiftHeld;
	octave[showBank ? in.bank : in.patternIndex] = showBank ? LedColour::yellow() : LedColour::greenOn();
	return true;
}

/** Step mode shows the per-step flags in yellow; keyboard mode shows them in green (p36, p42). */
static void fillStepFlags(const LedInputs& in, LedColour* octave) {
	const PanelControl& panel = *in.panel;
	if (panel.editStep < 0)
		return;

	const Step& step = in.pattern->steps[panel.editStep];
	const LedColour on = panel.mode == SeqMode::STEP ? LedColour::yellow() : LedColour::greenOn();

	if (step.glide)
		octave[4] = on;
	if (step.ratchet > MIN_RATCHET)
		octave[5] = on;
	if (step.accent)
		octave[6] = on;
	if (step.rest)
		octave[7] = on;
}

/** Page on LEDs 1-4 and the keyboard octave in red, sharing an LED by blinking between the two. */
static void fillStanding(const LedInputs& in, LedColour* octave) {
	const PanelControl& panel = *in.panel;
	const bool stepMode = panel.mode == SeqMode::STEP;

	// Step mode drops the red octave LED unless a step is being edited (p32, p36).
	const bool showOctave = !stepMode || panel.editStep >= 0;
	const bool showPage = stepMode || panel.recording || panel.editStep >= 0;

	if (showPage)
		octave[panel.page] = stepMode ? LedColour::yellow() : LedColour::greenOn();

	fillStepFlags(in, octave);

	if (!showOctave)
		return;

	const int index = clamp(in.octave - 1, 0, OCTAVE_LEDS - 1);
	const bool shared = showPage && index == panel.page;
	if (!shared) {
		octave[index].red = 1.f;
		return;
	}
	// Sharing an LED with the page indicator makes it alternate between the two colours (p27).
	octave[index] = in.blinkPhase < 0.5f ? LedColour::redOn() : LedColour::greenOn();
}

static void fillOctaveRow(const LedInputs& in, LedColour* octave) {
	for (int i = 0; i < OCTAVE_LEDS; i++)
		octave[i] = LedColour::off();

	if (fillAnimation(*in.panel, octave))
		return;
	if (fillReadout(*in.panel, octave))
		return;
	if (fillSaving(in, octave))
		return;
	if (fillPatternSelect(in, octave))
		return;
	fillStanding(in, octave);
}

static void fillStepRow(const LedInputs& in, float* step) {
	const PanelControl& panel = *in.panel;
	const bool stepMode = panel.mode == SeqMode::STEP;
	const bool editing = panel.editStep >= 0;

	for (int i = 0; i < WHITE_KEYS; i++) {
		const int index = panel.page * WHITE_KEYS + i;
		step[i] = 0.f;

		if (index == panel.editStep) {
			step[i] = in.blinkPhase < 0.5f ? 1.f : 0.f;
			continue;
		}
		// Editing in step mode turns every other step LED off (p44).
		if (stepMode && editing)
			continue;
		if (index == in.pattern->endStep) {
			step[i] = (in.halfRate && in.blinkPhase < 0.5f) ? 1.f : 0.f;
			continue;
		}
		if (index > in.pattern->endStep)
			continue;
		if (in.sequencer->running && index == in.sequencer->currentStep) {
			step[i] = 1.f;
			continue;
		}
		// Step mode shows rest status; keyboard mode shows which steps hold data (p30, p36).
		const Step& data = in.pattern->steps[index];
		const bool lit = stepMode ? !data.rest : data.written;
		step[i] = lit ? 0.35f : 0.f;
	}
}

/** Yellow while recording, alternating while editing a step, blinking red while running (p27, p33). */
static void fillTempoLed(const LedInputs& in, LedColour& tempo) {
	const PanelControl& panel = *in.panel;

	if (panel.recording) {
		tempo = LedColour::yellow();
		return;
	}
	if (panel.editStep >= 0) {
		tempo = in.blinkPhase < 0.5f ? LedColour::yellow() : LedColour::redOn();
		return;
	}
	if (in.sequencer->running) {
		tempo = in.blinkPhase < 0.5f ? LedColour::redOn() : LedColour::off();
		return;
	}
	tempo = LedColour::off();
}

void computeLeds(const LedInputs& in, LedState& out) {
	fillOctaveRow(in, out.octave);
	fillStepRow(in, out.step);
	fillTempoLed(in, out.tempo);
}

} // namespace m32
