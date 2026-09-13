# M32

A VCV Rack 2 module that recreates the Moog Mother-32: one oscillator, a Moog ladder filter, a
32-step sequencer and a 32-point patchbay, on a 60HP panel.

Not affiliated with or endorsed by Moog Music. MOOG and MOTHER-32 are their trademarks.

## Disclaimer

This is entirely vibe-coded. Every line was written by an LLM, working from the Mother-32 user manual.

It is **not** a sound-alike. The audio is a clean virtual-analog model, not a circuit simulation.

The goal is **learning the instrument**, not replacing it. Panel layout, button combos, LED behaviour
and sequencer rules follow the manual as closely as possible, so what you learn here should transfer
to the real thing.

## Install

Prerequisites:

| OS | Install |
|---|---|
| Linux | `gcc make git curl unzip jq zstd` |
| macOS | Xcode command line tools, then `brew install jq zstd` |
| Windows | [MSYS2](https://www.msys2.org) MinGW64, then `pacman -S mingw-w64-x86_64-gcc make git curl unzip jq zstd` |

Then:

```
git clone <this repo> && cd moog-mother32-emu
make sdk
make install
```

Restart Rack. The module appears in the browser under **M32**.

`make sdk` downloads the Rack SDK for your platform. `make install` builds and copies the plugin to:

| OS | Path |
|---|---|
| Linux | `~/.local/share/Rack2/plugins-lin-x64/` |
| macOS | `~/Library/Application Support/Rack2/plugins-mac-<arch>/` |
| Windows | `%LOCALAPPDATA%\Rack2\plugins-win-x64\` |

The build number is printed on the panel and in the right-click menu, so you can tell which build
Rack has loaded.

## Features

### Implemented

- VCO with saw and pulse, pulse width modulation, 1V/oct, linear FM
- Noise generator, mixer, external audio input
- Moog ladder filter, 24 dB/oct low pass and high pass, self-oscillation
- VCA, attack/decay envelope with sustain switch, LFO to audio rate, glide
- All 32 patch points with the hardware's normalling and voltage ranges
- Mult and voltage controlled mixer
- 32-step sequencer: gate length, ties, rests, accents, per-step glide, ratchets 1-4
- Swing amount and swing interval, clock division, forward/reverse/pendulum/random
- Hold, reset, settable end step
- KB and STEP modes, record with step-write, step select and edit
- 64 pattern slots across 8 banks, save, restore, initialize, bank and pattern select
- Live accent, live mute, live ratchet, live transpose
- The full LED language, including the transient readouts and save animations
- Everything saves with the Rack patch

### Not implemented

- Setup mode, all 8 pages
- MIDI input: note, clock, CC, velocity, pitch bend, aftertouch
- External and MIDI clock sync, PPQN settings, clock priority
- The TEMPO input's four modes, including Step Address CV
- 15 of the 16 ASSIGN output sources; only Sequencer Clock works
- Auto Save and Write Protect save modes

One deliberate difference: modifier buttons **latch**, because a mouse cannot hold one button while
pressing another.

Details in `docs/` — `behavior-spec.md` (what the manual says), `input-model.md` (the mouse
translation), `not-implemented.md` (the full gap list).
