#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Rounded panel control drawn directly rather than from SVG, so keys, pads and arrows can
share one look and show whether they are latched. */
struct PanelButton : app::Switch {
	NVGcolor face = nvgRGB(0x3a, 0x3d, 0x42);
	NVGcolor facePressed = nvgRGB(0x6e, 0x73, 0x7a);
	/** Latched controls stay lit so you can see what is being held. */
	NVGcolor faceLatched = nvgRGB(0xe0, 0x9b, 0x2e);
	float radius = 1.2f;

	PanelButton() {
		momentary = true;
	}

	/** Printed on the control itself, the way the step numbers and page ranges are. */
	std::string label;
	NVGcolor labelColour = nvgRGB(0xe6, 0xe7, 0xe9);
	float labelSize = 5.f;

	void draw(const DrawArgs& args) override {
		drawFace(args.vg, currentFace());
		drawLabel(args.vg);
		drawGlyph(args.vg);
	}

	virtual void drawGlyph(NVGcontext* vg) {}

	bool isOn() {
		engine::ParamQuantity* pq = getParamQuantity();
		return pq && pq->getValue() > 0.5f;
	}

protected:
	/** True when the control is held with nothing holding the mouse down on it. */
	bool sticky = false;

	virtual NVGcolor currentFace() {
		if (sticky)
			return faceLatched;
		return isOn() ? facePressed : face;
	}

	void drawFace(NVGcontext* vg, NVGcolor colour) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0.f, 0.f, box.size.x, box.size.y, mm2px(radius));
		nvgFillColor(vg, colour);
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGB(0x0e, 0x0f, 0x10));
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
	}

	void drawLabel(NVGcontext* vg) {
		if (label.empty())
			return;
		std::shared_ptr<window::Font> font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font)
			return;
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, labelSize);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, labelColour);
		nvgText(vg, box.size.x * 0.5f, box.size.y * 0.5f, label.c_str(), NULL);
	}

	void setValueSafe(float value) {
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq)
			pq->setValue(value);
	}
};

/** One of the thirteen keyboard pads. White keys are also the step buttons. */
struct PianoKey : PanelButton {
	PianoKey() {
		face = nvgRGB(0xd6, 0xd8, 0xdb);
		facePressed = nvgRGB(0x9b, 0x9e, 0xa3);
		labelColour = nvgRGB(0x1a, 0x1b, 0x1d);
		radius = 0.8f;
	}
};

struct BlackKey : PianoKey {
	BlackKey() {
		face = nvgRGB(0x26, 0x28, 0x2c);
		facePressed = nvgRGB(0x55, 0x59, 0x60);
		labelColour = nvgRGB(0xd0, 0xd2, 0xd6);
		labelSize = 4.4f;
	}
};

/** Clicking toggles. Used for SHIFT and PATTERN, which have no action of their own. */
struct LatchButton : PanelButton {
	LatchButton() {
		momentary = false;
	}

	NVGcolor currentFace() override {
		return isOn() ? faceLatched : face;
	}
};

/** A tap does the button's own job; pressing and holding latches it as a modifier.
The module sees only "held or not", exactly as the hardware does. */
struct TapHoldButton : PanelButton {
	static constexpr double LATCH_SECONDS = 0.25;

	void onDragStart(const DragStartEvent& e) override {
		releasing = sticky;
		if (releasing) {
			sticky = false;
			return;
		}
		pressedAt = system::getTime();
		PanelButton::onDragStart(e);
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (releasing) {
			releasing = false;
			setValueSafe(0.f);
			return;
		}
		if (system::getTime() - pressedAt >= LATCH_SECONDS) {
			sticky = true;
			return;
		}
		PanelButton::onDragEnd(e);
	}

private:
	double pressedAt = 0.0;
	bool releasing = false;
};

/** The (KB) and (STEP) arrows. */
struct ArrowButton : TapHoldButton {
	bool pointsRight = true;

	void drawGlyph(NVGcontext* vg) override {
		const float inset = box.size.x * 0.3f;
		const float top = box.size.y * 0.25f;
		const float bottom = box.size.y * 0.75f;
		const float tip = pointsRight ? box.size.x - inset : inset;
		const float back = pointsRight ? inset : box.size.x - inset;

		nvgBeginPath(vg);
		nvgMoveTo(vg, tip, box.size.y * 0.5f);
		nvgLineTo(vg, back, top);
		nvgLineTo(vg, back, bottom);
		nvgClosePath(vg);
		nvgFillColor(vg, nvgRGB(0xe6, 0xe7, 0xe9));
		nvgFill(vg);
	}
};

} // namespace m32
