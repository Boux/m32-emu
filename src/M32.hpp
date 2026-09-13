#pragma once
#include <rack.hpp>
#include "dsp/Voice.hpp"
#include "seq/Sequencer.hpp"
#include "seq/PanelControl.hpp"

using namespace rack;

namespace m32 {

static constexpr int OCTAVE_LEDS = 8;

struct M32 : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		VCO_WAVE_PARAM,
		PULSE_WIDTH_PARAM,
		MIX_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		VCA_MODE_PARAM,
		VOLUME_PARAM,

		GLIDE_PARAM,
		VCO_MOD_SOURCE_PARAM,
		VCO_MOD_AMOUNT_PARAM,
		VCO_MOD_DEST_PARAM,
		VCF_MODE_PARAM,
		VCF_MOD_SOURCE_PARAM,
		VCF_MOD_AMOUNT_PARAM,
		VCF_MOD_POLARITY_PARAM,

		TEMPO_PARAM,
		LFO_RATE_PARAM,
		LFO_WAVE_PARAM,
		ATTACK_PARAM,
		SUSTAIN_PARAM,
		DECAY_PARAM,
		VC_MIX_PARAM,

		HOLD_REST_PARAM,
		RESET_ACCENT_PARAM,
		PATTERN_PARAM,
		SHIFT_PARAM,
		RUN_STOP_PARAM,
		KB_PARAM,
		STEP_PARAM,

		KEY_PARAM,
		PARAMS_LEN = KEY_PARAM + KEYBOARD_KEYS
	};

	enum InputId {
		EXT_AUDIO_INPUT,
		MIX_CV_INPUT,
		VCA_CV_INPUT,
		VCF_CUTOFF_INPUT,
		VCF_RES_INPUT,
		VCO_1VOCT_INPUT,
		VCO_LIN_FM_INPUT,
		VCO_MOD_INPUT,
		LFO_RATE_INPUT,
		MIX_1_INPUT,
		MIX_2_INPUT,
		VC_MIX_CTRL_INPUT,
		MULT_INPUT,
		GATE_INPUT,
		TEMPO_INPUT,
		RUN_STOP_INPUT,
		RESET_INPUT,
		HOLD_INPUT,
		INPUTS_LEN
	};

	enum OutputId {
		VCA_OUTPUT,
		NOISE_OUTPUT,
		VCF_OUTPUT,
		VCO_SAW_OUTPUT,
		VCO_PULSE_OUTPUT,
		LFO_TRI_OUTPUT,
		LFO_SQ_OUTPUT,
		VC_MIX_OUTPUT,
		MULT_1_OUTPUT,
		MULT_2_OUTPUT,
		ASSIGN_OUTPUT,
		EG_OUTPUT,
		KB_OUTPUT,
		GATE_OUTPUT,
		OUTPUTS_LEN
	};

	enum LightId {
		ENUMS(OCTAVE_LIGHT, OCTAVE_LEDS * 3),
		ENUMS(STEP_LIGHT, WHITE_KEYS),
		ENUMS(TEMPO_LIGHT, 3),
		MIDI_LIGHT,
		LIGHTS_LEN
	};

	Voice voice;
	Sequencer sequencer;
	PanelControl panel;

	/** The pattern being played and edited. Saving copies it into memory. */
	Pattern pattern;
	PatternMemory memory;
	int bank = 0;
	int patternIndex = 0;

	int octave = DEFAULT_OCTAVE;
	int heldKey = -1;
	/** Live transposition stays put after the key is released, as a performance control. */
	float transpose = 0.f;
	/** Off by default: the hardware has no display and says everything through its LEDs. */
	bool showStatus = false;

	M32();
	void process(const ProcessArgs& args) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void onReset(const ResetEvent& e) override;

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;

private:
	dsp::SchmittTrigger gateInputTrigger;
	dsp::SchmittTrigger runStopInputTrigger;
	dsp::SchmittTrigger resetInputTrigger;
	dsp::SchmittTrigger holdInputTrigger;
	dsp::PulseGenerator clockPulse;
	/** Holds the gate open long enough to hear a step's pitch when it is selected for editing. */
	dsp::PulseGenerator auditionPulse;
	float blinkPhase = 0.f;

	void readControls();
	void readPatchInputs();
	void readPanel(float deltaTime);
	void applyPanelToSequencer();
	void processTransportInputs();
	void processSequencer(float deltaTime);
	void driveVoice(float deltaTime);
	void writeOutputs(float deltaTime);
	void updateLights(float deltaTime);
	void processUtilities();

	void updateOctaveLights(float deltaTime);
	void updateStepLights(float deltaTime);
	void updateTempoLight(float deltaTime);
	void fillOctaveLights(float* red, float* green) const;
	float stepBrightness(int step) const;

	float keyPitch(int key) const;
	float transposeFor(int key) const;
};

} // namespace m32
