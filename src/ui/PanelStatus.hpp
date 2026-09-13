#pragma once
#include <rack.hpp>
#include "../M32.hpp"
#include "PanelLayout.hpp"

namespace m32 {

using namespace rack;

/** The hardware tells you its mode through blink patterns on eight LEDs. A screen can just say it,
so this strip spells out the current mode, what a knob is about to do, and where the playhead is. */
struct PanelStatus : widget::Widget {
	M32* module = nullptr;

	void draw(const DrawArgs& args) override {
		if (!module || !module->showStatus)
			return;

		std::shared_ptr<window::Font> font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font)
			return;

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 6.5f);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, colour());

		const math::Vec p = mm2px(math::Vec(118.f, 125.5f));
		nvgText(args.vg, p.x, p.y, text().c_str(), NULL);
	}

private:
	NVGcolor colour() const {
		if (!module)
			return nvgRGB(0x9a, 0x9d, 0xa2);
		if (module->panel.saving || module->panel.setEndArmed)
			return nvgRGB(0xe0, 0x9b, 0x2e);
		if (module->panel.recording)
			return nvgRGB(0xe0, 0x6a, 0x5a);
		return nvgRGB(0x9a, 0x9d, 0xa2);
	}

	std::string text() const {
		if (!module)
			return "KB";

		const PanelControl& panel = module->panel;
		std::string status = panel.mode == SeqMode::KB ? "KB" : "STEP";

		if (panel.saving)
			return status + string::f("   SAVING TO BANK %d PATTERN %d - SHIFT+RUN/STOP TO CONFIRM",
				panel.saveBank + 1, panel.saveIndex + 1);
		if (panel.setEndArmed)
			return status + "   SET END - PRESS A STEP KEY";

		if (panel.recording)
			status += "   REC";
		if (panel.editStep >= 0)
			status += string::f("   EDIT STEP %d", panel.editStep + 1);
		if (panel.shiftHeld)
			status += "   SHIFT";
		if (panel.patternHeld)
			status += "   PATTERN";

		status += string::f("   BANK %d  PAT %d  STEP %d/%d", module->bank + 1, module->patternIndex + 1,
			module->sequencer.currentStep + 1, module->pattern.length());
		return status;
	}
};

} // namespace m32
