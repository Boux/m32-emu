# M32

A VCV Rack 2 module that reproduces the behavior of a 60HP semi-modular analog synthesizer:
one oscillator, a ladder filter, a 32-step sequencer and a 32-point patchbay.

The goal is behavioral parity, not circuit-level emulation. Every control, jack, normalled connection
and sequencer rule should do what the hardware does. The audio is a clean virtual-analog model rather
than a component simulation.

`docs/behavior-spec.md` is the reference, derived from the hardware's user manual.

## Building

    make sdk          # download the Rack SDK this builds against (2.6.6)
    make              # build plugin.so
    make test         # run the offline DSP, sequencer and panel checks
    make install      # package and install into your Rack user folder
    make build-number # print the current build number

## Build number

Every build that follows a source change bumps a counter and prints it:

    ==> M32 build 47

The same number is printed at the bottom of the panel and repeated in the module's right-click menu
along with the build time, so you can tell at a glance whether Rack has the build you just made.
A build with no source changes does not bump it.

The counter lives in `.build-number` and the generated header in `src/generated/`, both untracked.
It is a staleness indicator, not a release version; `plugin.json` keeps its own stable version so
installed packages do not multiply.

`make test` links against the SDK's `libRack` and needs no Rack installation.
`tools/screenshot.sh` renders the module in a real Rack under Xvfb; see `tools/README.md`.

## Status

Working:

- Full sound engine: oscillator (saw/pulse with PWM), noise, mixer, 24 dB/oct ladder filter in
  low pass and high pass, VCA, attack/decay envelope with sustain switch, LFO to audio rate, glide
- All 32 patch points with the hardware's normalling, plus the mult and the VC mixer
- Sequencer playback: up to 32 steps, gate length, ties, rests, accents, glide per step, ratchets 1-4,
  swing amount and swing interval, forward/reverse/pendulum/random order, hold, reset
- 64 pattern slots and full patch persistence
- Live keyboard play, octave selection, transposition during playback
- The full panel editing workflow: KB and STEP modes, record with step-write, step select and edit,
  set end step, page select, save/restore/initialize, bank and pattern selection, playback order,
  pattern rotate, live accent, live mute and live ratchet
- Knob catch-up, so GLIDE and TEMPO do not jump when a modifier releases them

Modifier buttons latch, because a mouse cannot hold one button while pressing another.
`docs/input-model.md` explains the translation and the knob multiplexing.

Not yet built:

- The optional step-grid overlay
- MIDI input, setup mode, and the remaining 15 ASSIGN output sources
- External and MIDI clock sync, PPQN settings, TEMPO input modes
- Save modes beyond Manual (Auto Save and Write Protect are setup-mode options)

## Naming

This is an independent implementation. It is not affiliated with or endorsed by any hardware
manufacturer, and deliberately uses none of their trademarks.
