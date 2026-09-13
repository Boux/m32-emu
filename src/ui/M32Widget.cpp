#include "../M32.hpp"
#include "../plugin.hpp"
#include "PanelLayout.hpp"
#include "PanelLabels.hpp"
#include "PanelStatus.hpp"
#include "Patchbay.hpp"
#include "PanelButtons.hpp"
#include "../generated/BuildInfo.hpp"

namespace m32 {

using namespace layout;

struct M32Widget : ModuleWidget {
	M32Widget(M32* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/M32.svg")));
		addChild(new PanelLabels);

		PanelStatus* status = new PanelStatus;
		status->module = module;
		addChild(status);

		addScrews();
		addSynthControls(module);
		addPatchbay(module);
		addSequencer(module);
	}

	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel(string::f("Build %d", M32_BUILD_NUMBER)));
		menu->addChild(createMenuLabel(M32_BUILD_TIME));

		M32* m = dynamic_cast<M32*>(module);
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem<bool>("Show panel state readout", "", &m->showStatus));
	}

private:
	static math::Vec at(Pos p) {
		return mm2px(math::Vec(p.x, p.y));
	}

	void addScrews() {
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}

	void addSynthControls(M32* module) {
		addParam(createParamCentered<RoundBigBlackKnob>(at(FREQUENCY), module, M32::FREQUENCY_PARAM));
		addParam(createParamCentered<CKSS>(at(VCO_WAVE), module, M32::VCO_WAVE_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(at(PULSE_WIDTH), module, M32::PULSE_WIDTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(MIX), module, M32::MIX_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(at(CUTOFF), module, M32::CUTOFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(RESONANCE), module, M32::RESONANCE_PARAM));
		addParam(createParamCentered<CKSS>(at(VCA_MODE), module, M32::VCA_MODE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(VOLUME), module, M32::VOLUME_PARAM));

		addParam(createParamCentered<RoundBlackKnob>(at(GLIDE), module, M32::GLIDE_PARAM));
		addParam(createParamCentered<CKSS>(at(VCO_MOD_SOURCE), module, M32::VCO_MOD_SOURCE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(VCO_MOD_AMOUNT), module, M32::VCO_MOD_AMOUNT_PARAM));
		addParam(createParamCentered<CKSS>(at(VCO_MOD_DEST), module, M32::VCO_MOD_DEST_PARAM));
		addParam(createParamCentered<CKSS>(at(VCF_MODE), module, M32::VCF_MODE_PARAM));
		addParam(createParamCentered<CKSS>(at(VCF_MOD_SOURCE), module, M32::VCF_MOD_SOURCE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(VCF_MOD_AMOUNT), module, M32::VCF_MOD_AMOUNT_PARAM));
		addParam(createParamCentered<CKSS>(at(VCF_MOD_POLARITY), module, M32::VCF_MOD_POLARITY_PARAM));

		addParam(createParamCentered<RoundBigBlackKnob>(at(TEMPO), module, M32::TEMPO_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(at(LFO_RATE), module, M32::LFO_RATE_PARAM));
		addParam(createParamCentered<CKSS>(at(LFO_WAVE), module, M32::LFO_WAVE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(ATTACK), module, M32::ATTACK_PARAM));
		addParam(createParamCentered<CKSS>(at(SUSTAIN), module, M32::SUSTAIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(DECAY), module, M32::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(VC_MIX), module, M32::VC_MIX_PARAM));
	}

	void addPatchbay(M32* module) {
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 4; col++)
				addPatchPoint(module, PATCHBAY[row][col], jack(col, row));
		}
	}

	void addPatchPoint(M32* module, const PatchbayCell& cell, Pos p) {
		if (cell.isOutput)
			addOutput(createOutputCentered<PJ301MPort>(at(p), module, cell.portId));
		else
			addInput(createInputCentered<PJ301MPort>(at(p), module, cell.portId));
	}

	void addSequencer(M32* module) {
		addTransport(module);
		addArrows(module);
		addKeyboard(module);

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(at(TEMPO_LED), module, M32::TEMPO_LIGHT));
		for (int i = 0; i < OCTAVE_LEDS; i++) {
			const Pos p = {OCTAVE_LED_1_X + OCTAVE_LED_STEP * i, OCTAVE_LED_Y};
			addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(at(p), module, M32::OCTAVE_LIGHT + i * 3));
		}
	}

	/** SHIFT and PATTERN latch, since a mouse cannot hold them while pressing something else. */
	void addTransport(M32* module) {
		addButton<PanelButton>(HOLD_REST, square(), module, M32::HOLD_REST_PARAM);
		addButton<PanelButton>(RESET_ACCENT, square(), module, M32::RESET_ACCENT_PARAM);
		addButton<PanelButton>(RUN_STOP, square(), module, M32::RUN_STOP_PARAM);
		addButton<LatchButton>(SHIFT, square(), module, M32::SHIFT_PARAM);
		addButton<LatchButton>(PATTERN, square(), module, M32::PATTERN_PARAM);
	}

	void addArrows(M32* module) {
		ArrowButton* kb = addButton<ArrowButton>(KB_BUTTON, math::Vec(ARROW_W, ARROW_H), module, M32::KB_PARAM);
		kb->pointsRight = false;
		addButton<ArrowButton>(STEP_BUTTON, math::Vec(ARROW_W, ARROW_H), module, M32::STEP_PARAM);
	}

	void addKeyboard(M32* module) {
		static const char* PAGE_LABELS[BLACK_KEYS] = {"1-8", "9-16", "17-24", "25-32", "SET END"};

		for (int i = 0; i < WHITE_KEYS; i++) {
			const Pos p = {whiteKeyX(i), WHITE_KEY_Y};
			PianoKey* key = addButton<PianoKey>(p, math::Vec(WHITE_KEY_W, WHITE_KEY_H), module, M32::KEY_PARAM + i);
			key->label = string::f("%d", i + 1);
			addChild(createLightCentered<SmallLight<RedLight>>(at({whiteKeyX(i), STEP_LED_Y}), module, M32::STEP_LIGHT + i));
		}

		for (int i = 0; i < BLACK_KEYS; i++) {
			const Pos p = {blackKeyX(i), BLACK_KEY_Y};
			BlackKey* key = addButton<BlackKey>(p, math::Vec(BLACK_KEY_W, BLACK_KEY_H), module, M32::KEY_PARAM + WHITE_KEYS + i);
			key->label = PAGE_LABELS[i];
		}
	}

	static math::Vec square() {
		return math::Vec(TRANSPORT_BUTTON, TRANSPORT_BUTTON);
	}

	template <typename T>
	T* addButton(Pos centre, math::Vec sizeMm, M32* module, int paramId) {
		T* widget = createParam<T>(math::Vec(0, 0), module, paramId);
		widget->box.size = mm2px(sizeMm);
		widget->box.pos = at(centre).minus(widget->box.size.div(2));
		addParam(widget);
		return widget;
	}
};

} // namespace m32

Model* modelM32 = createModel<m32::M32, m32::M32Widget>("M32");
