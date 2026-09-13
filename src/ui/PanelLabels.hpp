#pragma once
#include <rack.hpp>
#include "PanelLayout.hpp"
#include "Patchbay.hpp"
#include "../generated/BuildInfo.hpp"

namespace m32 {

using namespace rack;

/** All panel legends. nanosvg cannot render SVG text, so the panel artwork carries only
graphics and every label is drawn here. */
struct PanelLabels : widget::Widget {
	struct Entry {
		float x, y;
		const char* text;
	};

	void draw(const DrawArgs& args) override {
		font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font)
			return;

		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		drawTitles(args.vg);
		drawSwitchPositions(args.vg);
		drawScaleMarks(args.vg);
		drawPatchbay(args.vg);
		drawSequencer(args.vg);
		drawBranding(args.vg);

		Widget::draw(args);
	}

private:
	static constexpr float TITLE_SIZE = 7.2f;
	static constexpr float SMALL_SIZE = 5.6f;
	static constexpr float TINY_SIZE = 5.0f;

	static constexpr float TITLE_OFFSET = -10.5f;
	static constexpr float SWITCH_UPPER = -5.0f;
	static constexpr float SWITCH_LOWER = 5.0f;

	std::shared_ptr<window::Font> font;

	static NVGcolor ink() {
		return nvgRGB(0xe6, 0xe7, 0xe9);
	}

	static NVGcolor dimInk() {
		return nvgRGB(0x9a, 0x9d, 0xa2);
	}

	void text(NVGcontext* vg, float xMm, float yMm, const char* str, float size, NVGcolor color) {
		nvgFontSize(vg, size);
		nvgFillColor(vg, color);
		const math::Vec p = mm2px(math::Vec(xMm, yMm));
		nvgText(vg, p.x, p.y, str, NULL);
	}

	/** Output legends are printed as dark text in a light box, as on the hardware. */
	void invertedText(NVGcontext* vg, float xMm, float yMm, const char* str, float size) {
		nvgFontSize(vg, size);
		const math::Vec p = mm2px(math::Vec(xMm, yMm));

		float bounds[4];
		nvgTextBounds(vg, p.x, p.y, str, NULL, bounds);
		const float padX = 1.6f;
		const float padY = 0.9f;

		nvgBeginPath(vg);
		nvgRect(vg, bounds[0] - padX, bounds[1] - padY, bounds[2] - bounds[0] + 2 * padX, bounds[3] - bounds[1] + 2 * padY);
		nvgFillColor(vg, ink());
		nvgFill(vg);

		nvgFillColor(vg, nvgRGB(0x17, 0x18, 0x1a));
		nvgText(vg, p.x, p.y, str, NULL);
	}

	void drawTitles(NVGcontext* vg) {
		using namespace layout;
		static const Entry titles[] = {
			{FREQUENCY.x, FREQUENCY.y + TITLE_OFFSET, "FREQUENCY"},
			{VCO_WAVE.x, VCO_WAVE.y + TITLE_OFFSET, "VCO WAVE"},
			{PULSE_WIDTH.x, PULSE_WIDTH.y + TITLE_OFFSET, "PULSE WIDTH"},
			{MIX.x, MIX.y + TITLE_OFFSET, "MIX"},
			{CUTOFF.x, CUTOFF.y + TITLE_OFFSET, "CUTOFF"},
			{RESONANCE.x, RESONANCE.y + TITLE_OFFSET, "RESONANCE"},
			{VCA_MODE.x, VCA_MODE.y + TITLE_OFFSET, "VCA MODE"},
			{VOLUME.x, VOLUME.y + TITLE_OFFSET, "VOLUME"},

			{GLIDE.x, GLIDE.y + TITLE_OFFSET, "GLIDE"},
			{VCO_MOD_SOURCE.x, VCO_MOD_SOURCE.y + TITLE_OFFSET, "VCO MOD SOURCE"},
			{VCO_MOD_AMOUNT.x, VCO_MOD_AMOUNT.y + TITLE_OFFSET, "VCO MOD AMOUNT"},
			{VCO_MOD_DEST.x, VCO_MOD_DEST.y + TITLE_OFFSET, "VCO MOD DEST"},
			{VCF_MODE.x, VCF_MODE.y + TITLE_OFFSET, "VCF MODE"},
			{VCF_MOD_SOURCE.x, VCF_MOD_SOURCE.y + TITLE_OFFSET, "VCF MOD SOURCE"},
			{VCF_MOD_AMOUNT.x, VCF_MOD_AMOUNT.y + TITLE_OFFSET, "VCF MOD AMOUNT"},
			{VCF_MOD_POLARITY.x, VCF_MOD_POLARITY.y + TITLE_OFFSET, "VCF MOD POLARITY"},

			{TEMPO.x, TEMPO.y + TITLE_OFFSET, "TEMPO / GATE LENGTH"},
			{LFO_RATE.x, LFO_RATE.y + TITLE_OFFSET, "LFO RATE"},
			{LFO_WAVE.x, LFO_WAVE.y + TITLE_OFFSET, "LFO WAVE"},
			{ATTACK.x, ATTACK.y + TITLE_OFFSET, "ATTACK"},
			{SUSTAIN.x, SUSTAIN.y + TITLE_OFFSET, "SUSTAIN"},
			{DECAY.x, DECAY.y + TITLE_OFFSET, "DECAY"},
			{VC_MIX.x, VC_MIX.y + TITLE_OFFSET, "VC MIX"},
		};
		for (const Entry& e : titles)
			text(vg, e.x, e.y, e.text, TITLE_SIZE, ink());
	}

	void drawSwitchPositions(NVGcontext* vg) {
		using namespace layout;
		static const Entry positions[] = {
			{VCO_WAVE.x, VCO_WAVE.y + SWITCH_UPPER, "PULSE"},
			{VCO_WAVE.x, VCO_WAVE.y + SWITCH_LOWER, "SAW"},
			{VCA_MODE.x, VCA_MODE.y + SWITCH_UPPER, "ON"},
			{VCA_MODE.x, VCA_MODE.y + SWITCH_LOWER, "EG"},
			{VCO_MOD_SOURCE.x, VCO_MOD_SOURCE.y + SWITCH_UPPER, "EG / VCO MOD"},
			{VCO_MOD_SOURCE.x, VCO_MOD_SOURCE.y + SWITCH_LOWER, "LFO"},
			{VCO_MOD_DEST.x, VCO_MOD_DEST.y + SWITCH_UPPER, "PULSE WIDTH"},
			{VCO_MOD_DEST.x, VCO_MOD_DEST.y + SWITCH_LOWER, "FREQUENCY"},
			{VCF_MODE.x, VCF_MODE.y + SWITCH_UPPER, "HI PASS"},
			{VCF_MODE.x, VCF_MODE.y + SWITCH_LOWER, "LOW PASS"},
			{VCF_MOD_SOURCE.x, VCF_MOD_SOURCE.y + SWITCH_UPPER, "EG"},
			{VCF_MOD_SOURCE.x, VCF_MOD_SOURCE.y + SWITCH_LOWER, "LFO"},
			{VCF_MOD_POLARITY.x, VCF_MOD_POLARITY.y + SWITCH_UPPER, "+"},
			{VCF_MOD_POLARITY.x, VCF_MOD_POLARITY.y + SWITCH_LOWER, "-"},
			{LFO_WAVE.x, LFO_WAVE.y + SWITCH_UPPER, "SQUARE"},
			{LFO_WAVE.x, LFO_WAVE.y + SWITCH_LOWER, "TRIANGLE"},
			{SUSTAIN.x, SUSTAIN.y + SWITCH_UPPER, "ON"},
			{SUSTAIN.x, SUSTAIN.y + SWITCH_LOWER, "OFF"},
		};
		for (const Entry& e : positions)
			text(vg, e.x, e.y, e.text, SMALL_SIZE, ink());
	}

	void drawScaleMarks(NVGcontext* vg) {
		using namespace layout;
		static const Entry marks[] = {
			{MIX.x - 7.5f, MIX.y + 9.5f, "VCO"},
			{MIX.x + 7.5f, MIX.y + 9.5f, "NOISE / EXT"},
			{CUTOFF.x - 8.f, CUTOFF.y + 9.5f, "20Hz"},
			{CUTOFF.x + 8.f, CUTOFF.y + 9.5f, "20kHz"},
			{TEMPO.x, TEMPO.y + 9.5f, "(SWING)"},
			{VC_MIX.x - 7.f, VC_MIX.y + 9.5f, "LO / MIX 1"},
			{VC_MIX.x + 7.f, VC_MIX.y + 9.5f, "HI / MIX 2"},
		};
		for (const Entry& e : marks)
			text(vg, e.x, e.y, e.text, TINY_SIZE, dimInk());
	}

	void drawPatchbay(NVGcontext* vg) {
		using namespace layout;
		text(vg, jack(2, 0).x - 4.f, JACK_ROW_1 - 9.5f, "IN /", TINY_SIZE, ink());
		invertedText(vg, jack(2, 0).x + 3.5f, JACK_ROW_1 - 9.5f, "OUT", TINY_SIZE);

		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 4; col++) {
				const PatchbayCell& cell = PATCHBAY[row][col];
				const Pos p = jack(col, row);
				if (cell.isOutput)
					invertedText(vg, p.x, p.y - JACK_LABEL_OFFSET, cell.label, TINY_SIZE);
				else
					text(vg, p.x, p.y - JACK_LABEL_OFFSET, cell.label, TINY_SIZE, ink());
			}
		}
	}

	void drawSequencer(NVGcontext* vg) {
		using namespace layout;
		static const Entry labels[] = {
			{HOLD_REST.x, HOLD_REST.y + 5.5f, "HOLD / REST"},
			{RESET_ACCENT.x, RESET_ACCENT.y + 5.5f, "RESET / ACCENT"},
			{SHIFT.x, SHIFT.y + 5.5f, "(SHIFT)"},
			{PATTERN.x, PATTERN.y + 5.5f, "PATTERN (BANK)"},
			{RUN_STOP.x, RUN_STOP.y + 5.5f, "RUN / STOP (REC)"},
			{KB_BUTTON.x, KB_BUTTON.y + 6.f, "(KB)"},
			{STEP_BUTTON.x, STEP_BUTTON.y + 6.f, "(STEP)"},
		};
		for (const Entry& e : labels)
			text(vg, e.x, e.y, e.text, TINY_SIZE, ink());

		text(vg, OCTAVE_LED_1_X + OCTAVE_LED_STEP * 3.5f, OCTAVE_LED_Y - 4.5f, "OCTAVE / LOCATION", TINY_SIZE, ink());

	}

	void drawBranding(NVGcontext* vg) {
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		text(vg, 13.f, 125.f, "M32", 11.f, ink());
		text(vg, 29.f, 125.5f, "SEMI-MODULAR", 5.f, dimInk());
		nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		text(vg, 291.f, 125.5f, string::f("BUILD %d", M32_BUILD_NUMBER).c_str(), 6.5f, dimInk());
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	}
};

} // namespace m32
