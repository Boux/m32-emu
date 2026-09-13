#include <rack.hpp>
#include <cstdio>
#include <cmath>
#include "../src/M32.hpp"

using namespace rack;
using namespace m32;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail) {
	printf("%s %-50s %s\n", ok ? "  ok " : "FAIL", name, detail);
	if (!ok)
		failures++;
}

static void checkNear(const char* name, float actual, float expected, float tolerance) {
	char detail[128];
	snprintf(detail, sizeof(detail), "got %.2f, expected %.2f +/- %.2f", actual, expected, tolerance);
	check(name, std::fabs(actual - expected) <= tolerance, detail);
}

static void checkEq(const char* name, int actual, int expected) {
	char detail[96];
	snprintf(detail, sizeof(detail), "got %d, expected %d", actual, expected);
	check(name, actual == expected, detail);
}

static const float SR = 48000.f;

/** Drives the real module the way Rack does. */
struct Harness {
	M32 module;
	engine::Module::ProcessArgs args;

	Harness() {
		args.sampleRate = SR;
		args.sampleTime = 1.f / SR;
		args.frame = 0;
		set(M32::VCA_MODE_PARAM, 1.f); // VCA on, so pitch can be measured without the envelope
	}

	void set(int param, float value) {
		module.params[param].setValue(value);
	}

	void frames(int count) {
		for (int i = 0; i < count; i++) {
			module.process(args);
			args.frame++;
		}
	}

	/** A mouse click: press, hold a few frames, release. */
	void click(int param) {
		set(param, 1.f);
		frames(8);
		set(param, 0.f);
		frames(8);
	}

	/** Measures the oscillator's pitch from the saw jack, independent of the envelope. */
	float sawHz(float seconds = 0.25f) {
		int crossings = 0;
		float previous = module.outputs[M32::VCO_SAW_OUTPUT].getVoltage();
		const int total = int(seconds * SR);
		for (int i = 0; i < total; i++) {
			module.process(args);
			args.frame++;
			const float now = module.outputs[M32::VCO_SAW_OUTPUT].getVoltage();
			if (previous < 0.f && now >= 0.f)
				crossings++;
			previous = now;
		}
		return crossings / seconds;
	}

	float gateVolts() {
		return module.outputs[M32::GATE_OUTPUT].getVoltage();
	}

	bool running() {
		return module.sequencer.running;
	}
};

static float hzForVolts(float volts) {
	return dsp::FREQ_C4 * std::pow(2.f, volts);
}

/** Holding a key with the sequencer stopped should sound that key's pitch. */
static void testKeysPlayWhenStopped() {
	Harness rig;

	rig.set(M32::KEY_PARAM + 0, 1.f);
	rig.frames(64);
	checkNear("a held key opens the gate", rig.gateVolts(), 5.f, 0.1f);
	// Octave 4, low C. Octave 5 low C is 0 V, so octave 4 is -1 V.
	checkNear("low C at octave 4 sounds -1 V", rig.sawHz(), hzForVolts(-1.f), 3.f);
	rig.set(M32::KEY_PARAM + 0, 0.f);
	rig.frames(64);

	rig.set(M32::KEY_PARAM + 4, 1.f); // G
	rig.frames(64);
	checkNear("the G key sounds a fifth higher", rig.sawHz(), hzForVolts(-1.f + 7.f / 12.f), 3.f);
	rig.set(M32::KEY_PARAM + 4, 0.f);
	rig.frames(64);

	rig.set(M32::KEY_PARAM + 7, 1.f); // upper C
	rig.frames(64);
	checkNear("the upper C key sounds an octave higher", rig.sawHz(), hzForVolts(0.f), 3.f);
}

static void testRunStopToggles() {
	Harness rig;
	rig.click(M32::RUN_STOP_PARAM);
	check("RUN/STOP starts the sequencer", rig.running(), "running");
	rig.click(M32::RUN_STOP_PARAM);
	check("RUN/STOP stops it again", !rig.running(), "stopped");
}

/** After stopping, the keyboard has to take the voice back. */
static void testKeysPlayAfterStopping() {
	Harness rig;
	rig.click(M32::RUN_STOP_PARAM);
	rig.frames(int(SR * 0.5f));
	rig.click(M32::RUN_STOP_PARAM);
	rig.frames(64);

	rig.set(M32::KEY_PARAM + 4, 1.f);
	rig.frames(64);
	checkNear("a key still opens the gate after stopping", rig.gateVolts(), 5.f, 0.1f);
	checkNear("and still sounds its own pitch", rig.sawHz(), hzForVolts(-1.f + 7.f / 12.f), 3.f);
}

/** Pressing a key while running transposes, and low C at the default octave means no change. */
static void testTransposeWhileRunning() {
	Harness rig;
	rig.click(M32::RUN_STOP_PARAM);
	rig.frames(int(SR * 0.3f));
	const float untransposed = rig.sawHz();

	rig.set(M32::KEY_PARAM + 0, 1.f); // low C, the reference note
	rig.frames(int(SR * 0.5f));
	checkNear("low C at the default octave transposes by nothing", rig.sawHz(), untransposed, 3.f);
	rig.set(M32::KEY_PARAM + 0, 0.f);
	rig.frames(int(SR * 0.3f));

	rig.set(M32::KEY_PARAM + 7, 1.f); // upper C
	rig.frames(int(SR * 0.5f));
	checkNear("the upper C key transposes up an octave", rig.sawHz(), untransposed * 2.f, 6.f);
}

/** Transposition should stay put after the key is released, as a performance control. */
static void testTransposeLatches() {
	Harness rig;
	rig.click(M32::RUN_STOP_PARAM);
	rig.frames(int(SR * 0.3f));
	const float untransposed = rig.sawHz();

	rig.set(M32::KEY_PARAM + 7, 1.f);
	rig.frames(int(SR * 0.3f));
	rig.set(M32::KEY_PARAM + 7, 0.f);
	rig.frames(int(SR * 0.5f));

	checkNear("transposition holds after the key is released", rig.sawHz(), untransposed * 2.f, 6.f);
}

/** p56: these jacks are level driven. A high level runs, a low level stops, and it is not a toggle. */
static void testTransportJacksAreLevelDriven() {
	Harness rig;
	auto jack = [&](int input, float volts) {
		rig.module.inputs[input].setVoltage(volts);
		rig.frames(32);
	};

	jack(M32::RUN_STOP_INPUT, 5.f);
	check("a high RUN/STOP jack runs the sequencer", rig.running(), "running");
	jack(M32::RUN_STOP_INPUT, 0.f);
	check("a low RUN/STOP jack stops it", !rig.running(), "stopped");
	jack(M32::RUN_STOP_INPUT, 5.f);
	check("high again runs it rather than toggling off", rig.running(), "running");

	// RESET high parks the pattern on step 1 and repeats it.
	rig.frames(int(SR * 0.4f));
	jack(M32::RESET_INPUT, 5.f);
	rig.frames(int(SR * 0.6f));
	checkEq("a high RESET jack holds the pattern on step 1", rig.module.sequencer.currentStep, 0);

	jack(M32::RESET_INPUT, 0.f);
	rig.frames(int(SR * 0.6f));
	check("releasing RESET lets it advance again", rig.module.sequencer.currentStep != 0, "advancing");

	// HOLD high repeats whatever step is current.
	const int held = rig.module.sequencer.currentStep;
	jack(M32::HOLD_INPUT, 5.f);
	rig.frames(int(SR * 0.6f));
	checkEq("a high HOLD jack repeats the current step", rig.module.sequencer.currentStep, held);
}

/** p56: the transport jacks need about +3.2 V, so a 2 V signal must be ignored. */
static void testTransportJackThreshold() {
	Harness rig;
	rig.module.inputs[M32::RUN_STOP_INPUT].setVoltage(2.f);
	rig.frames(32);
	check("2 V is below the transport threshold", !rig.running(), "ignored");

	rig.module.inputs[M32::RUN_STOP_INPUT].setVoltage(5.f);
	rig.frames(32);
	check("5 V crosses it", rig.running(), "accepted");
}

static void testOctaveButtonsMoveKeyboard() {
	Harness rig;
	rig.click(M32::STEP_PARAM);
	rig.set(M32::KEY_PARAM + 0, 1.f);
	rig.frames(64);
	checkNear("raising the octave raises the key's pitch", rig.sawHz(), hzForVolts(0.f), 3.f);
}

int main() {
	random::init();

	printf("\nM32 module integration checks\n\n");
	testKeysPlayWhenStopped();
	testRunStopToggles();
	testKeysPlayAfterStopping();
	testTransposeWhileRunning();
	testTransposeLatches();
	testTransportJacksAreLevelDriven();
	testTransportJackThreshold();
	testOctaveButtonsMoveKeyboard();

	printf("\n%s (%d failures)\n\n", failures == 0 ? "all checks passed" : "CHECKS FAILED", failures);
	return failures == 0 ? 0 : 1;
}
