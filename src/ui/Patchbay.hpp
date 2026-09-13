#pragma once
#include "../M32.hpp"

namespace m32 {

struct PatchbayCell {
	int portId;
	bool isOutput;
	const char* label;
};

/** The patchbay grid exactly as it reads on the hardware: four columns, eight rows.
Output labels are printed white-on-black, inputs black-on-white. */
static constexpr PatchbayCell PATCHBAY[8][4] = {
	{{M32::EXT_AUDIO_INPUT, false, "EXT. AUDIO"}, {M32::MIX_CV_INPUT, false, "MIX CV"},
	 {M32::VCA_CV_INPUT, false, "VCA CV"}, {M32::VCA_OUTPUT, true, "VCA"}},

	{{M32::NOISE_OUTPUT, true, "NOISE"}, {M32::VCF_CUTOFF_INPUT, false, "VCF CUTOFF"},
	 {M32::VCF_RES_INPUT, false, "VCF RES."}, {M32::VCF_OUTPUT, true, "VCF"}},

	{{M32::VCO_1VOCT_INPUT, false, "VCO 1V/OCT"}, {M32::VCO_LIN_FM_INPUT, false, "VCO LIN FM"},
	 {M32::VCO_SAW_OUTPUT, true, "VCO SAW"}, {M32::VCO_PULSE_OUTPUT, true, "VCO PULSE"}},

	{{M32::VCO_MOD_INPUT, false, "VCO MOD"}, {M32::LFO_RATE_INPUT, false, "LFO RATE"},
	 {M32::LFO_TRI_OUTPUT, true, "LFO TRI"}, {M32::LFO_SQ_OUTPUT, true, "LFO SQ"}},

	{{M32::MIX_1_INPUT, false, "MIX 1"}, {M32::MIX_2_INPUT, false, "MIX 2"},
	 {M32::VC_MIX_CTRL_INPUT, false, "VC MIX CTRL"}, {M32::VC_MIX_OUTPUT, true, "VC MIX"}},

	{{M32::MULT_INPUT, false, "MULT"}, {M32::MULT_1_OUTPUT, true, "MULT 1"},
	 {M32::MULT_2_OUTPUT, true, "MULT 2"}, {M32::ASSIGN_OUTPUT, true, "ASSIGN"}},

	{{M32::GATE_INPUT, false, "GATE"}, {M32::EG_OUTPUT, true, "EG"},
	 {M32::KB_OUTPUT, true, "KB"}, {M32::GATE_OUTPUT, true, "GATE"}},

	{{M32::TEMPO_INPUT, false, "TEMPO"}, {M32::RUN_STOP_INPUT, false, "RUN / STOP"},
	 {M32::RESET_INPUT, false, "RESET"}, {M32::HOLD_INPUT, false, "HOLD"}},
};

} // namespace m32
