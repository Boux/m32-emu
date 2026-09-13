#pragma once
#include <rack.hpp>

namespace m32 {

using namespace rack;

/** Attack/decay envelope with a sustain switch.
With sustain on, the envelope holds at full while the gate is high and overlapping gates play legato.
With sustain off, attack falls straight into decay when it completes or when the gate is released,
whichever happens first, and every gate retriggers. */
struct Envelope {
	static constexpr float MIN_TIME = 0.001f;
	static constexpr float MAX_ATTACK = 4.f;
	static constexpr float MAX_DECAY = 10.f;

	float value = 0.f;
	float attackTime = MIN_TIME;
	float decayTime = MIN_TIME;
	bool sustainEnabled = false;

	void setGate(bool gate) {
		if (gate && !gateHigh)
			trigger();
		if (!gate && gateHigh && stage != Stage::IDLE)
			stage = Stage::DECAY;
		gateHigh = gate;
	}

	/** Restarts the attack stage even if a gate is already held, for ratchets and step retriggers. */
	void retrigger() {
		gateHigh = true;
		stage = Stage::ATTACK;
	}

	void process(float dt) {
		if (stage == Stage::ATTACK)
			processAttack(dt);
		else if (stage == Stage::DECAY)
			processDecay(dt);
		else if (stage == Stage::SUSTAIN)
			value = 1.f;
	}

	void reset() {
		value = 0.f;
		stage = Stage::IDLE;
		gateHigh = false;
	}

	bool isActive() const {
		return stage != Stage::IDLE;
	}

private:
	enum class Stage { IDLE, ATTACK, SUSTAIN, DECAY };

	/** Attack aims past full scale so the approach curve is the analog RC shape rather than a ramp. */
	static constexpr float ATTACK_TARGET = 1.2f;
	static constexpr float ATTACK_SHAPE = 1.7918f; // ln(ATTACK_TARGET / (ATTACK_TARGET - 1))

	Stage stage = Stage::IDLE;
	bool gateHigh = false;

	void trigger() {
		if (sustainEnabled && stage != Stage::IDLE)
			return;
		stage = Stage::ATTACK;
	}

	void processAttack(float dt) {
		const float tau = std::max(attackTime, MIN_TIME) / ATTACK_SHAPE;
		value += (ATTACK_TARGET - value) * (1.f - std::exp(-dt / tau));
		if (value < 1.f)
			return;
		value = 1.f;
		stage = (sustainEnabled && gateHigh) ? Stage::SUSTAIN : Stage::DECAY;
	}

	void processDecay(float dt) {
		const float tau = std::max(decayTime, MIN_TIME) / 4.6052f; // ln(100), decay to 1% of full
		value *= std::exp(-dt / tau);
		if (value > 1e-4f)
			return;
		value = 0.f;
		stage = Stage::IDLE;
	}
};

} // namespace m32
