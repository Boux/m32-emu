# M32 behavior specification

Everything here is taken from the Mother-32 user manual v2 (July 2020), which sits in the repo root.
Page numbers refer to that manual. This file is the reference for behavioral parity; the goal is that
every control does what the hardware does, not that the audio matches sample for sample.

## Signal path (p9)

    VCO ─┐
         ├─ MIXER ─ VCF ─ VCA ─ VOLUME ─ audio out
    NOISE┘           ▲     ▲
    (normalled to    │     │
     EXT AUDIO)     LFO/EG LFO/EG

Noise is normalled to the EXT AUDIO input. Patching EXT AUDIO replaces noise in the mixer.

## Sound engine

| Block | Behavior | Source |
|---|---|---|
| VCO | Saw or pulse, one at a time via VCO WAVE. Both have dedicated output jacks. | p11 |
| Pulse width | 2% to 98% from the knob. Modulation can reach 0% or 100%, which silences the output by design. | p11 |
| LFO | Triangle and square, 0.1 Hz to ~350 Hz, up to 600 Hz with CV at the LFO RATE jack. | p13 |
| VCF | Moog ladder, 24 dB/oct. LOW PASS is resonant; HI PASS is non-resonant. Cutoff 20 Hz–20 kHz. | p14 |
| VCF resonance | Self-oscillates above 3 o'clock. In HI PASS mode the resonance knob reintroduces low end and is documented as not working properly. | p14 |
| HI PASS resonance | Achieved by patching VCF out into EXT AUDIO; the MIX knob then acts as resonance and can self-oscillate. | p14 |
| EG | Attack and decay. SUSTAIN ON holds at full while gated and plays legato without retriggering. SUSTAIN OFF moves attack straight to decay on completion or note release, whichever is first, and every note retriggers. | p15–16 |
| VCA | EG or ON. | p16 |
| Glide | Portamento on notes from keyboard, MIDI and sequencer. Rate is global, never per-step. | p17, p26 |
| Accent | Routed internally to VCF and VCA at a fixed level. | p23 |

### VCO calibration reference (p61)

Octave 5, low C, FREQUENCY centered → KB CV outputs 0 V → middle C (C4, 261.6 Hz).
Octave 7, low C → C6 (1046.5 Hz). This fixes 1 V/oct with 0 V at C4, matching Rack's convention.

## Patchbay (p46–52)

4 columns × 8 rows, 32 jacks: 18 inputs, 14 outputs.

|  | col 1 | col 2 | col 3 | col 4 |
|---|---|---|---|---|
| 1 | EXT. AUDIO | MIX CV | VCA CV | **VCA** |
| 2 | **NOISE** | VCF CUTOFF | VCF RES. | **VCF** |
| 3 | VCO 1V/OCT | VCO LIN FM | **VCO SAW** | **VCO PULSE** |
| 4 | VCO MOD | LFO RATE | **LFO TRI** | **LFO SQ** |
| 5 | MIX 1 | MIX 2 | VC MIX CTRL | **VC MIX** |
| 6 | MULT | **MULT 1** | **MULT 2** | **ASSIGN** |
| 7 | GATE | **EG** | **KB** | **GATE** |
| 8 | TEMPO | RUN / STOP | RESET | HOLD |

Bold entries are outputs.

Normalling and special cases:
- NOISE is normalled to EXT AUDIO (p9).
- MIX 1 is normalled to 0 V, MIX 2 to +5 V, so the VC mixer doubles as a voltage source, attenuator and VCA. It is not connected to the synth voice and needs patch cables to do anything (p21).
- Patching VCO MOD while VCO MOD SOURCE is set to EG/VCO MOD replaces the EG with the external signal (p12).
- GATE input accepts 0 to +5 V and tolerates 10 V (p53).
- Hardware outputs are 0 to +5 V for gates and clocks (p52).

### ASSIGN output sources (p52)

1 ACCENT · 2 CLOCK (default) · 3 CLOCK/2 · 4 CLOCK/4 · 5 STEP RAMP · 6 STEP SAW · 7 STEP TRIANGLE ·
8 STEP RANDOM · 9 STEP 1 TRIGGER · 10 MIDI VELOCITY · 11 MIDI CHANNEL PRESSURE · 12 MIDI PITCH BEND ·
13 MIDI CC 1 · 14 MIDI CC 2 · 15 MIDI CC 4 · 16 MIDI CC 7

Clock and accent sources are 0 to +5 V. Step ramp/saw/triangle/random and the MIDI sources are -5 to +5 V.

## Sequencer data model

Per step (p24–27):
- pitch
- gate length, 1/8 to 8/8 in eight values; 8/8 is a tie that holds into the next step
- accent on/off
- rest on/off
- glide on/off
- ratchet count 1–4

Per pattern:
- up to 32 steps, end step settable
- swing amount and swing interval, both stored with the pattern (p20)
- clock division

Memory: 8 banks × 8 patterns = 64 locations (p23).

## Sequencer behavior

### Modes (p23)
- **KB mode** (default): play from the panel, record notes with step-write, transpose during playback.
- **STEP mode**: enable, mute and edit steps, including during playback. No record function.

Press SHIFT + (KB) or SHIFT + (STEP) to switch.

### The 13-key keyboard is a piano octave

White keys (C D E F G A B C) are also step buttons 1–8. Black keys (C# D# F# G# A#) are also the four
page selectors (1-8, 9-16, 17-24, 25-32) and SET END. The manual confirms this: "play the 'G' key
(located above Step LED 5)" (p34).

### Step-write (KB mode, p25–27)
- Playing a note or entering a rest writes the step and advances.
- Gate length carries over from the previous step when a new note is entered.
- A tie to a new note can also be made by playing the next note while holding the current one.
- A slide needs the previous step tied and glide on the target step.

### Editing (p30, p36)
- SHIFT + step button selects a step for editing without advancing. When stopped, the step's note sounds.
- SHIFT + the currently selected step exits editing.
- SHIFT + a page selector changes page.
- SHIFT + SET END, then SHIFT + page, then SHIFT + step sets the end step. The end step blinks at half tempo.

### Live performance (p29, p41)
- SHIFT + ACCENT: accent every step while held. Not stored.
- SHIFT + REST: mute the output while the sequencer keeps advancing. Not stored.
- SHIFT + rotate GLIDE: ratchet every step 1–4 while held. Not stored.
- SHIFT + rotate TEMPO: swing amount, -100% to +100%, stored per pattern.
- (KB)/(STEP) plus the keyboard transposes during playback. Not stored.
- HOLD repeats the current step for as long as it is held.
- In STEP mode, (KB)/(STEP) rotate the whole pattern forward or backward one step (p35).

Knob catch-up: after releasing SHIFT, the GLIDE and TEMPO knobs chase the physical position until
they catch up rather than jumping (p19).

### Playback order (p40)
Hold (KB) + (STEP) and press step button 1–4: forward, reverse, pendulum, random.

### Pattern memory (p31, p37–39)
- Save: hold SHIFT + RUN/STOP for one second, pick a location, press SHIFT + RUN/STOP again. RUN/STOP alone cancels.
- Restore: PATTERN + RESET.
- Initialize: SHIFT + RESET + PATTERN.
- Save modes: Manual (default), Auto Save, Write Protect (setup page 6).
- Unsaved changes are lost when the pattern location changes, unless Auto Save is on.

### Swing (p19–20)
Swing amount ranges -100% to +100%, 0 at knob center. Negative moves off-beats earlier; at -100% only
off-beats play. Positive moves on-beats later; at +100% only on-beats play.

Swing interval is independent of swing amount and is chosen from eight note lengths (two whole notes
down to a sixty-fourth) in dotted, triplet or straight form:
- hold (KB) + rotate GLIDE for dotted
- hold (STEP) + rotate GLIDE for triplet
- hold both + rotate GLIDE for straight

A step whose duration crosses an on/off-beat boundary finishes at the other phase's rate.

### Clock (p54–55)
Sources: internal, external analog at TEMPO, MIDI clock. Configurable input and output PPQN.
TEMPO input has four modes: tempo CV, single clock advance, analog clock, step address CV.
Pre-arming: hold (KB) + (STEP) and press RUN/STOP to wait for the first external clock.

## Setup mode globals (p58–60)

| Page | Parameter | Default |
|---|---|---|
| 1 | Assignable output jack | Sequencer clock output |
| 2 | MIDI channel | 1 |
| 3 | TEMPO input jack mode | Step advance / trigger |
| 4 | Clock input PPQN | 4 (sixteenth note) |
| 5 | Clock output PPQN | 4 (sixteenth note) |
| 6 | Save mode | Manual |
| 8.1 | Follow MIDI clock | On |
| 8.2 | Follow MIDI start/stop | On |
| 8.3 | Clock output swing | On |
| 8.4 | Accent out CV only | Off |
| 8.5 | Tempo input range | Off (-5 V to +5 V) |
| 8.6 | Delay pattern change | Off (immediate) |
| 8.7 | Load saved timing | On |

Note: p37 describes delay pattern change as normally on, while the defaults table on p60 lists it as
off. The defaults table wins here since it is the explicit reference.

## LED semantics

Octave / location LEDs, eight of them, red/green/yellow:
- red: current keyboard octave (1–8)
- green: current pattern page during record, or pattern location
- yellow: bank number, ratchet count while SHIFT + GLIDE, setup page
- all eight red: gate length readout
- LED 5/6/7/8 green or yellow: glide, ratchet > 1, accent, rest for the step being edited
- blinking green: save destination

Step LEDs, eight, red:
- solid: step has data (KB mode) or step is not a rest (STEP mode)
- blinking at tempo: step being edited
- blinking at half tempo: end step
- lit as the pattern advances: current step

Tempo LED:
- blinking red: internal clock running
- solid yellow: record mode active, clock paused
- alternating yellow and red: a step is being edited in STEP mode
