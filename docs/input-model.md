# Translating panel chords to a mouse

The hardware expects you to hold one button with a finger and press another with a second finger.
A mouse has one pointer, so the combos need a translation. This is the one place where the module
deliberately departs from the hardware, so it is written down here.

## Modifiers latch

Clicking SHIFT turns it on and leaves it on, lit amber. The next click completes the combo. SHIFT
stays on until you click it again, which keeps the multi-step flows (SET END, then a page, then a
step) down to one click each.

The computer's Shift key is not used for this. Rack already binds Shift+drag on knobs to fast
adjustment and Ctrl+drag to fine adjustment, which collides with SHIFT+GLIDE for ratchet and
SHIFT+TEMPO for swing, the two combos that need a held modifier most.

Four buttons act as modifiers, in two groups:

- **SHIFT and PATTERN** have no action of their own, so a click simply toggles the latch.
- **KB and STEP** do have their own actions, so they distinguish tap from hold. A quick click moves
  the octave or rotates the pattern. Pressing and holding past 250 ms latches them as a modifier.

A latched arrow that ends up unused still fires its own action when released, which is what the
hardware does when you press and release one without doing anything in between.

## The module never learns about any of this

The latch lives entirely in the widget. All it produces is a param that reads high while the button
counts as held, so the module sees the same button states the hardware sees and every rule in
`behavior-spec.md` is implemented directly against the manual. Driving the module from a MIDI
controller with real buttons would need no changes to the sequencer code.

## One knob, several meanings

GLIDE and TEMPO each drive several values depending on what is latched. This is true of the hardware
too, and it is the reason both need catch-up.

**GLIDE**

| Context | Meaning |
|---|---|
| nothing latched, no step selected | glide time |
| nothing latched, step selected | glide on/off for that step, and still the glide rate |
| SHIFT | ratchet count 1–4 |
| KB | swing interval, dotted |
| STEP | swing interval, triplet |
| KB + STEP | swing interval, straight |

**TEMPO**

| Context | Meaning |
|---|---|
| nothing latched, no step selected | tempo |
| nothing latched, step selected | gate length, 1/8 to 8/8 |
| SHIFT | swing amount, -100% to +100% |

Releasing a modifier leaves the knob pointing somewhere unrelated to what it now controls, so the
value it returns to holds still until the knob crosses it, then follows again. The manual describes
the same behavior on p19.

A modifier only takes a knob over once the knob actually moves. Holding KB and never touching GLIDE
still counts as a tap, so the octave changes on release.

## Buttons that mean two things

RESET/ACCENT and HOLD/REST resolve by priority:

1. a step is selected for editing → accent toggle / rest toggle
2. otherwise SHIFT is latched → live accent / live mute
3. otherwise → reset / hold
