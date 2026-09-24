# Multi-Instrument Remote Panel

A browser-based remote control panel for **three** bench instruments,
running on one ESP32-S3:

- **BK Precision 5491B** — DMM, RS-232
- **BK1823A** — Frequency counter, RS-232
- **Rigol DG4062** — Function generator, USB-TMC

Built for shared/makerspace use, where none of these are always within
arm's reach of whoever wants to use them. This project started as an
expansion of a single-instrument DMM-only panel — see `CHANGELOG.md`
for that history.

<!-- Screenshot placeholder — drop one in here before publishing -->

## Status: beta

This is under active, iterative development and bench-tested as it
goes — not a finished, polished release. The table below is an honest
account of what's solid versus what's freshly built and awaiting
real-hardware confirmation. If something in the "not yet confirmed"
column bites you, that's expected at this stage — please open an issue
rather than assume it's your setup.

## Status of each instrument panel

| Instrument | Transport | Status |
|---|---|---|
| BK Precision 5491B DMM | UART1 RS-232 | **Bench-confirmed, mature.** Full panel — Single/Multi-Function, Auto-Fetch, Range/Reference/Filter, HALT/RESUME, Data Logger, Local mode. |
| BK1823A Counter | UART2 RS-232 (GPIO5/6) | **Bench-confirmed.** Live reading, Function/Gate/Hold controls, Auto-Fetch. Freq A is the most heavily exercised mode; Period/Total/Ratio/Time Interval are wired against the confirmed command table but less exercised individually. Not a SCPI instrument — see main.cpp's file header for the full command reference. |
| Rigol DG4062 Func Gen | USB-TMC (native USB host) | **Bench-confirmed core; mixed maturity elsewhere.** See breakdown below — this is the newest and most actively-developed part of the panel. |

### Func Gen panel, in more detail

| Area | Status |
|---|---|
| Core (Freq/Period/Ampl/Offset/High/Low/Impedance/Phase/Output, all 8 waveforms incl. Arb/Harmonic/User) | Bench-confirmed |
| Sweep (Start/Stop/Center/Span, Step, Time/Return, Hold, Trig Src, Mark) | Bench-confirmed |
| Burst (N-Cycle/Gated/Infinite modes, Cycles/Period/Delay/Phase, Gate Pol, Trig Src, TrigOut/SlopeIn) | Bench-confirmed |
| Modulation — AM | Bench-confirmed |
| Modulation — PM, ASK, FSK, PSK, BPSK, QPSK, OSK | Built from the manual's own per-parameter command reference, **not yet bench-confirmed** |
| Modulation — 3FSK, 4FSK | Bench-confirmed (indexed hop-frequency syntax) |
| Modulation — FM, PWM | Not yet implemented (Type selection exists in the dropdown; no sub-parameter panel) |
| Harmonic | Order/Type/single active harmonic's Amplitude+Phase implemented; the full 15-harmonic matrix is deliberately out of scope (see below) |
| Save/Restore setup | Implemented — reads the currently-visible panel state into a copyable SCPI command list, and can replay a pasted list back. Deliberately the simple version: doesn't force a full re-query of every field, so anything never actually visited this session, or changed via the instrument's own front panel without a subsequent Refresh, won't be reflected. A full "gather everything" option remains a possible future addition. |

The **TOOLS tab** is a shared manual SCPI terminal with a target
selector (DMM / Counter / Func Gen) and doubles as the whole app's
unified activity log — every command sent and every response, from any
instrument or mechanism, is visible there.

## Known limitations / open items

- **Harmonic's per-harmonic matrix** (15 harmonics × individual
  Amplitude/Phase) is deliberately not implemented — the panel exposes
  Order/Type and the currently-selected harmonic's own Amplitude/Phase,
  not all 15 at once. A conscious scope decision, not an oversight.
- **Local-mode staleness gap:** switching to the instrument's own front
  panel halts all Func Gen polling by design. On returning to Remote,
  the panel re-syncs whichever mode/waveform is active when you exit —
  but if you visited a *different* mode or waveform while on the front
  panel and switched back before returning to Remote, that one's
  values won't be re-fetched until you visit it again. Same underlying
  reason the Save/Restore feature above is the "simple" version, not
  the "always guaranteed accurate" one.
- **Rear panel I/O (External trigger, Sync input, etc.) is untested.**
  Everything in the tables above has been confirmed through the front
  panel and USB-TMC; nothing routed through the rear connectors has
  been separately verified yet.
- **DG4062 TrigOut's own "Off" option is confirmed working; SlopeIn's
  is not** — SlopeIn only has Leading/Trailing on this unit's front
  panel, no Off, despite the two looking symmetric in the manual.
- **Func Gen isn't a pure relay** — see "Design philosophy" below. USB-TMC
  transfers are synchronous by nature; a dedicated FreeRTOS task plus a
  sequence-number verification system (below) keep this from causing
  the response-ordering or WebSocket-blocking issues it caused earlier
  in development, but it's worth knowing this instrument's traffic
  isn't handled identically to the DMM/Counter's simple relay.
- **No Serial Monitor** — see "Debug output" below.

## Why this exists

Each instrument's own remote interface (RS-232/SCPI, or USB-TMC) is
useful but not something a novice at a makerspace can point a phone at.
This project puts one normal web page in front of all three: click a
button, or type SCPI manually via the TOOLS tab, watch the result — no
terminal or driver required on the client side.

## Design philosophy: the ESP32 is (mostly) a dumb pipe

The DMM and Counter are relayed with **zero SCPI knowledge** on the
firmware side — a WS message tagged for that instrument goes straight
out that instrument's UART TX, unmodified; a complete line from that
UART goes straight back over WS, tagged, the instant it's assembled.
All pacing, timeouts, retry, and response validation live entirely in
the browser's JS (one arbiter per instrument).

**The function generator is the one deliberate exception.** USB-TMC
bulk transfers have no equivalent to "just relay bytes" — the host has
to synchronously drive a request/response transaction. See `main.cpp`'s
file header for the full writeup of this tradeoff, including a real
reliability bug it caused early on (a sustained WebSocket-blocking
window during a multi-query refresh, which triggered a self-perpetuating
disconnect loop) and the fix — moving USB-TMC transfers to their own
FreeRTOS task with a request queue.

### Response-ordering safety: sequence numbers

Because the Func Gen queue processes commands one at a time with real
transfer delays, the browser's own response-matching logic depends on
replies arriving in the same order the corresponding commands were
sent. That assumption held almost all the time — but a handful of real
scenarios (a stale reply from a since-closed browser tab, a background
liveness check landing mid-refresh, a very long-running query queue)
could occasionally break it, and each one took real bench debugging to
track down individually.

The actual fix: every outgoing Func Gen command now carries a
browser-assigned sequence number, the firmware echoes it back verbatim
with the response, and the browser checks the echoed number against
what it expects at the front of its own queue the instant a response
arrives — logging an explicit, unambiguous mismatch the moment reality
stops matching the FIFO assumption, rather than the response silently
landing on the wrong field. This is detection, not automatic recovery;
see the JS's own comments (search for `SEQ MISMATCH`) for the full
history of what this was built to catch.

## Hardware

- ESP32-S3 dev board (developed on a Hosyond N16R8 — 16MB flash / 8MB
  OPI PSRAM; single native USB-OTG port, **no separate USB-UART bridge
  chip** — this is why there's no Serial Monitor available in this
  build; see "Debug output" below)
- 2x MAX232 (or equivalent) RS-232 level shifters — one per RS-232
  instrument (DMM, Counter)
- BK Precision 5491B bench multimeter
- BK1823A frequency counter
- Rigol DG4062 function generator

### Wiring

| ESP32 | Signal | Instrument | Notes |
|---|---|---|---|
| GPIO17 | UART1 TX | DMM (via MAX232 #1) | Bench-confirmed |
| GPIO18 | UART1 RX | DMM (via MAX232 #1) | Bench-confirmed |
| GPIO6  | UART2 TX | Counter (via MAX232 #2) | Bench-confirmed |
| GPIO5  | UART2 RX | Counter (via MAX232 #2) | Bench-confirmed. Deliberately a non-strapping general-purpose pin — an earlier pass of this project used GPIO3 (a strapping pin) before correcting it. |
| Native USB-OTG | Host | Func Gen (USB-TMC) | Requires the pioarduino platform — see Software setup |

Both UARTs run at 9600 baud, 8N1.

One easy-to-repeat mistake worth flagging: RS-232 cable wire colors
aren't standardized — a cable that happens to have a black wire on the
ground pin is a coincidence, not a convention. Confirm each wire
against the actual DB9 pinout (or your instrument's manual) rather than
assuming by color; this cost real bench debugging time once already on
this exact project.

## Software setup

**Built with PlatformIO** (VS Code extension), not Arduino IDE. All
library dependencies, board settings, and USB-mode build flags live in
`platformio.ini`, checked into this repo.

### USB-TMC platform requirement

`platformio.ini` pins the **pioarduino** fork of the espressif32
platform, not the stock PlatformIO registry package. This isn't a
preference — the stock platform package (specifically 3.20017.241212 /
ESP-IDF 5.3.x, confirmed at the bench) has a bug that makes
`usb_host_interface_claim()` always return error 0x106 regardless of
what's actually connected, so the DG4062 never enumerates no matter how
correct the rest of the USB-TMC code is. Switching the platform line to
pioarduino resolved it outright. See `main.cpp`'s file header and
`platformio.ini`'s own comment for the full writeup, including the
exact pinned release to build against.

### Library dependencies (declared in `platformio.ini`)

- **ESPAsyncWebServer** (ESP32Async fork) — v3.11.2
- **AsyncTCP** (ESP32Async fork) — v3.4.10
- **ArduinoJson** — v7.x

Pinned via direct GitHub tags rather than PlatformIO registry package
names — ESPAsyncWebServer has several same-named forks in the wild, and
pinning the exact repo+tag avoids picking up the wrong one.

### USB mode (required for USB-TMC host mode)

Set via `build_flags` in `platformio.ini` — no manual IDE menu steps:
`-DARDUINO_USB_MODE=0 -DARDUINO_USB_CDC_ON_BOOT=0`.

### Project files

| File | Purpose |
|---|---|
| `main.cpp` | Firmware — relay for DMM/Counter, USB-TMC host for Func Gen |
| `platformio.ini` | Board, build flags, library dependencies, pioarduino platform pin |
| `index_html.h` | The entire browser app (HTML/CSS/JS) — all three panels |
| `secrets.h` | Your WiFi credentials — **gitignored, never committed** |

`index_html.h` stays a separate header rather than being merged into
`main.cpp` — see `main.cpp`'s own file header for why.

### First-time setup

1. Copy `secrets.h.example` to `secrets.h`, in `src/` alongside
   `main.cpp`.
2. Edit `secrets.h` with your actual WiFi SSID/password and `HELP_URL`.
3. Place `index_html.h` in `src/`, next to `main.cpp`.
4. Build (PlatformIO fetches the library dependencies automatically on
   first build) and flash.
5. Open a browser to the ESP32's IP or mDNS hostname (see "Debug
   output" below for how to find these without a Serial Monitor).
6. Plug in the DG4062 to the ESP32's native USB port; wire the DMM and
   counter via their MAX232 level shifters per the wiring table above.

## Debug output: no Serial Monitor in this build

Native USB is committed to USB-TMC host mode, so there's no USB Serial
Monitor available at all here. All firmware log/debug messages are sent
as WebSocket `sys` events instead, visible in the browser's TOOLS tab
(unified activity log) once a client connects.

**Practical consequence:** if the board never connects to WiFi, or USB
enumeration fails before you can get a browser open, those early
messages have nowhere to go — a small ring buffer catches the last ~24
boot-time messages and flushes them to the first browser that connects,
but a truly dead board (no WiFi at all) is invisible until you can
reach it another way.

## Known limitations carried over from the original DMM-only project

ACDC/dB measurement not implemented; Local mode sends no command
(relies on the meter's physical Shift button); Reference doesn't
display its acquired zero-point value; a handful of bench-observed but
not-fully-characterized quirks around low-frequency PER/autoranging.

## License

<!-- Not yet decided — fill in before publishing. -->

## References

- Rigol DG4000 series programming guide (CHM format) — used to confirm
  SCPI syntax for every Func Gen feature above. Where the manual and
  actual bench-observed behavior disagree, bench results are treated as
  authoritative for this specific unit (several real discrepancies were
  found and documented this way — see main.cpp and index_html.h's own
  comments for specifics, e.g. SlopeIn's missing "Off" option above).

## Acknowledgments

Developed iteratively at the bench, feature by feature, with real
hardware behavior treated as the final authority over any manual or
assumption when the two disagreed.
