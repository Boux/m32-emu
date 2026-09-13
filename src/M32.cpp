#include "M32.hpp"
#include "plugin.hpp"

namespace m32 {

M32::M32() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(FREQUENCY_PARAM, 0.f, 1.f, 0.5f, "Frequency");
	configSwitch(VCO_WAVE_PARAM, 0.f, 1.f, 0.f, "VCO wave", {"Saw", "Pulse"});
	configParam(PULSE_WIDTH_PARAM, 0.f, 1.f, 0.5f, "Pulse width", "%", 0.f, 96.f, 2.f);
	configParam(MIX_PARAM, 0.f, 1.f, 0.f, "Mix", "%", 0.f, 100.f);
	configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff");
	configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
	configSwitch(VCA_MODE_PARAM, 0.f, 1.f, 0.f, "VCA mode", {"EG", "On"});
	configParam(VOLUME_PARAM, 0.f, 1.f, 0.8f, "Volume", "%", 0.f, 100.f);

	configParam(GLIDE_PARAM, 0.f, 1.f, 0.f, "Glide", " s", 0.f, Glide::MAX_TIME);
	configSwitch(VCO_MOD_SOURCE_PARAM, 0.f, 1.f, 0.f, "VCO mod source", {"LFO", "EG / VCO mod"});
	configParam(VCO_MOD_AMOUNT_PARAM, 0.f, 1.f, 0.f, "VCO mod amount", "%", 0.f, 100.f);
	configSwitch(VCO_MOD_DEST_PARAM, 0.f, 1.f, 0.f, "VCO mod destination", {"Frequency", "Pulse width"});
	configSwitch(VCF_MODE_PARAM, 0.f, 1.f, 0.f, "VCF mode", {"Low pass", "Hi pass"});
	configSwitch(VCF_MOD_SOURCE_PARAM, 0.f, 1.f, 0.f, "VCF mod source", {"LFO", "EG"});
	configParam(VCF_MOD_AMOUNT_PARAM, 0.f, 1.f, 0.f, "VCF mod amount", "%", 0.f, 100.f);
	configSwitch(VCF_MOD_POLARITY_PARAM, 0.f, 1.f, 1.f, "VCF mod polarity", {"Negative", "Positive"});

	configParam(TEMPO_PARAM, 0.f, 1.f, 0.5f, "Tempo / gate length");
	configParam(LFO_RATE_PARAM, 0.f, 1.f, 0.5f, "LFO rate");
	configSwitch(LFO_WAVE_PARAM, 0.f, 1.f, 0.f, "LFO wave", {"Triangle", "Square"});
	configParam(ATTACK_PARAM, 0.f, 1.f, 0.f, "Attack", " s", 0.f, Envelope::MAX_ATTACK);
	configSwitch(SUSTAIN_PARAM, 0.f, 1.f, 0.f, "Sustain", {"Off", "On"});
	configParam(DECAY_PARAM, 0.f, 1.f, 0.3f, "Decay", " s", 0.f, Envelope::MAX_DECAY);
	configParam(VC_MIX_PARAM, 0.f, 1.f, 0.5f, "VC mix", "%", 0.f, 100.f);

	configButton(HOLD_REST_PARAM, "Hold / rest");
	configButton(RESET_ACCENT_PARAM, "Reset / accent");
	configSwitch(PATTERN_PARAM, 0.f, 1.f, 0.f, "Pattern (bank)", {"Released", "Held"});
	configSwitch(SHIFT_PARAM, 0.f, 1.f, 0.f, "Shift", {"Released", "Held"});
	configButton(RUN_STOP_PARAM, "Run / stop (rec)");
	configButton(KB_PARAM, "KB / octave down");
	configButton(STEP_PARAM, "Step / octave up");

	for (int i = 0; i < KEYBOARD_KEYS; i++)
		configButton(KEY_PARAM + i, string::f("Key %d", i + 1));

	configInput(EXT_AUDIO_INPUT, "External audio");
	configInput(MIX_CV_INPUT, "Mix CV");
	configInput(VCA_CV_INPUT, "VCA CV");
	configInput(VCF_CUTOFF_INPUT, "VCF cutoff");
	configInput(VCF_RES_INPUT, "VCF resonance");
	configInput(VCO_1VOCT_INPUT, "VCO 1V/oct");
	configInput(VCO_LIN_FM_INPUT, "VCO linear FM");
	configInput(VCO_MOD_INPUT, "VCO mod");
	configInput(LFO_RATE_INPUT, "LFO rate");
	configInput(MIX_1_INPUT, "Mix 1");
	configInput(MIX_2_INPUT, "Mix 2");
	configInput(VC_MIX_CTRL_INPUT, "VC mix control");
	configInput(MULT_INPUT, "Mult");
	configInput(GATE_INPUT, "Gate");
	configInput(TEMPO_INPUT, "Tempo");
	configInput(RUN_STOP_INPUT, "Run / stop");
	configInput(RESET_INPUT, "Reset");
	configInput(HOLD_INPUT, "Hold");

	configOutput(VCA_OUTPUT, "VCA");
	configOutput(NOISE_OUTPUT, "Noise");
	configOutput(VCF_OUTPUT, "VCF");
	configOutput(VCO_SAW_OUTPUT, "VCO saw");
	configOutput(VCO_PULSE_OUTPUT, "VCO pulse");
	configOutput(LFO_TRI_OUTPUT, "LFO triangle");
	configOutput(LFO_SQ_OUTPUT, "LFO square");
	configOutput(VC_MIX_OUTPUT, "VC mix");
	configOutput(MULT_1_OUTPUT, "Mult 1");
	configOutput(MULT_2_OUTPUT, "Mult 2");
	configOutput(ASSIGN_OUTPUT, "Assign");
	configOutput(EG_OUTPUT, "EG");
	configOutput(KB_OUTPUT, "KB");
	configOutput(GATE_OUTPUT, "Gate");
}

void M32::process(const ProcessArgs& args) {
	readControls();
	readPatchInputs();
	readPanel(args.sampleTime);
	applyPanelToSequencer();
	processTransportInputs();
	processSequencer(args.sampleTime);
	driveVoice(args.sampleTime);

	voice.process(args.sampleTime, args.sampleRate);

	writeOutputs(args.sampleTime);
	processUtilities();
	updateLights(args.sampleTime);
}

void M32::readControls() {
	voice.frequencyKnob = params[FREQUENCY_PARAM].getValue();
	voice.vcoWaveIsPulse = params[VCO_WAVE_PARAM].getValue() > 0.5f;
	voice.pulseWidthKnob = params[PULSE_WIDTH_PARAM].getValue();
	voice.mixKnob = params[MIX_PARAM].getValue();
	voice.cutoffKnob = params[CUTOFF_PARAM].getValue();
	voice.resonanceKnob = params[RESONANCE_PARAM].getValue();
	voice.vcaModeIsOn = params[VCA_MODE_PARAM].getValue() > 0.5f;
	voice.volumeKnob = params[VOLUME_PARAM].getValue();

	voice.vcoModSourceIsLfo = params[VCO_MOD_SOURCE_PARAM].getValue() < 0.5f;
	voice.vcoModAmount = params[VCO_MOD_AMOUNT_PARAM].getValue();
	voice.vcoModDestIsPulseWidth = params[VCO_MOD_DEST_PARAM].getValue() > 0.5f;
	voice.vcfModeIsHighPass = params[VCF_MODE_PARAM].getValue() > 0.5f;
	voice.vcfModSourceIsLfo = params[VCF_MOD_SOURCE_PARAM].getValue() < 0.5f;
	voice.vcfModAmount = params[VCF_MOD_AMOUNT_PARAM].getValue();
	voice.vcfModPolarityIsPositive = params[VCF_MOD_POLARITY_PARAM].getValue() > 0.5f;

	voice.lfoRateKnob = params[LFO_RATE_PARAM].getValue();
	voice.lfoWaveIsSquare = params[LFO_WAVE_PARAM].getValue() > 0.5f;
	voice.attackKnob = params[ATTACK_PARAM].getValue();
	voice.sustainIsOn = params[SUSTAIN_PARAM].getValue() > 0.5f;
	voice.decayKnob = params[DECAY_PARAM].getValue();
}

void M32::readPatchInputs() {
	voice.extAudioConnected = inputs[EXT_AUDIO_INPUT].isConnected();
	voice.extAudio = inputs[EXT_AUDIO_INPUT].getVoltage();
	voice.mixCv = inputs[MIX_CV_INPUT].getVoltage();
	voice.vcaCvConnected = inputs[VCA_CV_INPUT].isConnected();
	voice.vcaCv = inputs[VCA_CV_INPUT].getVoltage();
	voice.cutoffCv = inputs[VCF_CUTOFF_INPUT].getVoltage();
	voice.resonanceCv = inputs[VCF_RES_INPUT].getVoltage();
	voice.pitchCv = inputs[VCO_1VOCT_INPUT].getVoltage();
	voice.linearFm = inputs[VCO_LIN_FM_INPUT].getVoltage();
	voice.vcoModInConnected = inputs[VCO_MOD_INPUT].isConnected();
	voice.vcoModIn = inputs[VCO_MOD_INPUT].getVoltage();
	voice.lfoRateCv = inputs[LFO_RATE_INPUT].getVoltage();
}

float M32::keyPitch(int key) const {
	return float(octave - OCTAVE_AT_ZERO_VOLTS) + keySemitone(key) / 12.f;
}

/** Transposition is measured from low C of the default octave, so that key means no change. */
float M32::transposeFor(int key) const {
	return float(octave - DEFAULT_OCTAVE) + keySemitone(key) / 12.f;
}

void M32::readPanel(float deltaTime) {
	PanelInput in;
	in.shift = params[SHIFT_PARAM].getValue() > 0.5f;
	in.patternButton = params[PATTERN_PARAM].getValue() > 0.5f;
	in.runStop = params[RUN_STOP_PARAM].getValue() > 0.5f;
	in.resetAccent = params[RESET_ACCENT_PARAM].getValue() > 0.5f;
	in.holdRest = params[HOLD_REST_PARAM].getValue() > 0.5f;
	in.kb = params[KB_PARAM].getValue() > 0.5f;
	in.step = params[STEP_PARAM].getValue() > 0.5f;
	for (int i = 0; i < KEYBOARD_KEYS; i++)
		in.keys[i] = params[KEY_PARAM + i].getValue() > 0.5f;
	in.glideKnob = params[GLIDE_PARAM].getValue();
	in.tempoKnob = params[TEMPO_PARAM].getValue();
	in.dt = deltaTime;

	PanelTarget target;
	target.pattern = &pattern;
	target.memory = &memory;
	target.sequencer = &sequencer;
	target.bank = &bank;
	target.patternIndex = &patternIndex;
	target.octave = &octave;

	panel.process(in, target);
	voice.glideKnob = panel.out.glideTime;
}

void M32::applyPanelToSequencer() {
	sequencer.clock.bpm = panel.out.bpm;
	sequencer.liveAccent = panel.out.liveAccent;
	sequencer.liveMute = panel.out.liveMute;
	sequencer.liveRatchet = panel.out.liveRatchet;
	sequencer.holdHeld = panel.out.hold || holdInputTrigger.isHigh();
}

/** The transport jacks are level driven, not edge toggled (p56). RUN/STOP plays for as long as it
is high, RESET sits on step 1 while it is high, and the panel buttons still win if used afterwards.
These inputs trigger near +3.2 V and tolerate up to +15 V. */
void M32::processTransportInputs() {
	// Compare levels rather than edges: Rack's SchmittTrigger reports no edge on its first
	// transition out of the uninitialized state, which would swallow an already-high jack.
	runStopInputTrigger.process(inputs[RUN_STOP_INPUT].getVoltage(), GATE_LOW_VOLTS, GATE_HIGH_VOLTS);
	const bool runHigh = runStopInputTrigger.isHigh();
	if (runHigh && !runStopWasHigh)
		sequencer.start();
	else if (!runHigh && runStopWasHigh)
		sequencer.stop();
	runStopWasHigh = runHigh;

	resetInputTrigger.process(inputs[RESET_INPUT].getVoltage(), GATE_LOW_VOLTS, GATE_HIGH_VOLTS);
	const bool resetHigh = resetInputTrigger.isHigh();
	if (resetHigh && !resetWasHigh)
		sequencer.reset();
	sequencer.resetHeld = resetHigh;
	resetWasHigh = resetHigh;

	holdInputTrigger.process(inputs[HOLD_INPUT].getVoltage(), GATE_LOW_VOLTS, GATE_HIGH_VOLTS);
}

void M32::processSequencer(float deltaTime) {
	if (!sequencer.running)
		transpose = 0.f;
	else if (panel.out.noteKey >= 0)
		transpose = transposeFor(panel.out.noteKey);
	sequencer.transpose = transpose;

	sequencer.process(pattern, deltaTime);
	if (sequencer.clock.stepAdvanced && sequencer.running)
		clockPulse.trigger(1e-3f);
}

/** The sequencer owns the voice while running; otherwise the keyboard and the GATE jack do. */
void M32::driveVoice(float deltaTime) {
	const bool gateFromPatch = gateInputTrigger.process(inputs[GATE_INPUT].getVoltage(), 0.1f, 1.f);
	const bool patchGateHigh = inputs[GATE_INPUT].isConnected() && gateInputTrigger.isHigh();

	if (panel.out.audition) {
		voice.onNoteOn(panel.out.auditionPitch, false);
		voice.retriggerEnvelope();
		auditionPulse.trigger(0.3f);
	}
	const bool auditioning = auditionPulse.process(deltaTime);

	if (sequencer.running) {
		const SequencerOutput& seq = sequencer.output();
		if (seq.retrigger) {
			voice.onNoteOn(seq.pitch, seq.glide && voice.glideKnob > 0.f);
			voice.retriggerEnvelope();
		}
		// Set every sample so a tie into a new note still glides and still changes pitch.
		voice.notePitch = seq.pitch;
		voice.glideEnabled = seq.glide && voice.glideKnob > 0.f;
		voice.gate = seq.gate;
		voice.accent = seq.accent;
		heldKey = -1;
		return;
	}

	const int key = panel.out.noteKey;
	voice.accent = false;
	if (key >= 0 && key != heldKey)
		voice.onNoteOn(keyPitch(key), voice.glideKnob > 0.f);
	heldKey = key;

	voice.gate = (key >= 0) || patchGateHigh || auditioning;
	if (gateFromPatch)
		voice.retriggerEnvelope();
}

void M32::writeOutputs(float deltaTime) {
	outputs[VCA_OUTPUT].setVoltage(voice.vcaOut);
	outputs[NOISE_OUTPUT].setVoltage(voice.noiseOut);
	outputs[VCF_OUTPUT].setVoltage(voice.vcfOut);
	outputs[VCO_SAW_OUTPUT].setVoltage(voice.sawOut);
	outputs[VCO_PULSE_OUTPUT].setVoltage(voice.pulseOut);
	outputs[LFO_TRI_OUTPUT].setVoltage(voice.lfoTriOut);
	outputs[LFO_SQ_OUTPUT].setVoltage(voice.lfoSqOut);
	outputs[EG_OUTPUT].setVoltage(voice.egOut);
	outputs[KB_OUTPUT].setVoltage(voice.notePitch);
	outputs[GATE_OUTPUT].setVoltage(voice.gate ? 5.f : 0.f);
	outputs[ASSIGN_OUTPUT].setVoltage(clockPulse.process(deltaTime) ? 5.f : 0.f);
}

/** The mult and the VC mixer are wired to the patchbay only, never to the voice. */
void M32::processUtilities() {
	const float mult = inputs[MULT_INPUT].getVoltage();
	outputs[MULT_1_OUTPUT].setVoltage(mult);
	outputs[MULT_2_OUTPUT].setVoltage(mult);

	const float mix1 = inputs[MIX_1_INPUT].getNormalVoltage(0.f);
	const float mix2 = inputs[MIX_2_INPUT].getNormalVoltage(5.f);
	// A centred VC MIX knob is swept end to end by -5 V to +5 V (p50).
	const float blend = clamp(params[VC_MIX_PARAM].getValue() + inputs[VC_MIX_CTRL_INPUT].getVoltage() / 10.f, 0.f, 1.f);
	outputs[VC_MIX_OUTPUT].setVoltage(crossfade(mix1, mix2, blend));
}

/** All LED behaviour lives in computeLeds, which mirrors the panel's own language. */
void M32::updateLights(float deltaTime) {
	blinkPhase = sequencer.running ? sequencer.clock.stepPhase : std::fmod(blinkPhase + deltaTime * 4.f, 1.f);

	LedInputs in;
	in.panel = &panel;
	in.pattern = &pattern;
	in.sequencer = &sequencer;
	in.octave = octave;
	in.bank = bank;
	in.patternIndex = patternIndex;
	in.blinkPhase = blinkPhase;
	in.halfRate = (sequencer.clock.stepCount % 2) == 0;

	LedState leds;
	computeLeds(in, leds);

	for (int i = 0; i < OCTAVE_LEDS; i++) {
		lights[OCTAVE_LIGHT + i * 3 + 0].setBrightnessSmooth(leds.octave[i].red, deltaTime);
		lights[OCTAVE_LIGHT + i * 3 + 1].setBrightnessSmooth(leds.octave[i].green, deltaTime);
		lights[OCTAVE_LIGHT + i * 3 + 2].setBrightnessSmooth(0.f, deltaTime);
	}
	for (int i = 0; i < WHITE_KEYS; i++)
		lights[STEP_LIGHT + i].setBrightnessSmooth(leds.step[i], deltaTime);

	lights[TEMPO_LIGHT + 0].setBrightnessSmooth(leds.tempo.red, deltaTime);
	lights[TEMPO_LIGHT + 1].setBrightnessSmooth(leds.tempo.green, deltaTime);
	lights[TEMPO_LIGHT + 2].setBrightnessSmooth(0.f, deltaTime);
	lights[MIDI_LIGHT].setBrightnessSmooth(0.f, deltaTime);
}

void M32::onSampleRateChange(const SampleRateChangeEvent& e) {
	voice.reset();
}

/** Rack's Initialize. Clears the working pattern, all 64 saved slots and every panel mode. */
void M32::onReset(const ResetEvent& e) {
	Module::onReset(e);

	pattern.initialize();
	for (int i = 0; i < PATTERN_SLOTS; i++)
		memory.slots[i].initialize();

	panel = PanelControl();
	sequencer = Sequencer();
	bank = 0;
	patternIndex = 0;
	octave = DEFAULT_OCTAVE;
	heldKey = -1;
	transpose = 0.f;
	voice.reset();
}

json_t* M32::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "octave", json_integer(octave));
	json_object_set_new(rootJ, "bank", json_integer(bank));
	json_object_set_new(rootJ, "patternIndex", json_integer(patternIndex));
	json_object_set_new(rootJ, "playbackOrder", json_integer(int(sequencer.order)));
	json_object_set_new(rootJ, "pattern", pattern.toJson());
	json_object_set_new(rootJ, "memory", memory.toJson());
	json_object_set_new(rootJ, "panel", panel.toJson());
	json_object_set_new(rootJ, "showStatus", json_boolean(showStatus));
	return rootJ;
}

void M32::dataFromJson(json_t* rootJ) {
	json_t* octaveJ = json_object_get(rootJ, "octave");
	if (octaveJ)
		octave = clamp(int(json_integer_value(octaveJ)), MIN_OCTAVE, MAX_OCTAVE);

	json_t* bankJ = json_object_get(rootJ, "bank");
	if (bankJ)
		bank = clamp(int(json_integer_value(bankJ)), 0, BANKS - 1);

	json_t* indexJ = json_object_get(rootJ, "patternIndex");
	if (indexJ)
		patternIndex = clamp(int(json_integer_value(indexJ)), 0, PATTERNS_PER_BANK - 1);

	json_t* orderJ = json_object_get(rootJ, "playbackOrder");
	if (orderJ)
		sequencer.order = PlaybackOrder(clamp(int(json_integer_value(orderJ)), 0, 3));

	pattern.fromJson(json_object_get(rootJ, "pattern"));
	memory.fromJson(json_object_get(rootJ, "memory"));
	panel.fromJson(json_object_get(rootJ, "panel"));

	json_t* statusJ = json_object_get(rootJ, "showStatus");
	if (statusJ)
		showStatus = json_boolean_value(statusJ);
}

} // namespace m32
