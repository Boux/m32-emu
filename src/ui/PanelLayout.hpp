#pragma once

/** Every panel coordinate, in millimetres from the top left of the 60HP panel.
The widget and the panel artwork both work from these numbers. */
namespace m32 {
namespace layout {

struct Pos {
	float x, y;
};

static constexpr float PANEL_WIDTH = 304.8f;
static constexpr float PANEL_HEIGHT = 128.5f;

// Synth controls: three rows of eight columns.
static constexpr float COL_1 = 21.f;
static constexpr float COL_STEP = 26.f;
static constexpr float ROW_1 = 24.f;
static constexpr float ROW_2 = 50.f;
static constexpr float ROW_3 = 76.f;

static constexpr Pos synth(int col, float row) {
	return {COL_1 + COL_STEP * col, row};
}

// Row 1
static constexpr Pos FREQUENCY = synth(0, ROW_1);
static constexpr Pos VCO_WAVE = synth(1, ROW_1);
static constexpr Pos PULSE_WIDTH = synth(2, ROW_1);
static constexpr Pos MIX = synth(3, ROW_1);
static constexpr Pos CUTOFF = synth(4, ROW_1);
static constexpr Pos RESONANCE = synth(5, ROW_1);
static constexpr Pos VCA_MODE = synth(6, ROW_1);
static constexpr Pos VOLUME = synth(7, ROW_1);

// Row 2
static constexpr Pos GLIDE = synth(0, ROW_2);
static constexpr Pos VCO_MOD_SOURCE = synth(1, ROW_2);
static constexpr Pos VCO_MOD_AMOUNT = synth(2, ROW_2);
static constexpr Pos VCO_MOD_DEST = synth(3, ROW_2);
static constexpr Pos VCF_MODE = synth(4, ROW_2);
static constexpr Pos VCF_MOD_SOURCE = synth(5, ROW_2);
static constexpr Pos VCF_MOD_AMOUNT = synth(6, ROW_2);
static constexpr Pos VCF_MOD_POLARITY = synth(7, ROW_2);

// Row 3
static constexpr Pos TEMPO = synth(0, ROW_3);
static constexpr Pos LFO_RATE = synth(2, ROW_3);
static constexpr Pos LFO_WAVE = synth(3, ROW_3);
static constexpr Pos ATTACK = synth(4, ROW_3);
static constexpr Pos SUSTAIN = synth(5, ROW_3);
static constexpr Pos DECAY = synth(6, ROW_3);
static constexpr Pos VC_MIX = synth(7, ROW_3);

// Patchbay: four columns of eight rows, laid out as on the hardware.
static constexpr float JACK_COL_1 = 229.f;
static constexpr float JACK_COL_STEP = 19.f;
static constexpr float JACK_ROW_1 = 17.5f;
static constexpr float JACK_ROW_STEP = 13.6f;
static constexpr float JACK_LABEL_OFFSET = 5.6f;

static constexpr Pos jack(int col, int row) {
	return {JACK_COL_1 + JACK_COL_STEP * col, JACK_ROW_1 + JACK_ROW_STEP * row};
}

// Sequencer transport
static constexpr Pos HOLD_REST = {22.f, 99.f};
static constexpr Pos RESET_ACCENT = {22.f, 112.f};
static constexpr Pos TEMPO_LED = {37.f, 99.f};
static constexpr Pos SHIFT = {37.f, 112.f};
static constexpr Pos PATTERN = {52.f, 99.f};
static constexpr Pos RUN_STOP = {52.f, 112.f};

// Octave / location LEDs and the two arrow buttons
static constexpr float OCTAVE_LED_1_X = 66.f;
static constexpr float OCTAVE_LED_STEP = 4.6f;
static constexpr float OCTAVE_LED_Y = 97.f;
static constexpr Pos KB_BUTTON = {72.f, 112.f};
static constexpr Pos STEP_BUTTON = {85.f, 112.f};

/** The 13 pads form a piano octave. White keys are also the step buttons;
black keys are also the four page selectors and SET END. */
static constexpr float WHITE_KEY_1_X = 108.f;
static constexpr float WHITE_KEY_STEP = 12.f;
static constexpr float WHITE_KEY_Y = 112.5f;
static constexpr float BLACK_KEY_Y = 97.5f;
static constexpr float STEP_LED_Y = 121.5f;

/** Black keys sit between their neighbouring white keys: C# D# then a gap, then F# G# A#. */
static constexpr int BLACK_KEY_LEFT_WHITE[5] = {0, 1, 3, 4, 5};

static constexpr float WHITE_KEY_W = 10.f;
static constexpr float WHITE_KEY_H = 12.f;
static constexpr float BLACK_KEY_W = 8.5f;
static constexpr float BLACK_KEY_H = 9.f;
static constexpr float TRANSPORT_BUTTON = 7.f;
static constexpr float ARROW_W = 9.f;
static constexpr float ARROW_H = 7.f;

static constexpr float whiteKeyX(int i) {
	return WHITE_KEY_1_X + WHITE_KEY_STEP * i;
}

static constexpr float blackKeyX(int i) {
	return whiteKeyX(BLACK_KEY_LEFT_WHITE[i]) + WHITE_KEY_STEP / 2.f;
}

} // namespace layout
} // namespace m32
