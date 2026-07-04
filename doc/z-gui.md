# JS80P — Alternate GUI Plan

> Status: proposal / design lock-in. The synthesis engine (`Synth` and all DSP) is
> **not** changed by this work — only the GUI is replaced. This document is the
> reference for building a simpler, JUCE-based editor — a primary synth page plus
> a couple of focused tabs — with vector graphics.

## Table of contents

- [1. Goals](#1-goals)
- [2. What we're replacing (findings)](#2-what-were-replacing-findings)
  - [2.1 The GUI is cleanly separable](#21-the-gui-is-cleanly-separable)
  - [2.2 The DSP is decoupled from the toolkit](#22-the-dsp-is-decoupled-from-the-toolkit)
  - [2.3 The keyboard / spacebar bug](#23-the-keyboard--spacebar-bug)
  - [2.4 How modulation actually works](#24-how-modulation-actually-works)
  - [2.5 Per-voice vs global parameters](#25-per-voice-vs-global-parameters)
  - [2.6 Latched-at-note-on parameters](#26-latched-at-note-on-parameters)
- [3. Design principles](#3-design-principles)
- [4. Technology & editor architecture](#4-technology--editor-architecture)
- [5. The modulation UX](#5-the-modulation-ux)
- [6. The modulator allocator (critical subsystem)](#6-the-modulator-allocator-critical-subsystem)
- [7. Default surface](#7-default-surface)
- [8. Sane defaults (anti-foot-gun)](#8-sane-defaults-anti-foot-gun)
- [9. Locked decisions](#9-locked-decisions)
- [10. Implementation phases](#10-implementation-phases)
- [11. Open / iterative items](#11-open--iterative-items)

---

## 1. Goals

JS80P is a deep, MIDI-driven synthesizer VST with **724 parameters**, each mapped
1:1 to a GUI control across **9 tabs**. The current GUI is powerful but hard to
use: a fixed 2076×1200 raster canvas with 566 knobs, 60 toggles, and 96
selectors, every knob carrying a permanent modulation-slot + value-readout strip.

We want an **alternate GUI** that is:

1. **Significantly easier to use** — a curated main page plus a couple of focused
   tabs (Effects, Mod Matrix), not 9 dense tabs.
2. **Better looking** — vector graphics, resolution-independent, crisp at any size.
3. **Well-behaved in hosts** — keyboard events (e.g. spacebar transport) reach the
   DAW instead of being swallowed.
4. **Faithful to the engine** — the synthesis core is untouched; existing presets
   load and save unchanged.

Non-goal: changing any DSP behavior or the on-disk patch format.

---

## 2. What we're replacing (findings)

### 2.1 The GUI is cleanly separable

The GUI is split in the build (`Makefile`) into:

- `GUI_COMMON_SOURCES` — `src/gui/gui.cpp` (4,381 lines) + `src/gui/widgets.cpp`
  (2,518 lines): all layout and widget *logic*, toolkit-agnostic. These talk only
  to the abstract `WidgetBase` interface (`src/gui/gui.hpp`).
- `GUI_TARGET_PLATFORM_SOURCES` — `src/gui/win32.cpp` (1,281), `src/gui/xcb.cpp`
  (1,952), `src/gui/macos.mm` (686): concrete `Widget : WidgetBase`
  implementations of the drawing/eventing primitives per OS.

`WidgetBase` primitives are simple and portable: `fill_rectangle`, `draw_text`,
`draw_image`, `load_image` / `copy_image_region` / `downscale_image` /
`delete_image`, `monotonic_clock_ms`, `redraw`, `show`/`hide`/`focus`, and the
event handlers `paint`, `double_click`, `mouse_down`/`up`/`move`/`leave`/`wheel`.

**Implication:** we can replace the three platform files with a single new
`Widget` backend implemented against JUCE (see §4), and `gui.cpp`/`widgets.cpp`
compile unchanged — the entire original GUI then runs inside JUCE.

### 2.2 The DSP is decoupled from the toolkit

`Synth` exposes a narrow, thread-safe, GUI-agnostic seam:

- **GUI → DSP:** a lock-free SPSC queue via
  `push_message(MessageType, ParamId, ratio, controller_id)`
  (`SET_PARAM`, `SET_PARAM_SMOOTHLY`, `ASSIGN_CONTROLLER`, `REFRESH_PARAM`,
  `CLEAR`, `RANDOMIZE`, …), drained by the audio thread each render block.
- **DSP → GUI:** atomic reads `get_param_ratio_atomic(param_id)` and
  `get_param_controller_id_atomic(param_id)`, plus metadata helpers
  (`get_param_name`, `get_param_id`, `is_discrete_param`, `get_param_max_value`,
  `get_param_default_ratio`, `float_param_ratio_to_display_value`,
  `byte_param_ratio_to_display_value`).

Any new GUI reuses this exact interface. Patches serialize by parameter *name*
(`serializer.cpp`), so a redesigned GUI keeps full patch compatibility.

### 2.3 The keyboard / spacebar bug

JS80P has **no keyboard features at all** and never forwards keystrokes to the
host. Spacebar is swallowed only because native child controls take focus:

- **Win32:** each widget is a child `STATIC` HWND; `Widget::focus()` calls
  `SetFocus`, and the focused control's default proc consumes `WM_KEYDOWN`
  (`src/gui/win32.cpp`). No key handling exists in JS80P's own code.
- **macOS:** the `NSView` subclass overrides only mouse methods;
  `makeFirstResponder` on focus means keys hit a view with no `keyDown:`
  override (`src/gui/macos.mm`).
- **Linux/XCB:** key event masks are explicitly commented out
  (`src/gui/xcb.cpp`), so Linux is already fine.

Under JUCE this is controllable in one line: `setWantsKeyboardFocus(false)` (or
returning `false` from `keyPressed`). Nothing is lost because there are no
keyboard features to preserve.

### 2.4 How modulation actually works

JS80P is **not** a modulation matrix. It uses **single-source, full-range
substitution**:

- **One source slot per parameter.** Any parameter can have at most one
  "controller" assigned (MIDI CC, macro, LFO, envelope, aftertouch, note/velocity,
  peak follower).
- **The source replaces the knob.** `Param::get_value()` (`src/dsp/param.cpp`):
  if a source is assigned, it returns `ratio_to_value(source_output)` — the
  source's 0–100 % mapped across the parameter's **entire** [min, max] range. The
  manually-set value is **ignored**.
- **The source carries its own range, not the destination.** There is no
  per-connection depth on the parameter. Range/shape lives in the source:
  - an **LFO** sweeps between its own min / max / amplitude,
  - an **envelope** reaches its own DAHDSR levels,
  - a raw **MIDI CC** is a bare 0–100 % ramp, so to shape it you route through a
    **Macro** (MIN, MAX, SCALE, MIDPOINT, DISTORTION amount + curve, RANDOMNESS).

This is why the current GUI reserves a value-readout (40 px) + controller-label
(32 px) strip under **every** knob, and why clicking that strip opens a
full-screen 134-item source list (`ControllerSelector`, spanning the whole GUI).

Counts: **30 macros** (8 params each), **12 envelopes** (DAHDSR, 12 params each),
**8 LFOs**, and a **134-entry** user-selectable controller list backed by a
190-wide `ControllerId` space. Sources are already color-coded by type
(`CTL_COLOR_MACRO/_LFO/_ENVELOPE/…`).

Compared to mainstream soft-synths (Serum, Vital, Massive, u-he, Bitwig) which use
**additive, multi-source, per-connection bipolar depth** with drag-to-knob depth
rings, JS80P's substitution model is a minority paradigm. Its closest analogues
are **MIDI-learn / host automation** (the controller "takes the wheel") and
**Bitwig-style reusable modulators / macros** (a source object with its own
transfer curve).

### 2.5 Per-voice vs global parameters

Two-layer model: **leader** params live once in `Synth` (what the GUI edits);
**follower** params live per voice in `Voice::Params` (`src/voice.hpp`).

A follower decides at render time whether to track the leader (global /
paraphonic) or run its own value (per-voice / polyphonic).
`FloatParam::is_polyphonic()` (`src/dsp/param.cpp`):

```cpp
// leader:
bool is_polyphonic() { return envelope != NULL || has_lfo_with_envelope(); }
// follower:
bool is_following_leader() { return !leader->is_polyphonic() && !diff_channel; }
```

- **Structurally per-voice-capable (Osc / "Synth"-tab params):** noise level,
  waveform, pulse width, amplitude, velocity sensitivity, folding, portamento,
  detune / fine-detune, width, panning, volume, the 10 custom-waveform harmonics,
  **both per-oscillator filters** (type/freq/Q/gain/log/inaccuracy), plus
  modulator subharmonic and carrier distortion level. These exist once per voice.
- **Structurally global (one instance, never per-voice):** mixer/routing
  (MIX/PM/FM/AM/INVOL), the **entire effects chain** (~58 params), all **30
  macros**, note handling, MPE, tunings. LFOs are global objects unless they carry
  an amplitude envelope. Envelope *knobs* are global; only the running envelope
  *state* is per-voice.

**Runtime rule (non-obvious):** a per-voice-capable param becomes actually
polyphonic **only when an envelope — or an LFO with an amplitude envelope — is
assigned.** A plain MIDI CC, macro, or plain LFO does **not** make it per-voice;
it stays paraphonic (all voices share one live value).

### 2.6 Latched-at-note-on parameters

Some GUI knobs, turned mid-note, do **not** affect already-sounding voices. Three
distinct reasons:

**A. Onset-latched performance params** — read once in `Voice::note_on()` to
compute a per-voice constant, regardless of modulation. Turning them affects only
*subsequently* triggered notes:

| Control | Why frozen |
|---|---|
| Velocity Sensitivity | per-voice velocity computed at onset (`calculate_note_velocity`) |
| Portamento Length / Depth | glide ramp set up once at onset (`set_up_oscillator_frequency`) |
| Note→Pan Width (+ detune's pan contribution) | keyboard-position panning computed once (`calculate_note_panning`) |
| Tuning selector / base note pitch | note frequency looked up at onset (`get_note_frequency`) |
| Oscillator Inaccuracy / Instability | per-voice random drift seeded at onset |

**B. Envelope knobs in STATIC update mode** — the envelope's whole shape is
snapshotted at note-on (`Envelope::make_snapshot`) and frozen for that note:
Delay, Attack, Hold, Decay, Release, Initial, Peak, Sustain, Final, Scale, the
three Shapes, Time/Value Inaccuracy, Tempo Sync. In **DYN** and the four
voice-tracking modes (LST/OLD/LOW/HI) these instead update held voices, ramping
in over 0.1 s (`DYNAMIC_ENVELOPE_RAMP_TIME`). STATIC is what gives each note its
own snapshot (per-note velocity / key-tracking); DYN trades that for live
editability. The simplified GUI **always uses STATIC and does not expose the
mode at all** — DYN and the voice-tracking variants remain available only in the
detailed (original) GUI.

**C. Params with a source assigned** — the knob is ignored entirely
(substitution, §2.4), whether or not a note is playing. Distinct from A/B; solved
by the "assigned-knob-edits-the-modulator" behavior (§5).

Everything else (osc amplitude/volume/folding/PW/panning, filter freq/Q/gain,
waveform, harmonics, routing, the entire FX chain) updates live.

---

## 3. Design principles

1. **Engine untouched.** New GUI talks to `Synth` only through the existing
   message queue + atomic getters. Presets stay compatible in both directions.
2. **A primary synth page, plus a few focused tabs.** The main page is a single
   signal-flow layout — `Osc 1 → Mix / FM / PM / AM → Osc 2 → Filters → Out` —
   with a **global-macro strip** on top. The oscillators + mix fill the left
   ~2/3; the right ~1/3 is an always-visible **Modulators panel**. **Effects get
   their own tab** (they don't need heavy simplification — just cleaner
   organization). No 9-tab sprawl.
3. **Progressive disclosure.** ~40–60 curated controls on the main page; the raw
   30-macro / 12-env / 8-LFO grids and the full per-effect parameter sets are
   reached through the modulation system, the Effects tab, and the original-GUI
   toggle — not shown as always-on walls of knobs.
4. **Modulation is always visible.** The right third of the window is a
   permanently visible, scrollable stack of modulator cards (envelopes, LFOs);
   assigning one to a new destination duplicates it behind the scenes (§6). A Mod
   Matrix table gives a complementary full-assignment overview.
5. **Vector graphics via JUCE.** Deletes the `ParamStateImages` PNG-frame pipeline
   for the new widgets.
6. **Sane defaults that prevent the known foot-guns** (§8).

---

## 4. Technology & editor architecture

**Full JUCE plugin** — `juce::AudioProcessor` + `juce::AudioProcessorEditor`
wrapping the existing `Synth` DSP verbatim. Targets **VST3 + AU + standalone**.
VST2/FST is dropped (JUCE cannot build VST2; that was the sole reason for the
hand-rolled FST wrapper).

Benefits: keyboard/spacebar fixed for free (`setWantsKeyboardFocus(false)`);
vector rendering, resize, and HiDPI for free; ~4,600 lines of per-platform
windowing removed.

**Editor architecture — reuse, don't rewrite, the original GUI:**

Because the original GUI is abstracted behind `WidgetBase` (§2.1), we write **one
new `Widget` backend against `juce::Component` + `juce::Graphics`** — replacing
win32/xcb/macos with a single file. Each `WidgetBase` becomes a lightweight
`juce::Component` child; `draw_*` primitives map to `juce::Graphics`; the idle
timer becomes a `juce::Timer`; the parent-window plumbing (`ExternallyCreatedWindow`)
is replaced by the editor's root component.

Consequences:

- The **entire original detailed GUI runs inside JUCE unchanged**, vector-hosted,
  with the keyboard bug already fixed — *before any new UI exists*. This is
  Milestone 1 and de-risks the whole project.
- The **original GUI becomes the "detailed editing" mode.** A runtime **toggle**
  in the new GUI swaps which top-level component is shown (new simplified GUI ⇄
  original GUI); both share the same `Synth` bridge and the same window.
- The original GUI keeps its PNG knobs (acceptable for a power-user fallback); the
  new GUI gets fresh vector widgets.

---

## 5. The modulation UX

### 5.0 The engine constraint we are working around

JS80P is **not** an additive modulation matrix (§2.4). Each parameter has at most
**one** controller; when assigned, the controller's 0–100 % output is mapped across
the parameter's **entire** [min, max] range and the knob value is **ignored**. The
range/shape lives entirely in the *source* (envelope levels, LFO min/max, macro
min/max). Two destinations that want the same modulation *shape* but different
*ranges* therefore need **two separate source objects** from the finite pools
(12 envelopes, 8 LFOs, 30 macros).

To make this behave like a normal per-connection-depth modulation system without
touching the engine, the new GUI plays **three UI tricks**: it (a) re-maps what a
knob edits once a modulator is assigned, (b) **groups duplicate source slots** that
share a shape into a single on-screen editor, and (c) **auto-clones a fresh slot**
whenever a shared modulator is dropped on a new destination.

### 5.1 What an assigned knob edits (per-destination range)

When a modulator is assigned, the knob no longer writes the destination parameter
(the engine ignores it anyway). Instead the knob and its **modulation-range**
control drive the *source's* per-destination range parameters:

| Source | Knob position writes | Range control writes |
|---|---|---|
| **Envelope** | Initial, Sustain, **and** Final levels (INI = SUS = FIN, tracked together) | Peak level |
| **LFO** | Minimum value (MIN) | Maximum value (MAX) |
| **Macro** (on envelope time/scale params) | Macro Minimum (MIN) | Macro Maximum (MAX) |

So e.g. the filter cutoff defaults to **Env 2** assigned: the cutoff knob sets that
env's INI/SUS/FIN, and the modulation-range control sets its PEAK — exactly a
"base value + depth" feel, expressed through the substitution engine.

### 5.2 Grouping duplicates into one editor

The **rightmost panel** is a list of **envelope and LFO editors — one per distinct
shape** that is currently assigned somewhere:

- **Envelopes are grouped by their shape times** — equal **Hold, Attack, Decay,
  Release** ⇒ one editor. (Its INI/PK/SUS/FIN differ per destination and are edited
  at the knob, not here.) Editing the shared shape in the panel edits **all**
  grouped duplicate slots at once.
- **LFOs are grouped by shape** — equal **Waveform, Pulse Width, Frequency (Hz),
  Phase, Distortion, Randomness** ⇒ one editor; editing it updates every duplicate.
  (Each duplicate's MIN/MAX differ per destination and are edited at the knob.)

This yields the normal experience — *multiple controls sharing one modulator with
different depths* — even though behind the scenes each connection consumes its own
pool slot.

### 5.3 Assigning: auto-clone on drop

Assigning an **existing** modulator (a shape already in the panel) to a **new**
control:

1. allocates a **fresh unused** slot of that type from the pool,
2. **copies the shape** into it (envelope H/A/D/R; LFO wav/pw/freq/phs/dist/rand),
3. sets its per-destination range to **match the current knob position** (env
   INI/SUS/FIN = knob, or LFO MIN = knob), leaving PEAK/MAX at a sane default,
4. assigns that new slot as the destination's controller.

The user then adjusts the range (PEAK / MAX) at the knob. Because the clone shares
the group's shape key, it appears **inside the same panel editor** — the group just
gained another (hidden) member.

### 5.4 Macros — two distinct roles

1. **Global performance strip (top of the plugin).** Macros **1–8** are always
   visible knobs across a top strip. Each exposes only a **MIDI CC selector** (its
   input) and its value; no other modulation options in the new GUI. Intended for
   hardware/DAW automation; these CC bindings should survive INIT (§8.2) — see
   round-trip note below.
2. **Range modulator for envelope time/scale.** A modulator may also be assigned to
   an envelope's **SCL, DEL, ATK, HOLD, DEC, REL** params. Doing so consumes a fresh
   macro from the pool and uses it exactly like the LFO case: **current knob
   position → macro MIN**, **modulation range → macro MAX**. This lets a MIDI CC
   automate those envelope parameters.

---

## 6. The modulator grouping / allocator (critical subsystem)

The subsystem is a **shape-grouping allocator** over the flat engine pools. It never
changes the engine; it manages which pool slot backs which connection and keeps
duplicates in sync.

**Pools & per-slot parameters** (verified ids in `src/synth.hpp`):

- **Envelope N** (1–12), 12-param stride from `N1SCL=339`:
  - shape key: `HLD`, `ATK`, `DEC`, `REL`; shared shape also incl. `DEL`, `SCL`,
    the three shape curves, inaccuracy.
  - per-destination: `INI`, `PK`, `SUS`, `FIN`. Controller ids `ENVELOPE_1..6 =
    149..154`, `ENVELOPE_7..12 = 172..177`.
- **LFO N** (1–8), 7-param stride from `L1FRQ=484` (`FRQ,PHS,MIN,MAX,AMP,DST,RND`),
  plus `WAV=555+`, `PW`, `CEN`, `SYN`, `LOG`:
  - shape key: `WAV`, `PW`, `FRQ`, `PHS`, `DST`, `RND`.
  - per-destination: `MIN`, `MAX`. Controller ids `LFO_1..8 = 141..148`.
- **Macro N** (1–10 here), 7-param stride from `M1MID=129`
  (`MID,IN,MIN,MAX,SCL,DST,RND`): per-destination `MIN`, `MAX`; input CC via `MnIN`.
  Controller ids `MACRO_1.. = 131..`.

**Core operations:**

1. **Group scan** — read `get_param_controller_id_atomic()` across all params to
   find which slots are in use and by which destinations; bucket used slots by their
   shape key to form the panel's editor list.
2. **Allocate-on-assign** — take the lowest free slot of the type; if cloning from
   an existing group, copy the shape key + shared shape params into it.
3. **Edit-shape propagation** — an edit in a panel editor writes the changed shape
   param to **every** slot in that group.
4. **Free on unassign** — clearing a destination's controller returns its slot to
   the pool.
5. **Pool accounting surfaced** — show remaining env/LFO/macro capacity; when a pool
   is exhausted, fall back to sharing an existing identical slot (losing independent
   range) with a clear message rather than failing silently.

**Round-tripping / patch format (must-have, partly deferred):** slots are ordinary
envelopes/LFOs/macros, so **saved files stay standard JS80P patches** and open in
the original GUI. On **load**, the group scan reconstructs the grouped editor view
from the flat assignment map. The **macro-strip MIDI-CC bindings that persist across
INIT** cannot live in the patch format (which we must not change); the plan is for
the **new GUI to store those separately and re-apply them** even to existing
patches created via the new GUI — *design to be finalised in a later iteration.*

This subsystem is unit-testable in isolation (group scan / allocate / clone-on-drop
/ shape propagation / free) and should be tested against real presets.

---

## 7. Default surface

The layout is a **primary synth page** with a **global-macro strip** on top, plus
a separate **Effects tab**. Control lists below are expected to iterate.

### Global-macro strip (top of the main page)

**Macros 1–8** sit as a persistent strip across the top as **global performance
macros**. Each slot lets the user pick a **MIDI CC** as its input directly (a
small inline picker, not the full-screen list), name it, and use it as a mod
source anywhere. Crucially these CC bindings **persist across INIT**: clicking
"initialize patch" clears the sound but **keeps the global-macro CC assignments**,
so a performer's hardware mapping survives patch changes. (Macros 9–30 remain
general-purpose shaping macros managed by the allocator, §6.)

### Main synth page — left two-thirds

Two oscillator columns flank a narrow **Mix column**: `Osc 1 → Mix → Osc 2`. Each
oscillator section has a **fixed height** (so the two align) and a fixed row
layout, with its **filters directly below it**.

**Oscillator section rows (top → bottom):**

1. **Waveform selector** — a **button group of small waveform icons in 2 rows**
   (all 14 shapes visible at once, not a dropdown), current shape highlighted.
2. **Amp row:** AMP · VEL · WID · PAN. (Volume is hidden — redundant with AMP,
   pinned at 100 %.)
3. **Tuning row:** DTN · FIN · PRD · PRT (detune, fine, portamento depth/length).
4. **Type-specific row:** NOISE · FOLD · **SUB** for Osc 1 (modulator) / NOISE ·
   FOLD · **DIST** for Osc 2 (carrier, with its distortion-type selector).
5. **Per-type editor** (fixed-height slot): empty for most shapes, a **Pulse
   Width** control for pulse-family shapes, or **10 compact harmonic sliders** for
   Custom.

Small **inaccuracy controls** (osc **INA**/**INS**) sit in the section header as
screw-style mini knobs, mirroring the analog-drift controls in the current GUI.

**Filters — two collapsed areas.** JS80P has **4 oscillator filters**, grouped
into **two compact areas**: **Filters 1 & 2 below Osc 1**, **Filters 3 & 4 below
Osc 2**. Each area is a segmented editor (F1 | F2) exposing Type · Freq · Q · Gain
plus the log-scale toggles and freq/Q **inaccuracy screws**. By default **only
Filter 3's cutoff is mapped to Env 2** (see §8).

**Mix column (center):** the routing controls — **Mix · FM · PM · AM** — as **4
vertically-stacked knobs**, a deliberately narrow column so the oscillators keep
the width (the Mode selector sits at its head).

**Waveform selectors** for both oscillators and LFOs cover the same **14 shapes**
(Sine, Saw, Soft Saw, Inv Saw, Soft Inv Saw, Triangle, Soft Triangle, Square, Soft
Square, Pulse, Soft Pulse, Bipolar Pulse, Soft Bipolar Pulse, Custom), drawn as
vector glyphs in a small **2-row icon button group** (not a dropdown).

### Modulators panel — right third (always visible)

The right ~1/3 is a permanently visible, vertically-stacked, **scrollable** column
of modulator cards (§5a):

- **Defaults:** two distinct **envelope** cards pre-mapped to **Osc 2 volume** and
  **filter cutoff** (A · D · S · R + Env→target, always STATIC; Advanced:
  Delay/Hold, shapes, inaccuracy).
- **One LFO** card below (waveform icon selector · Rate · Depth · Destination).
- **Assign existing** → silent duplicate; **Add new** → fresh card; each consumes
  a pool slot (12 envelopes / 8 LFOs). Pool usage is shown; exhaustion falls back
  to sharing (§6).

### Effects tab

Effects are **not simplified** — the tab carries the **full set of edit controls**
and mirrors the current effects tab, just in a cleaner vertical arrangement:

- **Top row (no power switch):** Aux In · Vol 1 · Dist 1 · Dist 2 · Filter 1 ·
  Filter 2 · Vol 2 · Out (Vol 3) — the always-on input/gain, distortion, FX-filter,
  and output stages, each with their full controls (distortion type selectors,
  filter Type/Freq/Q/Gain + log toggles, etc.).
- Then one **row per effect, each with a power switch**, exposing all of that
  effect's parameters: **Tape**, **Chorus**, **Delay (Echo)** (incl. its sidechain
  compressor + reversed toggles), **Reverb** (incl. its sidechain compressor).

Envelopes and LFOs beyond the defaults appear as cards in the Modulators panel on
demand (up to 12 / 8). Everything else (macros 9–30, MPE/tuning, dual osc filters,
and less-common envelope/LFO parameters) is reachable via the original-GUI toggle,
Advanced panels, and the Mod Matrix, and loads/saves existing presets unchanged.

---

## 8. Sane defaults (anti-foot-gun)

1. **New patches ship polyphonic and shaped:** 2 oscillators active; **Env 1 →
   Osc 2 volume**, **Env 2 → Filter 3 cutoff** (Osc 2's first filter) — both as
   cards in the Modulators panel. Fixes the biggest surprise (a fresh JS80P
   patch is paraphonic with no per-note envelope). A new user gets normal synth
   behavior on note one.
2. **INIT keeps your controllers.** Clicking "initialize patch" resets the sound
   but **retains the global-macro (macros 1–8) MIDI CC bindings** (§7), so a
   performer's hardware mapping survives across patches.
3. **Envelopes are always STATIC** in the simplified GUI — the Static/Live and
   voice-tracking (LST/OLD/LOW/HI) modes are not exposed here at all (they remain
   in detailed editing). Removes the "why didn't my held note respond?" trap.
4. **Assignment depth starts sane:** LFO/env assignments default to a moderate
   range, not the full param sweep.
5. **Onset-latched controls flagged:** the ~6 "applies to new notes" params
   (§2.6.A) get a subtle badge/tooltip.
6. **Curated source list** filtered by capability tier, drag-to-assign, replacing
   the full-screen 134-item list.
7. **Sensible polyphony, filter, and output-level defaults** so the synth sounds
   pleasant out of the box.

---

## 9. Locked decisions

| Area | Decision |
|---|---|
| Plugin | Full JUCE — VST3 + AU + standalone. VST2/FST dropped. `Synth` DSP reused verbatim. |
| Editor architecture | New JUCE `Widget` backend replaces win32/xcb/macos; original GUI runs inside JUCE unchanged. |
| Rollout | New simplified GUI is primary; in-GUI **toggle** switches to the original GUI for detailed editing. Both live in the same plugin. |
| Layout | Global-macro strip on top. Left **2/3**: `Osc 1 → Mix → Osc 2`; each oscillator is a **fixed-height** section (waveform icons → amp row → tuning row → type-specific row → per-type editor) with its **filters directly below**. **4 oscillator filters** grouped into **2 collapsed areas** (F1&2 under Osc 1, F3&4 under Osc 2). **Mix = 4 vertical knobs**. Right **1/3**: always-visible scrollable **Modulators panel**. Separate **Effects tab**; Mod Matrix table as overview. Iterative. |
| Waveform selection | **2-row icon button group** (all 14 shapes visible, not a dropdown) for both oscillators and LFOs — vector glyphs. |
| Inaccuracy / randomness | The small analog-drift controls are carried over as **screw-style mini knobs**: osc INA/INS, filter freq/Q inaccuracy, envelope time/value inaccuracy, LFO randomness. |
| Global macros | Macros **1–8** are top-strip global performance macros with inline **MIDI CC** assignment; CC bindings **persist across INIT**. |
| Effects | Not simplified — **own tab** with the **full control set**, mirroring the current tab: static **top row** (Aux In · Vol 1 · Dist 1 · Dist 2 · Filter 1 · Filter 2 · Vol 2 · Out) + one **powered row each** for Tape / Chorus / Delay / Reverb. |
| Modulation | Clean knobs; colored ring + poly/global badge when assigned; drag-to-assign from a source tray; assigned-knob edits the modulator; **copy-on-write modulator allocator** for per-destination depth; Mod Matrix as source of truth; Macro transfer-function editor. |
| Envelope default | **Always STATIC**; Static/Live and voice-tracking modes not exposed in the simplified GUI (available in detailed editing). |
| New-patch default | 2 oscillators; Env 1 → Osc 2 volume, Env 2 → Filter 3 cutoff (Modulators-panel cards); polyphonic, shaped, pleasant out of the box. |

---

## 10. Implementation phases

1. **JUCE `Widget` backend + processor wrapper.** Original GUI runs inside a
   full-JUCE plugin; verify audio + spacebar-to-transport in a DAW.
   → *shippable, keyboard fixed, zero UI regression.*
2. **Synth param bridge** (all 724 IDs over the message queue + atomic getters) +
   patch load/save + the **modulator allocator** (copy-on-write pooling,
   ref-counted cleanup, load-time reverse mapping). Unit-test against real patches.
3. **Vector widget kit** — knob with assigned-ring/badge states, toggle, selector,
   source tray, mod picker; theme-able.
4. **Main synth page** — left-2/3 fixed-height oscillator sections (`Osc 1 → Mix →
   Osc 2`: waveform icon button-group → amp/tuning/type-specific rows → per-type
   editor) with their **4 filters in two collapsed areas** below; the **4-knob
   vertical Mix column**; right-1/3 always-visible **Modulators panel**
   (assign-duplicates / add-new over the 12-env & 8-LFO pools, with inaccuracy/rand
   screws); the **global-macro strip** (macros 1–8, MIDI CC picker, INIT-persistent
   bindings); the **full-control Effects tab**; and the new-patch default.
5. **Modulation UX** — drag-to-assign, assigned-knob-edits-modulator, Mod Matrix
   view, Macro curve editor, pool accounting/messaging.
6. **GUI toggle** wired; iterate the essentials set with real use.

---

## 11. Open / iterative items

- The essentials list (§7) will evolve with real use; treat it as a living set.
- Macro-pool policy details (how aggressively to auto-clone vs share) will be
  tuned once the allocator is exercised against real patches.
- Whether/when to revector the original GUI's PNG knobs (not required for the
  fallback mode).
- Theming (light/dark) and exact color tokens for source types.
