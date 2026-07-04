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

Coordinated surfaces, all editing the same underlying assignments:

**(a) Modulators panel — always visible, right third of the window.**

- A permanently visible, vertically-stacked, **scrollable** column of modulator
  cards. This is the primary modulation surface (§7 places it in the layout).
- **Defaults:** two **envelope** cards pre-mapped to **Osc 2 volume** and **filter
  cutoff**, then one **LFO** card below.
- **Assign an existing modulator** to another parameter → JS80P **silently
  duplicates** it (copy-on-write, §6) into a new card, so the new destination gets
  independent depth/shape.
- **Add new** (envelope or LFO) → a fresh, non-duplicated card that then has the
  same duplicate-on-assign behavior.
- Each operation consumes one slot from the pool (12 envelopes / 8 LFOs); pool
  usage and exhaustion are surfaced (§6).

**(b) Knobs — clean by default, smart when assigned.**

- Unassigned: a plain knob.
- Assigned: a **colored ring + small type badge** (reusing `CTL_COLOR_*`) shows
  what drives it; a live indicator shows the current driven value; a **poly/global
  badge** shows whether the assignment makes the param per-voice.
- **Turning an assigned knob edits the modulator, not a dead value.** Given the
  substitution engine:
  - Envelope / LFO assigned → the knob adjusts *that source's* depth/range for the
    parameter (env levels / LFO min-max-amp), via its dedicated (duplicated)
    per-destination modulator.
  - MIDI CC / global source assigned → the knob's range is expressed through a
    Macro (MIN/MAX = per-destination depth).

**(c) Mod Matrix table — complementary overview.**

- A table listing **every active source→destination assignment** as rows:
  `Source | Destination | Depth/Range | Poly? | remove`, built by scanning
  `get_param_controller_id_atomic()` across all params. Editing a row edits the
  same thing as the card and the knob ring; shared/duplicated modulators show a
  shared-slot indicator (§6). It complements — rather than replaces — the
  always-visible Modulators panel.

**(d) Macro editor — the one place to invest visually.**

- A **transfer-function graph** for MIN / MAX / MIDPOINT / DISTORTION-curve /
  RANDOMNESS. Macros are JS80P's entire "shaping" story; a good curve editor turns
  240 raw macro params into a handful of legible objects.

---

## 6. The modulator allocator (critical subsystem)

Per-destination depth is achieved by **carefully duplicating envelopes/LFOs (and
macros) behind the scenes**, with copy-on-write semantics over finite pools.

**Abstraction the user sees:** "this knob has its own modulator with its own
depth/shape." No slot numbers.

**Reality underneath:** finite pools — **12 envelopes, 8 LFOs, 30 macros** — each
a shared object. (Macros 1–8 are reserved as user-facing global performance
macros per §7, so the allocator draws its auto-depth macros from the remaining
pool.) Depth/range for an envelope *is* its level params
(initial/peak/sustain/final × scale); for an LFO it's min/max/amp; for a
global/MIDI source it's a macro's MIN/MAX. Two destinations sharing a slot
**cannot** have independent depth or shape, so independence requires a real clone.

**Copy-on-write allocator:**

1. Assigning "an envelope/LFO" to a knob grabs a free slot (or shares an identical
   existing one).
2. Destinations **share** a slot while their depth/shape are identical.
3. On the first edit of depth/shape for one destination, the allocator **clones**
   that slot's params into a fresh slot and reassigns only that destination —
   independent thereafter.
4. **Reference-counted cleanup:** unassigning a destination frees its clone when
   nothing else uses it.
5. **Pool accounting is surfaced, not hidden:** the UI shows remaining
   env/LFO/macro capacity and, when a pool is exhausted, degrades gracefully with
   a clear message ("out of independent envelopes — this knob will share
   Envelope 4") instead of failing silently.

**Two user operations map onto this** (surfaced in the Modulators panel, §5a):
*assigning an existing modulator* to a new destination duplicates its current
settings (copy-on-write); *adding a new modulator* allocates a fresh default one.
Both consume a pool slot (12 envelopes, 8 LFOs); the panel is the direct view of
those slots.

**Round-tripping (must-have):** clones are ordinary envelopes/LFOs/macros in the
patch, so saved files remain standard JS80P patches. On **load**, the allocator
reverse-engineers the flat 12/8/30 assignment map — detecting shared vs
independent slots — and reconstructs the clean per-destination view. Existing
presets open correctly; presets saved by the new GUI open in the original GUI.

This subsystem is testable in isolation (allocate / share / clone-on-edit /
free / load-time reverse mapping) and should be unit-tested against real patches.

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
