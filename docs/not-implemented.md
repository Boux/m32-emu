# What this does not do yet

Everything below is measured against the Mother-32 user manual v2 (July 2020). Page numbers refer
to it. The goal of the project is that someone can learn the instrument here and have that knowledge
transfer to the hardware, so this file exists to say exactly where that breaks down.

Three separate categories, and the difference matters:

- **Missing** - the feature does not exist. You cannot learn it here.
- **Approximated** - it exists and behaves correctly, but a number in it is a guess because the
  manual never states it. It will feel slightly different from hardware.
- **Deviations** - it works differently on purpose, and why.

---

## Missing features

### Setup mode (p58-60)

The entire global settings menu. Entered on hardware with SHIFT + RESET + SET END + step key 8,
which is also the only way in or out. Eight pages:

| Page | Parameter | Status of what it configures |
|---|---|---|
| 1 | ASSIGN output jack, 16 sources | only source 2, Sequencer Clock, exists |
| 2 | MIDI channel 1-16 | no MIDI at all |
| 3 | TEMPO input jack mode, 4 modes | none of the four exist |
| 4 | Clock input PPQN | no external clock |
| 5 | Clock output PPQN | clock output is fixed at one pulse per step |
| 6 | Save mode | only Manual Save works |
| 7 | reserved | nothing to do |
| 8 | seven on/off toggles | see below |

Page 8 toggles, none of which exist: Follow MIDI Clock, Follow MIDI Start/Stop, Clock Output Swing,
Accent Out CV Only, Tempo Input Range, Delay Pattern Change, Load Saved Timing.

Two of those have fixed behaviour here that happens to match the hardware default (p60): pattern
changes take effect immediately (Delay Pattern Change off), and loading a pattern restores the
timing saved with it (Load Saved Timing on).

### ASSIGN output sources (p52)

Fifteen of the sixteen are missing. Only Sequencer Clock works, which is the hardware default.
Absent: Accent, Clock/2, Clock/4, Step Ramp, Step Saw, Step Triangle, Step Random, Step 1 Trigger,
MIDI Velocity, MIDI Channel Pressure, MIDI Pitch Bend, MIDI CC 1, CC 2, CC 4 and CC 7.

Step Random and Step Ramp in particular are real performance tools, not just configuration.

### MIDI input (p56-57)

Nothing. No note input, clock, start/stop, velocity, channel pressure, pitch bend or CC. The panel
has no MIDI DIN jack drawn on it either, since Rack routes MIDI through its own modules.

### The TEMPO input's four modes (p55)

The jack exists but does nothing. All four modes are missing:

1. **Tempo CV** - summed with the TEMPO knob, -5 to +5 V spanning 20 to 300 BPM
2. **Single Clock Advance** (the hardware default) - one step per rising edge
3. **Analog Clock** - sync to any regular clock at the configured PPQN
4. **Step Address CV** - the sequencer is not clocked at all; the input voltage selects the step
   directly, so a ramp LFO plays the pattern and any other shape plays it in another order

Step Address CV is a whole playing technique that cannot be learned here.

### External and MIDI clock sync (p54-55)

The clock priority rules are not implemented: analog clock overrides MIDI clock overrides internal.
Neither is PPQN in or out, nor pre-arming the sequencer for an external clock with
(KB) + (STEP) + RUN/STOP (p40).

The Tempo LED therefore never turns green, since green means an external clock source.

### Save modes (p38)

Only Manual Save. Auto Save (edits commit as you make them, PATTERN + RESET restores the original)
and Write Protect (edits are playable but cannot be saved) are missing.

---

## Approximated

These work, but the manual never gives a number, so the value is a judgement call and will not match
hardware exactly.

| Control | Assumed | Note |
|---|---|---|
| FREQUENCY knob range | plus or minus 2 octaves | manual only fixes the centre position, at middle C |
| VCO linear FM depth | 200 Hz per volt | manual gives only the input range |
| Attack time | 1 ms to 4 s | not stated |
| Decay time | 1 ms to 10 s | not stated |
| Glide time | 0 to 4 s | not stated |
| Accent depth | 1.5 V of cutoff, 1.4x VCA gain | manual says only "a fixed level" |
| Filter saturation curve | tuned so self-oscillation reaches about 4 V | manual gives only the output range |
| Pattern length after initialize | 16 steps | not stated |

Everything the manual *does* state numerically is implemented to match, and is listed in
`behavior-spec.md`.

---

## Deliberate deviations

**Modifier buttons latch.** A mouse has one pointer and cannot hold SHIFT while pressing a step key.
Clicking a modifier turns it on until clicked off; KB and STEP tap for their own action and latch on
a press longer than 250 ms. This is the one unavoidable difference, and it is why setup mode's
four-button entry chord is not reachable yet. `input-model.md` covers it in full.

**Right-click Initialize.** Rack's own reset, which clears the working pattern, all 64 saved slots
and every panel mode. No hardware equivalent; the panel's own SHIFT + PATTERN + RESET still works
and only clears the working pattern.

**An optional state readout.** Off by default, since the hardware says everything through its LEDs
and learning to read them is the point. Available in the right-click menu as a learning aid.

**No MIDI DIN jack on the panel.** There is nothing for it to do.

---

## What is covered

For contrast, the parts you can learn here as they are on hardware: the whole synth voice and its
modulation routing, all 32 patch points with their normalling and voltage ranges, the sequencer
including gate length, ties, rests, accents, glide, ratchets, swing amount and interval, clock
division, the four playback orders, hold and reset, both KB and STEP editing workflows, record with
step-write, set end step, page select, pattern and bank selection, save, restore and initialize, the
live accent, mute and ratchet performance functions, and the complete LED language.
