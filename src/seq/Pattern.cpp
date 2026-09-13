#include "Pattern.hpp"

namespace m32 {

float noteLengthBeats(NoteLength length, NoteForm form) {
	static const float BASE[8] = {8.f, 4.f, 2.f, 1.f, 0.5f, 0.25f, 0.125f, 0.0625f};
	static const float FORM[3] = {1.f, 1.5f, 2.f / 3.f};
	return BASE[int(length)] * FORM[int(form)];
}

static int readEnum(json_t* j, const char* key, int fallback, int count) {
	json_t* v = json_object_get(j, key);
	if (!v)
		return fallback;
	return clamp(int(json_integer_value(v)), 0, count - 1);
}

json_t* Timing::toJson() const {
	json_t* j = json_object();
	json_object_set_new(j, "clockDivision", json_integer(int(clockDivision)));
	json_object_set_new(j, "clockForm", json_integer(int(clockForm)));
	json_object_set_new(j, "swingInterval", json_integer(int(swingInterval)));
	json_object_set_new(j, "swingForm", json_integer(int(swingForm)));
	json_object_set_new(j, "swingAmount", json_real(swingAmount));
	return j;
}

void Timing::fromJson(json_t* j) {
	if (!j)
		return;
	clockDivision = NoteLength(readEnum(j, "clockDivision", int(NoteLength::SIXTEENTH), 8));
	clockForm = NoteForm(readEnum(j, "clockForm", int(NoteForm::STRAIGHT), 3));
	swingInterval = NoteLength(readEnum(j, "swingInterval", int(NoteLength::EIGHTH), 8));
	swingForm = NoteForm(readEnum(j, "swingForm", int(NoteForm::STRAIGHT), 3));

	json_t* amountJ = json_object_get(j, "swingAmount");
	if (amountJ)
		swingAmount = clamp(float(json_number_value(amountJ)), -1.f, 1.f);
}

json_t* Pattern::toJson() const {
	json_t* j = json_object();
	json_object_set_new(j, "endStep", json_integer(endStep));
	json_object_set_new(j, "timing", timing.toJson());

	json_t* stepsJ = json_array();
	for (const Step& step : steps)
		json_array_append_new(stepsJ, step.toJson());
	json_object_set_new(j, "steps", stepsJ);
	return j;
}

void Pattern::fromJson(json_t* j) {
	if (!j)
		return;

	json_t* endJ = json_object_get(j, "endStep");
	if (endJ)
		endStep = clamp(int(json_integer_value(endJ)), 0, MAX_STEPS - 1);

	timing.fromJson(json_object_get(j, "timing"));

	json_t* stepsJ = json_object_get(j, "steps");
	if (!stepsJ)
		return;
	for (int i = 0; i < MAX_STEPS; i++)
		steps[i].fromJson(json_array_get(stepsJ, i));
}

json_t* PatternMemory::toJson() const {
	json_t* j = json_array();
	for (const Pattern& pattern : slots)
		json_array_append_new(j, pattern.toJson());
	return j;
}

void PatternMemory::fromJson(json_t* j) {
	if (!j)
		return;
	for (int i = 0; i < PATTERN_SLOTS; i++)
		slots[i].fromJson(json_array_get(j, i));
}

} // namespace m32
