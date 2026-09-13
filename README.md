# M32

A VCV Rack 2 module that recreates the Moog Mother-32: one oscillator, a Moog ladder filter, a
32-step sequencer and a 32-point patchbay, on a 60HP panel.

**Not affiliated with or endorsed by Moog Music. MOOG and MOTHER-32 are their trademarks.**

https://github.com/user-attachments/assets/4c020a8d-e9ab-4dd4-b65a-432e8bfc305f

## Disclaimer

This is entirely vibe-coded. Every line was written by an LLM, working from the Mother-32 user manual.

The intention is **not** to re-create the exact sound of the original.

I made this so I could have a way to learn how the instrument works and practice with it without having the need to physically get one.

I am not sure if everything works like it's supposed to, I've only done surface-level testing.

## Install

### Easiest: download a build

1. Go to the [Releases page](https://github.com/Boux/m32-emu/releases) and download the
   `.vcvplugin` file for your system.
2. Open VCV Rack. In the menu bar choose **Help → Open user folder**.
3. Inside, open the folder whose name starts with `plugins-`.
4. Drag the `.vcvplugin` file into it.
5. Quit VCV Rack and open it again.

The module appears in the browser under **M32**. Right-click empty rack space to open the browser.

### Building it yourself

You only need this if there is no release for your system.

#### macOS

1. Open **Terminal** (press `Cmd`+`Space`, type `Terminal`, press Enter).
2. Install Apple's compiler. Paste this, press Enter, then click **Install** in the popup and wait:

   ```
   xcode-select --install
   ```

3. Install [Homebrew](https://brew.sh) if you do not already have it, then:

   ```
   brew install jq zstd
   ```

4. Download and build:

   ```
   git clone https://github.com/Boux/m32-emu.git
   cd m32-emu
   make sdk
   make install
   ```

5. Quit VCV Rack and open it again.

#### Windows

1. Download and run the installer from [msys2.org](https://www.msys2.org). Accept the defaults.
2. From the Start menu open **MSYS2 MINGW64**. It must be the one named MINGW64, not MSYS or UCRT64.
3. Paste this and press Enter. If the window closes partway, reopen it and run it again:

   ```
   pacman -Syu
   ```

4. Install the build tools. Press Enter to accept when it asks:

   ```
   pacman -S --needed git make curl unzip zstd mingw-w64-x86_64-gcc mingw-w64-x86_64-jq
   ```

5. Download and build:

   ```
   git clone https://github.com/Boux/m32-emu.git
   cd m32-emu
   make sdk
   make install
   ```

6. Quit VCV Rack and open it again.

#### Linux

```
# Debian/Ubuntu
sudo apt install build-essential git curl unzip jq zstd
# Arch
sudo pacman -S base-devel git curl unzip jq zstd

git clone https://github.com/Boux/m32-emu.git && cd m32-emu
make sdk && make install
```

`make sdk` fetches the Rack SDK for your platform; `make install` builds and copies the plugin into
your Rack user folder. `make test` runs the offline checks.

The build number is printed at the bottom of the panel and in the module's right-click menu, so you
can tell which build Rack has loaded.

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
