#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// This HTML/JS content lives in its own header file, deliberately
// separate from the main .ino, specifically so Arduino IDE's auto
// function-prototype generator never scans it — that scanner is a naive
// text pattern-matcher, not a real C++ parser, and doesn't understand raw
// string literal boundaries. See the main .ino's file header for the
// full explanation and project notes "Arduino IDE Gotchas" for the
// general writeup. Do not move this content back into the .ino.
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Multi-Instrument Remote Panel</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      background: #1a1a1a;
      color: #e0e0e0;
      display: flex;
      flex-direction: column;
      height: 100dvh;
      overflow: hidden;
    }

    /* ── Title bar ── */
    #titlebar {
      background: #222;
      border-bottom: 1px solid #555;
      padding: 6px 16px;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    /* ── Top-level instrument tabs ──
       Replaces the old static <h1> — this app now fronts three
       instruments plus a shared SCPI terminal, so the titlebar's left
       side became a tab strip instead of a fixed title. */
    #tabs { display: flex; gap: 4px; overflow-x: auto; }
    .tab-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 6px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.85em;
      white-space: nowrap;
    }
    .tab-btn:hover { border-color: #556; color: #aaccff; }
    .tab-btn.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }
    #titlebar-right { display: flex; align-items: center; gap: 12px; flex-shrink: 0; }

    /* ── Tab panels ──
       #app-panels replaces body-as-single-app-container. Exactly one
       .tab-panel is display:flex (via .active) at a time; the rest are
       display:none, which also hides any position:fixed descendants
       (halt-resume-btn, datalogger, submenu overlay) regardless of the
       fixed positioning — display:none on an ancestor always wins.
       Background JS (WS traffic, Auto-Fetch, Data Logger) keeps running
       for a hidden tab; only its rendering pauses. */
    #app-panels { flex: 1; min-height: 0; display: flex; overflow: hidden; }
    .tab-panel { display: none; width: 100%; min-height: 0; overflow: hidden; }
    .tab-panel.active { display: flex; flex-direction: column; flex: 1; }
    #help-btn {
      background: none;
      border: 1px solid #888;
      border-radius: 50%;
      color: #ddd;
      width: 22px;
      height: 22px;
      min-width: 22px;
      font-size: 0.8em;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      line-height: 1;
    }
    #help-btn:hover { border-color: #aaccff; color: #aaccff; }
    #status-stack { display: flex; flex-direction: column; align-items: flex-end; gap: 2px; }
    #link-status-row { display: flex; gap: 8px; }
    .link-status { font-size: 0.7em; color: #666; }
    .link-status.link-ok   { color: #44cc44; }
    .link-status.link-dead { color: #cc4444; }
    #ws-status { font-size: 0.8em; color: #cc4444; }
    #ws-status.connected { color: #44cc44; }
    #comm-status { font-size: 0.75em; color: #888; }
    #comm-status.comm-ok   { color: #44cc44; }
    #comm-status.comm-fail { color: #cc4444; }
    #dmm-subheader {
      padding: 2px 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid #2a2a2a;
      flex-shrink: 0;
    }
    #dmm-subheader-label { font-size: 0.75em; color: #777; letter-spacing: 0.1em; }

    /* ── Main area: reading + function buttons ── */
    #main {
      flex: 1;
      min-height: 0;
      display: flex;
      overflow-y: auto;
      overflow-x: hidden;
    }

    /* ── Reading panel ── */
    #reading-panel {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: flex-start;
      padding: 30px 40px;
    }

    /* Cycle readings panel — the sole reading display. Normal flex flow,
       centers via reading-panel's own justify-content. */
    #cycle-readings-panel {
      display: flex;
      flex-direction: column;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: clamp(1.8em, 5vw, 4.2em);
    }
    .cycle-row {
      display: flex;
      flex-wrap: wrap;
      align-items: baseline;
      gap: 16px;
      line-height: 1.5;
    }
    .cycle-row .cr-label {
      color: #aaa;
      width: 8.0em;
      text-align: right;
      font-size: 0.5em;
    }
    .cycle-row .cr-value {
      color: #fff;
      min-width: 7em;
    }
    .cycle-row .cr-value.cr-fresh { color: #00ee88; }
    .cycle-row.multi-only { display: none; }  /* shown only when checked
      AND Multi-Function mode is on — see updateMultiFunctionVisibility() */
    .cycle-row.multi-only.shown { display: flex; }

    /* ── Function buttons ── */
    #func-panel {
      width: clamp(220px, 22vw, 300px);
      border-left: 1px solid #555;
      display: flex;
      flex-direction: column;
      padding: 12px 8px;
      gap: 6px;
    }
    #func-panel-label {
      font-size: 0.7em;
      color: #777;
      text-align: center;
      margin-bottom: 4px;
      letter-spacing: 0.1em;
    }
    .func-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 8px 6px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: clamp(1.1em, 2vw, 1.8em);
      text-align: left;
      width: 100%;
    }
    .func-btn:hover { border-color: #556; color: #aaccff; }
    .func-btn.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }
    /* Button-driven loop membership: color reflects membership state
       (white/gray) EXCEPT when the button is genuinely the active
       single-selected function (green wins then — see setActiveFunc()
       in JS). The rule below needs higher specificity than
       .chk-checked's 3 classes, or a checked-and-active button would
       incorrectly stay white. */
    .func-btn.loop-btn { color: #aaa; }         /* unchecked: gray (default) */
    .func-btn.loop-btn.chk-checked { color: #fff; }  /* checked: white */
    .func-btn.loop-btn.active,
    .func-btn.loop-btn.chk-checked.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }

    /* ── Bottom menu bar ── */
    #menubar {
      border-top: 1px solid #555;
      background: #1e1e1e;
    }
    #menu-btns {
      display: flex;
      border-bottom: 1px solid #2a2a2a;
    }
    .menu-btn {
      flex: 1;
      background: none;
      border: none;
      border-right: 1px solid #2a2a2a;
      color: #aaa;
      padding: 7px 4px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1.7em;
      text-align: center;
    }
    .menu-btn:last-child { border-right: none; }
    .menu-btn:hover { color: #aaccff; background: #222; }
    .menu-btn.exp-btn { color: #bb9944; }
    .menu-btn.exp-btn:hover { color: #ddbb55; }
    #menu-states {
      display: flex;
    }
    .menu-state {
      flex: 1;
      text-align: center;
      font-size: 1.5em;
      color: #777;
      padding: 4px 2px;
      border-right: 1px solid #2a2a2a;
    }
    .menu-state:last-child { border-right: none; }

    /* ── Submenu popup (Range / Reference / Filter) ── */
    #submenu-overlay {
      display: none;
      position: fixed;
      inset: 0;
      background: rgba(0,0,0,0.4);
      z-index: 19;
    }
    #submenu-overlay.open { display: block; }
    #submenu-popup {
      display: none;
      position: fixed;
      bottom: 60px;
      left: 20px;
      width: 320px;
      max-width: 90vw;
      background: #2a2618;
      border: 1px solid #886633;
      border-radius: 6px;
      padding: 12px;
      z-index: 20;
    }
    #submenu-popup.open { display: block; }
    #submenu-title {
      font-size: 0.85em;
      color: #ddaa55;
      letter-spacing: 0.1em;
      margin-bottom: 8px;
      text-align: center;
    }
    #submenu-options {
      display: flex;
      flex-direction: column;
      gap: 4px;
      margin-bottom: 8px;
    }
    .submenu-opt {
      background: #332d1e;
      border: 1px solid #776633;
      border-radius: 4px;
      color: #f0e0b8;
      padding: 8px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.9em;
      text-align: left;
      width: 100%;
    }
    .submenu-opt:hover { border-color: #ddaa55; color: #ffee99; }
    .submenu-opt:disabled {
      opacity: 0.4;
      cursor: not-allowed;
      border-color: #776633;
      color: #665533;
    }
    .submenu-opt:disabled:hover { border-color: #776633; color: #665533; }
    .submenu-opt.current {
      border-color: #ffcc33;
      color: #ffee99;
      font-weight: bold;
    }
    .menu-btn:disabled {
      color: #666;
      cursor: not-allowed;
    }
    .menu-btn:disabled:hover { color: #666; background: none; }
    #submenu-warn {
      font-size: 0.7em;
      color: #997733;
      text-align: center;
      line-height: 1.4;
    }

    /* Auto-Fetch status — a passive indicator, not a button. Toggled from
       the Advanced menu. */
    #autofetch-indicator {
      position: fixed;
      bottom: 100px;
      left: 10px;
      color: #66dddd;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.8em;
      letter-spacing: 0.05em;
      z-index: 10;
    }
    #halt-resume-btn {
      position: fixed;
      bottom: 130px;
      left: 10px;
      background: #2a1414;
      border: 1px solid #663333;
      border-radius: 4px;
      color: #cc6666;
      padding: 6px 14px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.8em;
      letter-spacing: 0.1em;
      z-index: 10;
    }
    #halt-resume-btn:hover { color: #ff8888; border-color: #aa4444; }
    #halt-resume-btn.resuming { color: #66cc88; border-color: #337744; }
    #halt-resume-btn.resuming:hover { color: #88eeaa; border-color: #44aa66; }
    #halt-resume-btn:disabled { opacity: 0.4; cursor: not-allowed; }

    /* Multi-Function toggle. Not just diagnostic scaffolding — likely
       permanent. "on" class gives it a lit appearance while the loop is
       running, since its label text doesn't change. */
    #multifunction-btn {
      margin: 4px 0;
      background: #241a2e;
      border: 1px solid #664488;
      border-radius: 4px;
      color: #cc88ff;
      padding: 10px 6px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: clamp(0.9em, 1.6vw, 1.4em);
      letter-spacing: 0.05em;
      width: 100%;
      text-align: center;
    }
    #multifunction-btn:hover { color: #eeaaff; border-color: #9955bb; }
    #multifunction-btn.on {
      background: #3a2650;
      border-color: #cc88ff;
      color: #eeccff;
    }
    #multifunction-btn:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }

    /* ── Stub panels (Func Gen / Counter, pending real controls) ── */
    /* ── Counter panel (BK1823A) ── */
    #counter-reading-row {
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 30px 16px;
      flex-shrink: 0;
    }
    #counter-reading {
      font-size: 2.6em;
      color: #00ee88;
      font-weight: bold;
      letter-spacing: 0.02em;
    }
    .ctr-row {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 8px;
      padding: 10px 16px;
      border-top: 1px solid #2a2a2a;
    }
    .ctr-label { color: #777; font-size: 0.8em; letter-spacing: 0.1em; margin-right: 4px; }
    .ctr-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 8px 14px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.95em;
    }
    .ctr-btn:hover { border-color: #556; color: #aaccff; }
    .ctr-btn.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }
    .ctr-toggle-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 8px 14px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.95em;
    }
    .ctr-toggle-btn:hover { border-color: #556; color: #aaccff; }

    /* ── Func Gen panel (Rigol DG4062) ── */
    #gen-output-row {
      display: flex;
      gap: 12px;
      align-items: center;
      /* was 16px all sides — container was roughly double the button
         height; this now clears it by just a small margin, not a
         duplicate of the button's own vertical padding */
      padding: 8px 16px;
      border-bottom: 1px solid #2a2a2a;
      flex-shrink: 0;
    }
    .gen-output-btn {
      background: none;
      border: 2px solid #555;
      border-radius: 6px;
      color: #aaa;
      padding: 14px 28px;
      font-size: 1.05em;
      font-weight: bold;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
    }
    .gen-output-btn.gen-output-on {
      background: #123322;
      border-color: #00ee88;
      color: #00ee88;
    }
    #gen-query-status {
      /* fills the space to the left of the buttons, pushing them to
         the right — makes #gen-refresh-btn's own old
         margin-left: auto unnecessary now */
      flex: 1;
      background: none;
      border: none;
      /* road-caution-sign orangish yellow, by explicit request */
      color: #ffb300;
      text-align: center;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.95em;
      cursor: default;
      outline: none;
    }
    #gen-refresh-btn, #gen-local-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 6px;
      color: #aaa;
      padding: 10px 18px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
    }
    #gen-channels-row {
      display: flex;
      gap: 16px;
      padding: 16px;
      flex: 1;
      min-height: 0;
      overflow-y: auto;
      flex-wrap: wrap;
    }
    .gen-channel {
      flex: 1;
      min-width: 320px;
      border: 1px solid #444;
      border-radius: 8px;
      padding: 14px;
      background: #161616;
    }
    .gen-channel-header {
      font-size: 1.2em;
      font-weight: bold;
      color: #ccc;
      margin-bottom: 12px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .gen-channel-header .gen-output-btn {
      margin-left: auto;
      font-size: 0.7em;
      padding: 8px 16px;
    }
    .gen-ch-dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #555;
      display: inline-block;
    }
    .gen-ch-dot.gen-ch-on { background: #00ee88; }
    .gen-func-row { display: flex; flex-wrap: wrap; gap: 6px; margin-bottom: 40px; }
    /* 40px (up from 14px) specifically to clear the Arb dropdown's own
       height — it's absolutely positioned right under its button (see
       .gen-arb-btn-wrap below), so without enough room here it visually
       overlapped whatever core content came right after the function
       row. Harmless extra gap for every other waveform too. */
    .gen-arb-btn-wrap { position: relative; display: inline-block; }
    .gen-arb-btn-wrap select {
      display: none;
      position: absolute;
      top: 100%;
      left: 0;
      margin-top: 4px;
      z-index: 20;
      max-width: 200px;
    }
    /* Visible purely when the Arb button itself is highlighted — no JS
       needed to manage this separately, it just follows the same
       active-class logic every other Function button already uses.
       Both rules here are class/element-only (no ID selectors) so
       specificity comparison works as intended — an earlier version
       accidentally used ID selectors in the "hidden" rule above, which
       always outranked this "show" rule regardless of the button's
       active state, so the dropdown could never actually appear. */
    .gen-arb-btn-wrap .gen-func-btn.active + select {
      display: block;
    }
    .gen-func-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 6px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.85em;
    }
    .gen-func-btn.active { color: #00ee88; border-color: #00ee88; font-weight: bold; }
    /* Harmonic Type buttons (EVEN/ODD/ALL/USER) — same look as
       .gen-func-btn, deliberately its own class so it can never be
       accidentally matched by selectGenFunc()'s querySelectorAll
       (which keys off data-func, not data-harmtype, but a shared
       class name was still needless ambiguity). */
    .gen-harmtype-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 6px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.85em;
    }
    .gen-harmtype-btn.active { color: #00ee88; border-color: #00ee88; font-weight: bold; }
    .gen-harm-title {
      color: #aaa;
      font-weight: bold;
      font-size: 0.9em;
      margin: 10px 0 6px;
      letter-spacing: 0.03em;
    }
    .gen-harm-title:first-child { margin-top: 2px; }
    .gen-field-row { display: flex; align-items: center; gap: 6px; margin-bottom: 9px; flex-wrap: wrap; }
    .gen-field-label { width: 55px; color: #888; font-size: 0.85em; flex-shrink: 0; }
    .gen-field-label-wide { width: 130px; }
    .gen-field-input {
      background: #1a1a1a;
      border: 1px solid #666;
      border-radius: 3px;
      color: #e0e0e0;
      padding: 6px 8px;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      width: 140px;
    }
    .gen-field-unit { color: #888; font-size: 0.85em; min-width: 28px; }
    .gen-set-btn {
      background: #223;
      border: 1px solid #446;
      border-radius: 3px;
      color: #aaccff;
      padding: 6px 10px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.85em;
    }
    .gen-quick-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 3px;
      color: #999;
      padding: 6px 8px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.8em;
    }
    .gen-quick-btn.active { color: #00ee88; border-color: #00ee88; font-weight: bold; }
    .gen-toggle-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 8px 14px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
    }
    .gen-mod-select {
      background: #1a1a1a;
      border: 1px solid #666;
      border-radius: 3px;
      color: #e0e0e0;
      padding: 7px 8px;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
    }
    [disabled].gen-output-btn, [disabled].gen-func-btn, [disabled].gen-set-btn,
    [disabled].gen-quick-btn, [disabled].gen-toggle-btn, [disabled].gen-mod-select,
    [disabled].gen-field-input, [disabled].gen-harmtype-btn,
    [disabled]#gen-refresh-btn { opacity: 0.4; cursor: default; }

    /* Shared placeholder modal */
    .gen-modal-overlay {
      display: none;
      position: fixed;
      inset: 0;
      background: rgba(0, 0, 0, 0.6);
      z-index: 50;
      align-items: center;
      justify-content: center;
    }
    .gen-modal-overlay.open { display: flex; }
    .gen-modal-box {
      background: #1a1a1a;
      border: 1px solid #555;
      border-radius: 8px;
      padding: 22px 26px;
      max-width: 360px;
      text-align: center;
    }
    .gen-modal-title {
      font-size: 1.2em;
      color: #00ee88;
      font-weight: bold;
      margin-bottom: 10px;
    }
    .gen-modal-body {
      color: #ccc;
      margin-bottom: 18px;
      font-size: 0.95em;
      line-height: 1.4;
    }
    .gen-modal-close {
      background: #223;
      border: 1px solid #446;
      border-radius: 4px;
      color: #aaccff;
      padding: 8px 20px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      margin-top: 10px;
    }
    .gen-modal-box-wide {
      max-width: 480px;
      text-align: left;
    }
    .gen-modal-box-wide .gen-modal-title { text-align: center; }
    .gen-modal-note {
      color: #777;
      font-size: 0.75em;
      font-style: italic;
      flex-basis: 100%;
    }

    /* ── Mode/Type/modifiers (Mod/Sweep/Burst, inline in the channel card) ── */
    .gen-hidden { display: none !important; }
    .gen-mode-row { margin-bottom: 4px; }
    .gen-mode-btn.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }
    .gen-modifiers-col {
      border-top: 1px dashed #333;
      margin-top: 6px;
      padding-top: 8px;
    }
    .gen-modrows .gen-modal-note {
      display: block;
      margin-bottom: 6px;
    }

    .stub-panel {
      flex: 1;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      text-align: center;
      padding: 40px;
      gap: 12px;
      color: #999;
    }
    .stub-panel h2 { color: #ccc; font-size: 1.3em; }
    .stub-panel a { color: #aaccff; }

    /* ── TOOLS tab — shared terminal/activity log for all instruments ──
       Full-panel now (was a small floating popup when there was only
       one instrument to log). */
    #scpi-target-row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 10px 16px;
      border-bottom: 1px solid #555;
      flex-shrink: 0;
    }
    #scpi-target-label { color: #777; font-size: 0.8em; letter-spacing: 0.1em; }
    .scpi-target-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 4px;
      color: #aaa;
      padding: 6px 14px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 0.9em;
    }
    .scpi-target-btn:hover { border-color: #556; color: #aaccff; }
    .scpi-target-btn.active {
      color: #00ee88;
      font-weight: bold;
      border-color: #00ee88;
    }
    #scpi-term-log {
      flex: 1;
      min-height: 0;
      overflow-y: auto;
      padding: 12px 16px;
      font-size: 1.1em;
      color: #88cc88;
    }
    .log-sent { color: #88aaff; }
    .log-resp { color: #88cc88; }
    .log-none { color: #aaa; font-style: italic; }
    .log-err  { color: #ff8866; }
    .log-sys  { color: #999; }
    .log-tag  { color: #666; }  /* [DMM]/[Counter]/[FuncGen] prefix on
                                   interleaved log lines */
    #scpi-term-input-row {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
      padding: 10px 16px;
      border-top: 1px solid #555;
      flex-shrink: 0;
    }
    #scpi-term-input {
      flex: 1;
      background: #1a1a1a;
      border: 1px solid #666;
      border-radius: 3px;
      color: #e0e0e0;
      padding: 8px 10px;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1.2em;
    }
    #scpi-term-send {
      background: #223;
      border: 1px solid #446;
      border-radius: 3px;
      color: #aaccff;
      padding: 8px 16px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1.1em;
    }
    #scpi-term-clear {
      background: none;
      border: 1px solid #555;
      border-radius: 3px;
      color: #777;
      padding: 8px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1.1em;
    }

    /* ── Data Logger — same visual language as the SCPI terminal, on the
       opposite side so both can be open at once without overlapping. ── */
    #datalogger {
      display: none;
      position: fixed;
      bottom: 72px;
      left: 160px;
      width: 480px;
      max-width: 90vw;
      background: #111;
      border: 1px solid #666;
      border-radius: 6px;
      padding: 10px;
      z-index: 9;
    }
    #datalogger.open { display: block; }
    #datalogger-header { display: flex; gap: 6px; margin-bottom: 8px; }
    #datalogger-startstop {
      background: #142a14;
      border: 1px solid #336633;
      border-radius: 3px;
      color: #88cc88;
      padding: 5px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1em;
    }
    #datalogger-download {
      background: #223;
      border: 1px solid #446;
      border-radius: 3px;
      color: #aaccff;
      padding: 5px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1em;
    }
    #datalogger-download:disabled,
    #datalogger-startstop:disabled,
    #datalogger-close:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
    #datalogger-close {
      background: none;
      border: 1px solid #777;
      border-radius: 3px;
      color: #eee;
      padding: 5px 8px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1em;
    }
    #datalogger-log {
      height: 220px;
      overflow-y: auto;
      overflow-x: auto;
      white-space: pre;
      font-size: 0.9em;
      color: #ccddcc;
      background: #0a0a0a;
      border: 1px solid #555;
      border-radius: 3px;
      padding: 6px 8px;
    }

    /* ── Func Gen Config panel — save/restore setup, same visual
       language as the Data Logger above, on the Func Gen tab. ── */
    #gen-config-btn {
      background: none;
      border: 1px solid #555;
      border-radius: 6px;
      color: #aaa;
      padding: 10px 18px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
    }
    #genconfig {
      display: none;
      position: fixed;
      bottom: 72px;
      right: 40px;
      width: 520px;
      max-width: 90vw;
      background: #111;
      border: 1px solid #666;
      border-radius: 6px;
      padding: 10px;
      z-index: 9;
    }
    #genconfig.open { display: block; }
    #genconfig-header {
      display: flex;
      align-items: center;
      gap: 6px;
      margin-bottom: 8px;
    }
    #genconfig-title { flex: 1; color: #eee; font-weight: bold; }
    #genconfig-close {
      background: none;
      border: 1px solid #777;
      border-radius: 3px;
      color: #eee;
      padding: 5px 8px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1em;
    }
    .genconfig-section { margin-bottom: 10px; }
    .genconfig-label { color: #aaa; font-size: 0.85em; margin-bottom: 4px; }
    #genconfig-export, #genconfig-import {
      width: 100%;
      height: 110px;
      box-sizing: border-box;
      resize: vertical;
      white-space: pre;
      font-size: 0.85em;
      color: #ccddcc;
      background: #0a0a0a;
      border: 1px solid #555;
      border-radius: 3px;
      padding: 6px 8px;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      margin-bottom: 6px;
    }
    #genconfig-generate, #genconfig-restore {
      background: #142a14;
      border: 1px solid #336633;
      border-radius: 3px;
      color: #88cc88;
      padding: 5px 12px;
      cursor: pointer;
      font-family: 'Cascadia Code', 'Consolas', 'DejaVu Sans Mono', 'Courier New', monospace;
      font-size: 1em;
    }

    /* ── Phone-width layout ──
       Below this, the side-by-side reading+func-panel layout no longer
       fits regardless of font/scale fixes, so it stacks instead. Desktop
       and tablet (including the ~800px-wide Android tablet case) stay on
       the fluid side-by-side layout above this breakpoint. */
    @media (max-width: 600px) {
      #main {
        flex-direction: column;
        overflow-y: auto;
      }
      #reading-panel {
        padding: 16px 20px;
        align-items: center;
      }
      #cycle-readings-panel {
        font-size: 2.2em;
      }
      #func-panel {
        width: 100%;
        flex-direction: row;
        flex-wrap: wrap;
        border-left: none;
        border-top: 1px solid #555;
      }
      #func-panel-label {
        flex: 1 0 100%;
      }
      .func-btn,
      #multifunction-btn {
        width: auto;
        flex: 1 1 auto;
      }
      /* Data Logger becomes a full-width (capped) panel instead of a
         fixed-position 480px box. The SCPI terminal no longer needs
         this treatment — it's a full tab-panel now, not a floating
         popup, so it's already full-width on any screen size. */
      #datalogger {
        left: 10px;
        right: 10px;
        width: auto;
        max-width: none;
      }
      #tabs {
        gap: 2px;
      }
      .tab-btn {
        padding: 6px 8px;
        font-size: 0.75em;
      }
    }

    /* ── Short-viewport layout (phone landscape, or any short window) ──
       Orthogonal to the width breakpoint above: this is a HEIGHT problem,
       not a width one. Phone landscape is wide enough to keep the
       side-by-side reading+func-panel structure (correct — confirmed by
       testing), but too short for the fixed em-based type scale, which
       never shrinks on its own. Threshold set well above the ~360-430px
       typical logical height of a landscape phone to leave headroom for
       mobile browser chrome (e.g. Chrome's address bar) eating into the
       usable area beyond what the viewport height alone reports. */
    @media (max-height: 500px) {
      #main {
        overflow-y: auto;
      }
      #reading-panel {
        padding: 8px 20px;
      }
      #cycle-readings-panel {
        font-size: 2em;
      }
      #func-panel {
        padding: 6px 6px;
        gap: 3px;
      }
      #func-panel-label {
        font-size: 0.6em;
        margin-bottom: 2px;
      }
      .func-btn,
      #multifunction-btn {
        padding: 3px 6px;
        font-size: 1.1em;
      }
    }
  </style>
</head>
<body>

  <!-- Title bar — now houses the instrument tab strip. WS connection
       status is global (one WS for the whole app); per-instrument comm
       status (e.g. DMM's arbiter COMM ok/fail) lives inside that
       instrument's own tab-panel instead, since it's meaningless for a
       tab that isn't even the active instrument. -->
  <div id="titlebar">
    <div id="tabs">
      <button class="tab-btn active" id="tabbtn-dmm" onclick="showTab('dmm')">DMM: BK5491B</button>
      <button class="tab-btn" id="tabbtn-funcgen" onclick="showTab('funcgen')">Func Gen: DG4062</button>
      <button class="tab-btn" id="tabbtn-counter" onclick="showTab('counter')">Counter: BK1823A</button>
      <button class="tab-btn" id="tabbtn-scpi" onclick="showTab('scpi')">TOOLS</button>
    </div>
    <div id="titlebar-right">
      <button id="help-btn" title="Help">?</button>
      <div id="status-stack">
        <span id="ws-status">DISCONNECTED</span>
        <div id="link-status-row">
          <span class="link-status" id="link-dmm">DMM: ?</span>
          <span class="link-status" id="link-counter">CTR: ?</span>
          <span class="link-status" id="link-funcgen">GEN: ?</span>
        </div>
      </div>
    </div>
  </div>

  <div id="app-panels">
  <div class="tab-panel active" id="tab-dmm">

  <!-- Small DMM-scoped sub-header: comm status only. The rest of what
       used to live in the global titlebar (help, ws-status) is shared
       across all tabs now — see #titlebar above. -->
  <div id="dmm-subheader"><span id="dmm-subheader-label">BK 5491B DMM</span><span id="comm-status"></span></div>

  <!-- Main: reading + function buttons -->
  <div id="main">

    <div id="reading-panel">
      <!-- Top row ("Voltage DC" slot) doubles as a catch-all for any
           non-cycling reading (Auto-Fetch polling, manual button clicks,
           terminal FETCH?) regardless of which function is actually
           active — a deliberate placeholder, not a real design (label
           can be mismatched with the value shown). Always visible; the
           other five rows only show during Multi-Function mode. -->
      <div id="cycle-readings-panel">
        <div class="cycle-row" id="row-VOLT:DC"><span class="cr-label" id="top-row-label">Voltage DC:&nbsp;&nbsp;</span><span class="cr-value" id="cr-VOLT:DC">&mdash;</span></div>
        <div class="cycle-row multi-only" id="row-CURR:DC"><span class="cr-label">Current DC:&nbsp;&nbsp;</span><span class="cr-value" id="cr-CURR:DC">&mdash;</span></div>
        <div class="cycle-row multi-only" id="row-VOLT:AC"><span class="cr-label">Voltage AC:&nbsp;&nbsp;</span><span class="cr-value" id="cr-VOLT:AC">&mdash;</span></div>
        <div class="cycle-row multi-only" id="row-CURR:AC"><span class="cr-label">Current AC:&nbsp;&nbsp;</span><span class="cr-value" id="cr-CURR:AC">&mdash;</span></div>
        <div class="cycle-row multi-only" id="row-FREQ"><span class="cr-label">Frequency:&nbsp;&nbsp;</span><span class="cr-value" id="cr-FREQ">&mdash;</span></div>
        <div class="cycle-row multi-only" id="row-PER"><span class="cr-label">Period:&nbsp;&nbsp;</span><span class="cr-value" id="cr-PER">&mdash;</span></div>
      </div>
    </div>

    <div id="func-panel">
      <div id="func-panel-label">FUNCTION</div>
      <!-- Six loop-eligible function buttons are dual-purpose: when
           Multi-Function is off, clicking one selects it (same as
           RES/DIOD/CONT). When Multi-Function is on, clicking one
           toggles its loop membership instead — see
           onFunctionButtonClick(). Button color reflects loop membership
           (white checked / gray unchecked) at all times, whether or not
           Multi-Function is currently running, since membership itself
           persists across on/off. Likely permanent, not experimental
           scaffolding — left unstyled/plain rather than given the
           amber/purple "temporary" treatment the diagnostic tools
           elsewhere use. -->
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('VOLT:DC')" id="btn-VOLT:DC">Voltage DC</button>
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('CURR:DC')" id="btn-CURR:DC">Current DC</button>
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('VOLT:AC')" id="btn-VOLT:AC">Voltage AC</button>
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('CURR:AC')" id="btn-CURR:AC">Current AC</button>
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('FREQ')" id="btn-FREQ">Frequency</button>
      <button class="func-btn loop-btn chk-checked" onclick="onFunctionButtonClick('PER')" id="btn-PER">Period</button>
      <!-- Multi-Function toggle (formerly "Cycle Test") — moved here per
           user request, between Period and Resistance. No longer just
           diagnostic scaffolding; flagged as likely-permanent. -->
      <button id="multifunction-btn" onclick="toggleMultiFunction()">Multi-Function (OFF)</button>
      <button class="func-btn" onclick="selectExclusiveFunction('RES')"  id="btn-RES">Resistance</button>
      <button class="func-btn" onclick="selectExclusiveFunction('DIOD')" id="btn-DIOD">Diode</button>
      <button class="func-btn" onclick="selectExclusiveFunction('CONT')" id="btn-CONT">Continuity</button>
    </div>

  </div>

  <!-- Bottom menu bar -->
  <div id="menubar">
    <div id="menu-btns">
      <button class="menu-btn exp-btn" id="range-menu-btn" onclick="openSubmenu('range', this)">Range</button>
      <button class="menu-btn exp-btn" id="reference-menu-btn" onclick="openSubmenu('reference', this)">Reference</button>
      <button class="menu-btn exp-btn" id="filter-menu-btn" onclick="openSubmenu('filter', this)">Filter</button>
      <button class="menu-btn" id="advanced-menu-btn" onclick="openSubmenu('advanced', this)">Advanced</button>
    </div>
    <div id="menu-states">
      <div class="menu-state" id="state-range">?</div>
      <div class="menu-state" id="state-reference">?</div>
      <div class="menu-state" id="state-filter">?</div>
      <div class="menu-state">&nbsp;</div>
    </div>
  </div>

  <!-- Submenu popup (Range / Reference / Filter) -->
  <div id="submenu-overlay" onclick="closeSubmenu()"></div>
  <div id="submenu-popup">
    <div id="submenu-title"></div>
    <div id="submenu-options"></div>
    <div id="submenu-warn"></div>
  </div>

  <!-- Passive text indicator, not a button — toggle lives in the
       Advanced menu. Blank when off, "Auto-Fetch: ON" when on. -->
  <div id="autofetch-indicator"></div>
  <!-- Universal channel halt/resume — manual (this button) or automatic
       (see haltChannel()/resumeChannel(), used by every routine that
       needs exclusive use of the channel for more than an instant).
       Disabled for the full duration of any automatic routine, so a
       manual click can never land mid-routine. -->
  <button id="halt-resume-btn" onclick="toggleHaltResume()">HALT</button>

  <!-- RESET lives as an Advanced-menu option — confirmReset() has no
       dedicated button of its own. SCPI used to be an Advanced-menu
       item too (a private floating terminal, toggleTerminal()) — that's
       now the shared TOOLS tab below instead, so Advanced's "SCPI" item
       just switches tabs (see openSubmenu('advanced', ...) in JS) and
       preselects DMM as the target. -->

  <!-- Data Logger panel — see openDataLogger()/startDataLogger() in JS.
       Function/checkbox configuration is frozen for the whole logging
       session, same disabling pattern as any halt-covered routine, just
       held open-ended (start/stop) instead of running to automatic
       completion. -->
  <div id="datalogger">
    <div id="datalogger-header">
      <button id="datalogger-startstop" onclick="toggleDataLogger()">Start Logging</button>
      <button id="datalogger-download" onclick="downloadDataLoggerCsv()" disabled>Download CSV</button>
      <button id="datalogger-close" onclick="closeDataLogger()">Close</button>
    </div>
    <div id="datalogger-log"></div>
  </div>

  </div><!-- /#tab-dmm -->

  <!-- ── Function Gen tab (Rigol DG4062) ──
       Confirmed bench-working (real SCPI instrument). Two independent
       channels shown side-by-side, mirroring the front panel's own
       CH1/CH2 layout — see main.cpp's file header for the confirmed
       command set. Deliberately different interaction model from the
       DMM/Counter panels: Output ON/OFF ONLY EVER changes on an
       explicit click, never automatically (unlike DMM/Counter forcing
       known state on connect) — this instrument actively drives a real
       circuit, so surprising it on tab-open could disrupt whatever it's
       connected to. Frequency/Amplitude/Offset/Impedance/Modulation are
       user-edited fields populated by an explicit query on connect/
       Refresh, not continuously polled — see the JS section for the
       full reasoning. Mod Type dropdown covers all 12 types the manual
       (and this unit's own front panel) document: AM/FM/PM/ASK/FSK/PSK
       plus PWM/BPSK/QPSK/3FSK/4FSK/OSK. Two have prerequisites per the
       manual — PWM only available when Pulse is the active waveform,
       OSK only when Sine is — NOT enforced here; selecting one without
       its prerequisite waveform active will presumably error at the
       instrument, not fail silently, but the panel doesn't pre-check or
       explain this yet. Modulation sub-parameters (MFreq/FMDev/depth/
       etc.) are out of scope for this pass, per explicit decision —
       only ON/OFF + Type selection. -->
  <div class="tab-panel" id="tab-funcgen">
    <div id="gen-output-row">
      <input type="text" id="gen-query-status" readonly>
      <button id="gen-refresh-btn" onclick="refreshBothGenChannels()">Refresh</button>
      <button id="gen-local-btn" onclick="enterGenLocalMode()">Local</button>
      <button id="gen-config-btn" onclick="openGenConfig()">Config</button>
    </div>

    <!-- Config panel — save/restore the generator's setup across a power
         cycle. Reads straight from the currently-visible UI fields (the
         values already sitting in the DOM), not a fresh query pass —
         deliberately simple, by explicit request, rather than the more
         thorough (but much slower, ~110-query) "gather everything"
         approach that's still on the table as a future option. This
         means fields never actually visited this session (still blank
         or "...") are correctly skipped, but so is anything changed via
         the instrument's own front panel while in Local mode without a
         Refresh since — a known, accepted limitation of this simple
         version. Per-harmonic Amplitude/Phase are also excluded: they're
         indexed per-harmonic-number, and this only ever knows whichever
         one was last viewed, not the full set — see generateGenConfig()
         in JS for the complete field list actually covered. -->
    <div id="genconfig">
      <div id="genconfig-header">
        <span id="genconfig-title">Save / Restore Setup</span>
        <button id="genconfig-close" onclick="closeGenConfig()">Close</button>
      </div>
      <div class="genconfig-section">
        <div class="genconfig-label">Export — copy this to a file to save today's setup:</div>
        <textarea id="genconfig-export" readonly placeholder="Click Generate to fill this in from the current panel."></textarea>
        <button id="genconfig-generate" onclick="generateGenConfig()">Generate</button>
      </div>
      <div class="genconfig-section">
        <div class="genconfig-label">Import — paste a saved list here, then click Restore:</div>
        <textarea id="genconfig-import" placeholder="Paste previously-saved commands here."></textarea>
        <button id="genconfig-restore" onclick="restoreGenConfig()">Restore</button>
      </div>
    </div>

    <!-- Local mode — halts all Func Gen traffic while the physical
         instrument's own front-panel buttons are in use. See
         enterGenLocalMode()/exitGenLocalMode() in JS. -->
    <div id="gen-local-modal" class="gen-modal-overlay">
      <div class="gen-modal-box">
        <div class="gen-modal-title">Local Operation</div>
        <div class="gen-modal-body">
          Press Burst/Local on the instrument.<br><br>
          Front panel buttons are then active.<br><br>
          Click Done to resume remote mode.
        </div>
        <button class="gen-modal-close" onclick="exitGenLocalMode()">Done</button>
      </div>
    </div>

    <div id="gen-channels-row">

      <!-- CH1 -->
      <div class="gen-channel" id="gen-ch1">
        <div class="gen-channel-header"><span class="gen-ch-dot" id="gen-ch1-dot"></span>CH1<button id="gen-output1-btn" class="gen-output-btn" onclick="toggleGenOutput(1)">OUTPUT 1</button></div>
        <div class="gen-func-row">
          <button class="gen-func-btn" data-func="SIN" onclick="selectGenFunc(1,'SIN')">Sine</button>
          <button class="gen-func-btn" data-func="SQU" onclick="selectGenFunc(1,'SQU')">Square</button>
          <button class="gen-func-btn" data-func="RAMP" onclick="selectGenFunc(1,'RAMP')">Ramp</button>
          <button class="gen-func-btn" data-func="PULSE" onclick="selectGenFunc(1,'PULSE')">Pulse</button>
          <button class="gen-func-btn" data-func="NOISE" onclick="selectGenFunc(1,'NOISE')">Noise</button>
          <span class="gen-arb-btn-wrap"><button class="gen-func-btn" data-func="ARB" onclick="selectGenArbCh(1)">Arb</button>
            <select class="gen-mod-select" id="gen-ch1-arb-select" onchange="selectArbWaveCh(1)">
              <optgroup label="Common">
                <option value="DC">DC</option>
                <option value="ABSSINE">ABSSINE</option>
                <option value="ABSSINEHALF">ABSSINEHALF</option>
                <option value="AMPALT">AMPALT</option>
                <option value="ATTALT">ATTALT</option>
                <option value="GAUSSPULSE">GAUSSPULSE</option>
                <option value="NEGRAMP">NEGRAMP</option>
                <option value="NPULSE">NPULSE</option>
                <option value="PPULSE">PPULSE</option>
                <option value="SINETRA">SINETRA</option>
                <option value="SINEVER">SINEVER</option>
                <option value="STAIRDN">STAIRDN</option>
                <option value="STAIRUD">STAIRUD</option>
                <option value="STAIRUP">STAIRUP</option>
                <option value="TRAPEZIA">TRAPEZIA</option>
              </optgroup>
              <optgroup label="Engineering">
                <option value="BANDLIMITED">BANDLIMITED</option>
                <option value="BUTTERWORTH">BUTTERWORTH</option>
                <option value="CHEBYSHEV1">CHEBYSHEV1</option>
                <option value="CHEBYSHEV2">CHEBYSHEV2</option>
                <option value="COMBIN">COMBIN</option>
                <option value="CPULSE">CPULSE</option>
                <option value="CWPULSE">CWPULSE</option>
                <option value="DAMPEDOSC">DAMPEDOSC</option>
                <option value="DUALTONE">DUALTONE</option>
                <option value="GAMMA">GAMMA</option>
                <option value="GATEVIBR">GATEVIBR</option>
                <option value="LFMPULSE">LFMPULSE</option>
                <option value="MCNOSIE">MCNOSIE</option>
                <option value="NIMHDISCHARGE">NIMHDISCHARGE</option>
                <option value="PAHCUR">PAHCUR</option>
                <option value="QUAKE">QUAKE</option>
                <option value="RADAR">RADAR</option>
                <option value="RIPPLE">RIPPLE</option>
                <option value="ROUNDHALF">ROUNDHALF</option>
                <option value="ROUNDPM">ROUNDPM</option>
                <option value="STEPRESP">STEPRESP</option>
                <option value="SWINGOSC">SWINGOSC</option>
                <option value="TV">TV</option>
                <option value="VOICE">VOICE</option>
                <option value="THREEAM">THREEAM</option>
                <option value="THREEFM">THREEFM</option>
                <option value="THREEPM">THREEPM</option>
                <option value="THREEPWM">THREEPWM</option>
                <option value="THREEPFM">THREEPFM</option>
              </optgroup>
              <optgroup label="Bio / Medical / Automotive">
                <option value="CARDIAC">CARDIAC</option>
                <option value="EOG">EOG</option>
                <option value="EEG">EEG</option>
                <option value="EMG">EMG</option>
                <option value="PULSILOGRAM">PULSILOGRAM</option>
                <option value="RESSPEED">RESSPEED</option>
                <option value="LFPULSE">LFPULSE</option>
                <option value="TENS1">TENS1</option>
                <option value="TENS2">TENS2</option>
                <option value="TENS3">TENS3</option>
                <option value="IGNITION">IGNITION</option>
                <option value="ISO167502SP">ISO167502SP</option>
                <option value="ISO167502VR">ISO167502VR</option>
                <option value="ISO76372TP1">ISO76372TP1</option>
                <option value="ISO76372TP2A">ISO76372TP2A</option>
                <option value="ISO76372TP2B">ISO76372TP2B</option>
                <option value="ISO76372TP3A">ISO76372TP3A</option>
                <option value="ISO76372TP3B">ISO76372TP3B</option>
                <option value="ISO76372TP4">ISO76372TP4</option>
                <option value="ISO76372TP5A">ISO76372TP5A</option>
                <option value="ISO76372TP5B">ISO76372TP5B</option>
                <option value="SCR">SCR</option>
                <option value="SURGE">SURGE</option>
              </optgroup>
              <optgroup label="Math Functions">
                <option value="AIRY">AIRY</option>
                <option value="BESSELJ">BESSELJ</option>
                <option value="BESSELY">BESSELY</option>
                <option value="CAUCHY">CAUCHY</option>
                <option value="CUBIC">CUBIC</option>
                <option value="DIRICHLET">DIRICHLET</option>
                <option value="ERF">ERF</option>
                <option value="ERFC">ERFC</option>
                <option value="ERFCINV">ERFCINV</option>
                <option value="ERFINV">ERFINV</option>
                <option value="EXPFALL">EXPFALL</option>
                <option value="EXPRISE">EXPRISE</option>
                <option value="GAUSS">GAUSS</option>
                <option value="HAVERSINE">HAVERSINE</option>
                <option value="LAGUERRE">LAGUERRE</option>
                <option value="LAPLACE">LAPLACE</option>
                <option value="LEGEND">LEGEND</option>
                <option value="LOG">LOG</option>
                <option value="LOGNORMAL">LOGNORMAL</option>
                <option value="LORENTZ">LORENTZ</option>
                <option value="MAXWELL">MAXWELL</option>
                <option value="RAYLEIGH">RAYLEIGH</option>
                <option value="VERSIERA">VERSIERA</option>
                <option value="WEIBULL">WEIBULL</option>
                <option value="X2DATA">X2DATA</option>
                <option value="COSH">COSH</option>
                <option value="COSINT">COSINT</option>
                <option value="COT">COT</option>
                <option value="COTHCON">COTHCON</option>
                <option value="COTHPRO">COTHPRO</option>
                <option value="CSCCON">CSCCON</option>
                <option value="CSCPRO">CSCPRO</option>
                <option value="CSCHCON">CSCHCON</option>
                <option value="CSCHPRO">CSCHPRO</option>
                <option value="RECIPCON">RECIPCON</option>
                <option value="RECIPPRO">RECIPPRO</option>
                <option value="SECCON">SECCON</option>
                <option value="SECPRO">SECPRO</option>
                <option value="SECH">SECH</option>
                <option value="SINC">SINC</option>
                <option value="SINH">SINH</option>
                <option value="SININT">SININT</option>
                <option value="SQRT">SQRT</option>
                <option value="TAN">TAN</option>
                <option value="TANH">TANH</option>
                <option value="ACOS">ACOS</option>
                <option value="ACOSH">ACOSH</option>
                <option value="ACOTCON">ACOTCON</option>
                <option value="ACOTPRO">ACOTPRO</option>
                <option value="ACOTHCON">ACOTHCON</option>
                <option value="ACOTHPRO">ACOTHPRO</option>
                <option value="ACSCCON">ACSCCON</option>
                <option value="ACSCPRO">ACSCPRO</option>
                <option value="ACSCHCON">ACSCHCON</option>
                <option value="ACSCHPRO">ACSCHPRO</option>
                <option value="ASECCON">ASECCON</option>
                <option value="ASECPRO">ASECPRO</option>
                <option value="ASECH">ASECH</option>
                <option value="ASIN">ASIN</option>
                <option value="ASINH">ASINH</option>
                <option value="ATAN">ATAN</option>
                <option value="ATANH">ATANH</option>
              </optgroup>
              <optgroup label="Window Functions">
                <option value="BARLETT">BARLETT</option>
                <option value="BARTHANN">BARTHANN</option>
                <option value="BLACKMAN">BLACKMAN</option>
                <option value="BLACKMANH">BLACKMANH</option>
                <option value="BOHMANWIN">BOHMANWIN</option>
                <option value="BOXCAR">BOXCAR</option>
                <option value="CHEBWIN">CHEBWIN</option>
                <option value="FLATTOPWIN">FLATTOPWIN</option>
                <option value="HAMMING">HAMMING</option>
                <option value="HANNING">HANNING</option>
                <option value="KAISER">KAISER</option>
                <option value="NUTTALLWIN">NUTTALLWIN</option>
                <option value="PARZENWIN">PARZENWIN</option>
                <option value="TAYLORWIN">TAYLORWIN</option>
                <option value="TRIANG">TRIANG</option>
                <option value="TUKEYWIN">TUKEYWIN</option>
              </optgroup>
              <optgroup label="Custom">
                <option value="CUSTOM">CUSTom</option>
              </optgroup>
            </select>
          </span>
          <button class="gen-func-btn" data-func="HARM" onclick="selectGenHarmonic(1)">Harmonic</button>
          <button class="gen-func-btn" data-func="USER" onclick="selectGenUserCh(1)">User*</button>
        </div>
        <div class="gen-field-row" id="gen-ch1-freq-row">
          <span class="gen-field-label">Freq</span>
          <input type="text" class="gen-field-input" id="gen-ch1-freq" onkeydown="if(event.key==='Enter')setGenFreq(1)">
          <span class="gen-field-unit">Hz</span>
          <button class="gen-set-btn" onclick="setGenFreq(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Period</span>
          <input type="text" class="gen-field-input" id="gen-ch1-period" onkeydown="if(event.key==='Enter')setGenPeriodCh(1)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPeriodCh(1)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Ampl</span>
          <input type="text" class="gen-field-input" id="gen-ch1-ampl" onkeydown="if(event.key==='Enter')setGenAmpl(1)">
          <span class="gen-field-unit">Vpp</span>
          <button class="gen-set-btn" onclick="setGenAmpl(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Max</span>
          <input type="text" class="gen-field-input" id="gen-ch1-high" onkeydown="if(event.key==='Enter')setGenHigh(1)">
          <span class="gen-field-unit">V</span>
          <button class="gen-set-btn" onclick="setGenHigh(1)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Offset</span>
          <input type="text" class="gen-field-input" id="gen-ch1-offset" onkeydown="if(event.key==='Enter')setGenOffset(1)">
          <span class="gen-field-unit">Vdc</span>
          <button class="gen-set-btn" onclick="setGenOffset(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Min</span>
          <input type="text" class="gen-field-input" id="gen-ch1-low" onkeydown="if(event.key==='Enter')setGenLow(1)">
          <span class="gen-field-unit">V</span>
          <button class="gen-set-btn" onclick="setGenLow(1)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Imp</span>
          <input type="text" class="gen-field-input" id="gen-ch1-imp" onkeydown="if(event.key==='Enter')setGenImp(1)">
          <span class="gen-field-unit">&Omega;</span>
          <button class="gen-set-btn" onclick="setGenImp(1)">Set</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(1,'MIN')">Min</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(1,'50')">50</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(1,'MAX')">Max</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(1,'INF')">HighZ</button>
        </div>
        <div class="gen-field-row" id="gen-ch1-phase-row">
          <span class="gen-field-label">Phase</span>
          <input type="text" class="gen-field-input" id="gen-ch1-phase" onkeydown="if(event.key==='Enter')setGenPhaseCh(1)">
          <span class="gen-field-unit">&deg;</span>
          <button class="gen-set-btn" onclick="setGenPhaseCh(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Sync:</span>
          <button class="gen-toggle-btn" id="gen-ch1-alignphase-btn" onclick="alignPhaseCh(1)" disabled>Align Phase</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch1-squ-dutycycle-row">
          <span class="gen-field-label">Duty</span>
          <input type="text" class="gen-field-input" id="gen-ch1-squ-dutycycle" onkeydown="if(event.key==='Enter')setGenDutyCycleCh(1)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenDutyCycleCh(1)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch1-ramp-symmetry-row">
          <span class="gen-field-label">Symmetry</span>
          <input type="text" class="gen-field-input" id="gen-ch1-ramp-symmetry" onkeydown="if(event.key==='Enter')setGenSymmetryCh(1)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenSymmetryCh(1)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch1-pulse-dutycycle-row">
          <span class="gen-field-label">Duty</span>
          <input type="text" class="gen-field-input" id="gen-ch1-pulse-dutycycle" onkeydown="if(event.key==='Enter')setGenPulseDutyCycleCh(1)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenPulseDutyCycleCh(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Width</span>
          <input type="text" class="gen-field-input" id="gen-ch1-pulse-width" onkeydown="if(event.key==='Enter')setGenPulseWidthCh(1)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseWidthCh(1)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch1-pulse-edges-row">
          <span class="gen-field-label">Lead</span>
          <input type="text" class="gen-field-input" id="gen-ch1-pulse-leading" onkeydown="if(event.key==='Enter')setGenPulseLeadingCh(1)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseLeadingCh(1)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Trail</span>
          <input type="text" class="gen-field-input" id="gen-ch1-pulse-trailing" onkeydown="if(event.key==='Enter')setGenPulseTrailingCh(1)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseTrailingCh(1)">Set</button>
        </div>
        <div id="gen-ch1-dynamic-area">
          <div class="gen-field-row gen-mode-row" id="gen-ch1-mode-row">
            <span class="gen-field-label">Mode:</span>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch1-mode-mod" onclick="selectGenMode(1,'mod')">Mod: Off</button>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch1-mode-sweep" onclick="selectGenMode(1,'sweep')">Sweep: Off</button>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch1-mode-burst" onclick="selectGenMode(1,'burst')">Burst: Off</button>
          </div>
          <div id="gen-ch1-type-and-modifiers">
            <div class="gen-field-row">
              <span class="gen-field-label">Type:</span>
              <select class="gen-mod-select gen-hidden" id="gen-ch1-mod-typesel" onchange="selectGenModType(1)">
                <option value="AM">AM</option>
                <option value="FM">FM</option>
                <option value="PM">PM</option>
                <option value="ASK">ASK</option>
                <option value="FSK">FSK</option>
                <option value="PSK">PSK</option>
                <option value="PWM">PWM</option>
                <option value="BPSK">BPSK</option>
                <option value="QPSK">QPSK</option>
                <option value="3FSK">3FSK</option>
                <option value="4FSK">4FSK</option>
                <option value="OSK">OSK</option>
              </select>
              <select class="gen-mod-select gen-hidden" id="gen-ch1-sweep-typesel" onchange="setSweepSpacingCh(1)">
                <option value="LINear">Linear</option>
                <option value="LOGarithmic">Log</option>
                <option value="STEp">Step</option>
              </select>
              <span class="gen-hidden" id="gen-ch1-sweep-step-group">
                <span class="gen-field-label" style="margin-left:12px;">Steps</span>
                <input type="text" class="gen-field-input" id="gen-ch1-sweep-step" onkeydown="if(event.key==='Enter')setSweepStepCh(1)">
                <span class="gen-field-unit">#</span>
                <button class="gen-set-btn" onclick="setSweepStepCh(1)">Set</button>
              </span>
              <select class="gen-mod-select gen-hidden" id="gen-ch1-burst-typesel" onchange="setBurstModeCh(1)">
                <option value="TRIGgered">N-Cycle</option>
                <option value="GATed">Gated</option>
                <option value="INFinity">Infinite</option>
              </select>
            </div>
            <div class="gen-modifiers-col">
              <div class="gen-modrows gen-hidden" id="gen-ch1-mod-modrows">
                <div class="gen-hidden" id="gen-ch1-mod-am-rows">
                <div class="gen-field-row">
                  <span class="gen-field-label">Source</span>
                  <select class="gen-mod-select" id="gen-ch1-mod-am-source" onchange="updateModAmSourceGrayingCh(1)">
                    <option value="INTernal">Internal</option>
                    <option value="EXTernal">External</option>
                  </select>
                  <button class="gen-set-btn" onclick="setModAmSourceCh(1)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Mod Freq</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-mod-am-freq" onkeydown="if(event.key==='Enter')setModAmFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" id="gen-ch1-mod-am-freq-set" onclick="setModAmFreqCh(1)">Set</button>
                  <span class="gen-modal-note">Internal source only</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Shape</span>
                  <select class="gen-mod-select" id="gen-ch1-mod-am-shape">
                    <option value="SINusoid">Sine</option>
                    <option value="SQUare">Square</option>
                    <option value="TRIangle">Triangle</option>
                    <option value="RAMP">Ramp</option>
                    <option value="NRAMp">Neg Ramp</option>
                    <option value="NOISe">Noise</option>
                    <option value="USER">User</option>
                  </select>
                  <button class="gen-set-btn" id="gen-ch1-mod-am-shape-set" onclick="setModAmShapeCh(1)">Set</button>
                  <span class="gen-modal-note">Internal source only</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Depth</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-mod-am-depth" onkeydown="if(event.key==='Enter')setModAmDepthCh(1)">
                  <span class="gen-field-unit">%</span>
                  <button class="gen-set-btn" onclick="setModAmDepthCh(1)">Set</button>
                </div>
                <div class="gen-field-row">
                  <button class="gen-toggle-btn" id="gen-ch1-mod-am-dssc-btn" data-on="0" data-label="DSSC" onclick="toggleModAmDsscCh(1)">DSSC: OFF</button>
                  <span class="gen-modal-note">confirmed: :SOUR&lt;n&gt;:AM:DSSC ON|OFF</span>
                </div>
                </div>
                <!-- PM — mirrors AM's shape exactly (Source/Freq/Shape/
                     Deviation instead of Depth). Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch1-mod-pm-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-pm-source" onchange="updateModSourceGrayingCh(1,'pm')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModPmSourceCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Mod Freq</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-pm-freq" onkeydown="if(event.key==='Enter')setModPmFreqCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch1-mod-pm-freq-set" onclick="setModPmFreqCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Shape</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-pm-shape">
                      <option value="SINusoid">Sine</option>
                      <option value="SQUare">Square</option>
                      <option value="TRIangle">Triangle</option>
                      <option value="RAMP">Ramp</option>
                      <option value="NRAMp">Neg Ramp</option>
                      <option value="NOISe">Noise</option>
                      <option value="USER">User</option>
                    </select>
                    <button class="gen-set-btn" id="gen-ch1-mod-pm-shape-set" onclick="setModPmShapeCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Deviation</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-pm-deviation" onkeydown="if(event.key==='Enter')setModPmDeviationCh(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModPmDeviationCh(1)">Set</button>
                  </div>
                </div>
                <!-- ASK — Source/Rate/Amplitude/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch1-mod-ask-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-ask-source" onchange="updateModSourceGrayingCh(1,'ask')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModAskSourceCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-ask-rate" onkeydown="if(event.key==='Enter')setModAskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch1-mod-ask-rate-set" onclick="setModAskRateCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Amplitude</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-ask-ampl" onkeydown="if(event.key==='Enter')setModAskAmplCh(1)">
                    <span class="gen-field-unit">Vpp</span>
                    <button class="gen-set-btn" onclick="setModAskAmplCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-ask-polarity" onchange="setModAskPolarityCh(1)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- FSK — Source/Rate/Hop Freq/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch1-mod-fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-fsk-source" onchange="updateModSourceGrayingCh(1,'fsk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModFskSourceCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-fsk-rate" onkeydown="if(event.key==='Enter')setModFskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch1-mod-fsk-rate-set" onclick="setModFskRateCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-fsk-hopfreq" onkeydown="if(event.key==='Enter')setModFskHopFreqCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModFskHopFreqCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-fsk-polarity" onchange="setModFskPolarityCh(1)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- PSK — Source/Rate/Phase/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch1-mod-psk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-psk-source" onchange="updateModSourceGrayingCh(1,'psk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModPskSourceCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-psk-rate" onkeydown="if(event.key==='Enter')setModPskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch1-mod-psk-rate-set" onclick="setModPskRateCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-psk-phase" onkeydown="if(event.key==='Enter')setModPskPhaseCh(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModPskPhaseCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-psk-polarity" onchange="setModPskPolarityCh(1)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- BPSK — no Source/Polarity (manual confirms neither
                     exists for this type); Data replaces them. -->
                <div class="gen-hidden" id="gen-ch1-mod-bpsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-bpsk-rate" onkeydown="if(event.key==='Enter')setModBpskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModBpskRateCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-bpsk-phase" onkeydown="if(event.key==='Enter')setModBpskPhaseCh(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModBpskPhaseCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Data</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-bpsk-data" onchange="setModBpskDataCh(1)">
                      <option value="01">01</option>
                      <option value="10">10</option>
                      <option value="PN15">PN15</option>
                      <option value="PN21">PN21</option>
                    </select>
                  </div>
                </div>
                <!-- QPSK — same shape as BPSK, three phases instead of
                     one, Data limited to PN15/PN21 (no 01/10). -->
                <div class="gen-hidden" id="gen-ch1-mod-qpsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-qpsk-rate" onkeydown="if(event.key==='Enter')setModQpskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModQpskRateCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-qpsk-phase1" onkeydown="if(event.key==='Enter')setModQpskPhase1Ch(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase1Ch(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-qpsk-phase2" onkeydown="if(event.key==='Enter')setModQpskPhase2Ch(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase2Ch(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 3</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-qpsk-phase3" onkeydown="if(event.key==='Enter')setModQpskPhase3Ch(1)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase3Ch(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Data</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-qpsk-data" onchange="setModQpskDataCh(1)">
                      <option value="PN15">PN15</option>
                      <option value="PN21">PN21</option>
                    </select>
                  </div>
                </div>
                <!-- 3FSK — Rate + 2 additional indexed hop frequencies
                     (n=1,2; the base Freq field is the carrier itself).
                     Manual: ":MOD:3FSKey[:FREQuency] <n>,<freq>". -->
                <div class="gen-hidden" id="gen-ch1-mod-3fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-3fsk-rate" onkeydown="if(event.key==='Enter')setMod3fskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskRateCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-3fsk-freq1" onkeydown="if(event.key==='Enter')setMod3fskFreqCh(1,1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskFreqCh(1,1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-3fsk-freq2" onkeydown="if(event.key==='Enter')setMod3fskFreqCh(1,2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskFreqCh(1,2)">Set</button>
                  </div>
                </div>
                <!-- 4FSK — same idea, n=1,2,3. -->
                <div class="gen-hidden" id="gen-ch1-mod-4fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-4fsk-rate" onkeydown="if(event.key==='Enter')setMod4fskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskRateCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-4fsk-freq1" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(1,1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(1,1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-4fsk-freq2" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(1,2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(1,2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 3</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-4fsk-freq3" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(1,3)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(1,3)">Set</button>
                  </div>
                </div>
                <!-- OSK — Source/Rate/Time. No Shape/Polarity. -->
                <div class="gen-hidden" id="gen-ch1-mod-osk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch1-mod-osk-source" onchange="updateModSourceGrayingCh(1,'osk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModOskSourceCh(1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-osk-rate" onkeydown="if(event.key==='Enter')setModOskRateCh(1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch1-mod-osk-rate-set" onclick="setModOskRateCh(1)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Time</span>
                    <input type="text" class="gen-field-input" id="gen-ch1-mod-osk-time" onkeydown="if(event.key==='Enter')setModOskTimeCh(1)">
                    <span class="gen-field-unit">s</span>
                    <button class="gen-set-btn" onclick="setModOskTimeCh(1)">Set</button>
                    <span class="gen-modal-note">range depends on current Rate</span>
                  </div>
                </div>
              </div>
              <div class="gen-modrows gen-hidden" id="gen-ch1-sweep-modrows">
                <div class="gen-field-row">
                  <span class="gen-field-label">Sweep</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-time" onkeydown="if(event.key==='Enter')setSweepTimeCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepTimeCh(1)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Return</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-rtime" onkeydown="if(event.key==='Enter')setSweepRTimeCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepRTimeCh(1)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Start</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-startfreq" onkeydown="if(event.key==='Enter')setSweepStartFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepStartFreqCh(1)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Center</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-centerfreq" onkeydown="if(event.key==='Enter')setSweepCenterFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepCenterFreqCh(1)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Stop</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-stopfreq" onkeydown="if(event.key==='Enter')setSweepStopFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepStopFreqCh(1)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Span</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-spanfreq" onkeydown="if(event.key==='Enter')setSweepSpanFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepSpanFreqCh(1)">Set</button>
                  <span class="gen-modal-note">swap Start/Stop to reverse direction</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Hold Start</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-htimestart" onkeydown="if(event.key==='Enter')setSweepHTimeStartCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepHTimeStartCh(1)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Hold Stop</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-htimestop" onkeydown="if(event.key==='Enter')setSweepHTimeStopCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepHTimeStopCh(1)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Trig Src</span>
                  <select class="gen-mod-select" id="gen-ch1-sweep-trigsrc" onchange="setSweepTrigSrcCh(1)">
                    <option value="INTernal">Internal</option>
                    <option value="EXTernal">External</option>
                    <option value="MANual">Manual</option>
                  </select>
                  <button class="gen-quick-btn gen-hidden" id="gen-ch1-sweep-trignow-btn" onclick="sweepManualTriggerCh(1)">Trigger Now</button>
                </div>
                <div class="gen-field-row">
                  <button class="gen-toggle-btn" id="gen-ch1-sweep-markstate-btn" data-on="0" data-label="Mark" onclick="toggleSweepMarkStateCh(1)">Mark: OFF</button>
                  <input type="text" class="gen-field-input" id="gen-ch1-sweep-markfreq" onkeydown="if(event.key==='Enter')setSweepMarkFreqCh(1)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepMarkFreqCh(1)">Set</button>
                </div>
              </div>
              <div class="gen-modrows gen-hidden" id="gen-ch1-burst-modrows">
                <div class="gen-field-row" id="gen-ch1-burst-ncycles-row">
                  <span class="gen-field-label">Cycles</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-burst-ncycles" onkeydown="if(event.key==='Enter')setBurstNCyclesCh(1)">
                  <span class="gen-field-unit">cyc</span>
                  <button class="gen-set-btn" onclick="setBurstNCyclesCh(1)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch1-burst-period-row">
                  <span class="gen-field-label">Period</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-burst-period" onkeydown="if(event.key==='Enter')setBurstPeriodCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setBurstPeriodCh(1)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch1-burst-tdelay-row">
                  <span class="gen-field-label">Delay</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-burst-tdelay" onkeydown="if(event.key==='Enter')setBurstTDelayCh(1)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setBurstTDelayCh(1)">Set</button>
                </div>
                <div class="gen-field-row gen-hidden" id="gen-ch1-burst-gatepol-row">
                  <span class="gen-field-label">Gate Pol</span>
                  <select class="gen-mod-select" id="gen-ch1-burst-gatepol" onchange="setBurstGatePolCh(1)">
                    <option value="NORMal">Positive</option>
                    <option value="INVerted">Negative</option>
                  </select>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Phase</span>
                  <input type="text" class="gen-field-input" id="gen-ch1-burst-phase" onkeydown="if(event.key==='Enter')setBurstPhaseCh(1)">
                  <span class="gen-field-unit">&deg;</span>
                  <button class="gen-set-btn" onclick="setBurstPhaseCh(1)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch1-burst-trigsrc-row">
                  <span class="gen-field-label">Trig Src</span>
                  <select class="gen-mod-select" id="gen-ch1-burst-trigsrc" onchange="setBurstTrigSrcCh(1)">
                    <option value="INTernal" id="gen-ch1-burst-trigsrc-int-opt">Internal</option>
                    <option value="EXTernal">External</option>
                    <option value="MANual">Manual</option>
                  </select>
                  <button class="gen-quick-btn gen-hidden" id="gen-ch1-burst-trignow-btn" onclick="burstManualTriggerCh(1)">Trigger Now</button>
                  <span class="gen-hidden" id="gen-ch1-burst-trigout-group">
                    <span class="gen-field-label" style="margin-left:12px;">TrigOut</span>
                    <select class="gen-mod-select" id="gen-ch1-burst-trigout" onchange="setBurstTrigOutCh(1)">
                      <option value="OFF">Off</option>
                      <option value="POSitive">Leading</option>
                      <option value="NEGative">Trailing</option>
                    </select>
                  </span>
                  <span class="gen-hidden" id="gen-ch1-burst-slopein-group">
                    <span class="gen-field-label" style="margin-left:12px;">SlopeIn</span>
                    <select class="gen-mod-select" id="gen-ch1-burst-slopein" onchange="setBurstSlopeInCh(1)">
                      <option value="POSitive">Leading</option>
                      <option value="NEGative">Trailing</option>
                    </select>
                  </span>
                </div>
              </div>
            </div>
          </div>
          <div class="gen-modrows gen-hidden" id="gen-ch1-harmonic-mock">
            <div class="gen-harm-title">Harmonics parameters</div>
            <div class="gen-field-row">
              <span class="gen-field-label">Harmonics:</span>
              <button class="gen-harmtype-btn" data-harmtype="EVEN" onclick="selectHarmTypeCh(1,'EVEN')">EVEN</button>
              <button class="gen-harmtype-btn" data-harmtype="ODD" onclick="selectHarmTypeCh(1,'ODD')">ODD</button>
              <button class="gen-harmtype-btn" data-harmtype="ALL" onclick="selectHarmTypeCh(1,'ALL')">ALL</button>
              <button class="gen-harmtype-btn" data-harmtype="USER" onclick="selectHarmTypeCh(1,'USER')">USER SELECTED</button>
            </div>
            <div class="gen-harm-title">Harmonic Values:</div>
            <div class="gen-field-row">
              <select class="gen-mod-select" id="gen-ch1-harm-index" onchange="selectHarmIndexCh(1)">
                <option value="2">2</option><option value="3">3</option><option value="4">4</option><option value="5">5</option><option value="6">6</option><option value="7">7</option><option value="8">8</option><option value="9">9</option><option value="10">10</option><option value="11">11</option><option value="12">12</option><option value="13">13</option><option value="14">14</option><option value="15">15</option><option value="16">16</option>
              </select>
              <span class="gen-field-label">Ampl:</span>
              <input type="text" class="gen-field-input" id="gen-ch1-harm-ampl" onkeydown="if(event.key==='Enter')setHarmAmplPhaseCh(1)">
              <span class="gen-field-unit">Vpp</span>
              <span class="gen-field-label">Phase:</span>
              <input type="text" class="gen-field-input" id="gen-ch1-harm-phaseoffset" onkeydown="if(event.key==='Enter')setHarmAmplPhaseCh(1)">
              <span class="gen-field-unit">deg</span>
              <button class="gen-set-btn" onclick="setHarmAmplPhaseCh(1)">Set</button>
            </div>
          </div>
        </div>
      </div>

      <!-- CH2 -->
      <div class="gen-channel" id="gen-ch2">
        <div class="gen-channel-header"><span class="gen-ch-dot" id="gen-ch2-dot"></span>CH2<button id="gen-output2-btn" class="gen-output-btn" onclick="toggleGenOutput(2)">OUTPUT 2</button></div>
        <div class="gen-func-row">
          <button class="gen-func-btn" data-func="SIN" onclick="selectGenFunc(2,'SIN')">Sine</button>
          <button class="gen-func-btn" data-func="SQU" onclick="selectGenFunc(2,'SQU')">Square</button>
          <button class="gen-func-btn" data-func="RAMP" onclick="selectGenFunc(2,'RAMP')">Ramp</button>
          <button class="gen-func-btn" data-func="PULSE" onclick="selectGenFunc(2,'PULSE')">Pulse</button>
          <button class="gen-func-btn" data-func="NOISE" onclick="selectGenFunc(2,'NOISE')">Noise</button>
          <span class="gen-arb-btn-wrap"><button class="gen-func-btn" data-func="ARB" onclick="selectGenArbCh(2)">Arb</button>
            <select class="gen-mod-select" id="gen-ch2-arb-select" onchange="selectArbWaveCh(2)">
              <optgroup label="Common">
                <option value="DC">DC</option>
                <option value="ABSSINE">ABSSINE</option>
                <option value="ABSSINEHALF">ABSSINEHALF</option>
                <option value="AMPALT">AMPALT</option>
                <option value="ATTALT">ATTALT</option>
                <option value="GAUSSPULSE">GAUSSPULSE</option>
                <option value="NEGRAMP">NEGRAMP</option>
                <option value="NPULSE">NPULSE</option>
                <option value="PPULSE">PPULSE</option>
                <option value="SINETRA">SINETRA</option>
                <option value="SINEVER">SINEVER</option>
                <option value="STAIRDN">STAIRDN</option>
                <option value="STAIRUD">STAIRUD</option>
                <option value="STAIRUP">STAIRUP</option>
                <option value="TRAPEZIA">TRAPEZIA</option>
              </optgroup>
              <optgroup label="Engineering">
                <option value="BANDLIMITED">BANDLIMITED</option>
                <option value="BUTTERWORTH">BUTTERWORTH</option>
                <option value="CHEBYSHEV1">CHEBYSHEV1</option>
                <option value="CHEBYSHEV2">CHEBYSHEV2</option>
                <option value="COMBIN">COMBIN</option>
                <option value="CPULSE">CPULSE</option>
                <option value="CWPULSE">CWPULSE</option>
                <option value="DAMPEDOSC">DAMPEDOSC</option>
                <option value="DUALTONE">DUALTONE</option>
                <option value="GAMMA">GAMMA</option>
                <option value="GATEVIBR">GATEVIBR</option>
                <option value="LFMPULSE">LFMPULSE</option>
                <option value="MCNOSIE">MCNOSIE</option>
                <option value="NIMHDISCHARGE">NIMHDISCHARGE</option>
                <option value="PAHCUR">PAHCUR</option>
                <option value="QUAKE">QUAKE</option>
                <option value="RADAR">RADAR</option>
                <option value="RIPPLE">RIPPLE</option>
                <option value="ROUNDHALF">ROUNDHALF</option>
                <option value="ROUNDPM">ROUNDPM</option>
                <option value="STEPRESP">STEPRESP</option>
                <option value="SWINGOSC">SWINGOSC</option>
                <option value="TV">TV</option>
                <option value="VOICE">VOICE</option>
                <option value="THREEAM">THREEAM</option>
                <option value="THREEFM">THREEFM</option>
                <option value="THREEPM">THREEPM</option>
                <option value="THREEPWM">THREEPWM</option>
                <option value="THREEPFM">THREEPFM</option>
              </optgroup>
              <optgroup label="Bio / Medical / Automotive">
                <option value="CARDIAC">CARDIAC</option>
                <option value="EOG">EOG</option>
                <option value="EEG">EEG</option>
                <option value="EMG">EMG</option>
                <option value="PULSILOGRAM">PULSILOGRAM</option>
                <option value="RESSPEED">RESSPEED</option>
                <option value="LFPULSE">LFPULSE</option>
                <option value="TENS1">TENS1</option>
                <option value="TENS2">TENS2</option>
                <option value="TENS3">TENS3</option>
                <option value="IGNITION">IGNITION</option>
                <option value="ISO167502SP">ISO167502SP</option>
                <option value="ISO167502VR">ISO167502VR</option>
                <option value="ISO76372TP1">ISO76372TP1</option>
                <option value="ISO76372TP2A">ISO76372TP2A</option>
                <option value="ISO76372TP2B">ISO76372TP2B</option>
                <option value="ISO76372TP3A">ISO76372TP3A</option>
                <option value="ISO76372TP3B">ISO76372TP3B</option>
                <option value="ISO76372TP4">ISO76372TP4</option>
                <option value="ISO76372TP5A">ISO76372TP5A</option>
                <option value="ISO76372TP5B">ISO76372TP5B</option>
                <option value="SCR">SCR</option>
                <option value="SURGE">SURGE</option>
              </optgroup>
              <optgroup label="Math Functions">
                <option value="AIRY">AIRY</option>
                <option value="BESSELJ">BESSELJ</option>
                <option value="BESSELY">BESSELY</option>
                <option value="CAUCHY">CAUCHY</option>
                <option value="CUBIC">CUBIC</option>
                <option value="DIRICHLET">DIRICHLET</option>
                <option value="ERF">ERF</option>
                <option value="ERFC">ERFC</option>
                <option value="ERFCINV">ERFCINV</option>
                <option value="ERFINV">ERFINV</option>
                <option value="EXPFALL">EXPFALL</option>
                <option value="EXPRISE">EXPRISE</option>
                <option value="GAUSS">GAUSS</option>
                <option value="HAVERSINE">HAVERSINE</option>
                <option value="LAGUERRE">LAGUERRE</option>
                <option value="LAPLACE">LAPLACE</option>
                <option value="LEGEND">LEGEND</option>
                <option value="LOG">LOG</option>
                <option value="LOGNORMAL">LOGNORMAL</option>
                <option value="LORENTZ">LORENTZ</option>
                <option value="MAXWELL">MAXWELL</option>
                <option value="RAYLEIGH">RAYLEIGH</option>
                <option value="VERSIERA">VERSIERA</option>
                <option value="WEIBULL">WEIBULL</option>
                <option value="X2DATA">X2DATA</option>
                <option value="COSH">COSH</option>
                <option value="COSINT">COSINT</option>
                <option value="COT">COT</option>
                <option value="COTHCON">COTHCON</option>
                <option value="COTHPRO">COTHPRO</option>
                <option value="CSCCON">CSCCON</option>
                <option value="CSCPRO">CSCPRO</option>
                <option value="CSCHCON">CSCHCON</option>
                <option value="CSCHPRO">CSCHPRO</option>
                <option value="RECIPCON">RECIPCON</option>
                <option value="RECIPPRO">RECIPPRO</option>
                <option value="SECCON">SECCON</option>
                <option value="SECPRO">SECPRO</option>
                <option value="SECH">SECH</option>
                <option value="SINC">SINC</option>
                <option value="SINH">SINH</option>
                <option value="SININT">SININT</option>
                <option value="SQRT">SQRT</option>
                <option value="TAN">TAN</option>
                <option value="TANH">TANH</option>
                <option value="ACOS">ACOS</option>
                <option value="ACOSH">ACOSH</option>
                <option value="ACOTCON">ACOTCON</option>
                <option value="ACOTPRO">ACOTPRO</option>
                <option value="ACOTHCON">ACOTHCON</option>
                <option value="ACOTHPRO">ACOTHPRO</option>
                <option value="ACSCCON">ACSCCON</option>
                <option value="ACSCPRO">ACSCPRO</option>
                <option value="ACSCHCON">ACSCHCON</option>
                <option value="ACSCHPRO">ACSCHPRO</option>
                <option value="ASECCON">ASECCON</option>
                <option value="ASECPRO">ASECPRO</option>
                <option value="ASECH">ASECH</option>
                <option value="ASIN">ASIN</option>
                <option value="ASINH">ASINH</option>
                <option value="ATAN">ATAN</option>
                <option value="ATANH">ATANH</option>
              </optgroup>
              <optgroup label="Window Functions">
                <option value="BARLETT">BARLETT</option>
                <option value="BARTHANN">BARTHANN</option>
                <option value="BLACKMAN">BLACKMAN</option>
                <option value="BLACKMANH">BLACKMANH</option>
                <option value="BOHMANWIN">BOHMANWIN</option>
                <option value="BOXCAR">BOXCAR</option>
                <option value="CHEBWIN">CHEBWIN</option>
                <option value="FLATTOPWIN">FLATTOPWIN</option>
                <option value="HAMMING">HAMMING</option>
                <option value="HANNING">HANNING</option>
                <option value="KAISER">KAISER</option>
                <option value="NUTTALLWIN">NUTTALLWIN</option>
                <option value="PARZENWIN">PARZENWIN</option>
                <option value="TAYLORWIN">TAYLORWIN</option>
                <option value="TRIANG">TRIANG</option>
                <option value="TUKEYWIN">TUKEYWIN</option>
              </optgroup>
              <optgroup label="Custom">
                <option value="CUSTOM">CUSTom</option>
              </optgroup>
            </select>
          </span>
          <button class="gen-func-btn" data-func="HARM" onclick="selectGenHarmonic(2)">Harmonic</button>
          <button class="gen-func-btn" data-func="USER" onclick="selectGenUserCh(2)">User*</button>
        </div>
        <div class="gen-field-row" id="gen-ch2-freq-row">
          <span class="gen-field-label">Freq</span>
          <input type="text" class="gen-field-input" id="gen-ch2-freq" onkeydown="if(event.key==='Enter')setGenFreq(2)">
          <span class="gen-field-unit">Hz</span>
          <button class="gen-set-btn" onclick="setGenFreq(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Period</span>
          <input type="text" class="gen-field-input" id="gen-ch2-period" onkeydown="if(event.key==='Enter')setGenPeriodCh(2)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPeriodCh(2)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Ampl</span>
          <input type="text" class="gen-field-input" id="gen-ch2-ampl" onkeydown="if(event.key==='Enter')setGenAmpl(2)">
          <span class="gen-field-unit">Vpp</span>
          <button class="gen-set-btn" onclick="setGenAmpl(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Max</span>
          <input type="text" class="gen-field-input" id="gen-ch2-high" onkeydown="if(event.key==='Enter')setGenHigh(2)">
          <span class="gen-field-unit">V</span>
          <button class="gen-set-btn" onclick="setGenHigh(2)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Offset</span>
          <input type="text" class="gen-field-input" id="gen-ch2-offset" onkeydown="if(event.key==='Enter')setGenOffset(2)">
          <span class="gen-field-unit">Vdc</span>
          <button class="gen-set-btn" onclick="setGenOffset(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Min</span>
          <input type="text" class="gen-field-input" id="gen-ch2-low" onkeydown="if(event.key==='Enter')setGenLow(2)">
          <span class="gen-field-unit">V</span>
          <button class="gen-set-btn" onclick="setGenLow(2)">Set</button>
        </div>
        <div class="gen-field-row">
          <span class="gen-field-label">Imp</span>
          <input type="text" class="gen-field-input" id="gen-ch2-imp" onkeydown="if(event.key==='Enter')setGenImp(2)">
          <span class="gen-field-unit">&Omega;</span>
          <button class="gen-set-btn" onclick="setGenImp(2)">Set</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(2,'MIN')">Min</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(2,'50')">50</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(2,'MAX')">Max</button>
          <button class="gen-quick-btn" onclick="setGenImpKeyword(2,'INF')">HighZ</button>
        </div>
        <div class="gen-field-row" id="gen-ch2-phase-row">
          <span class="gen-field-label">Phase</span>
          <input type="text" class="gen-field-input" id="gen-ch2-phase" onkeydown="if(event.key==='Enter')setGenPhaseCh(2)">
          <span class="gen-field-unit">&deg;</span>
          <button class="gen-set-btn" onclick="setGenPhaseCh(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Sync:</span>
          <button class="gen-toggle-btn" id="gen-ch2-alignphase-btn" onclick="alignPhaseCh(2)" disabled>Align Phase</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch2-squ-dutycycle-row">
          <span class="gen-field-label">Duty</span>
          <input type="text" class="gen-field-input" id="gen-ch2-squ-dutycycle" onkeydown="if(event.key==='Enter')setGenDutyCycleCh(2)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenDutyCycleCh(2)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch2-ramp-symmetry-row">
          <span class="gen-field-label">Symmetry</span>
          <input type="text" class="gen-field-input" id="gen-ch2-ramp-symmetry" onkeydown="if(event.key==='Enter')setGenSymmetryCh(2)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenSymmetryCh(2)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch2-pulse-dutycycle-row">
          <span class="gen-field-label">Duty</span>
          <input type="text" class="gen-field-input" id="gen-ch2-pulse-dutycycle" onkeydown="if(event.key==='Enter')setGenPulseDutyCycleCh(2)">
          <span class="gen-field-unit">%</span>
          <button class="gen-set-btn" onclick="setGenPulseDutyCycleCh(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Width</span>
          <input type="text" class="gen-field-input" id="gen-ch2-pulse-width" onkeydown="if(event.key==='Enter')setGenPulseWidthCh(2)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseWidthCh(2)">Set</button>
        </div>
        <div class="gen-field-row gen-hidden" id="gen-ch2-pulse-edges-row">
          <span class="gen-field-label">Lead</span>
          <input type="text" class="gen-field-input" id="gen-ch2-pulse-leading" onkeydown="if(event.key==='Enter')setGenPulseLeadingCh(2)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseLeadingCh(2)">Set</button>
          <span class="gen-field-label" style="margin-left:12px;">Trail</span>
          <input type="text" class="gen-field-input" id="gen-ch2-pulse-trailing" onkeydown="if(event.key==='Enter')setGenPulseTrailingCh(2)">
          <span class="gen-field-unit">s</span>
          <button class="gen-set-btn" onclick="setGenPulseTrailingCh(2)">Set</button>
        </div>
        <div id="gen-ch2-dynamic-area">
          <div class="gen-field-row gen-mode-row" id="gen-ch2-mode-row">
            <span class="gen-field-label">Mode:</span>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch2-mode-mod" onclick="selectGenMode(2,'mod')">Mod: Off</button>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch2-mode-sweep" onclick="selectGenMode(2,'sweep')">Sweep: Off</button>
            <button class="gen-toggle-btn gen-mode-btn" id="gen-ch2-mode-burst" onclick="selectGenMode(2,'burst')">Burst: Off</button>
          </div>
          <div id="gen-ch2-type-and-modifiers">
            <div class="gen-field-row">
              <span class="gen-field-label">Type:</span>
              <select class="gen-mod-select gen-hidden" id="gen-ch2-mod-typesel" onchange="selectGenModType(2)">
                <option value="AM">AM</option>
                <option value="FM">FM</option>
                <option value="PM">PM</option>
                <option value="ASK">ASK</option>
                <option value="FSK">FSK</option>
                <option value="PSK">PSK</option>
                <option value="PWM">PWM</option>
                <option value="BPSK">BPSK</option>
                <option value="QPSK">QPSK</option>
                <option value="3FSK">3FSK</option>
                <option value="4FSK">4FSK</option>
                <option value="OSK">OSK</option>
              </select>
              <select class="gen-mod-select gen-hidden" id="gen-ch2-sweep-typesel" onchange="setSweepSpacingCh(2)">
                <option value="LINear">Linear</option>
                <option value="LOGarithmic">Log</option>
                <option value="STEp">Step</option>
              </select>
              <span class="gen-hidden" id="gen-ch2-sweep-step-group">
                <span class="gen-field-label" style="margin-left:12px;">Steps</span>
                <input type="text" class="gen-field-input" id="gen-ch2-sweep-step" onkeydown="if(event.key==='Enter')setSweepStepCh(2)">
                <span class="gen-field-unit">#</span>
                <button class="gen-set-btn" onclick="setSweepStepCh(2)">Set</button>
              </span>
              <select class="gen-mod-select gen-hidden" id="gen-ch2-burst-typesel" onchange="setBurstModeCh(2)">
                <option value="TRIGgered">N-Cycle</option>
                <option value="GATed">Gated</option>
                <option value="INFinity">Infinite</option>
              </select>
            </div>
            <div class="gen-modifiers-col">
              <div class="gen-modrows gen-hidden" id="gen-ch2-mod-modrows">
                <div class="gen-hidden" id="gen-ch2-mod-am-rows">
                <div class="gen-field-row">
                  <span class="gen-field-label">Source</span>
                  <select class="gen-mod-select" id="gen-ch2-mod-am-source" onchange="updateModAmSourceGrayingCh(2)">
                    <option value="INTernal">Internal</option>
                    <option value="EXTernal">External</option>
                  </select>
                  <button class="gen-set-btn" onclick="setModAmSourceCh(2)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Mod Freq</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-mod-am-freq" onkeydown="if(event.key==='Enter')setModAmFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" id="gen-ch2-mod-am-freq-set" onclick="setModAmFreqCh(2)">Set</button>
                  <span class="gen-modal-note">Internal source only</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Shape</span>
                  <select class="gen-mod-select" id="gen-ch2-mod-am-shape">
                    <option value="SINusoid">Sine</option>
                    <option value="SQUare">Square</option>
                    <option value="TRIangle">Triangle</option>
                    <option value="RAMP">Ramp</option>
                    <option value="NRAMp">Neg Ramp</option>
                    <option value="NOISe">Noise</option>
                    <option value="USER">User</option>
                  </select>
                  <button class="gen-set-btn" id="gen-ch2-mod-am-shape-set" onclick="setModAmShapeCh(2)">Set</button>
                  <span class="gen-modal-note">Internal source only</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Depth</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-mod-am-depth" onkeydown="if(event.key==='Enter')setModAmDepthCh(2)">
                  <span class="gen-field-unit">%</span>
                  <button class="gen-set-btn" onclick="setModAmDepthCh(2)">Set</button>
                </div>
                <div class="gen-field-row">
                  <button class="gen-toggle-btn" id="gen-ch2-mod-am-dssc-btn" data-on="0" data-label="DSSC" onclick="toggleModAmDsscCh(2)">DSSC: OFF</button>
                  <span class="gen-modal-note">confirmed: :SOUR&lt;n&gt;:AM:DSSC ON|OFF</span>
                </div>
                </div>
                <!-- PM — mirrors AM's shape exactly (Source/Freq/Shape/
                     Deviation instead of Depth). Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch2-mod-pm-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-pm-source" onchange="updateModSourceGrayingCh(2,'pm')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModPmSourceCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Mod Freq</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-pm-freq" onkeydown="if(event.key==='Enter')setModPmFreqCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch2-mod-pm-freq-set" onclick="setModPmFreqCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Shape</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-pm-shape">
                      <option value="SINusoid">Sine</option>
                      <option value="SQUare">Square</option>
                      <option value="TRIangle">Triangle</option>
                      <option value="RAMP">Ramp</option>
                      <option value="NRAMp">Neg Ramp</option>
                      <option value="NOISe">Noise</option>
                      <option value="USER">User</option>
                    </select>
                    <button class="gen-set-btn" id="gen-ch2-mod-pm-shape-set" onclick="setModPmShapeCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Deviation</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-pm-deviation" onkeydown="if(event.key==='Enter')setModPmDeviationCh(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModPmDeviationCh(2)">Set</button>
                  </div>
                </div>
                <!-- ASK — Source/Rate/Amplitude/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch2-mod-ask-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-ask-source" onchange="updateModSourceGrayingCh(2,'ask')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModAskSourceCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-ask-rate" onkeydown="if(event.key==='Enter')setModAskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch2-mod-ask-rate-set" onclick="setModAskRateCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Amplitude</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-ask-ampl" onkeydown="if(event.key==='Enter')setModAskAmplCh(2)">
                    <span class="gen-field-unit">Vpp</span>
                    <button class="gen-set-btn" onclick="setModAskAmplCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-ask-polarity" onchange="setModAskPolarityCh(2)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- FSK — Source/Rate/Hop Freq/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch2-mod-fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-fsk-source" onchange="updateModSourceGrayingCh(2,'fsk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModFskSourceCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-fsk-rate" onkeydown="if(event.key==='Enter')setModFskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch2-mod-fsk-rate-set" onclick="setModFskRateCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-fsk-hopfreq" onkeydown="if(event.key==='Enter')setModFskHopFreqCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModFskHopFreqCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-fsk-polarity" onchange="setModFskPolarityCh(2)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- PSK — Source/Rate/Phase/Polarity. Manual-confirmed. -->
                <div class="gen-hidden" id="gen-ch2-mod-psk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-psk-source" onchange="updateModSourceGrayingCh(2,'psk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModPskSourceCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-psk-rate" onkeydown="if(event.key==='Enter')setModPskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch2-mod-psk-rate-set" onclick="setModPskRateCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-psk-phase" onkeydown="if(event.key==='Enter')setModPskPhaseCh(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModPskPhaseCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Polarity</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-psk-polarity" onchange="setModPskPolarityCh(2)">
                      <option value="POSitive">Positive</option>
                      <option value="NEGative">Negative</option>
                    </select>
                  </div>
                </div>
                <!-- BPSK — no Source/Polarity (manual confirms neither
                     exists for this type); Data replaces them. -->
                <div class="gen-hidden" id="gen-ch2-mod-bpsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-bpsk-rate" onkeydown="if(event.key==='Enter')setModBpskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModBpskRateCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-bpsk-phase" onkeydown="if(event.key==='Enter')setModBpskPhaseCh(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModBpskPhaseCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Data</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-bpsk-data" onchange="setModBpskDataCh(2)">
                      <option value="01">01</option>
                      <option value="10">10</option>
                      <option value="PN15">PN15</option>
                      <option value="PN21">PN21</option>
                    </select>
                  </div>
                </div>
                <!-- QPSK — same shape as BPSK, three phases instead of
                     one, Data limited to PN15/PN21 (no 01/10). -->
                <div class="gen-hidden" id="gen-ch2-mod-qpsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-qpsk-rate" onkeydown="if(event.key==='Enter')setModQpskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setModQpskRateCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-qpsk-phase1" onkeydown="if(event.key==='Enter')setModQpskPhase1Ch(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase1Ch(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-qpsk-phase2" onkeydown="if(event.key==='Enter')setModQpskPhase2Ch(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase2Ch(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Phase 3</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-qpsk-phase3" onkeydown="if(event.key==='Enter')setModQpskPhase3Ch(2)">
                    <span class="gen-field-unit">&deg;</span>
                    <button class="gen-set-btn" onclick="setModQpskPhase3Ch(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Data</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-qpsk-data" onchange="setModQpskDataCh(2)">
                      <option value="PN15">PN15</option>
                      <option value="PN21">PN21</option>
                    </select>
                  </div>
                </div>
                <!-- 3FSK — Rate + 2 additional indexed hop frequencies
                     (n=1,2; the base Freq field is the carrier itself).
                     Manual: ":MOD:3FSKey[:FREQuency] <n>,<freq>". -->
                <div class="gen-hidden" id="gen-ch2-mod-3fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-3fsk-rate" onkeydown="if(event.key==='Enter')setMod3fskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskRateCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-3fsk-freq1" onkeydown="if(event.key==='Enter')setMod3fskFreqCh(2,1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskFreqCh(2,1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-3fsk-freq2" onkeydown="if(event.key==='Enter')setMod3fskFreqCh(2,2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod3fskFreqCh(2,2)">Set</button>
                  </div>
                </div>
                <!-- 4FSK — same idea, n=1,2,3. -->
                <div class="gen-hidden" id="gen-ch2-mod-4fsk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-4fsk-rate" onkeydown="if(event.key==='Enter')setMod4fskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskRateCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 1</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-4fsk-freq1" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(2,1)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(2,1)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 2</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-4fsk-freq2" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(2,2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(2,2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Hop Freq 3</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-4fsk-freq3" onkeydown="if(event.key==='Enter')setMod4fskFreqCh(2,3)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" onclick="setMod4fskFreqCh(2,3)">Set</button>
                  </div>
                </div>
                <!-- OSK — Source/Rate/Time. No Shape/Polarity. -->
                <div class="gen-hidden" id="gen-ch2-mod-osk-rows">
                  <div class="gen-field-row">
                    <span class="gen-field-label">Source</span>
                    <select class="gen-mod-select" id="gen-ch2-mod-osk-source" onchange="updateModSourceGrayingCh(2,'osk')">
                      <option value="INTernal">Internal</option>
                      <option value="EXTernal">External</option>
                    </select>
                    <button class="gen-set-btn" onclick="setModOskSourceCh(2)">Set</button>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Rate</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-osk-rate" onkeydown="if(event.key==='Enter')setModOskRateCh(2)">
                    <span class="gen-field-unit">Hz</span>
                    <button class="gen-set-btn" id="gen-ch2-mod-osk-rate-set" onclick="setModOskRateCh(2)">Set</button>
                    <span class="gen-modal-note">Internal source only</span>
                  </div>
                  <div class="gen-field-row">
                    <span class="gen-field-label">Time</span>
                    <input type="text" class="gen-field-input" id="gen-ch2-mod-osk-time" onkeydown="if(event.key==='Enter')setModOskTimeCh(2)">
                    <span class="gen-field-unit">s</span>
                    <button class="gen-set-btn" onclick="setModOskTimeCh(2)">Set</button>
                    <span class="gen-modal-note">range depends on current Rate</span>
                  </div>
                </div>
              </div>
              <div class="gen-modrows gen-hidden" id="gen-ch2-sweep-modrows">
                <div class="gen-field-row">
                  <span class="gen-field-label">Sweep</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-time" onkeydown="if(event.key==='Enter')setSweepTimeCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepTimeCh(2)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Return</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-rtime" onkeydown="if(event.key==='Enter')setSweepRTimeCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepRTimeCh(2)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Start</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-startfreq" onkeydown="if(event.key==='Enter')setSweepStartFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepStartFreqCh(2)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Center</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-centerfreq" onkeydown="if(event.key==='Enter')setSweepCenterFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepCenterFreqCh(2)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Stop</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-stopfreq" onkeydown="if(event.key==='Enter')setSweepStopFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepStopFreqCh(2)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Span</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-spanfreq" onkeydown="if(event.key==='Enter')setSweepSpanFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepSpanFreqCh(2)">Set</button>
                  <span class="gen-modal-note">swap Start/Stop to reverse direction</span>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Hold Start</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-htimestart" onkeydown="if(event.key==='Enter')setSweepHTimeStartCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepHTimeStartCh(2)">Set</button>
                  <span class="gen-field-label" style="margin-left:12px;">Hold Stop</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-htimestop" onkeydown="if(event.key==='Enter')setSweepHTimeStopCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setSweepHTimeStopCh(2)">Set</button>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Trig Src</span>
                  <select class="gen-mod-select" id="gen-ch2-sweep-trigsrc" onchange="setSweepTrigSrcCh(2)">
                    <option value="INTernal">Internal</option>
                    <option value="EXTernal">External</option>
                    <option value="MANual">Manual</option>
                  </select>
                  <button class="gen-quick-btn gen-hidden" id="gen-ch2-sweep-trignow-btn" onclick="sweepManualTriggerCh(2)">Trigger Now</button>
                </div>
                <div class="gen-field-row">
                  <button class="gen-toggle-btn" id="gen-ch2-sweep-markstate-btn" data-on="0" data-label="Mark" onclick="toggleSweepMarkStateCh(2)">Mark: OFF</button>
                  <input type="text" class="gen-field-input" id="gen-ch2-sweep-markfreq" onkeydown="if(event.key==='Enter')setSweepMarkFreqCh(2)">
                  <span class="gen-field-unit">Hz</span>
                  <button class="gen-set-btn" onclick="setSweepMarkFreqCh(2)">Set</button>
                </div>
              </div>
              <div class="gen-modrows gen-hidden" id="gen-ch2-burst-modrows">
                <div class="gen-field-row" id="gen-ch2-burst-ncycles-row">
                  <span class="gen-field-label">Cycles</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-burst-ncycles" onkeydown="if(event.key==='Enter')setBurstNCyclesCh(2)">
                  <span class="gen-field-unit">cyc</span>
                  <button class="gen-set-btn" onclick="setBurstNCyclesCh(2)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch2-burst-period-row">
                  <span class="gen-field-label">Period</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-burst-period" onkeydown="if(event.key==='Enter')setBurstPeriodCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setBurstPeriodCh(2)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch2-burst-tdelay-row">
                  <span class="gen-field-label">Delay</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-burst-tdelay" onkeydown="if(event.key==='Enter')setBurstTDelayCh(2)">
                  <span class="gen-field-unit">s</span>
                  <button class="gen-set-btn" onclick="setBurstTDelayCh(2)">Set</button>
                </div>
                <div class="gen-field-row gen-hidden" id="gen-ch2-burst-gatepol-row">
                  <span class="gen-field-label">Gate Pol</span>
                  <select class="gen-mod-select" id="gen-ch2-burst-gatepol" onchange="setBurstGatePolCh(2)">
                    <option value="NORMal">Positive</option>
                    <option value="INVerted">Negative</option>
                  </select>
                </div>
                <div class="gen-field-row">
                  <span class="gen-field-label">Phase</span>
                  <input type="text" class="gen-field-input" id="gen-ch2-burst-phase" onkeydown="if(event.key==='Enter')setBurstPhaseCh(2)">
                  <span class="gen-field-unit">&deg;</span>
                  <button class="gen-set-btn" onclick="setBurstPhaseCh(2)">Set</button>
                </div>
                <div class="gen-field-row" id="gen-ch2-burst-trigsrc-row">
                  <span class="gen-field-label">Trig Src</span>
                  <select class="gen-mod-select" id="gen-ch2-burst-trigsrc" onchange="setBurstTrigSrcCh(2)">
                    <option value="INTernal" id="gen-ch2-burst-trigsrc-int-opt">Internal</option>
                    <option value="EXTernal">External</option>
                    <option value="MANual">Manual</option>
                  </select>
                  <button class="gen-quick-btn gen-hidden" id="gen-ch2-burst-trignow-btn" onclick="burstManualTriggerCh(2)">Trigger Now</button>
                  <span class="gen-hidden" id="gen-ch2-burst-trigout-group">
                    <span class="gen-field-label" style="margin-left:12px;">TrigOut</span>
                    <select class="gen-mod-select" id="gen-ch2-burst-trigout" onchange="setBurstTrigOutCh(2)">
                      <option value="OFF">Off</option>
                      <option value="POSitive">Leading</option>
                      <option value="NEGative">Trailing</option>
                    </select>
                  </span>
                  <span class="gen-hidden" id="gen-ch2-burst-slopein-group">
                    <span class="gen-field-label" style="margin-left:12px;">SlopeIn</span>
                    <select class="gen-mod-select" id="gen-ch2-burst-slopein" onchange="setBurstSlopeInCh(2)">
                      <option value="POSitive">Leading</option>
                      <option value="NEGative">Trailing</option>
                    </select>
                  </span>
                </div>
              </div>
            </div>
          </div>
          <div class="gen-modrows gen-hidden" id="gen-ch2-harmonic-mock">
            <div class="gen-harm-title">Harmonics parameters</div>
            <div class="gen-field-row">
              <span class="gen-field-label">Harmonics:</span>
              <button class="gen-harmtype-btn" data-harmtype="EVEN" onclick="selectHarmTypeCh(2,'EVEN')">EVEN</button>
              <button class="gen-harmtype-btn" data-harmtype="ODD" onclick="selectHarmTypeCh(2,'ODD')">ODD</button>
              <button class="gen-harmtype-btn" data-harmtype="ALL" onclick="selectHarmTypeCh(2,'ALL')">ALL</button>
              <button class="gen-harmtype-btn" data-harmtype="USER" onclick="selectHarmTypeCh(2,'USER')">USER SELECTED</button>
            </div>
            <div class="gen-harm-title">Harmonic Values:</div>
            <div class="gen-field-row">
              <select class="gen-mod-select" id="gen-ch2-harm-index" onchange="selectHarmIndexCh(2)">
                <option value="2">2</option><option value="3">3</option><option value="4">4</option><option value="5">5</option><option value="6">6</option><option value="7">7</option><option value="8">8</option><option value="9">9</option><option value="10">10</option><option value="11">11</option><option value="12">12</option><option value="13">13</option><option value="14">14</option><option value="15">15</option><option value="16">16</option>
              </select>
              <span class="gen-field-label">Ampl:</span>
              <input type="text" class="gen-field-input" id="gen-ch2-harm-ampl" onkeydown="if(event.key==='Enter')setHarmAmplPhaseCh(2)">
              <span class="gen-field-unit">Vpp</span>
              <span class="gen-field-label">Phase:</span>
              <input type="text" class="gen-field-input" id="gen-ch2-harm-phaseoffset" onkeydown="if(event.key==='Enter')setHarmAmplPhaseCh(2)">
              <span class="gen-field-unit">deg</span>
              <button class="gen-set-btn" onclick="setHarmAmplPhaseCh(2)">Set</button>
            </div>
          </div>
        </div>
      </div>

    </div>
  </div>

  <!-- ── Counter tab (BK1823A) ──
       Confirmed bench-working. NOT SCPI — the BK1823A uses short
       single-letter+digit command tokens (R/F/G/H/D), confirmed both at
       the bench and against the manufacturer's manual. See main.cpp's
       file header for the full confirmed command table and the model-
       variant identification (this unit is the "1.5(3.0)GHz, U/C"
       variant — that's why F5 has no function and RPM never appears).
       Kept intentionally simpler than the DMM panel for this pass — no
       Data Logger, no Halt/Resume — per explicit scope decision. -->
  <div class="tab-panel" id="tab-counter">
    <div id="counter-reading-row">
      <span id="counter-reading">---</span>
    </div>
    <div class="ctr-row">
      <span class="ctr-label">FUNCTION:</span>
      <button class="ctr-btn active" data-func="0" onclick="selectCounterFunc(0)">Freq A</button>
      <button class="ctr-btn" data-func="1" onclick="selectCounterFunc(1)">Freq B</button>
      <button class="ctr-btn" data-func="2" onclick="selectCounterFunc(2)">Freq C</button>
      <button class="ctr-btn" data-func="3" onclick="selectCounterFunc(3)">Period</button>
      <button class="ctr-btn" data-func="4" onclick="selectCounterFunc(4)">Total</button>
      <!-- F5 deliberately omitted — confirmed unused/NC on this model
           variant, both at the bench and per the manual's table. -->
      <button class="ctr-btn" data-func="6" onclick="selectCounterFunc(6)">Ratio A/B</button>
      <button class="ctr-btn" data-func="7" onclick="selectCounterFunc(7)">Time Int A&rarr;B</button>
    </div>
    <div class="ctr-row">
      <span class="ctr-label">GATE:</span>
      <button class="ctr-btn" data-gate="0" onclick="selectCounterGate(0)">0.01s</button>
      <button class="ctr-btn" data-gate="1" onclick="selectCounterGate(1)">0.1s</button>
      <button class="ctr-btn active" data-gate="2" onclick="selectCounterGate(2)">1s</button>
      <button class="ctr-btn" data-gate="3" onclick="selectCounterGate(3)">10s</button>
    </div>
    <div class="ctr-row">
      <button id="counter-hold-btn" class="ctr-toggle-btn" onclick="toggleCounterHold()">Hold: OFF</button>
      <button id="counter-autofetch-btn" class="ctr-toggle-btn" onclick="toggleCounterAutoFetch()">Auto-Fetch: OFF</button>
    </div>
  </div>

  <!-- ── TOOLS tab — shared manual terminal for all three instruments ──
       Replaces the old single-instrument floating #terminal. Every
       command the app sends or receives, from any instrument or
       mechanism (DMM's Auto-Fetch/Multi-Function, firmware sys/debug
       events, or manual entry here), gets logged here — see appLog()
       in JS. This makes the TOOLS tab double as the whole app's activity
       log, not just a place to type commands. -->
  <div class="tab-panel" id="tab-scpi">
    <div id="scpi-target-row">
      <span id="scpi-target-label">TARGET:</span>
      <button class="scpi-target-btn active" id="scpi-target-dmm" onclick="setScpiTarget('dmm')">DMM</button>
      <button class="scpi-target-btn" id="scpi-target-funcgen" onclick="setScpiTarget('funcgen')">Func Gen</button>
      <button class="scpi-target-btn" id="scpi-target-counter" onclick="setScpiTarget('counter')">Counter</button>
    </div>
    <div id="scpi-term-log"></div>
    <div id="scpi-term-input-row">
      <!-- Auto-halt on focus/blur only applies when target is "dmm" —
           see haltChannel()/resumeChannel() calls in setScpiTarget()'s
           focus handler in JS; halting has no meaning for counter/
           funcgen, which have no channelBusy concept yet. -->
      <input id="scpi-term-input" type="text" placeholder="SCPI command...">
      <button id="scpi-term-send" onclick="scpiSend()">Send</button>
      <button id="scpi-term-clear" onclick="scpiClear()">Clear</button>
    </div>
  </div>

  </div><!-- /#app-panels -->


<script>
  // ═══════════════════════════════════════════════════════════════════════
  // Shared app-level layer — tabs, WebSocket transport, unified activity
  // log. Used by all three instrument panels plus the TOOLS tab itself.
  // Per-instrument logic (DMM panel, and eventually Counter/FuncGen
  // panels) lives further down and calls into wsSend()/appLog() here
  // rather than touching the WebSocket directly.
  // ═══════════════════════════════════════════════════════════════════════

  // ── Tabs ──────────────────────────────────────────────────────────────────
  // Tracks whichever tab was active immediately before the current one,
  // so switching INTO the TOOLS tab specifically can auto-select the
  // matching instrument as its target — see showTab() below. Matches
  // the default active tab in the HTML (tabbtn-dmm has class="active"
  // by default).
  let currentTabId = 'dmm';
  const TAB_TO_SCPI_TARGET = { dmm: 'dmm', counter: 'counter', funcgen: 'funcgen' };
  // 'scpi' intentionally has no mapping — switching from SCPI to SCPI
  // isn't a real case, and this keeps a manual target choice made while
  // already on the TOOLS tab from being overridden by anything odd.

  function showTab(tabId) {
    const previousTabId = currentTabId;
    document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    const panel = document.getElementById('tab-' + tabId);
    const btn   = document.getElementById('tabbtn-' + tabId);
    if (panel) panel.classList.add('active');
    if (btn) btn.classList.add('active');
    currentTabId = tabId;
    if (tabId === 'scpi' && TAB_TO_SCPI_TARGET[previousTabId]) {
      setScpiTarget(TAB_TO_SCPI_TARGET[previousTabId]);
    }
  }

  // ── Unified activity log (the TOOLS tab's terminal) ──────────────────────
  // Every command sent and every response/error/sys event received, from
  // any instrument or mechanism (DMM's Auto-Fetch/Multi-Function, firmware
  // boot/debug sys events, or manual entry in the TOOLS tab), lands here —
  // this is now the single terminal for the whole app, not just a place to
  // type commands. target is 'dmm'/'counter'/'funcgen', or null/undefined
  // for firmware-level sys events that aren't tied to one instrument.
  const TARGET_TAGS = { dmm: '[DMM]', counter: '[CTR]', funcgen: '[GEN]' };
  function appLog(target, text, cls) {
    const log = document.getElementById('scpi-term-log');
    if (!log) return;
    const d = document.createElement('div');
    const tag = target ? (TARGET_TAGS[target] || ('[' + String(target).toUpperCase() + ']')) : '';
    d.textContent = tag ? (tag + ' ' + text) : text;
    if (cls) d.className = cls;
    log.appendChild(d);
    log.scrollTop = log.scrollHeight;
  }

  // ── Instrument liveness / connection status ─────────────────────────────
  // Browser<->ESP is the WebSocket itself (ws-status — instant, via
  // onopen/onclose). The three instrument links (ESP<->DMM/Counter/
  // FuncGen) are relayed over RS-232/USB by a firmware that has no
  // visibility into whether anything is actually plugged in on the far
  // end for DMM/Counter (a deliberate "dumb pipe" — see main.cpp) — so
  // "connected" here specifically means "responded to a query
  // recently," not "cable is plugged in."
  //
  // Redesigned (v0.5) around a tight per-probe timeout instead of the
  // original long passive/active dual-threshold scheme — each probe now
  // gets its own short timeout (LIVENESS_TIMEOUT_MS), and a single
  // failed probe is enough to declare "not responding" immediately,
  // logging a "(timeout)" line and a friendly "<model> disconnected or
  // off?" line using the instrument's actual model name. Also: DMM/
  // Counter/Func Gen's own "full init on connect" routines (VOLT:DC
  // selection, R1/F0/G2/H0, the big multi-query refresh) now ONLY run
  // once a transition-to-alive is actually observed — not blindly on
  // every connect — since firing all that traffic at a target already
  // known to be absent was the original motivating bug (first found
  // with Func Gen's 34-query burst, then confirmed the same pattern
  // applied to DMM/Counter's smaller bursts too).
  const LIVENESS_TIMEOUT_MS = 3000;   // per-probe wait, matches this
                                        // app's existing DMM/Counter
                                        // query-timeout convention
  const LIVENESS_RETRY_MS   = 3000;   // pause between cycles while dead/unknown
  const LIVENESS_IDLE_MS    = 10000;  // pause between probes once alive —
                                        // don't hammer a working link

  const INSTRUMENT_NAMES = { dmm: 'BK5491B', counter: 'BK1823A', funcgen: 'DG4062' };

  let linkAlive       = { dmm: null, counter: null, funcgen: null };  // null = unknown yet
  let linkPending      = { dmm: false, counter: false, funcgen: false };  // probe in flight?
  let linkTimer        = { dmm: null, counter: null, funcgen: null };     // pending probe's timeout handle
  let linkLastAttempt  = { dmm: 0, counter: 0, funcgen: 0 };
  // Stopgap per explicit request: once a device has been probed once
  // this connection (success or failure), stop retrying it — the
  // repeated "not responding" lines from continuous re-probing were
  // making the SCPI screen hard to read. This is deliberately blunt;
  // a more thoughtful redesign of the whole liveness sequence (and how
  // the Tools tab presents it) is explicitly deferred to a later
  // request, not attempted here. Func Gen's event-based detection (USB
  // attach/detach — see the 'sys' message handling in ws.onmessage)
  // is a separate code path and still works regardless of this flag;
  // only the periodic polling in livenessTick() is affected.
  let hasProbedOnce = { dmm: false, counter: false, funcgen: false };

  function updateLinkStatusUI(target) {
    const el = document.getElementById('link-' + target);
    if (!el) return;
    const label = { dmm: 'DMM', counter: 'CTR', funcgen: 'GEN' }[target];
    if (linkAlive[target] === true) {
      el.textContent = label + ': OK';
      el.className = 'link-status link-ok';
    } else if (linkAlive[target] === false) {
      el.textContent = label + ': --';
      el.className = 'link-status link-dead';
    } else {
      el.textContent = label + ': ?';
      el.className = 'link-status';
    }
  }

  function clearLivenessPending(target) {
    if (linkTimer[target]) { clearTimeout(linkTimer[target]); linkTimer[target] = null; }
    linkPending[target] = false;
  }

  function markLinkAlive(target) {
    // Any successful response is fresh proof of life, not just an
    // explicit probe's own reply — reset the "when did we last hear
    // from this target" clock here so livenessTick() doesn't also
    // schedule a redundant background check while a real refresh burst
    // is still actively producing responses. Confirmed at the bench
    // that responses can take roughly 1 second apart, so a 26+-query
    // refresh can run well past LIVENESS_IDLE_MS (10s) — without this,
    // the periodic tick could fire an unrelated liveness probe *in the
    // middle* of an in-progress burst, injecting an extra query into
    // the same FIFO queue mid-stream. By explicit request: no reason to
    // run the aliveness test while we're already in the middle of
    // getting real data back.
    linkLastAttempt[target] = Date.now();
    const wasAlive = linkAlive[target];
    linkAlive[target] = true;
    updateLinkStatusUI(target);
    if (wasAlive !== true) {
      appLog(null, '-- ' + target.toUpperCase() + ' link: connected --', 'log-sys');
      // A transition INTO alive is exactly when it's worth running each
      // instrument's own "sync UI to real state" full-init routine —
      // deliberately NOT run blindly on every connect anymore (see
      // file-level note above).
      if (target === 'funcgen') {
        refreshBothGenChannels();
        // Note: this alone doesn't cover a mode's own sub-parameters
        // (AM's Source/Freq/Shape/Depth, Sweep's/Burst's fields) — but
        // no need to force-fetch those here anymore. Now that Type
        // row/modifier visibility strictly follows real on/off state
        // (see updateGenModeVisibility), setGenModeButtonDisplay()
        // already fetches a mode's fields itself the moment the real
        // 'mod'/'sweep-state'/'burst-state' query response (queued
        // above) confirms it's actually on — correctly showing nothing
        // if none of them are.
      } else if (target === 'dmm') {
        dmmFullInit();
      } else if (target === 'counter') {
        counterFullInit();
      }
    }
  }

  function markLinkDead(target) {
    const wasAlive = linkAlive[target];
    linkAlive[target] = false;
    updateLinkStatusUI(target);
    if (wasAlive !== false) {
      appLog(null, '-- ' + target.toUpperCase() + ' link: not responding --', 'log-sys');
    }
  }

  function resetLinkStatuses() {
    ['dmm', 'counter', 'funcgen'].forEach(t => {
      linkAlive[t] = null;
      clearLivenessPending(t);
      linkLastAttempt[t] = 0;
      hasProbedOnce[t] = false;
      updateLinkStatusUI(t);
    });
  }

  function onLivenessTimeout(target) {
    linkPending[target] = false;
    linkTimer[target] = null;
    appLog(target, '(timeout)', 'log-none');
    markLinkDead(target);
    appLog(target, '\u26a0 ' + INSTRUMENT_NAMES[target] + ' disconnected or off?', 'log-err');
  }

  function sendLivenessProbe(target) {
    if (linkPending[target]) return;  // already waiting on one
    if (target === 'dmm') {
      if (channelBusy()) return;  // don't collide with real in-flight DMM traffic
      linkPending.dmm = true;
      dmmSend('*IDN?');
    } else if (target === 'counter') {
      if (counterAwaitingRead) return;  // don't collide with Auto-Fetch
      linkPending.counter = true;
      counterSend('D1');  // parameter "don't care" per the manual
    } else if (target === 'funcgen') {
      linkPending.funcgen = true;
      // The raw "*IDN?" already logs itself via wsSend()'s own chokepoint
      // — this adds an explicit, visually distinct marker so it's never
      // ambiguous in the log which *IDN? is a background liveness check
      // versus something else. By explicit request, after real
      // confusion at the bench trying to tell them apart after the
      // fact.
      appLog('funcgen', '(liveness check)', 'log-sys');
      // Routed through the same FIFO queue as every other Func Gen
      // query (not a raw wsSend) — bypassing that queue is exactly the
      // response-ordering bug this project spent two sessions fixing;
      // see genSend()'s own comment. ch=0 is a harmless placeholder —
      // the 'liveness' field tag doesn't touch any channel-specific UI.
      queueGenQuery(0, 'liveness', '*IDN?');
    }
    linkTimer[target] = setTimeout(() => onLivenessTimeout(target), LIVENESS_TIMEOUT_MS);
  }

  function livenessTick() {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;  // browser<->ESP
                                                             // itself down;
                                                             // nothing to probe
    const now = Date.now();
    ['dmm', 'counter', 'funcgen'].forEach(target => {
      // Paused while the physical front panel is in use — see
      // enterGenLocalMode(). Nothing else auto-fires GEN traffic (no
      // Auto-Fetch equivalent for Func Gen), so this plus setGenBusy(true)
      // disabling every button together fully halt all GEN commands
      // while the Local mode popup is open.
      if (target === 'funcgen' && inGenLocalMode) return;
      // Func Gen: only actively probed until first confirmed alive.
      // After that, USB attach/detach are real hardware events already
      // reported via 'sys' messages (see the ws.onmessage handler
      // above) — continuing to poll every ~10s afterward only added a
      // repeating disable/enable flicker (setGenBusy toggling on every
      // probe) with no real benefit, and was occasionally eating
      // clicks that landed during that window. DMM/Counter have no such
      // event to listen for (pure RS-232 relays — the firmware
      // genuinely cannot tell if anything's plugged in), so they still
      // need to poll. One accepted gap either way: a Func Gen that's
      // still physically attached but has internally hung wouldn't be
      // caught by either approach, since no USB detach event would fire.
      if (target === 'funcgen' && linkAlive.funcgen === true) return;
      if (linkPending[target]) return;  // mid-probe already; its own
                                          // timeout will resolve it
      if (hasProbedOnce[target]) return;  // one check per connect — see
                                            // the flag's own comment
      const interval = (linkAlive[target] === true) ? LIVENESS_IDLE_MS : LIVENESS_RETRY_MS;
      if (now - linkLastAttempt[target] >= interval) {
        linkLastAttempt[target] = now;
        hasProbedOnce[target] = true;
        sendLivenessProbe(target);
      }
    });
  }
  setInterval(livenessTick, 1000);

  // ── WebSocket ────────────────────────────────────────────────────────────
  // Single shared connection for the whole app. Every outgoing message now
  // carries a target — see the firmware's onWsEvent() for the matching
  // JSON envelope. This is a breaking protocol change from the original
  // single-instrument project (which sent raw SCPI text with no envelope
  // at all); see MultiInstrument_Panel.ino's file header for why.
  let ws;
  let reconnTimer = null;

  // Generation guard — HYPOTHESIS-DRIVEN, not a confirmed root-cause fix.
  // Prompted by a real "[GEN] queue full, command dropped" repeating
  // after an ESP32 hardware restart (with nothing else obviously wrong
  // — the browser tab was never reloaded). Best working theory: the
  // ESP's own network stack coming up after a restart can cause the
  // underlying TCP connection to flap/reconnect more than once in quick
  // succession before settling; if that produced more than one
  // WebSocket object with live callbacks for a brief window, each would
  // independently run its own connect-time logic and liveness probing
  // against the SAME firmware-side queue, plausibly explaining a fast
  // pile-up. This guard makes that class of bug impossible regardless
  // of whether this specific theory is exactly right: each socket
  // captures its own generation number at creation, and every one of
  // its callbacks checks that generation is still current before doing
  // anything — a superseded socket's late-arriving events become no-ops
  // instead of acting twice. Worth watching whether "queue full"
  // recurs after this; if it does, the cause is something else.
  let wsGeneration = 0;

  function connect() {
    if (reconnTimer) { clearTimeout(reconnTimer); reconnTimer = null; }
    wsGeneration++;
    const myGen = wsGeneration;
    ws = new WebSocket('ws://' + location.hostname + '/ws');

    ws.onopen = () => {
      if (myGen !== wsGeneration) return;  // superseded — ignore
      setWsStatus(true);
      appLog(null, '-- connected --', 'log-sys');
      resetLinkStatuses();  // instrument links are unknown again until
                             // something actually responds
      onDmmConnect();      // DMM-specific connect behavior — see DMM section below
      onCounterConnect();  // Counter-specific — see Counter section below
      onGenConnect();      // Func Gen-specific — see Func Gen section below
    };

    ws.onclose = () => {
      if (myGen !== wsGeneration) return;  // superseded — ignore
      setWsStatus(false);
      appLog(null, '-- disconnected, reconnecting --', 'log-sys');
      resetLinkStatuses();  // can't know instrument state without the WS link
      onDmmDisconnect();      // DMM-specific — see DMM section below
      onCounterDisconnect();  // Counter-specific — see Counter section below
      onGenDisconnect();      // Func Gen-specific — see Func Gen section below
      reconnTimer = setTimeout(connect, 2000);
    };

    ws.onerror = () => {
      if (myGen !== wsGeneration) return;
      appLog(null, '-- error --', 'log-sys');
    };

    ws.onmessage = e => {
      if (myGen !== wsGeneration) return;  // superseded — ignore
      let o;
      try { o = JSON.parse(e.data); }
      catch (_) { appLog(null, e.data, 'log-resp'); return; }

      if (o.event === 'sys') {
        appLog(null, o.msg, 'log-sys');
        // Func Gen is USB, not RS-232 — the firmware already knows the
        // instant a device is attached/detached (real hardware events,
        // already broadcast as these exact sys messages — see
        // usbClientEventCb() in main.cpp) and reports it here well
        // before any poll would notice. Listening to these directly
        // is what lets livenessTick() stop periodically re-probing
        // Func Gen once it's confirmed alive — see that function's own
        // comment for why the periodic version was a real problem
        // (visible button disable/enable flicker on a ~10s cycle,
        // occasionally eating a click).
        if (/DG4062 TMC interface claimed/.test(o.msg)) {
          clearLivenessPending('funcgen');
          markLinkAlive('funcgen');
        } else if (/Function gen disconnected/.test(o.msg)) {
          clearLivenessPending('funcgen');
          markLinkDead('funcgen');
        }
        return;
      }
      if (o.event === 'resp' || o.event === 'err') {
        const isErr = o.event === 'err';
        // A stray *IDN? reply — from an old browser tab's leftover
        // probe, a timed-out one that got answered late anyway,
        // whatever the source — is real and confirmed to happen: the
        // firmware broadcasts every Func Gen response to every
        // connected client (ws.textAll()), with no concept of which
        // browser session actually asked the question. "Rigol
        // Technologies..." is a completely distinctive, unmistakable
        // string — nothing else this app ever queries could produce
        // it — so it's a safe, reliable signature to catch on
        // specifically, rather than the more invasive fix (tracking
        // which client asked each question, only answering that one)
        // that would need real firmware changes. Only treated as
        // "stray" if we're not actually expecting an IDN-type reply
        // right now (i.e. a real liveness probe genuinely is at the
        // front of the queue) — in that case it's the real thing and
        // falls through to normal handling below.
        if (o.target === 'funcgen' && !isErr && /^Rigol/.test(o.msg.trim())) {
          const expectingLiveness = genQueryQueue.length > 0 && genQueryQueue[0].field === 'liveness';
          if (!expectingLiveness) {
            appLog('funcgen', '(stray IDN reply ignored — not currently expecting one)', 'log-sys');
            markLinkAlive('funcgen');  // still genuine proof of life —
                                         // credit it, just don't apply
                                         // it to whatever's actually
                                         // queued
            return;
          }
        }
        // The actual fix: verify the firmware's echoed seq against
        // whatever this browser session currently believes is at the
        // front of its own FIFO queue. A mismatch means the ordering
        // assumption genQueryQueue's whole design depends on has
        // broken — for whatever reason, on this occasion — and inside
        // this Func Gen dispatch is now unsound. This detects that
        // instantly and unambiguously (the debugging pain every
        // earlier fix in this project's history was reverse-engineered
        // from) instead of the response getting silently misapplied to
        // whatever's at the front of the queue regardless. Detection
        // only, deliberately — this does NOT attempt to resync the
        // queue or recover automatically; see genQueryQueue's own
        // comment for why that's a separate, harder problem.
        if (o.target === 'funcgen' && o.seq !== undefined) {
          const expected = genQueryQueue.length > 0 ? genQueryQueue[0].seq : null;
          if (expected !== null && o.seq !== expected) {
            appLog('funcgen', '\u26a0 SEQ MISMATCH: expected reply to #' + expected +
              ', got #' + o.seq + ' for "' + o.msg + '" — response desync detected', 'log-err');
          }
        }
        // Func Gen's firmware already knows definitively whether a
        // device is present (no need to wait out a probe timeout for
        // this specific, unambiguous case) — suppress its raw error
        // text in favor of one clean line matching DMM/Counter's own
        // "disconnected or off?" wording, instead of showing both.
        const suppressRaw = isErr && o.target === 'funcgen' && /No DG4062 connected/.test(o.msg);
        if (!suppressRaw) {
          appLog(o.target, (isErr ? '  \u26a0 ' : '  \u2192 ') + o.msg, isErr ? 'log-err' : 'log-resp');
        }
        // Liveness: any successful response proves that link is up,
        // regardless of which (if any) other consumer below also acts
        // on it — also clears a pending probe's own timeout, whether or
        // not this specific message was actually answering the probe
        // (any traffic is equally valid proof of life).
        if (!isErr && (o.target === 'dmm' || o.target === 'counter' || o.target === 'funcgen')) {
          clearLivenessPending(o.target);
          markLinkAlive(o.target);
        } else if (suppressRaw) {
          clearLivenessPending('funcgen');
          markLinkDead('funcgen');
          appLog('funcgen', '\u26a0 ' + INSTRUMENT_NAMES.funcgen + ' disconnected or off?', 'log-err');
        }
        // Per-instrument response handling — all three panels now have
        // real logic wired up.
        if (o.target === 'dmm' && !isErr) {
          if (arbiterHandleIncoming(o.msg)) return;
          handleRespLegacy(o.msg);
        } else if (o.target === 'counter' && !isErr) {
          handleCounterResponse(o.msg);
        } else if (o.target === 'funcgen') {
          if (isErr) handleFuncgenError();
          else handleFuncgenResponse(o.msg);
        }
      }
    };
  }

  function wsSend(target, cmd, seq) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;
    appLog(target, cmd, 'log-sent');  // single chokepoint — every outgoing
                                        // command logs itself, callers don't
                                        // need to
    const payload = { target: target, cmd: cmd };
    if (seq !== undefined) payload.seq = seq;
    ws.send(JSON.stringify(payload));
  }

  function setWsStatus(c) {
    const el = document.getElementById('ws-status');
    el.textContent = c ? 'CONNECTED TO WIFI' : 'DISCONNECTED';
    el.className = c ? 'connected' : '';
  }

  // ── TOOLS tab controls ─────────────────────────────────────────────────────
  let scpiTarget = 'dmm';
  function setScpiTarget(t) {
    scpiTarget = t;
    document.querySelectorAll('.scpi-target-btn').forEach(b => b.classList.remove('active'));
    const btn = document.getElementById('scpi-target-' + t);
    if (btn) btn.classList.add('active');
  }
  function scpiSend() {
    const inp = document.getElementById('scpi-term-input');
    const cmd = inp.value.trim();
    if (!cmd) return;
    if (scpiTarget === 'funcgen') {
      // Real, confirmed bug — this was a plain wsSend() with no seq,
      // completely bypassing genQueryQueue. Found via tonight's own
      // diagnostics: every "stray" Func Gen response chased this whole
      // session (client ID always the same, seq always -1) traced back
      // to commands sent through THIS path, not to multiple browser
      // tabs or firmware timing as earlier theories assumed. Routing
      // through the same tracked queue as every other Func Gen send —
      // field tag 'scpi-manual' has no specific UI element to update
      // (the command could be anything), the raw response already logs
      // itself via the shared dispatcher regardless.
      queueGenQuery(0, 'scpi-manual', cmd);
    } else {
      wsSend(scpiTarget, cmd);
    }
    inp.value = '';
    inp.focus();
  }
  function scpiClear() { document.getElementById('scpi-term-log').innerHTML = ''; }
  document.getElementById('scpi-term-input').addEventListener('keydown', e => {
    if (e.key === 'Enter') scpiSend();
  });
  // Auto-halt on focus/blur, same behavior as the old private DMM
  // terminal — but conditional now, since halting only means something
  // when the TOOLS tab's selected target is the DMM (Counter/FuncGen have
  // no channelBusy concept yet).
  document.getElementById('scpi-term-input').addEventListener('focus', () => {
    if (scpiTarget === 'dmm') haltChannel();
  });
  document.getElementById('scpi-term-input').addEventListener('blur', () => {
    if (scpiTarget === 'dmm') resumeChannel();
  });

  // ═══════════════════════════════════════════════════════════════════════
  // Func Gen panel (Rigol DG4062) — real SCPI instrument, dual-channel.
  // Command set confirmed at the bench; see main.cpp's file header.
  //
  // Interaction model is deliberately different from DMM/Counter:
  // - Output ON/OFF ONLY EVER changes from an explicit click here — never
  //   forced on connect/tab-open/channel-switch. Unlike the DMM/Counter
  //   (read-mostly instruments), this one actively drives a real circuit;
  //   surprising its output state could disrupt whatever it's connected
  //   to. This was an explicit decision, not an oversight.
  // - Function/Freq/Amplitude/Offset/Impedance/Modulation are editable
  //   fields, not continuously polled — they only change when someone's
  //   actively adjusting them (here or at the bench), so constant
  //   polling would just be unnecessary USB-TMC traffic on a link that's
  //   already the slowest of the three (see main.cpp's file header on
  //   why Func Gen commands are synchronous/blocking). Instead: query
  //   once on connect, and again on demand via the Refresh button.
  // - Modulation sub-parameters (MFreq/FMDev/depth/etc.) are explicitly
  //   out of scope for this pass — only ON/OFF + Type selection. Use the
  //   TOOLS tab (target Func Gen) for anything deeper.
  //
  // Refresh mechanism: fires up to 8 queries per channel in a burst
  // without waiting for each individually. This relies on the firmware
  // processing Func Gen commands fully synchronously, one at a time (see
  // main.cpp) — responses are therefore guaranteed to arrive back in the
  // same order the queries were sent, so a simple FIFO queue (genQueryQueue)
  // correctly matches each response to the field that asked for it,
  // without needing a full request/response-tagged arbiter like the
  // DMM's. A timeout guards against a query that never gets a response
  // (e.g. a transport error) permanently jamming the queue.
  // ═══════════════════════════════════════════════════════════════════════

  // Full built-in FUNC:SHAPe keyword list (confirmed via the manual),
  // used to recognize an Arb-family waveform in a passive 'func' query
  // response — see that case in applyGenField(). 'HARM'/'HARMONIC' and
  // 'USER' are deliberately NOT in this set; they're handled by their
  // own dedicated logic (selectGenHarmonic/selectGenUserCh) instead.
  const GEN_ARB_NAMES = new Set(['ABSSINE', 'ABSSINEHALF', 'ACOS', 'ACOSH', 'ACOTCON', 'ACOTHCON', 'ACOTHPRO', 'ACOTPRO', 'ACSCCON', 'ACSCHCON', 'ACSCHPRO', 'ACSCPRO', 'AIRY', 'AMPALT', 'ASECCON', 'ASECH', 'ASECPRO', 'ASIN', 'ASINH', 'ATAN', 'ATANH', 'ATTALT', 'BANDLIMITED', 'BARLETT', 'BARTHANN', 'BESSELJ', 'BESSELY', 'BLACKMAN', 'BLACKMANH', 'BOHMANWIN', 'BOXCAR', 'BUTTERWORTH', 'CARDIAC', 'CAUCHY', 'CHEBWIN', 'CHEBYSHEV1', 'CHEBYSHEV2', 'COMBIN', 'COSH', 'COSINT', 'COT', 'COTHCON', 'COTHPRO', 'CPULSE', 'CSCCON', 'CSCHCON', 'CSCHPRO', 'CSCPRO', 'CUBIC', 'CUSTOM', 'CWPULSE', 'DAMPEDOSC', 'DC', 'DIRICHLET', 'DUALTONE', 'EEG', 'EMG', 'EOG', 'ERF', 'ERFC', 'ERFCINV', 'ERFINV', 'EXPFALL', 'EXPRISE', 'FLATTOPWIN', 'GAMMA', 'GATEVIBR', 'GAUSS', 'GAUSSPULSE', 'HAMMING', 'HANNING', 'HAVERSINE', 'IGNITION', 'ISO167502SP', 'ISO167502VR', 'ISO76372TP1', 'ISO76372TP2A', 'ISO76372TP2B', 'ISO76372TP3A', 'ISO76372TP3B', 'ISO76372TP4', 'ISO76372TP5A', 'ISO76372TP5B', 'KAISER', 'LAGUERRE', 'LAPLACE', 'LEGEND', 'LFMPULSE', 'LFPULSE', 'LOG', 'LOGNORMAL', 'LORENTZ', 'MAXWELL', 'MCNOSIE', 'NEGRAMP', 'NIMHDISCHARGE', 'NPULSE', 'NUTTALLWIN', 'PAHCUR', 'PARZENWIN', 'PPULSE', 'PULSILOGRAM', 'QUAKE', 'RADAR', 'RAYLEIGH', 'RECIPCON', 'RECIPPRO', 'RESSPEED', 'RIPPLE', 'ROUNDHALF', 'ROUNDPM', 'SCR', 'SECCON', 'SECH', 'SECPRO', 'SINC', 'SINETRA', 'SINEVER', 'SINH', 'SININT', 'SQRT', 'STAIRDN', 'STAIRUD', 'STAIRUP', 'STEPRESP', 'SURGE', 'SWINGOSC', 'TAN', 'TANH', 'TAYLORWIN', 'TENS1', 'TENS2', 'TENS3', 'THREEAM', 'THREEFM', 'THREEPFM', 'THREEPM', 'THREEPWM', 'TRAPEZIA', 'TRIANG', 'TUKEYWIN', 'TV', 'VERSIERA', 'VOICE', 'WEIBULL', 'X2DATA']);

  let genOutputState = { 1: false, 2: false };
  // Populated by initGenSetButtonGating() — lets setGenBusy() re-enable
  // whichever Set button belongs to the currently-focused field, if
  // any, once a request finishes. See setGenBusy()'s own comment for
  // why this exists.
  const genFieldToSetBtn = new Map();
  // Halts all Func Gen traffic while the physical instrument's own
  // front panel is in use — see enterGenLocalMode()/exitGenLocalMode().
  let inGenLocalMode = false;
  // FIFO of {ch, field, seq} — see comment above genQueryQueue's own
  // dequeue logic for the ordering-hazard history this exists to guard
  // against. `seq` added after repeated real-world cases (confirmed at
  // the bench, several sessions) where a response arrived that did NOT
  // actually answer whatever was at the front of this queue — some
  // combination of stale firmware-side backlog and/or the liveness
  // probe's own timing interacting badly with a long-running refresh.
  // Every prior fix addressed a SPECIFIC mechanism found by manually
  // reverse-engineering a log after the fact — slow, and repeatedly
  // wrong about the exact cause. This is the actual fix for the whole
  // CLASS of problem: every outgoing Func Gen command now carries a
  // browser-assigned seq number (genSeqCounter), the firmware echoes it
  // back verbatim with its response (see main.cpp's pushEvent/pushErr),
  // and the shared ws.onmessage dispatcher checks the echoed seq
  // against genQueryQueue[0].seq the instant a funcgen response
  // arrives — logging an unambiguous "SEQ MISMATCH" line the moment
  // reality stops matching the FIFO assumption, instead of silently
  // misapplying a response to the wrong field. Detection only for now,
  // deliberately — auto-recovery (resyncing the queue once a mismatch
  // is caught) is a separate, harder problem to get right, not
  // attempted here.
  let genQueryQueue  = [];
  let genSeqCounter  = 0;  // monotonically increasing; every outgoing
                             // Func Gen command gets the next value
  let genQueryTimeout = null;
  // Real bug found via the new seq-mismatch detection above: this was
  // 5000 (5 seconds), which is nowhere near enough. Responses run
  // roughly 1 second apart (confirmed at the bench), and a full refresh
  // burst can reach 30+ queries — meaning an item near the back of the
  // queue can legitimately still be waiting 25-29 seconds for its own
  // turn, correctly in flight the whole time, nothing actually wrong.
  // At 5 seconds, that entry's timeout fired and silently dropped it
  // while it was still genuinely on its way — and when its real,
  // correctly-processed response arrived moments later, it landed on
  // whatever was now at the front of the queue instead, off by exactly
  // one. That's the exact "expected #28, got #27" cascade seen at the
  // bench — a single silently-dropped entry shifting everything after
  // it. 60 seconds comfortably covers even a large burst with margin.
  // This isn't the only safety net against a genuinely unresponsive
  // device, either — the separate liveness-probe system (see
  // LIVENESS_TIMEOUT_MS, 3 seconds) already detects real hardware
  // failure far faster than this; this timeout's only job is "don't
  // get permanently stuck," not "detect a dead device quickly."
  const GEN_QUERY_TIMEOUT_MS = 60000;

  // Raw wire send — no queue bookkeeping beyond assigning this command's
  // own seq number. Only used internally by queueGenQuery() and
  // genSend() below; nothing else should call this directly. Returns
  // the seq assigned, so callers that need to track it (queueGenQuery)
  // can.
  function genSendRaw(cmd) {
    const seq = ++genSeqCounter;
    wsSend('funcgen', cmd, seq);
    return seq;
  }

  // Every non-query (SET) command sent to the func gen produces a real
  // fabricated "OK" acknowledgment on the wire (see main.cpp's
  // handleFuncgenCommand). If that OK arrives while genQueryQueue is
  // non-empty — e.g. you click a Set button, then click a Mode toggle
  // before the OK comes back, starting a fresh refresh — it gets
  // silently consumed as if it were the answer to whatever field query
  // is next in line, and every field after it shifts by one slot. This
  // was a REAL bug, confirmed at the bench: Source/Mod Freq/Shape/
  // Depth/DSSC all displaying each other's values, shifted by one.
  // setGenAmpl/setGenOffset/setGenHigh/setGenLow already individually
  // guarded against this (see their own comments), but every other
  // Set/Toggle/Trigger function built afterward (AM, Sweep, Burst) sent
  // via bare genSend() and never got the same protection — the same
  // gap, repeated ~25 times. Fixed once, here, instead of patching
  // every call site individually: every non-query genSend() now
  // automatically reserves its own inert queue slot for its OK, so it
  // can never be mistaken for someone else's answer regardless of what
  // else happens to be in flight. (Actual queries always go through
  // queueGenQuery() directly with a real field tag, never bare
  // genSend() — so the '?' check below never intercepts those. Worth
  // knowing: if this branch were ever actually hit, it would send via
  // genSendRaw() without a matching genQueryQueue entry, and its
  // response would trigger a false "SEQ MISMATCH" — see the seq-
  // checking comment on genQueryQueue's own declaration.)
  function genSend(cmd) {
    if (cmd.endsWith('?')) {
      genSendRaw(cmd);
    } else {
      queueGenQuery(0, 'ignore', cmd);
    }
  }

  // Disables all Func Gen controls while a refresh is in flight — guards
  // against, e.g., an Output click landing while queued query responses
  // are still arriving, which would otherwise get its fabricated "OK"
  // incorrectly consumed as if it were the next queued field's answer.
  // Set buttons are handled specially: they always get disabled here on
  // busy=true (safety still applies — don't allow a Set mid-refresh).
  // On busy=false they're NOT blindly re-enabled the way everything
  // else is — their real default is disabled-until-focused (see
  // initGenSetButtonGating), and blanket-enabling them here would
  // defeat that every time a refresh completes. Instead, only the
  // button belonging to whatever field is still focused right now (if
  // any) gets re-enabled, via genFieldToSetBtn. Real gap found at the
  // bench: pressing Enter to Set a value (rather than clicking the
  // button) never fires a blur/focus cycle on the field, so the
  // focus-gating listeners never got a chance to re-enable it — the
  // button stayed stuck disabled even though the field was still
  // focused the whole time, until the user clicked away and back.
  function setGenBusy(busy) {
    document.querySelectorAll(
      '.gen-func-btn, .gen-set-btn, .gen-quick-btn, .gen-toggle-btn, ' +
      '.gen-mod-select, .gen-output-btn, .gen-harmtype-btn, #gen-refresh-btn'
    ).forEach(el => {
      if (el.classList.contains('gen-set-btn') && !busy) return;
      el.disabled = busy;
    });
    if (!busy) {
      const activeBtn = genFieldToSetBtn.get(document.activeElement);
      if (activeBtn) activeBtn.disabled = false;
    }
  }

  // ── Local mode — mirrors the DMM panel's own Local mode ────────────────
  // Halts all Func Gen traffic (reusing setGenBusy's disable-everything
  // mechanism, plus pausing the funcgen liveness probe — see
  // livenessTick()) while the physical instrument's own front-panel
  // buttons are in use, so nothing here races against whatever the user
  // is doing by hand at the bench. Deliberately no way to dismiss the
  // modal except "Done" (no click-outside-to-close) — the whole point
  // is a deliberate confirmation, not something to brush past
  // accidentally.
  function enterGenLocalMode() {
    inGenLocalMode = true;
    setGenBusy(true);
    document.getElementById('gen-local-modal').classList.add('open');
  }

  function exitGenLocalMode() {
    inGenLocalMode = false;
    document.getElementById('gen-local-modal').classList.remove('open');
    setGenBusy(false);
    // Re-sync with reality — the user may have changed Function/Freq/
    // Ampl/etc. from the instrument's own front panel while in Local
    // mode, the same way DMM's own Local mode re-queries on exit.
    // setGenModeButtonDisplay() (triggered by this refresh's own
    // 'mod'/'sweep-state'/'burst-state' responses) handles fetching a
    // mode's fields itself if it's confirmed actually on — see its own
    // comment.
    refreshBothGenChannels();
  }

  // ── Config panel — save/restore setup across a power cycle ─────────────
  // Deliberately the simple version, by explicit request: reads straight
  // from whatever's currently sitting in the visible UI fields, not a
  // fresh ~110-query gather-everything pass (which would take upward of
  // 20-30 seconds and was judged not worth it for what an average user
  // needs). A field never actually visited this session — still blank
  // or "..." — is correctly skipped rather than exported as wrong.
  // Known, accepted gap: anything changed via the instrument's own front
  // panel while in Local mode, on a mode/waveform not revisited before
  // Done, won't be reflected here either — see exitGenLocalMode()'s own
  // comment for the same underlying limitation. A full, always-accurate
  // "gather everything" button remains a documented future option if
  // this gap ever turns out to matter in practice.
  function openGenConfig() {
    document.getElementById('genconfig').classList.add('open');
  }
  function closeGenConfig() {
    document.getElementById('genconfig').classList.remove('open');
  }

  // One entry per settable parameter this covers. cmd uses {ch} as a
  // placeholder. Deliberately excludes per-harmonic Amplitude/Phase
  // (:HARMonic:AMPL/PHASe n,val) — those are indexed per harmonic
  // number, and the panel only ever knows whichever one was last
  // viewed, not the full set, so exporting them would silently claim
  // more than is actually known. Harmonic's Order and Type themselves
  // (single values, not per-index) are included.
  const GEN_CONFIG_FIELDS = [
    // Core
    { id: 'freq',   cmd: ':SOUR{ch}:FREQ' },
    { id: 'period', cmd: ':SOUR{ch}:PERiod' },
    { id: 'ampl',   cmd: ':SOUR{ch}:VOLT' },
    { id: 'offset', cmd: ':SOUR{ch}:VOLT:OFFS' },
    { id: 'high',   cmd: ':SOUR{ch}:VOLT:HIGH' },
    { id: 'low',    cmd: ':SOUR{ch}:VOLT:LOW' },
    { id: 'imp',    cmd: ':OUTP{ch}:IMP' },
    { id: 'phase',  cmd: ':SOUR{ch}:PHASe' },
    // Waveform-specific extras
    { id: 'squ-dutycycle',   cmd: ':SOUR{ch}:FUNC:SQUare:DCYCle' },
    { id: 'ramp-symmetry',   cmd: ':SOUR{ch}:FUNC:RAMP:SYMMetry' },
    { id: 'pulse-dutycycle', cmd: ':SOUR{ch}:PULSe:DCYCle' },
    { id: 'pulse-width',     cmd: ':SOUR{ch}:PULSe:WIDTh' },
    { id: 'pulse-leading',   cmd: ':SOUR{ch}:PULSe:TRANsition:LEADing' },
    { id: 'pulse-trailing',  cmd: ':SOUR{ch}:PULSe:TRANsition:TRAiling' },
    // Harmonic — single-value fields only, see the note above
    { id: 'harm-index', cmd: ':SOUR{ch}:HARMonic:ORDEr' },
    // Mod: Type + AM sub-parameters
    { id: 'mod-typesel',   cmd: ':SOUR{ch}:MOD:TYPE' },
    { id: 'mod-am-source', cmd: ':SOUR{ch}:MOD:AM:SOURce' },
    { id: 'mod-am-freq',   cmd: ':SOUR{ch}:MOD:AM:INTernal:FREQuency' },
    { id: 'mod-am-shape',  cmd: ':SOUR{ch}:MOD:AM:INTernal:FUNCtion' },
    { id: 'mod-am-depth',  cmd: ':SOUR{ch}:MOD:AM' },
    // Sweep
    { id: 'sweep-typesel',    cmd: ':SOUR{ch}:SWEep:SPACing' },
    { id: 'sweep-startfreq',  cmd: ':SOUR{ch}:FREQuency:STARt' },
    { id: 'sweep-stopfreq',   cmd: ':SOUR{ch}:FREQuency:STOP' },
    { id: 'sweep-centerfreq', cmd: ':SOUR{ch}:FREQuency:CENTer' },
    { id: 'sweep-spanfreq',   cmd: ':SOUR{ch}:FREQuency:SPAN' },
    { id: 'sweep-step',       cmd: ':SOUR{ch}:SWEep:STEP' },
    { id: 'sweep-time',       cmd: ':SOUR{ch}:SWEep:TIME' },
    { id: 'sweep-htimestart', cmd: ':SOUR{ch}:SWEep:HTIMe:STARt' },
    { id: 'sweep-htimestop',  cmd: ':SOUR{ch}:SWEep:HTIMe:STOP' },
    { id: 'sweep-rtime',      cmd: ':SOUR{ch}:SWEep:RTIMe' },
    { id: 'sweep-trigsrc',    cmd: ':SOUR{ch}:SWEep:TRIGger:SOURce' },
    { id: 'sweep-markfreq',   cmd: ':SOUR{ch}:MARKer:FREQuency' },
    // Burst
    { id: 'burst-typesel', cmd: ':SOUR{ch}:BURSt:MODE' },
    { id: 'burst-ncycles', cmd: ':SOUR{ch}:BURSt:NCYCles' },
    { id: 'burst-phase',   cmd: ':SOUR{ch}:BURSt:PHASe' },
    { id: 'burst-period',  cmd: ':SOUR{ch}:BURSt:INTernal:PERiod' },
    { id: 'burst-gatepol', cmd: ':SOUR{ch}:BURSt:GATE:POLarity' },
    { id: 'burst-trigsrc', cmd: ':SOUR{ch}:BURSt:TRIGger:SOURce' },
    { id: 'burst-trigout', cmd: ':SOUR{ch}:BURSt:TRIGger:TRIGOut' },
    { id: 'burst-slopein', cmd: ':SOUR{ch}:BURSt:TRIGger:SLOPe' },
    { id: 'burst-tdelay',  cmd: ':SOUR{ch}:BURSt:TDELay' },
  ];

  // Button-driven ON/OFF fields — read from dataset.on rather than
  // .value. Mode state (Mod/Sweep/Burst) is part of the always-queried
  // core burst, so it's reliable once at least one Refresh has run this
  // session; DSSC and Mark State are lazy like everything else above,
  // gated the same way (visible + actually set at some point).
  const GEN_CONFIG_TOGGLES = [
    { id: 'mode-mod',            cmd: ':SOUR{ch}:MOD:STATe' },
    { id: 'mode-sweep',          cmd: ':SOUR{ch}:SWEep:STATe' },
    { id: 'mode-burst',          cmd: ':SOUR{ch}:BURSt:STATe' },
    { id: 'mod-am-dssc-btn',     cmd: ':SOUR{ch}:AM:DSSC' },
    { id: 'sweep-markstate-btn', cmd: ':SOUR{ch}:MARKer:STATe' },
  ];

  // Function is a special case — which command to send depends on which
  // waveform is currently active, tracked in genCurrentFunc[ch]. Returns
  // null if Function was never actually discovered this session (no
  // Refresh has run yet), same "skip what's unknown" rule as every
  // other field here.
  function genFunctionCommand(ch) {
    const f = genCurrentFunc[ch];
    if (!f) return null;
    if (f === 'HARM') return ':SOUR' + ch + ':FUNC HARM';
    if (f === 'CUSTOM') return ':SOUR' + ch + ':FUNC:SHAPe CUSTOM';
    if (f === 'ARB') {
      const sel = document.getElementById('gen-ch' + ch + '-arb-select');
      return ':SOUR' + ch + ':FUNC:SHAPe ' + (sel && sel.value ? sel.value : 'ARB');
    }
    return ':SOUR' + ch + ':FUNC ' + f;  // SIN/SQU/RAMP/PULSE/NOISE
  }

  function generateGenConfig() {
    const lines = [];
    [1, 2].forEach(ch => {
      lines.push('; ---- Channel ' + ch + ' ----');
      const funcCmd = genFunctionCommand(ch);
      if (funcCmd) lines.push(funcCmd);
      GEN_CONFIG_FIELDS.forEach(f => {
        const el = document.getElementById('gen-ch' + ch + '-' + f.id);
        if (!el || el.offsetParent === null) return;  // not currently shown
        const val = el.value;
        if (!val || val === '...') return;  // never queried / still pending
        lines.push(f.cmd.replace('{ch}', ch) + ' ' + val);
      });
      GEN_CONFIG_TOGGLES.forEach(t => {
        const btn = document.getElementById('gen-ch' + ch + '-' + t.id);
        if (!btn || btn.offsetParent === null || btn.dataset.on === undefined) return;
        lines.push(t.cmd.replace('{ch}', ch) + ' ' + (btn.dataset.on === '1' ? 'ON' : 'OFF'));
      });
      // Output last, deliberately — everything else should be configured
      // correctly before the signal actually goes live, not the other
      // way around.
      if (genOutputState[ch] !== undefined) {
        lines.push(':OUTP' + ch + ':STAT ' + (genOutputState[ch] ? 'ON' : 'OFF'));
      }
    });
    document.getElementById('genconfig-export').value = lines.join('\n');
  }

  function restoreGenConfig() {
    const text = document.getElementById('genconfig-import').value;
    text.split('\n')
      .map(l => l.trim())
      .filter(l => l && !l.startsWith(';'))
      .forEach(cmd => queueGenQuery(0, 'ignore', cmd));
  }

  function queueGenQuery(ch, field, cmd) {
    const seq = genSendRaw(cmd);
    genQueryQueue.push({ ch, field, seq });
    armGenQueryTimeout();
  }

  function armGenQueryTimeout() {
    clearTimeout(genQueryTimeout);
    // Reflects literally "queries pending reply" — genQueryQueue's own
    // length — not the broader setGenBusy() flag, which also covers
    // Local mode for an unrelated reason (the physical front panel
    // being in use, nothing to do with queries in flight). By explicit
    // request.
    const statusEl = document.getElementById('gen-query-status');
    if (genQueryQueue.length === 0) {
      if (statusEl) statusEl.value = '';
      // Don't let the queue simply emptying re-enable controls if
      // Local mode's own halt is still supposed to be active — see
      // enterGenLocalMode(). Only "Done" should ever lift that one.
      if (!inGenLocalMode) setGenBusy(false);
      return;
    }
    if (statusEl) statusEl.value = 'Values being queried.  Please wait.';
    setGenBusy(true);
    genQueryTimeout = setTimeout(() => {
      if (genQueryQueue.length > 0) {
        const dropped = genQueryQueue.shift();
        // Was completely silent before — now that this actually only
        // fires after a genuinely long, realistic wait (60s, not 5),
        // it firing at all is worth knowing about, not hiding.
        appLog('funcgen', '\u26a0 TIMEOUT: no reply to #' + dropped.seq +
          ' (' + dropped.field + ', ch' + dropped.ch + ') after ' +
          GEN_QUERY_TIMEOUT_MS + 'ms — giving up and resyncing', 'log-err');
        // Real gap found at the bench: the firmware's own internal
        // queue has no idea this browser just gave up on something —
        // without telling it, whatever it was still working on (this
        // dropped entry, or anything queued behind it on the firmware
        // side) gets answered anyway, arbitrarily later, landing on
        // whatever this session happens to be asking about by then.
        // Confirmed: a long, continuous session (no WS/USB disconnect
        // at all, so neither of the earlier queue-flush fixes ever
        // fired) accumulated multiple such stragglers over time, each
        // one widening the seq mismatch by one more. __RESYNC__ closes
        // that: tell the firmware to give up too, and clear everything
        // else this session was still tracking — none of it can be
        // trusted once the firmware's own queue has been wiped out
        // from under it.
        if (genQueryQueue.length > 0) {
          appLog('funcgen', '(clearing ' + genQueryQueue.length +
            ' other pending — none can be trusted once the firmware queue resets)', 'log-sys');
        }
        genQueryQueue = [];
        wsSend('funcgen', '__RESYNC__');
      }
      armGenQueryTimeout();
    }, GEN_QUERY_TIMEOUT_MS);
  }

  function refreshBothGenChannels() {
    // Blank the text inputs to "..." immediately, before any response has
    // come back. Each field updates individually the instant its own
    // response arrives (not all at once when the whole batch finishes),
    // so without this a field with a still-pending query looks
    // indistinguishable from one that's already confirmed — this makes
    // "not answered yet" visually obvious instead of silently showing
    // whatever was there before the refresh was clicked.
    [1, 2].forEach(ch => {
      ['freq', 'period', 'ampl', 'offset', 'high', 'low', 'imp', 'phase'].forEach(f => {
        document.getElementById('gen-ch' + ch + '-' + f).value = '...';
      });
    });
    // Interleaved by field, CH1 then CH2, rather than fully completing
    // CH1 before starting CH2 — matches how the eyes actually scan the
    // two-column layout: across each row before moving down to the
    // next one, not down one whole column before starting the other.
    // By explicit request.
    queueGenQuery(1, 'func',   ':SOUR1:FUNC?');
    queueGenQuery(2, 'func',   ':SOUR2:FUNC?');
    queueGenQuery(1, 'output', ':OUTP1:STAT?');
    queueGenQuery(2, 'output', ':OUTP2:STAT?');
    queueGenQuery(1, 'freq',   ':SOUR1:FREQ?');
    queueGenQuery(2, 'freq',   ':SOUR2:FREQ?');
    queueGenQuery(1, 'period', ':SOUR1:PERiod?');
    queueGenQuery(2, 'period', ':SOUR2:PERiod?');
    queueGenQuery(1, 'ampl',   ':SOUR1:VOLT?');
    queueGenQuery(2, 'ampl',   ':SOUR2:VOLT?');
    queueGenQuery(1, 'high',   ':SOUR1:VOLT:HIGH?');
    queueGenQuery(2, 'high',   ':SOUR2:VOLT:HIGH?');
    queueGenQuery(1, 'offset', ':SOUR1:VOLT:OFFS?');
    queueGenQuery(2, 'offset', ':SOUR2:VOLT:OFFS?');
    queueGenQuery(1, 'low',    ':SOUR1:VOLT:LOW?');
    queueGenQuery(2, 'low',    ':SOUR2:VOLT:LOW?');
    // Imp before Phase — matches the actual visual row order (Imp's row
    // sits above Phase's row in the layout), not the order they came up
    // in conversation.
    queueGenQuery(1, 'imp',    ':OUTP1:IMP?');
    queueGenQuery(2, 'imp',    ':OUTP2:IMP?');
    queueGenQuery(1, 'phase',  ':SOUR1:PHASe?');
    queueGenQuery(2, 'phase',  ':SOUR2:PHASe?');
    // MOD:TYPE? is deliberately NOT queried here unconditionally —
    // by explicit request, since there's no point knowing the
    // modulation type when Mod isn't even on. Moved to
    // refreshModeFields()'s 'mod' branch, which only runs once Mod is
    // confirmed on — matching how Sweep's own Type (sweep-spacing) and
    // Burst's own Type (burst-mode) already only fetch that way.
    queueGenQuery(1, 'mod', ':SOUR1:MOD?');
    queueGenQuery(2, 'mod', ':SOUR2:MOD?');
    // Just the ON/OFF state for the Mode row's Sweep/Burst indicators —
    // NOT their sub-parameter fields (Start Freq, Count, etc.). See
    // refreshModeFields() below for why those are deliberately lazy.
    queueGenQuery(1, 'sweep-state', ':SOUR1:SWEep:STATe?');
    queueGenQuery(2, 'sweep-state', ':SOUR2:SWEep:STATe?');
    queueGenQuery(1, 'burst-state', ':SOUR1:BURSt:STATe?');
    queueGenQuery(2, 'burst-state', ':SOUR2:BURSt:STATe?');
  }

  // Fetches a single mode's Type + sub-parameter fields — called from
  // selectGenMode() whenever the visible mode changes, NOT from
  // refreshBothGenChannels(). Deliberately lazy:
  // Sweep has ~10 sub-fields and Burst has ~6, and each query has a
  // ~1s+ firmware-side delay (see main.cpp) — eagerly fetching all of
  // that for both channels on every connect/Refresh click would
  // roughly triple the wait, mostly for fields nobody's looking at yet.
  // Fetching only the currently-viewed mode's fields keeps opening the
  // tab fast, at the cost of a brief "..." pause the first time you
  // actually switch to view Sweep or Burst.
  function refreshModeFields(ch, mode) {
    if (mode === 'sweep') {
      ['sweep-startfreq', 'sweep-stopfreq', 'sweep-centerfreq', 'sweep-spanfreq',
       'sweep-step', 'sweep-time',
       'sweep-htimestart', 'sweep-htimestop', 'sweep-rtime', 'sweep-markfreq'
      ].forEach(f => { document.getElementById('gen-ch' + ch + '-' + f).value = '...'; });
      queueGenQuery(ch, 'sweep-spacing',    ':SOUR' + ch + ':SWEep:SPACing?');
      queueGenQuery(ch, 'sweep-startfreq',  ':SOUR' + ch + ':FREQuency:STARt?');
      queueGenQuery(ch, 'sweep-stopfreq',   ':SOUR' + ch + ':FREQuency:STOP?');
      queueGenQuery(ch, 'sweep-centerfreq', ':SOUR' + ch + ':FREQuency:CENTer?');
      queueGenQuery(ch, 'sweep-spanfreq',   ':SOUR' + ch + ':FREQuency:SPAN?');
      queueGenQuery(ch, 'sweep-step',       ':SOUR' + ch + ':SWEep:STEP?');
      queueGenQuery(ch, 'sweep-time',       ':SOUR' + ch + ':SWEep:TIME?');
      queueGenQuery(ch, 'sweep-htimestart', ':SOUR' + ch + ':SWEep:HTIMe:STARt?');
      queueGenQuery(ch, 'sweep-htimestop',  ':SOUR' + ch + ':SWEep:HTIMe:STOP?');
      queueGenQuery(ch, 'sweep-rtime',      ':SOUR' + ch + ':SWEep:RTIMe?');
      queueGenQuery(ch, 'sweep-trigsrc',    ':SOUR' + ch + ':SWEep:TRIGger:SOURce?');
      queueGenQuery(ch, 'sweep-markstate',  ':SOUR' + ch + ':MARKer:STATe?');
      queueGenQuery(ch, 'sweep-markfreq',   ':SOUR' + ch + ':MARKer:FREQuency?');
    } else if (mode === 'burst') {
      ['burst-ncycles', 'burst-phase', 'burst-period', 'burst-tdelay']
        .forEach(f => { document.getElementById('gen-ch' + ch + '-' + f).value = '...'; });
      queueGenQuery(ch, 'burst-mode',    ':SOUR' + ch + ':BURSt:MODE?');
      queueGenQuery(ch, 'burst-ncycles', ':SOUR' + ch + ':BURSt:NCYCles?');
      queueGenQuery(ch, 'burst-phase',   ':SOUR' + ch + ':BURSt:PHASe?');
      queueGenQuery(ch, 'burst-period',  ':SOUR' + ch + ':BURSt:INTernal:PERiod?');
      queueGenQuery(ch, 'burst-gatepol', ':SOUR' + ch + ':BURSt:GATE:POLarity?');
      queueGenQuery(ch, 'burst-trigsrc', ':SOUR' + ch + ':BURSt:TRIGger:SOURce?');
      queueGenQuery(ch, 'burst-trigout', ':SOUR' + ch + ':BURSt:TRIGger:TRIGOut?');
      queueGenQuery(ch, 'burst-slopein', ':SOUR' + ch + ':BURSt:TRIGger:SLOPe?');
    } else if (mode === 'mod') {
      // Only Type itself is queried unconditionally here — which
      // specific sub-parameters to fetch depends on which of the 12
      // types is actually active, so that dispatch now lives in the
      // 'modtype' response case itself (queueModTypeSubParams()) rather
      // than being hardcoded to AM here regardless of the real type.
      document.getElementById('gen-ch' + ch + '-mod-typesel').value = '';
      queueGenQuery(ch, 'modtype', ':SOUR' + ch + ':MOD:TYPE?');
    }
  }

  function handleFuncgenResponse(msg) {
    if (genQueryQueue.length === 0) return;  // e.g. a manual SCPI-tab
                                                // command, or an Output
                                                // toggle's fabricated OK —
                                                // not part of a refresh
    const { ch, field } = genQueryQueue.shift();
    clearTimeout(genQueryTimeout);
    applyGenField(ch, field, msg);
    armGenQueryTimeout();
  }

  function handleFuncgenError() {
    // A queued query itself failed (e.g. transport error) — drop that
    // one slot immediately rather than waiting for the timeout.
    if (genQueryQueue.length === 0) return;
    genQueryQueue.shift();
    clearTimeout(genQueryTimeout);
    armGenQueryTimeout();
  }

  function applyGenField(ch, field, msg) {
    switch (field) {
      case 'func': {
        const val = msg.trim();
        // The instrument's query response uses the long form
        // ("HARMONIC"), confirmed at the bench — but every SET command
        // elsewhere in this app (including this panel's own Harmonic
        // button) uses the short form ("HARM"). Recognizing both here
        // avoids a real bug: without this, discovering Harmonic mode
        // passively (e.g. on reconnect, if the instrument was already
        // in that mode from before) never matched, so the panel never
        // switched to/populated the Harmonic view — only clicking the
        // button directly worked. This makes the passive path behave
        // exactly like the click path. Same idea extended to Arb: if
        // the response is one of the ~150 built-in named waveforms
        // (GEN_ARB_NAMES), switch to the Arb view and reflect the real
        // selection in its dropdown.
        //
        // CUSTOM is deliberately its OWN independent case, not routed
        // through Arb at all — even though CUSTOM happens to share the
        // same underlying SCPI keyword space as the ~150 arb names
        // (confirmed at the bench: CUSTOM is what the User Key shortcut
        // resolves to — see selectGenUserCh()'s own comment), the User
        // button and view should have no connection to Arb's dropdown
        // or state whatsoever, by explicit request. Discovering CUSTOM
        // highlights User* and shows the normal view — exactly what
        // clicking User* itself does — never touching Arb's dropdown.
        const isHarm   = (val === 'HARM' || val === 'HARMONIC');
        const isCustom = (val === 'CUSTOM');
        const isArb    = !isCustom && GEN_ARB_NAMES.has(val);
        document.querySelectorAll('#gen-ch' + ch + ' .gen-func-btn').forEach(b => {
          if (isHarm) b.classList.toggle('active', b.dataset.func === 'HARM');
          else if (isCustom) b.classList.toggle('active', b.dataset.func === 'USER');
          else if (isArb) b.classList.toggle('active', b.dataset.func === 'ARB');
          else b.classList.toggle('active', b.dataset.func === val);
        });
        if (isHarm) {
          showGenView(ch, 'harmonic');
          setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.HARM);
          refreshHarmonicFields(ch);
          updateGenWaveExtras(ch, 'HARM', true);
        } else if (isCustom) {
          showGenView(ch, 'normal');
          setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.USER);
          updateGenWaveExtras(ch, 'CUSTOM', true);
        } else if (isArb) {
          showGenView(ch, 'normal');
          setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.ARB);
          const select = document.getElementById('gen-ch' + ch + '-arb-select');
          if ([...select.options].some(o => o.value === val)) select.value = val;
          updateGenWaveExtras(ch, 'ARB', true);
        } else {
          showGenView(ch, 'normal');
          setGenModeRowAvailability(ch, GEN_ALLOWED_MODES[val] || ['mod', 'sweep', 'burst']);
          updateGenWaveExtras(ch, val, true);
        }
        break;
      }
      case 'freq':
        document.getElementById('gen-ch' + ch + '-freq').value = formatGenNumber(msg);
        break;
      case 'period':
        document.getElementById('gen-ch' + ch + '-period').value = formatGenNumber(msg);
        break;
      case 'ampl':
        document.getElementById('gen-ch' + ch + '-ampl').value = formatGenNumber(msg);
        break;
      case 'offset':
        document.getElementById('gen-ch' + ch + '-offset').value = formatGenNumber(msg);
        break;
      case 'high':
        document.getElementById('gen-ch' + ch + '-high').value = formatGenNumber(msg);
        break;
      case 'low':
        document.getElementById('gen-ch' + ch + '-low').value = formatGenNumber(msg);
        break;
      case 'ignore':
        break;  // a set command's own fabricated OK — see comment above
                 // setGenAmpl/setGenOffset/setGenHigh/setGenLow
      case 'imp':
        document.getElementById('gen-ch' + ch + '-imp').value = msg.trim();
        break;
      case 'phase':
        document.getElementById('gen-ch' + ch + '-phase').value = formatGenNumber(msg);
        break;
      case 'squ-dutycycle':
        document.getElementById('gen-ch' + ch + '-squ-dutycycle').value = formatGenNumber(msg);
        break;
      case 'ramp-symmetry':
        document.getElementById('gen-ch' + ch + '-ramp-symmetry').value = formatGenNumber(msg);
        break;
      case 'pulse-dutycycle':
        document.getElementById('gen-ch' + ch + '-pulse-dutycycle').value = formatGenNumber(msg);
        break;
      case 'pulse-width':
        document.getElementById('gen-ch' + ch + '-pulse-width').value = formatGenNumber(msg);
        break;
      case 'pulse-leading':
        document.getElementById('gen-ch' + ch + '-pulse-leading').value = formatGenNumber(msg);
        break;
      case 'pulse-trailing':
        document.getElementById('gen-ch' + ch + '-pulse-trailing').value = formatGenNumber(msg);
        break;
      case 'output': {
        const on = /^(ON|1)$/i.test(msg.trim());
        genOutputState[ch] = on;
        updateGenOutputBtn(ch);
        break;
      }
      case 'mod': {
        const on = /^(ON|1)$/i.test(msg.trim());
        if (!on) {
          // Nothing to show — MOD:TYPE? isn't even queried while Mod
          // is off anymore (see refreshBothGenChannels()'s own comment), so
          // don't leave a stale value sitting in the dropdown either.
          document.getElementById('gen-ch' + ch + '-mod-typesel').value = '';
        }
        setGenModeButtonDisplay(ch, 'mod', on);
        break;
      }
      case 'modtype':
        document.getElementById('gen-ch' + ch + '-mod-typesel').value = msg.trim();
        updateModTypeVisibility(ch);
        queueModTypeSubParams(ch, msg.trim());
        break;
      case 'sweep-state': {
        const on = /^(ON|1)$/i.test(msg.trim());
        setGenModeButtonDisplay(ch, 'sweep', on);
        break;
      }
      case 'burst-state': {
        const on = /^(ON|1)$/i.test(msg.trim());
        setGenModeButtonDisplay(ch, 'burst', on);
        break;
      }
      case 'sweep-spacing':
        document.getElementById('gen-ch' + ch + '-sweep-typesel').value = normalizeGenSelectValue(msg);
        updateSweepStepVisibility(ch);
        break;
      case 'sweep-startfreq':
        document.getElementById('gen-ch' + ch + '-sweep-startfreq').value = formatGenNumber(msg);
        break;
      case 'sweep-stopfreq':
        document.getElementById('gen-ch' + ch + '-sweep-stopfreq').value = formatGenNumber(msg);
        break;
      case 'sweep-centerfreq':
        document.getElementById('gen-ch' + ch + '-sweep-centerfreq').value = formatGenNumber(msg);
        break;
      case 'sweep-spanfreq':
        document.getElementById('gen-ch' + ch + '-sweep-spanfreq').value = formatGenNumber(msg);
        break;
      case 'sweep-step':
        document.getElementById('gen-ch' + ch + '-sweep-step').value = formatGenNumber(msg);
        break;
      case 'sweep-time':
        document.getElementById('gen-ch' + ch + '-sweep-time').value = formatGenNumber(msg);
        break;
      case 'sweep-htimestart':
        document.getElementById('gen-ch' + ch + '-sweep-htimestart').value = formatGenNumber(msg);
        break;
      case 'sweep-htimestop':
        document.getElementById('gen-ch' + ch + '-sweep-htimestop').value = formatGenNumber(msg);
        break;
      case 'sweep-rtime':
        document.getElementById('gen-ch' + ch + '-sweep-rtime').value = formatGenNumber(msg);
        break;
      case 'sweep-trigsrc':
        document.getElementById('gen-ch' + ch + '-sweep-trigsrc').value = normalizeGenSelectValue(msg);
        updateSweepTrigNowVisibility(ch);
        break;
      case 'sweep-markstate': {
        const on = /^(ON|1)$/i.test(msg.trim());
        const btn = document.getElementById('gen-ch' + ch + '-sweep-markstate-btn');
        btn.textContent = btn.dataset.label + ': ' + (on ? 'ON' : 'OFF');
        btn.dataset.on = on ? '1' : '0';
        break;
      }
      case 'sweep-markfreq':
        document.getElementById('gen-ch' + ch + '-sweep-markfreq').value = formatGenNumber(msg);
        break;
      case 'burst-mode':
        document.getElementById('gen-ch' + ch + '-burst-typesel').value = normalizeGenSelectValue(msg);
        updateBurstGatePolVisibility(ch);
        updateBurstNonGatedFieldsVisibility(ch);
        updateBurstTrigEdgeVisibility(ch);
        break;
      case 'burst-ncycles':
        document.getElementById('gen-ch' + ch + '-burst-ncycles').value = formatGenNumber(msg);
        break;
      case 'burst-phase':
        document.getElementById('gen-ch' + ch + '-burst-phase').value = formatGenNumber(msg);
        break;
      case 'burst-period':
        document.getElementById('gen-ch' + ch + '-burst-period').value = formatGenNumber(msg);
        break;
      case 'burst-gatepol':
        document.getElementById('gen-ch' + ch + '-burst-gatepol').value = normalizeGenSelectValue(msg);
        break;
      case 'burst-trigsrc':
        document.getElementById('gen-ch' + ch + '-burst-trigsrc').value = normalizeGenSelectValue(msg);
        updateBurstTrigNowVisibility(ch);
        updateBurstTrigEdgeVisibility(ch);
        break;
      case 'burst-trigout':
        document.getElementById('gen-ch' + ch + '-burst-trigout').value = normalizeGenSelectValue(msg);
        break;
      case 'burst-slopein':
        document.getElementById('gen-ch' + ch + '-burst-slopein').value = normalizeGenSelectValue(msg);
        break;
      case 'burst-tdelay':
        document.getElementById('gen-ch' + ch + '-burst-tdelay').value = formatGenNumber(msg);
        break;
      case 'mod-am-source':
        document.getElementById('gen-ch' + ch + '-mod-am-source').value = normalizeGenSelectValue(msg);
        updateModAmSourceGrayingCh(ch);
        break;
      case 'mod-am-freq':
        document.getElementById('gen-ch' + ch + '-mod-am-freq').value = formatGenNumber(msg);
        break;
      case 'mod-am-shape':
        document.getElementById('gen-ch' + ch + '-mod-am-shape').value = msg.trim();
        break;
      case 'mod-am-depth':
        document.getElementById('gen-ch' + ch + '-mod-am-depth').value = formatGenNumber(msg);
        break;
      case 'mod-am-dssc': {
        const on = /^(ON|1)$/i.test(msg.trim());
        const btn = document.getElementById('gen-ch' + ch + '-mod-am-dssc-btn');
        btn.textContent = btn.dataset.label + ': ' + (on ? 'ON' : 'OFF');
        btn.dataset.on = on ? '1' : '0';
        break;
      }
      // ── PM ──
      case 'mod-pm-source':
        document.getElementById('gen-ch' + ch + '-mod-pm-source').value = normalizeGenSelectValue(msg);
        updateModSourceGrayingCh(ch, 'pm');
        break;
      case 'mod-pm-freq':
        document.getElementById('gen-ch' + ch + '-mod-pm-freq').value = formatGenNumber(msg);
        break;
      case 'mod-pm-shape':
        document.getElementById('gen-ch' + ch + '-mod-pm-shape').value = msg.trim();
        break;
      case 'mod-pm-deviation':
        document.getElementById('gen-ch' + ch + '-mod-pm-deviation').value = formatGenNumber(msg);
        break;
      // ── ASK ──
      case 'mod-ask-source':
        document.getElementById('gen-ch' + ch + '-mod-ask-source').value = normalizeGenSelectValue(msg);
        updateModSourceGrayingCh(ch, 'ask');
        break;
      case 'mod-ask-rate':
        document.getElementById('gen-ch' + ch + '-mod-ask-rate').value = formatGenNumber(msg);
        break;
      case 'mod-ask-ampl':
        document.getElementById('gen-ch' + ch + '-mod-ask-ampl').value = formatGenNumber(msg);
        break;
      case 'mod-ask-polarity':
        document.getElementById('gen-ch' + ch + '-mod-ask-polarity').value = normalizeGenSelectValue(msg);
        break;
      // ── FSK ──
      case 'mod-fsk-source':
        document.getElementById('gen-ch' + ch + '-mod-fsk-source').value = normalizeGenSelectValue(msg);
        updateModSourceGrayingCh(ch, 'fsk');
        break;
      case 'mod-fsk-rate':
        document.getElementById('gen-ch' + ch + '-mod-fsk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-fsk-hopfreq':
        document.getElementById('gen-ch' + ch + '-mod-fsk-hopfreq').value = formatGenNumber(msg);
        break;
      case 'mod-fsk-polarity':
        document.getElementById('gen-ch' + ch + '-mod-fsk-polarity').value = normalizeGenSelectValue(msg);
        break;
      // ── PSK ──
      case 'mod-psk-source':
        document.getElementById('gen-ch' + ch + '-mod-psk-source').value = normalizeGenSelectValue(msg);
        updateModSourceGrayingCh(ch, 'psk');
        break;
      case 'mod-psk-rate':
        document.getElementById('gen-ch' + ch + '-mod-psk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-psk-phase':
        document.getElementById('gen-ch' + ch + '-mod-psk-phase').value = formatGenNumber(msg);
        break;
      case 'mod-psk-polarity':
        document.getElementById('gen-ch' + ch + '-mod-psk-polarity').value = normalizeGenSelectValue(msg);
        break;
      // ── BPSK — no Source/Polarity, see queueModTypeSubParams() ──
      case 'mod-bpsk-rate':
        document.getElementById('gen-ch' + ch + '-mod-bpsk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-bpsk-phase':
        document.getElementById('gen-ch' + ch + '-mod-bpsk-phase').value = formatGenNumber(msg);
        break;
      case 'mod-bpsk-data':
        document.getElementById('gen-ch' + ch + '-mod-bpsk-data').value = msg.trim();
        break;
      // ── QPSK ──
      case 'mod-qpsk-rate':
        document.getElementById('gen-ch' + ch + '-mod-qpsk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-qpsk-phase1':
        document.getElementById('gen-ch' + ch + '-mod-qpsk-phase1').value = formatGenNumber(msg);
        break;
      case 'mod-qpsk-phase2':
        document.getElementById('gen-ch' + ch + '-mod-qpsk-phase2').value = formatGenNumber(msg);
        break;
      case 'mod-qpsk-phase3':
        document.getElementById('gen-ch' + ch + '-mod-qpsk-phase3').value = formatGenNumber(msg);
        break;
      case 'mod-qpsk-data':
        document.getElementById('gen-ch' + ch + '-mod-qpsk-data').value = msg.trim();
        break;
      // ── 3FSK / 4FSK — indexed hop frequencies ──
      case 'mod-3fsk-rate':
        document.getElementById('gen-ch' + ch + '-mod-3fsk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-3fsk-freq1':
        document.getElementById('gen-ch' + ch + '-mod-3fsk-freq1').value = formatGenNumber(msg);
        break;
      case 'mod-3fsk-freq2':
        document.getElementById('gen-ch' + ch + '-mod-3fsk-freq2').value = formatGenNumber(msg);
        break;
      case 'mod-4fsk-rate':
        document.getElementById('gen-ch' + ch + '-mod-4fsk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-4fsk-freq1':
        document.getElementById('gen-ch' + ch + '-mod-4fsk-freq1').value = formatGenNumber(msg);
        break;
      case 'mod-4fsk-freq2':
        document.getElementById('gen-ch' + ch + '-mod-4fsk-freq2').value = formatGenNumber(msg);
        break;
      case 'mod-4fsk-freq3':
        document.getElementById('gen-ch' + ch + '-mod-4fsk-freq3').value = formatGenNumber(msg);
        break;
      // ── OSK ──
      case 'mod-osk-source':
        document.getElementById('gen-ch' + ch + '-mod-osk-source').value = normalizeGenSelectValue(msg);
        updateModSourceGrayingCh(ch, 'osk');
        break;
      case 'mod-osk-rate':
        document.getElementById('gen-ch' + ch + '-mod-osk-rate').value = formatGenNumber(msg);
        break;
      case 'mod-osk-time':
        document.getElementById('gen-ch' + ch + '-mod-osk-time').value = formatGenNumber(msg);
        break;
      case 'harm-order': {
        const n = formatGenNumber(msg);
        document.getElementById('gen-ch' + ch + '-harm-index').value = n;
        // Corrects a possible mismatch: refreshHarmonicFields() had to
        // query Ampl/Phase using whatever the dropdown already showed
        // BEFORE this response arrived (no dynamic query-chaining —
        // see that function's own comment), which could be wrong if
        // the real instrument's current harmonic differs. Now that the
        // actual value is known, re-query for real so the fields never
        // silently show the wrong harmonic's data.
        queueGenQuery(ch, 'harm-ampl',        ':SOUR' + ch + ':HARMonic:AMPL? ' + n);
        queueGenQuery(ch, 'harm-phaseoffset', ':SOUR' + ch + ':HARMonic:PHASe? ' + n);
        break;
      }
      case 'harm-type': {
        const t = msg.trim();
        document.querySelectorAll('#gen-ch' + ch + ' .gen-harmtype-btn').forEach(b =>
          b.classList.toggle('active', b.dataset.harmtype === t));
        updateHarmIndexOptionsCh(ch, t);
        break;
      }
      case 'harm-ampl':
        document.getElementById('gen-ch' + ch + '-harm-ampl').value = formatGenNumber(msg);
        break;
      case 'harm-phaseoffset':
        document.getElementById('gen-ch' + ch + '-harm-phaseoffset').value = formatGenNumber(msg);
        break;
      case 'arb-func-check': {
        const val = msg.trim();
        // Only reflect the real selection if it's actually one of the
        // recognized arb names — if the channel is genuinely on
        // something else (e.g. still SIN), there's nothing real to
        // show yet. Nothing is ever sent from this case either way;
        // the dropdown just waits at its default for an actual pick.
        if (GEN_ARB_NAMES.has(val)) {
          document.getElementById('gen-ch' + ch + '-arb-select').value = val;
        }
        break;
      }
      case 'liveness':
        // The raw response text already logged itself via the shared
        // ws.onmessage dispatcher — this just labels it clearly as the
        // expected answer to the "(liveness check)" line logged when
        // the probe was sent, rather than a value that needs
        // explaining. markLinkAlive('funcgen') already ran in that same
        // shared dispatcher for any successful response, including
        // this one — nothing else to do here.
        appLog('funcgen', '(expected liveness reply)', 'log-sys');
        break;
      case 'scpi-manual':
        break;  // no specific UI field to update — the raw response
                 // already logged itself via the shared dispatcher; this
                 // entry exists purely so it's tracked in genQueryQueue
                 // like everything else
    }
  }

  // Updates a Mode-row button's (Mod/Sweep/Burst) displayed ON/OFF state
  // to reflect a queried value from the instrument. Distinct from
  // setGenModeState() (used for user-initiated clicks, which also sends
  // a command) — this one only updates the display. It DOES update
  // visibility and, if newly discovered as on, fetch that mode's
  // fields — matching the click path (selectGenMode), since visibility
  // now strictly follows actual on/off state (see updateGenModeVisibility).
  function setGenModeButtonDisplay(ch, mode, on) {
    const btn = document.getElementById('gen-ch' + ch + '-mode-' + mode);
    const label = mode.charAt(0).toUpperCase() + mode.slice(1);
    btn.textContent = label + ': ' + (on ? 'On' : 'Off');
    btn.dataset.on = on ? '1' : '0';
    btn.classList.toggle('active', on);
    updateGenModeVisibility(ch);
    if (on) refreshModeFields(ch, mode);
  }

  // Responses come back as plain integers or scientific notation
  // (e.g. "2.000000E-01", confirmed at the bench) — parseFloat handles
  // both natively. Re-formats to plain decimal for a cleaner editable
  // field rather than showing raw scientific notation.
  function formatGenNumber(raw) {
    const n = parseFloat(raw);
    return Number.isNaN(n) ? raw.trim() : String(n);
  }

  // Every enumerated (dropdown-backed) query response comes back
  // ABBREVIATED per the manual (e.g. "INT", "EXT", "LIN", "TRIG") — but
  // this panel's <select> elements use full spelled-out option values
  // ("INTernal", "LINear", "TRIGgered", etc.), since that's what a
  // long-form SET command reads most clearly in the HTML. Setting
  // .value to the raw abbreviated response never matches any <option>,
  // so the dropdown silently fails to reflect real state after a query
  // — this was a real, previously-unnoticed bug affecting Source,
  // Sweep Spacing, Sweep/Burst Trigger Source, Burst Mode, and Gate
  // Polarity. This normalizes the abbreviation back to the matching
  // long form before assigning .value.
  const GEN_SELECT_ABBR = {
    INT: 'INTernal', EXT: 'EXTernal', MAN: 'MANual',
    LIN: 'LINear', LOG: 'LOGarithmic', STE: 'STEp',
    TRIG: 'TRIGgered', GAT: 'GATed', INF: 'INFinity',
    NORM: 'NORMal', INV: 'INVerted',
    POS: 'POSitive', NEG: 'NEGative',
  };
  function normalizeGenSelectValue(raw) {
    const key = raw.trim().toUpperCase();
    return GEN_SELECT_ABBR[key] || raw.trim();
  }

  // ── Output — ONLY ever changes on explicit click, see file-level note ──
  function toggleGenOutput(ch) {
    genOutputState[ch] = !genOutputState[ch];
    genSend(':OUTP' + ch + ':STAT ' + (genOutputState[ch] ? 'ON' : 'OFF'));
    updateGenOutputBtn(ch);
  }
  function updateGenOutputBtn(ch) {
    const btn = document.getElementById('gen-output' + ch + '-btn');
    btn.classList.toggle('gen-output-on', genOutputState[ch]);
    const dot = document.getElementById('gen-ch' + ch + '-dot');
    if (dot) dot.classList.toggle('gen-ch-on', genOutputState[ch]);
    updateAlignPhaseAvailability();
  }

  // Align Phase only makes sense with both channels actually driving a
  // signal — grayed out unless Output 1 AND Output 2 are both ON.
  // Called whenever either channel's Output state changes, from a click
  // or from a query response — see updateGenOutputBtn() above.
  function updateAlignPhaseAvailability() {
    const bothOn = genOutputState[1] === true && genOutputState[2] === true;
    [1, 2].forEach(ch => {
      const btn = document.getElementById('gen-ch' + ch + '-alignphase-btn');
      if (btn) btn.disabled = !bothOn;
    });
  }

  // ── Function / Freq / Amplitude / Offset / Impedance ────────────────────
  // Switches which of the two mutually-exclusive views is shown below
  // the Function button row: 'normal' (Mode:/Type:/modifiers — used by
  // every Function except Harmonic, including Arb/User) or 'harmonic'.
  // Arb used to have its own third view here, but its dropdown now
  // lives directly under its own button (shown purely via CSS keyed off
  // that button's active class — see .gen-arb-btn-wrap), by explicit
  // request to decouple it from the shared dynamic area entirely.
  function showGenView(ch, view) {
    document.getElementById('gen-ch' + ch + '-type-and-modifiers').classList.toggle('gen-hidden', view !== 'normal');
    document.getElementById('gen-ch' + ch + '-harmonic-mock').classList.toggle('gen-hidden', view !== 'harmonic');
  }

  function selectGenFunc(ch, type) {
    document.querySelectorAll('#gen-ch' + ch + ' .gen-func-btn').forEach(b =>
      b.classList.toggle('active', b.dataset.func === type));
    genSend(':SOUR' + ch + ':FUNC ' + type);
    showGenView(ch, 'normal');
    const allowed = GEN_ALLOWED_MODES[type] || ['mod', 'sweep', 'burst'];
    turnOffDisallowedGenModes(ch, allowed);
    setGenModeRowAvailability(ch, allowed);
    updateGenWaveExtras(ch, type);
  }
  // Frequency can silently get clamped by the instrument's own limits —
  // confirmed at the bench: commanding above the 60 MHz ceiling clamps
  // to 60000000 and shows a transient "upper limit" message on the
  // instrument's own display that disappears after a moment. Without a
  // re-query afterward, this panel kept showing whatever was typed, not
  // what actually took effect — a real, confirmed discrepancy. Re-
  // queries Freq after setting it, using the same "set queues as an
  // inert 'ignore' entry, then queue the real follow-up query" pattern
  // as setGenAmpl() below, for the same ordering-safety reason (the
  // set command's own fabricated "OK" needs a slot too, or it gets
  // consumed as if it were the query's answer).
  function setGenFreq(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FREQ ' + document.getElementById('gen-ch' + ch + '-freq').value);
    queueGenQuery(ch, 'freq', ':SOUR' + ch + ':FREQ?');
    // Period is the inverse representation of Freq (Period = 1/Freq) —
    // treated the same way Ampl/Offset and High/Low keep each other in
    // sync: setting one re-queries the other. Not explicitly stated as
    // linked in the manual the way DCYCle/WIDTh are, but they're the
    // same underlying parameter expressed two ways, so the same
    // treatment applies.
    queueGenQuery(ch, 'period', ':SOUR' + ch + ':PERiod?');
    // Confirmed at the bench: Duty Cycle can auto-adjust to 40% or 60%
    // if Freq crosses the 10 MHz boundary while it was outside the
    // 40-60 range (the instrument's own allowed Duty Cycle window
    // narrows above 10 MHz — see updateGenWaveExtras()'s own history).
    // Only relevant while actually on Square.
    if (genCurrentFunc[ch] === 'SQU') {
      queueGenQuery(ch, 'squ-dutycycle', ':SOUR' + ch + ':FUNC:SQUare:DCYCle?');
    } else if (genCurrentFunc[ch] === 'PULSE') {
      // Same idea, longer chain (documented, not yet bench-confirmed):
      // Freq change -> period changes -> Width/Duty Cycle bounds
      // change -> Width itself may auto-adjust -> that can then force
      // the edge times to auto-adjust too (edge <= 0.625 x width). Not
      // worth trying to predict which actually moved — just re-check
      // all four.
      queueGenQuery(ch, 'pulse-dutycycle', ':SOUR' + ch + ':PULSe:DCYCle?');
      queueGenQuery(ch, 'pulse-width',     ':SOUR' + ch + ':PULSe:WIDTh?');
      queueGenQuery(ch, 'pulse-leading',   ':SOUR' + ch + ':PULSe:TRANsition:LEADing?');
      queueGenQuery(ch, 'pulse-trailing',  ':SOUR' + ch + ':PULSe:TRANsition:TRAiling?');
    }
  }
  // Same inverse-representation relationship as setGenFreq() above,
  // just setting the other side of the pair.
  function setGenPeriodCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PERiod ' + document.getElementById('gen-ch' + ch + '-period').value);
    queueGenQuery(ch, 'period', ':SOUR' + ch + ':PERiod?');
    queueGenQuery(ch, 'freq', ':SOUR' + ch + ':FREQ?');
  }
  // Amplitude has two mutually-exclusive representations on this
  // instrument — Vpp+Offset, or High/Low absolute levels — confirmed at
  // the bench that setting either automatically recalculates the other
  // pair. These four functions keep the panel in sync with that: each
  // queues a follow-up query for the OTHER representation after sending
  // its own set command — AND, just as importantly, re-queries ITSELF
  // too. That second part was a real, confirmed bug: all four limits
  // (Ampl, Offset, and by extension Max/Min) silently clamp when
  // exceeded, and without self-re-querying, the field you actually
  // typed into kept showing the invalid value forever — only the
  // OTHER pair would indirectly reflect the real clamped result,
  // making it impossible to see what actually happened from the field
  // you were looking at. Critically, the set command's own fabricated
  // "OK" response is ALSO queued (as an inert 'ignore' field) rather
  // than left unaccounted for — otherwise that OK would arrive first on
  // the wire and get incorrectly consumed as if it were the answer to
  // the first follow-up query, shifting every subsequent response by
  // one slot. This is exactly the ordering hazard the FIFO queue design
  // depends on getting right (see the Func Gen section's file header).
  function setGenAmpl(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':VOLT ' + document.getElementById('gen-ch' + ch + '-ampl').value);
    queueGenQuery(ch, 'ampl',   ':SOUR' + ch + ':VOLT?');
    queueGenQuery(ch, 'high',   ':SOUR' + ch + ':VOLT:HIGH?');
    queueGenQuery(ch, 'low',    ':SOUR' + ch + ':VOLT:LOW?');
  }
  function setGenOffset(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':VOLT:OFFS ' + document.getElementById('gen-ch' + ch + '-offset').value);
    queueGenQuery(ch, 'offset', ':SOUR' + ch + ':VOLT:OFFS?');
    queueGenQuery(ch, 'high',   ':SOUR' + ch + ':VOLT:HIGH?');
    queueGenQuery(ch, 'low',    ':SOUR' + ch + ':VOLT:LOW?');
  }
  function setGenHigh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':VOLT:HIGH ' + document.getElementById('gen-ch' + ch + '-high').value);
    queueGenQuery(ch, 'high',   ':SOUR' + ch + ':VOLT:HIGH?');
    queueGenQuery(ch, 'ampl',   ':SOUR' + ch + ':VOLT?');
    queueGenQuery(ch, 'offset', ':SOUR' + ch + ':VOLT:OFFS?');
  }
  function setGenLow(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':VOLT:LOW ' + document.getElementById('gen-ch' + ch + '-low').value);
    queueGenQuery(ch, 'low',    ':SOUR' + ch + ':VOLT:LOW?');
    queueGenQuery(ch, 'ampl',   ':SOUR' + ch + ':VOLT?');
    queueGenQuery(ch, 'offset', ':SOUR' + ch + ':VOLT:OFFS?');
  }
  // Imp doesn't clamp the same way (it has its own Min/Max quick
  // buttons rather than a hard numeric ceiling being silently
  // enforced), but the same self-re-query principle applies for
  // consistency and to correctly resolve the MIN/MAX/INF keyword
  // shortcuts to their real numeric result rather than showing the
  // keyword itself.
  function setGenImp(ch) {
    queueGenQuery(ch, 'ignore', ':OUTP' + ch + ':IMP ' + document.getElementById('gen-ch' + ch + '-imp').value);
    queueGenQuery(ch, 'imp', ':OUTP' + ch + ':IMP?');
  }
  function setGenImpKeyword(ch, kw) {
    queueGenQuery(ch, 'ignore', ':OUTP' + ch + ':IMP ' + kw);
    queueGenQuery(ch, 'imp', ':OUTP' + ch + ':IMP?');
  }

  // ── Mode: Mod / Sweep / Burst — mutually exclusive, inline per channel ──
  // Clicking any of the three Mode buttons toggles that mode's own
  // ON/OFF state (turning OFF whichever of the other two was ON, if
  // any — the instrument only runs one of these at a time) and shows
  // its Type selector + modifier rows if it just turned ON.
  //
  // Visibility strictly follows actual on/off state now — if NONE of
  // Mod/Sweep/Burst is on, the Type row and everything below it is
  // blank, not showing stale content from whichever was last viewed.
  // This reverses an earlier design (a separate genVisibleMode tracker
  // let you view a mode's fields even after turning it off) by
  // explicit later request, confirmed confusing at the bench — what's
  // on screen should match what's actually active, full stop.
  function selectGenMode(ch, mode) {
    const btn = document.getElementById('gen-ch' + ch + '-mode-' + mode);
    const turningOn = btn.dataset.on !== '1';
    if (turningOn) {
      ['mod', 'sweep', 'burst'].forEach(m => {
        if (m === mode) return;
        const otherBtn = document.getElementById('gen-ch' + ch + '-mode-' + m);
        if (otherBtn.dataset.on === '1') setGenModeState(ch, m, false);
      });
    } else if (mode === 'mod') {
      // Nothing to show — MOD:TYPE? isn't queried while Mod is off
      // anymore (see refreshBothGenChannels()'s own comment) — matches the
      // same blanking done on the query-response path (the 'mod' case
      // in applyGenField).
      document.getElementById('gen-ch' + ch + '-mod-typesel').value = '';
    }
    setGenModeState(ch, mode, turningOn);
    updateGenModeVisibility(ch);
    if (turningOn) refreshModeFields(ch, mode);  // nothing to fetch if
                                                    // turning off — its
                                                    // fields are about
                                                    // to be hidden anyway
  }

  function setGenModeState(ch, mode, on) {
    const btn = document.getElementById('gen-ch' + ch + '-mode-' + mode);
    btn.dataset.on = on ? '1' : '0';
    btn.classList.toggle('active', on);
    const label = mode.charAt(0).toUpperCase() + mode.slice(1);
    btn.textContent = label + ': ' + (on ? 'On' : 'Off');
    const cmdWord = { mod: 'MOD', sweep: 'SWEep', burst: 'BURSt' }[mode];
    genSend(':SOUR' + ch + ':' + cmdWord + ':STATe ' + (on ? 'ON' : 'OFF'));
  }

  // Shows the Type selector + modifier rows for whichever mode is
  // currently actually ON — blank if none is. See the file-level note
  // above for why this no longer tracks "last viewed" independently of
  // on/off state.
  function updateGenModeVisibility(ch) {
    const active = ['mod', 'sweep', 'burst'].find(m => {
      const b = document.getElementById('gen-ch' + ch + '-mode-' + m);
      return b.dataset.on === '1';
    }) || null;
    ['mod', 'sweep', 'burst'].forEach(m => {
      const typesel = document.getElementById('gen-ch' + ch + '-' + m + '-typesel');
      const modrows = document.getElementById('gen-ch' + ch + '-' + m + '-modrows');
      if (typesel) typesel.classList.toggle('gen-hidden', m !== active);
      if (modrows) modrows.classList.toggle('gen-hidden', m !== active);
    });
    updateSweepStepVisibility(ch);
    updateSweepTrigNowVisibility(ch);
    updateBurstTrigNowVisibility(ch);
    updateBurstGatePolVisibility(ch);
    updateBurstNonGatedFieldsVisibility(ch);
    updateBurstTrigEdgeVisibility(ch);
    updateModTypeVisibility(ch);
  }

  // Steps only means anything when Sweep is the active mode AND its
  // Type is specifically Step (Linear/Log sweeps don't use step count
  // at all) — by explicit request, shown right next to the Type
  // dropdown itself rather than as its own always-visible row with a
  // "only used when..." note. Called on every mode switch (Sweep
  // becoming active/inactive) and every place Sweep's Type can change
  // — a user pick or a passive query response discovering it.
  function updateSweepStepVisibility(ch) {
    const typesel = document.getElementById('gen-ch' + ch + '-sweep-typesel');
    const stepGroup = document.getElementById('gen-ch' + ch + '-sweep-step-group');
    if (!typesel || !stepGroup) return;
    const sweepActive = !typesel.classList.contains('gen-hidden');
    stepGroup.classList.toggle('gen-hidden', !(sweepActive && typesel.value === 'STEp'));
  }

  // Trigger Now only means anything when Trig Src is set to Manual —
  // by explicit request, hidden otherwise rather than always shown.
  // Called right after a user pick (setSweepTrigSrcCh) and when the
  // real value is discovered via a passive query response.
  function updateSweepTrigNowVisibility(ch) {
    const trigsrc = document.getElementById('gen-ch' + ch + '-sweep-trigsrc');
    const btn = document.getElementById('gen-ch' + ch + '-sweep-trignow-btn');
    if (!trigsrc || !btn) return;
    btn.classList.toggle('gen-hidden', trigsrc.value !== 'MANual');
  }

  // Same idea as Sweep's own version above, but by explicit request
  // also highlighted (not just shown) when Manual is selected — Burst's
  // Trigger Now is the one actually needed to make anything happen in
  // Manual mode, so the extra visual emphasis is warranted here in a
  // way it wasn't asked for on Sweep's.
  function updateBurstTrigNowVisibility(ch) {
    const trigsrc = document.getElementById('gen-ch' + ch + '-burst-trigsrc');
    const btn = document.getElementById('gen-ch' + ch + '-burst-trignow-btn');
    if (!trigsrc || !btn) return;
    const isManual = trigsrc.value === 'MANual';
    btn.classList.toggle('gen-hidden', !isManual);
    btn.classList.toggle('active', isManual);
  }

  // Gate Pol only means anything in Gated mode — by explicit request,
  // hidden otherwise. Called on a user pick (setBurstModeCh), a passive
  // query response discovering the real Type, and a mode switch
  // (Mod/Sweep -> Burst).
  function updateBurstGatePolVisibility(ch) {
    const typesel = document.getElementById('gen-ch' + ch + '-burst-typesel');
    const row = document.getElementById('gen-ch' + ch + '-burst-gatepol-row');
    if (!typesel || !row) return;
    const burstActive = !typesel.classList.contains('gen-hidden');
    row.classList.toggle('gen-hidden', !(burstActive && typesel.value === 'GATed'));
  }

  // Confirmed at the bench: in Gated mode, only Gate Pol and Phase
  // actually apply. N-Cycle and Infinite share Delay and Trig Src, but
  // diverge on the rest — Cycles and Period only mean anything in
  // N-Cycle (Infinite has no cycle count or internal re-trigger period
  // concept), and Infinite's own Trig Src drops Internal entirely
  // (nothing to space out on a timer when the burst never stops) —
  // leaving only External/Manual there, each still showing its own
  // SlopeIn/TrigOut exactly as before. Phase itself is never touched
  // here — it stays visible in every mode. Called on a user pick
  // (setBurstModeCh), a passive query response discovering the real
  // Type, and a mode switch (Mod/Sweep -> Burst).
  function updateBurstNonGatedFieldsVisibility(ch) {
    const typesel = document.getElementById('gen-ch' + ch + '-burst-typesel');
    if (!typesel) return;
    const burstActive = !typesel.classList.contains('gen-hidden');
    const isNCycle = typesel.value === 'TRIGgered';
    const isInfinite = typesel.value === 'INFinity';
    ['ncycles-row', 'period-row'].forEach(id => {
      const row = document.getElementById('gen-ch' + ch + '-burst-' + id);
      if (row) row.classList.toggle('gen-hidden', !(burstActive && isNCycle));
    });
    ['tdelay-row', 'trigsrc-row'].forEach(id => {
      const row = document.getElementById('gen-ch' + ch + '-burst-' + id);
      if (row) row.classList.toggle('gen-hidden', !(burstActive && (isNCycle || isInfinite)));
    });
    const intOpt = document.getElementById('gen-ch' + ch + '-burst-trigsrc-int-opt');
    if (intOpt) intOpt.hidden = burstActive && isInfinite;
  }

  // TrigOut and SlopeIn are mutually exclusive, and now depend on BOTH
  // Mode and Trig Src together, not Mode alone — confirmed at the bench
  // by watching the real front panel's soft buttons in Local mode:
  // Gate Pol shows only in Gated mode (unrelated to Trig Src); in
  // N-Cycle/Infinite mode, TrigOut shows UNLESS Trig Src is External,
  // in which case SlopeIn shows instead (the manual confirms SlopeIn's
  // own command, :BURSt:TRIGger:SLOPe, is "only available when external
  // trigger source is selected" — consistent with what was observed).
  // Called on a user pick of either Mode or Trig Src, a passive query
  // response discovering either real value, and a mode switch
  // (Mod/Sweep -> Burst).
  function updateBurstTrigEdgeVisibility(ch) {
    const typesel = document.getElementById('gen-ch' + ch + '-burst-typesel');
    const trigsrc = document.getElementById('gen-ch' + ch + '-burst-trigsrc');
    const trigoutGroup = document.getElementById('gen-ch' + ch + '-burst-trigout-group');
    const slopeinGroup = document.getElementById('gen-ch' + ch + '-burst-slopein-group');
    if (!typesel || !trigsrc || !trigoutGroup || !slopeinGroup) return;
    const burstActive = !typesel.classList.contains('gen-hidden');
    const modeOk = typesel.value === 'TRIGgered' || typesel.value === 'INFinity';
    const isExternal = trigsrc.value === 'EXTernal';
    trigoutGroup.classList.toggle('gen-hidden', !(burstActive && modeOk && !isExternal));
    slopeinGroup.classList.toggle('gen-hidden', !(burstActive && modeOk && isExternal));
  }

  // ── Harmonic — real waveform selection AND real sub-parameters ─────────
  // HARM is a confirmed valid FUNC:SHAPe keyword; the Harmonic subsystem
  // itself (:HARMonic:ORDEr/TYPe/AMPL/PHASe) is confirmed via the Rigol
  // DG4000 series manual. (The general :PHASe and :PHASe:INITiate
  // commands are also confirmed, but live in the always-visible core
  // block now — see setGenPhaseCh()/alignPhaseCh() — not here; Phase is
  // a property of the base/carrier waveform, not Harmonic-specific.)
  // Selecting Sine/Square/Ramp/Pulse/Noise switches back to the normal
  // Type:/modifiers area (see selectGenFunc).
  //
  // Mod/Sweep/Burst are NOT AVAILABLE for Harmonic (not merely off) —
  // the entire Mode row is hidden, rather than shown disabled with
  // "N/A" text (an earlier design, reversed by explicit later request —
  // see setGenModeRowAvailability()). Scope note: only Harmonic has
  // actually been confirmed as needing this treatment; Pulse and Noise
  // are currently treated as available (same as Sine/Square/Ramp/Arb)
  // since that hasn't been separately confirmed either way — worth
  // checking at the bench.
  //
  // AMPL/PHASe both need a harmonic-order index ("sn", 2-16) as their
  // first parameter — this UI has ONE Order field per channel, reused
  // as that index for whichever of AMPL/PHASe you click Set on. That's
  // a deliberate simplification standing in for "the real way to do
  // this," a full matrix of all 15 harmonics with independent controls
  // — not built yet, by explicit agreement.
  function selectGenHarmonic(ch) {
    document.querySelectorAll('#gen-ch' + ch + ' .gen-func-btn').forEach(b =>
      b.classList.toggle('active', b.dataset.func === 'HARM'));
    genSend(':SOUR' + ch + ':FUNC HARM');
    turnOffDisallowedGenModes(ch, GEN_ALLOWED_MODES.HARM);
    setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.HARM);
    showGenView(ch, 'harmonic');
    refreshHarmonicFields(ch);
    updateGenWaveExtras(ch, 'HARM');
  }

  // ── Arb — real selection among ~150 built-in named waveforms ───────────
  // (see the categorized dropdown in the HTML — confirmed via the
  // manual's full FUNC:SHAPe keyword list). Clicking the Arb button
  // sends :FUNC ARB — confirmed at the bench that this genuinely
  // switches the channel into Arb mode (mirroring the physical front
  // panel's own Arb button), distinct from directly naming a specific
  // waveform via :FUNC:SHAPe. Then queries the full :FUNC:SHAPe? form
  // (not the shorter :FUNC?) to find out which specific waveform that
  // switch landed on, and reflects it in the dropdown — see the
  // 'arb-func-check' case in applyGenField() for how the response is
  // handled. Earlier version queried instead of sending anything at
  // all, to avoid a different real bug (blindly sending the dropdown's
  // default value, which was silently switching the waveform to DC on
  // every click) — sending :FUNC ARB specifically doesn't have that
  // problem, since ARB is a mode switch, not a specific waveform name.
  // Picking a name from the dropdown afterward DOES send that selection
  // immediately (no separate Set button, matching the pattern already
  // used for Sweep Spacing/Burst Mode/Harmonic Type) — see
  // selectArbWaveCh() below. Unlike Harmonic, Mod/Sweep/Burst stay
  // available for Arb waveforms — the manual describes them as
  // layering on top of "the carrier waveform corresponding to the
  // function," and arb waveforms are just another selectable
  // FUNC:SHAPe value, not excluded the way Harmonic explicitly is.
  //
  // NOT implemented: creating/uploading a custom waveform (drawing your
  // own point data via the TRACe:DATA subsystem) — a genuinely
  // different, much larger feature than picking a name from this list.
  // Use the TOOLS tab for that in the meantime.
  function selectGenArbCh(ch) {
    document.querySelectorAll('#gen-ch' + ch + ' .gen-func-btn').forEach(b =>
      b.classList.toggle('active', b.dataset.func === 'ARB'));
    showGenView(ch, 'normal');
    turnOffDisallowedGenModes(ch, GEN_ALLOWED_MODES.ARB);
    setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.ARB);
    genSend(':SOUR' + ch + ':FUNC ARB');
    queueGenQuery(ch, 'arb-func-check', ':SOUR' + ch + ':FUNC:SHAPe?');
    updateGenWaveExtras(ch, 'ARB');
  }
  function selectArbWaveCh(ch) {
    const select = document.getElementById('gen-ch' + ch + '-arb-select');
    genSend(':SOUR' + ch + ':FUNC:SHAPe ' + select.value);
  }

  // ── User — a single reserved slot, not a picker ─────────────────────────
  // Confirmed at the bench: on this hardware, the User Key shortcut and
  // the Custom waveform slot are the same underlying mechanism — "USER"
  // as a literal SCPI keyword doesn't do anything here (this was
  // originally implemented per the manual's generic keyword list, which
  // documents USER and CUSTom as two separate FUNC:SHAPe values; that
  // didn't hold up on real hardware). The correct command is
  // :FUNC:SHAPe CUSTOM — confirmed both directions (SET and the query
  // response use the exact same spelling). Practical consequence: a
  // FUNC? query while the User Key is active reports back "CUSTOM" —
  // the 'func' case in applyGenField() specifically recognizes this and
  // highlights this button (not Arb's) when it happens, by explicit
  // request, even though the view shown is still the Arb dropdown with
  // Custom selected (an accurate reflection of the real selection,
  // just not literally "User" — that distinction doesn't exist at the
  // SCPI level on this instrument).
  function selectGenUserCh(ch) {
    document.querySelectorAll('#gen-ch' + ch + ' .gen-func-btn').forEach(b =>
      b.classList.toggle('active', b.dataset.func === 'USER'));
    genSend(':SOUR' + ch + ':FUNC:SHAPe CUSTOM');
    showGenView(ch, 'normal');
    turnOffDisallowedGenModes(ch, GEN_ALLOWED_MODES.USER);
    setGenModeRowAvailability(ch, GEN_ALLOWED_MODES.USER);
    updateGenWaveExtras(ch, 'CUSTOM');
  }

  // Which of Mod/Sweep/Burst each Function actually supports — confirmed
  // at the bench, not all-or-nothing. Noise and Harmonic support none;
  // Pulse specifically disallows Sweep only, while Mod and Burst work
  // fine on it; everything else (Sine/Square/Ramp/Arb/User) allows all
  // three. Falls back to all three for anything not listed here.
  const GEN_ALLOWED_MODES = {
    SIN:   ['mod', 'sweep', 'burst'],
    SQU:   ['mod', 'sweep', 'burst'],
    RAMP:  ['mod', 'sweep', 'burst'],
    PULSE: ['mod', 'burst'],           // Sweep NOT allowed — confirmed at the bench
    NOISE: [],                          // none allowed — confirmed at the bench
    ARB:   ['mod', 'sweep', 'burst'],   // confirmed at the bench
    USER:  ['mod', 'sweep', 'burst'],   // confirmed at the bench
    HARM:  [],                          // none allowed — confirmed at the bench
  };

  // Tracks which Function is currently active per channel — needed so
  // setGenFreq() can tell whether a Duty Cycle re-check applies (see
  // updateGenWaveExtras()'s own comment). Uses the same short codes as
  // GEN_ALLOWED_MODES (SIN/SQU/RAMP/PULSE/NOISE/ARB/USER/HARM); Arb and
  // User specifically use those placeholder codes rather than the exact
  // discovered waveform name, since nothing downstream cares which
  // specific name it is — only whether it's SQU/RAMP/NOISE or not.
  let genCurrentFunc = { 1: 'SIN', 2: 'SIN' };

  // Centralizes everything that depends on which Function is currently
  // active, called from every function-selecting path (click or passive
  // discovery) with whatever the real resulting value is:
  //
  // - Freq re-query: confirmed at the bench that switching Function can
  //   silently change Freq (e.g. Sine at 30 MHz -> Square clamps to its
  //   own 25 MHz ceiling; Square -> Ramp clamps further to 1 MHz) — so
  //   Freq needs re-checking every time Function actually changes. But
  //   during a passive refresh (isRefresh=true), nothing changed —
  //   refreshBothGenChannels() already queries Freq/Period separately,
  //   unconditionally, moments earlier in the same burst, so re-
  //   querying them again here was pure waste. Used to be accepted as
  //   unavoidable; isn't anymore — the caller now says which situation
  //   this is, so the redundant round-trip only happens when it's
  //   actually needed (a genuine Function switch), not on every refresh.
  // - Noise has no Freq or Phase concept at all — confirmed at the
  //   bench — so both rows hide entirely rather than showing
  //   meaningless values.
  // - Square has its own Duty Cycle, Ramp its own Symmetry — confirmed
  //   at the bench — each shown and queried only for its own waveform.
  function updateGenWaveExtras(ch, funcVal, isRefresh) {
    genCurrentFunc[ch] = funcVal;
    if (!isRefresh) {
      queueGenQuery(ch, 'freq',   ':SOUR' + ch + ':FREQ?');
      queueGenQuery(ch, 'period', ':SOUR' + ch + ':PERiod?');
    }

    const hideFreqPhase = (funcVal === 'NOISE');
    // Period now lives on the same row as Freq (see the HTML) — no
    // separate period-row element to toggle, freq-row covers both.
    document.getElementById('gen-ch' + ch + '-freq-row').classList.toggle('gen-hidden', hideFreqPhase);
    document.getElementById('gen-ch' + ch + '-phase-row').classList.toggle('gen-hidden', hideFreqPhase);

    const showDutyCycle = (funcVal === 'SQU');
    const showSymmetry  = (funcVal === 'RAMP');
    const showPulse     = (funcVal === 'PULSE');
    document.getElementById('gen-ch' + ch + '-squ-dutycycle-row').classList.toggle('gen-hidden', !showDutyCycle);
    document.getElementById('gen-ch' + ch + '-ramp-symmetry-row').classList.toggle('gen-hidden', !showSymmetry);
    document.getElementById('gen-ch' + ch + '-pulse-dutycycle-row').classList.toggle('gen-hidden', !showPulse);
    document.getElementById('gen-ch' + ch + '-pulse-edges-row').classList.toggle('gen-hidden', !showPulse);
    if (showDutyCycle) {
      document.getElementById('gen-ch' + ch + '-squ-dutycycle').value = '...';
      queueGenQuery(ch, 'squ-dutycycle', ':SOUR' + ch + ':FUNC:SQUare:DCYCle?');
    } else if (showSymmetry) {
      document.getElementById('gen-ch' + ch + '-ramp-symmetry').value = '...';
      queueGenQuery(ch, 'ramp-symmetry', ':SOUR' + ch + ':FUNC:RAMP:SYMMetry?');
    } else if (showPulse) {
      // Pulse uses its own dedicated :PULSe:... subsystem, NOT
      // :FUNCtion:PULSe:... like Square/Ramp's own extras — confirmed
      // in the manual, a genuinely different command path. Duty Cycle
      // and Width are directly linked (manual states this explicitly —
      // changing either auto-adjusts the other), and both edge times
      // are bounded by Width (edge <= 0.625 x width) — the Duty
      // Cycle/Width relationship is documented explicitly; the rest of
      // this cascade (down to the edges) is inferred from that, not
      // bench-confirmed yet, unlike Square's Freq<->Duty relationship.
      ['pulse-dutycycle', 'pulse-width', 'pulse-leading', 'pulse-trailing'].forEach(f => {
        document.getElementById('gen-ch' + ch + '-' + f).value = '...';
      });
      queueGenQuery(ch, 'pulse-dutycycle', ':SOUR' + ch + ':PULSe:DCYCle?');
      queueGenQuery(ch, 'pulse-width',     ':SOUR' + ch + ':PULSe:WIDTh?');
      queueGenQuery(ch, 'pulse-leading',   ':SOUR' + ch + ':PULSe:TRANsition:LEADing?');
      queueGenQuery(ch, 'pulse-trailing',  ':SOUR' + ch + ':PULSe:TRANsition:TRAiling?');
    }
  }

  function setGenDutyCycleCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FUNC:SQUare:DCYCle ' + document.getElementById('gen-ch' + ch + '-squ-dutycycle').value);
    queueGenQuery(ch, 'squ-dutycycle', ':SOUR' + ch + ':FUNC:SQUare:DCYCle?');
  }
  function setGenSymmetryCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FUNC:RAMP:SYMMetry ' + document.getElementById('gen-ch' + ch + '-ramp-symmetry').value);
    queueGenQuery(ch, 'ramp-symmetry', ':SOUR' + ch + ':FUNC:RAMP:SYMMetry?');
  }
  // Pulse's own dedicated :PULSe:... subsystem — see updateGenWaveExtras()'s
  // comment for the full cross-field picture. Duty Cycle and Width are
  // directly linked (manual states this explicitly) — setting either
  // one can move the other, which can in turn force the edge times to
  // auto-adjust (edge <= 0.625 x width), so all three other fields get
  // re-checked from each of these two. Leading/Trailing only constrain
  // themselves in the other direction — no evidence they feed back into
  // Duty Cycle or Width, so those two only re-query themselves.
  function setGenPulseDutyCycleCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PULSe:DCYCle ' + document.getElementById('gen-ch' + ch + '-pulse-dutycycle').value);
    queueGenQuery(ch, 'pulse-dutycycle', ':SOUR' + ch + ':PULSe:DCYCle?');
    queueGenQuery(ch, 'pulse-width',    ':SOUR' + ch + ':PULSe:WIDTh?');
    queueGenQuery(ch, 'pulse-leading',  ':SOUR' + ch + ':PULSe:TRANsition:LEADing?');
    queueGenQuery(ch, 'pulse-trailing', ':SOUR' + ch + ':PULSe:TRANsition:TRAiling?');
  }
  function setGenPulseWidthCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PULSe:WIDTh ' + document.getElementById('gen-ch' + ch + '-pulse-width').value);
    queueGenQuery(ch, 'pulse-width',    ':SOUR' + ch + ':PULSe:WIDTh?');
    queueGenQuery(ch, 'pulse-dutycycle', ':SOUR' + ch + ':PULSe:DCYCle?');
    queueGenQuery(ch, 'pulse-leading',  ':SOUR' + ch + ':PULSe:TRANsition:LEADing?');
    queueGenQuery(ch, 'pulse-trailing', ':SOUR' + ch + ':PULSe:TRANsition:TRAiling?');
  }
  function setGenPulseLeadingCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PULSe:TRANsition:LEADing ' + document.getElementById('gen-ch' + ch + '-pulse-leading').value);
    queueGenQuery(ch, 'pulse-leading', ':SOUR' + ch + ':PULSe:TRANsition:LEADing?');
  }
  function setGenPulseTrailingCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PULSe:TRANsition:TRAiling ' + document.getElementById('gen-ch' + ch + '-pulse-trailing').value);
    queueGenQuery(ch, 'pulse-trailing', ':SOUR' + ch + ':PULSe:TRANsition:TRAiling?');
  }

  // Turns off any of Mod/Sweep/Burst that's currently on but no longer
  // allowed for the Function just selected (e.g. switching from Sine,
  // with Sweep on, to Pulse — Sweep isn't valid there and needs to
  // actually be turned off, not just hidden while still running).
  function turnOffDisallowedGenModes(ch, allowedModes) {
    ['mod', 'sweep', 'burst'].forEach(m => {
      if (allowedModes.includes(m)) return;
      const btn = document.getElementById('gen-ch' + ch + '-mode-' + m);
      if (btn && btn.dataset.on === '1') setGenModeState(ch, m, false);
    });
  }

  // Hides the entire Mode row when NO mode is supported at all (Noise,
  // Harmonic) — simpler than a row of permanently-disabled buttons, by
  // earlier explicit request. For a partial case like Pulse, the row
  // stays visible but only the specifically-disallowed button (Sweep)
  // is hidden, leaving Mod/Burst usable.
  function setGenModeRowAvailability(ch, allowedModes) {
    const row = document.getElementById('gen-ch' + ch + '-mode-row');
    const anyAllowed = allowedModes.length > 0;
    if (row) row.classList.toggle('gen-hidden', !anyAllowed);
    ['mod', 'sweep', 'burst'].forEach(m => {
      const btn = document.getElementById('gen-ch' + ch + '-mode-' + m);
      if (btn) btn.classList.toggle('gen-hidden', !allowedModes.includes(m));
    });
  }

  // Queries Order/Type/Start Phase, and Ampl/Phase Offset for a default
  // harmonic index (2 — the manual's own stated default for both AMPL
  // and PHASe's <sn> parameter) so the panel shows real values instead
  // of blanks the moment Harmonic is selected. Genuinely dynamic
  // chaining (query Order first, then use ITS fresh value as the index
  // for Ampl/Phase) isn't implemented — see the Order-as-index
  // simplification note above.
  function refreshHarmonicFields(ch) {
    ['harm-ampl', 'harm-phaseoffset'].forEach(f => {
      document.getElementById('gen-ch' + ch + '-' + f).value = '...';
    });
    const n = document.getElementById('gen-ch' + ch + '-harm-index').value;  // whatever the dropdown currently shows
    queueGenQuery(ch, 'harm-order',       ':SOUR' + ch + ':HARMonic:ORDEr?');
    queueGenQuery(ch, 'harm-type',        ':SOUR' + ch + ':HARMonic:TYPe?');
    queueGenQuery(ch, 'harm-ampl',        ':SOUR' + ch + ':HARMonic:AMPL? ' + n);
    queueGenQuery(ch, 'harm-phaseoffset', ':SOUR' + ch + ':HARMonic:PHASe? ' + n);
  }

  // Picking a harmonic number does two things at once: sends the real
  // :HARMonic:ORDEr command (this dropdown IS the Order control now,
  // no separate row), AND re-queries Ampl/Phase for that specific
  // number so the boxes always show what's actually stored for
  // whichever harmonic is currently selected — not stale values left
  // over from a previous selection.
  function selectHarmIndexCh(ch) {
    const n = document.getElementById('gen-ch' + ch + '-harm-index').value;
    genSend(':SOUR' + ch + ':HARMonic:ORDEr ' + n);
    document.getElementById('gen-ch' + ch + '-harm-ampl').value = '...';
    document.getElementById('gen-ch' + ch + '-harm-phaseoffset').value = '...';
    queueGenQuery(ch, 'harm-ampl',        ':SOUR' + ch + ':HARMonic:AMPL? ' + n);
    queueGenQuery(ch, 'harm-phaseoffset', ':SOUR' + ch + ':HARMonic:PHASe? ' + n);
  }
  function selectHarmTypeCh(ch, type) {
    document.querySelectorAll('#gen-ch' + ch + ' .gen-harmtype-btn').forEach(b =>
      b.classList.toggle('active', b.dataset.harmtype === type));
    genSend(':SOUR' + ch + ':HARMonic:TYPe ' + type);
    updateHarmIndexOptionsCh(ch, type);
  }

  // Real bench-observed problem this fixes: with Type=ODD selected,
  // trying to set Order to an even number (e.g. 2) makes the
  // instrument show an error and silently not change anything — a
  // confusing "the dropdown let me pick this, but it didn't work"
  // trap. Rather than validate after the fact, the dropdown itself now
  // only ever offers numbers valid for the current Type (EVEN → even
  // numbers, ODD → odd numbers, ALL/USER → the full 2-16 range), so an
  // invalid combination can't be selected in the first place. Called
  // whenever Type changes, whether from a click or discovered passively
  // via a query response.
  function updateHarmIndexOptionsCh(ch, type) {
    const select = document.getElementById('gen-ch' + ch + '-harm-index');
    const current = select.value;
    let values;
    if (type === 'EVEN') {
      values = [2, 4, 6, 8, 10, 12, 14, 16];
    } else if (type === 'ODD') {
      values = [3, 5, 7, 9, 11, 13, 15];
    } else {
      // ALL, USER, or Type not yet known — full range, no restriction.
      values = [];
      for (let i = 2; i <= 16; i++) values.push(i);
    }
    select.innerHTML = values.map(v => '<option value="' + v + '">' + v + '</option>').join('');
    if (values.includes(Number(current))) {
      select.value = current;  // still valid under the new Type — keep it
    } else {
      // No longer valid (or nothing was selected yet) — fall back to
      // the first valid option and re-sync the instrument/fields to
      // match, same as if the user had picked it themselves.
      select.value = String(values[0]);
      selectHarmIndexCh(ch);
    }
  }

  // Phase is a general channel-level property of the base/carrier
  // waveform (:SOUR<n>:PHASe) — not Harmonic-specific, despite the
  // function name's history (it used to live inside the Harmonic view,
  // labeled "Start Phase" then "Fundamental Phase" along the way).
  // Moved to the always-visible core block per explicit request — it
  // appears on the instrument's own main display regardless of which
  // waveform/mode is active, so it belongs there in this panel too.
  // Re-queries itself after setting — confirmed at the bench that Phase
  // also silently clamps at its own limits, same as Ampl/Offset/Max/
  // Min, so the field needs correcting to the real result rather than
  // trusting whatever was typed.
  function setGenPhaseCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':PHASe ' + document.getElementById('gen-ch' + ch + '-phase').value);
    queueGenQuery(ch, 'phase', ':SOUR' + ch + ':PHASe?');
  }
  function setHarmAmplCh(ch) {
    const n = document.getElementById('gen-ch' + ch + '-harm-index').value;
    const val = document.getElementById('gen-ch' + ch + '-harm-ampl').value;
    genSend(':SOUR' + ch + ':HARMonic:AMPL ' + n + ',' + val);
  }
  function setHarmPhaseOffsetCh(ch) {
    const n = document.getElementById('gen-ch' + ch + '-harm-index').value;
    const val = document.getElementById('gen-ch' + ch + '-harm-phaseoffset').value;
    genSend(':SOUR' + ch + ':HARMonic:PHASe ' + n + ',' + val);
  }
  // Ampl and Phase live on one combined row now (single Set button) —
  // AMPL and PHASe are still two separate SCPI commands under the
  // hood, so this just fires both individual functions back to back
  // rather than duplicating their logic.
  function setHarmAmplPhaseCh(ch) {
    setHarmAmplCh(ch);
    setHarmPhaseOffsetCh(ch);
  }
  function alignPhaseCh(ch) {
    // Guard even though the button is already disabled via the UI —
    // cheap insurance against anything calling this programmatically.
    // Manual also notes this is invalid while either channel is in
    // modulation mode — not separately checked here, only the Output
    // gate you specifically asked for (see updateAlignPhaseAvailability).
    if (genOutputState[1] !== true || genOutputState[2] !== true) return;
    // NOTE: the manual documents :PHASe:INITiate ("Execute align
    // phase"); a different command (:PHASe:SYNC) was also suggested as
    // possibly needed depending on firmware revision — flagged, not yet
    // resolved. Using the manual's documented form here; swap/add
    // :PHASe:SYNC if the bench shows INITiate doesn't do it.
    genSend(':SOUR' + ch + ':PHASe:INITiate');
  }

  // ── Modulation Type (Type: row when Mod is the visible mode) ───────────
  function selectGenModType(ch) {
    const type = document.getElementById('gen-ch' + ch + '-mod-typesel').value;
    genSend(':SOUR' + ch + ':MOD:TYPE ' + type);
    updateModTypeVisibility(ch);
    queueModTypeSubParams(ch, type);
  }

  // ── AM sub-parameters — confirmed via the Rigol DG4000 series manual
  // and bench testing (including DSSC, whose command isn't in the
  // manual at all — found empirically). Only meaningful when Type = AM
  // is selected; shown regardless (same "some fields may be inert
  // depending on mode" pattern already used for Burst's Count/Gate
  // Polarity). Mod Freq and Shape are additionally gated on Source —
  // the manual states Shape "is only available when internal
  // modulation source is selected," and Mod Freq (Internal Frequency)
  // is the same by name — confirmed as a real point of confusion at
  // the bench (trying to set either while Source=External looks like
  // nothing happens, since there's nothing for them to affect). See
  // updateModAmSourceGrayingCh() below.
  function setModAmSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:AM:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-am-source').value);
  }
  // Grays out Mod Freq/Shape (input + Set button) when Source is
  // External — they have no effect in that mode. Called on every local
  // dropdown change (before the Set button is even clicked, so the
  // graying previews immediately) and after a real Source value comes
  // back from a query (see the 'mod-am-source' case in applyGenField),
  // so it also reflects true instrument state on connect/refresh.
  function updateModAmSourceGrayingCh(ch) {
    const isExternal = document.getElementById('gen-ch' + ch + '-mod-am-source').value === 'EXTernal';
    document.getElementById('gen-ch' + ch + '-mod-am-freq').disabled = isExternal;
    document.getElementById('gen-ch' + ch + '-mod-am-freq-set').disabled = isExternal;
    document.getElementById('gen-ch' + ch + '-mod-am-shape').disabled = isExternal;
    document.getElementById('gen-ch' + ch + '-mod-am-shape-set').disabled = isExternal;
  }
  function setModAmFreqCh(ch) {
    genSend(':SOUR' + ch + ':MOD:AM:INTernal:FREQuency ' + document.getElementById('gen-ch' + ch + '-mod-am-freq').value);
  }
  function setModAmShapeCh(ch) {
    genSend(':SOUR' + ch + ':MOD:AM:INTernal:FUNCtion ' + document.getElementById('gen-ch' + ch + '-mod-am-shape').value);
  }
  function setModAmDepthCh(ch) {
    genSend(':SOUR' + ch + ':MOD:AM ' + document.getElementById('gen-ch' + ch + '-mod-am-depth').value);
  }
  function toggleModAmDsscCh(ch) {
    // Confirmed at the bench: :SOUR<n>:AM:DSSC ON|OFF — notably NOT
    // nested under :MOD:AM: like the other AM sub-parameters above, its
    // own top-level :AM:DSSC node instead.
    const btn = document.getElementById('gen-ch' + ch + '-mod-am-dssc-btn');
    const nowOn = btn.dataset.on !== '1';
    btn.textContent = btn.dataset.label + ': ' + (nowOn ? 'ON' : 'OFF');
    btn.dataset.on = nowOn ? '1' : '0';
    genSend(':SOUR' + ch + ':AM:DSSC ' + (nowOn ? 'ON' : 'OFF'));
  }

  // Which of the 12 Type options actually have their own row-block
  // built — AM through OSK below, in the same order as the manual's
  // own command groupings. FM and PWM are deliberately not in this
  // list yet (out of scope for this pass, by explicit request) — their
  // Type selection still works via the dropdown itself, they just have
  // no sub-parameter block to show.
  const GEN_MOD_TYPES_WITH_ROWS = ['am', 'pm', 'ask', 'fsk', 'psk', 'bpsk', 'qpsk', '3fsk', '4fsk', 'osk'];

  // Shows only the block matching Type's current value, hides the rest
  // — same mutual-exclusion pattern as Sweep/Burst's own Type-driven
  // rows. Called on a user pick (selectGenModType), a passive query
  // response discovering the real Type (case 'modtype'), and a mode
  // switch (Sweep/Burst -> Mod).
  function updateModTypeVisibility(ch) {
    const typesel = document.getElementById('gen-ch' + ch + '-mod-typesel');
    if (!typesel) return;
    const modActive = !typesel.classList.contains('gen-hidden');
    const current = typesel.value.toLowerCase();
    GEN_MOD_TYPES_WITH_ROWS.forEach(t => {
      const block = document.getElementById('gen-ch' + ch + '-mod-' + t + '-rows');
      if (block) block.classList.toggle('gen-hidden', !(modActive && current === t));
    });
  }

  // Dispatches to the correct set of sub-parameter queries for
  // whichever Type is actually active — called from selectGenModType()
  // (user pick, immediate) and the 'modtype' response case (passive
  // discovery, on Refresh or a mode switch). Every command below is
  // taken directly from the manual's own SOURce:MOD command group, one
  // file per parameter, not inferred from AM's shape.
  function queueModTypeSubParams(ch, type) {
    const t = type.toUpperCase();
    if (t === 'AM') {
      ['mod-am-freq', 'mod-am-depth'].forEach(f => {
        document.getElementById('gen-ch' + ch + '-' + f).value = '...';
      });
      queueGenQuery(ch, 'mod-am-source', ':SOUR' + ch + ':MOD:AM:SOURce?');
      queueGenQuery(ch, 'mod-am-freq',   ':SOUR' + ch + ':MOD:AM:INTernal:FREQuency?');
      queueGenQuery(ch, 'mod-am-shape',  ':SOUR' + ch + ':MOD:AM:INTernal:FUNCtion?');
      queueGenQuery(ch, 'mod-am-depth',  ':SOUR' + ch + ':MOD:AM?');
      queueGenQuery(ch, 'mod-am-dssc',   ':SOUR' + ch + ':AM:DSSC?');
    } else if (t === 'PM') {
      queueGenQuery(ch, 'mod-pm-source',    ':SOUR' + ch + ':MOD:PM:SOURce?');
      queueGenQuery(ch, 'mod-pm-freq',      ':SOUR' + ch + ':MOD:PM:INTernal:FREQuency?');
      queueGenQuery(ch, 'mod-pm-shape',     ':SOUR' + ch + ':MOD:PM:INTernal:FUNCtion?');
      queueGenQuery(ch, 'mod-pm-deviation', ':SOUR' + ch + ':MOD:PM:DEViation?');
    } else if (t === 'ASK') {
      queueGenQuery(ch, 'mod-ask-source',   ':SOUR' + ch + ':MOD:ASKey:SOURce?');
      queueGenQuery(ch, 'mod-ask-rate',     ':SOUR' + ch + ':MOD:ASKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-ask-ampl',     ':SOUR' + ch + ':MOD:ASKey:AMPLitude?');
      queueGenQuery(ch, 'mod-ask-polarity', ':SOUR' + ch + ':MOD:ASKey:POLarity?');
    } else if (t === 'FSK') {
      queueGenQuery(ch, 'mod-fsk-source',   ':SOUR' + ch + ':MOD:FSKey:SOURce?');
      queueGenQuery(ch, 'mod-fsk-rate',     ':SOUR' + ch + ':MOD:FSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-fsk-hopfreq',  ':SOUR' + ch + ':MOD:FSKey?');
      queueGenQuery(ch, 'mod-fsk-polarity', ':SOUR' + ch + ':MOD:FSKey:POLarity?');
    } else if (t === 'PSK') {
      queueGenQuery(ch, 'mod-psk-source',   ':SOUR' + ch + ':MOD:PSKey:SOURce?');
      queueGenQuery(ch, 'mod-psk-rate',     ':SOUR' + ch + ':MOD:PSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-psk-phase',    ':SOUR' + ch + ':MOD:PSKey:PHASe?');
      queueGenQuery(ch, 'mod-psk-polarity', ':SOUR' + ch + ':MOD:PSKey:POLarity?');
    } else if (t === 'BPSK') {
      // Manual-confirmed: BPSK has no Source or Polarity command at
      // all — Data (01/10/PN15/PN21) is the only selector, unlike
      // ASK/FSK/PSK above.
      queueGenQuery(ch, 'mod-bpsk-rate',  ':SOUR' + ch + ':MOD:BPSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-bpsk-phase', ':SOUR' + ch + ':MOD:BPSKey:PHASe?');
      queueGenQuery(ch, 'mod-bpsk-data',  ':SOUR' + ch + ':MOD:BPSKey:DATA?');
    } else if (t === 'QPSK') {
      // Same as BPSK, three phases instead of one, Data limited to
      // PN15/PN21 (no 01/10) — both confirmed in the manual.
      queueGenQuery(ch, 'mod-qpsk-rate',   ':SOUR' + ch + ':MOD:QPSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-qpsk-phase1', ':SOUR' + ch + ':MOD:QPSKey:PHASe1?');
      queueGenQuery(ch, 'mod-qpsk-phase2', ':SOUR' + ch + ':MOD:QPSKey:PHASe2?');
      queueGenQuery(ch, 'mod-qpsk-phase3', ':SOUR' + ch + ':MOD:QPSKey:PHASe3?');
      queueGenQuery(ch, 'mod-qpsk-data',   ':SOUR' + ch + ':MOD:QPSKey:DATA?');
    } else if (t === '3FSK') {
      // Manual: ":MOD:3FSKey[:FREQuency]? <n>" — n=1,2 (the base Freq
      // field is the carrier itself, not part of this indexed pair).
      // Not bench-confirmed whether the query needs the index passed
      // as shown here or some other form — worth checking at the bench
      // alongside everything else in this batch.
      queueGenQuery(ch, 'mod-3fsk-rate',  ':SOUR' + ch + ':MOD:3FSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-3fsk-freq1', ':SOUR' + ch + ':MOD:3FSKey? 1');
      queueGenQuery(ch, 'mod-3fsk-freq2', ':SOUR' + ch + ':MOD:3FSKey? 2');
    } else if (t === '4FSK') {
      // Same idea, n=1,2,3.
      queueGenQuery(ch, 'mod-4fsk-rate',  ':SOUR' + ch + ':MOD:4FSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-4fsk-freq1', ':SOUR' + ch + ':MOD:4FSKey? 1');
      queueGenQuery(ch, 'mod-4fsk-freq2', ':SOUR' + ch + ':MOD:4FSKey? 2');
      queueGenQuery(ch, 'mod-4fsk-freq3', ':SOUR' + ch + ':MOD:4FSKey? 3');
    } else if (t === 'OSK') {
      queueGenQuery(ch, 'mod-osk-source', ':SOUR' + ch + ':MOD:OSKey:SOURce?');
      queueGenQuery(ch, 'mod-osk-rate',   ':SOUR' + ch + ':MOD:OSKey:INTernal:RATE?');
      queueGenQuery(ch, 'mod-osk-time',   ':SOUR' + ch + ':MOD:OSKey:TIME?');
    }
    // FM and PWM deliberately not handled — no row-block built for
    // them yet, so nothing to populate.
  }

  // Generic version of updateModAmSourceGrayingCh() above, for the new
  // Source-having types — grays out whichever of Freq/Shape or Rate is
  // "Internal source only" (per the manual, same wording repeated for
  // each of these) whenever Source is set to External.
  function updateModSourceGrayingCh(ch, type) {
    const src = document.getElementById('gen-ch' + ch + '-mod-' + type + '-source');
    if (!src) return;
    const isExternal = src.value === 'EXTernal';
    ['freq', 'freq-set', 'rate', 'rate-set', 'shape', 'shape-set'].forEach(suffix => {
      const el = document.getElementById('gen-ch' + ch + '-mod-' + type + '-' + suffix);
      if (el) el.disabled = isExternal;
    });
  }

  // ── PM sub-parameters — mirrors AM's own structure exactly (manual
  // confirms the same Source/INTernal:FREQuency/INTernal:FUNCtion shape),
  // just Deviation (0-360 deg) in place of Depth. ──────────────────────
  function setModPmSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PM:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-pm-source').value);
  }
  function setModPmFreqCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PM:INTernal:FREQuency ' + document.getElementById('gen-ch' + ch + '-mod-pm-freq').value);
  }
  function setModPmShapeCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PM:INTernal:FUNCtion ' + document.getElementById('gen-ch' + ch + '-mod-pm-shape').value);
  }
  function setModPmDeviationCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PM:DEViation ' + document.getElementById('gen-ch' + ch + '-mod-pm-deviation').value);
  }

  // ── ASK sub-parameters ───────────────────────────────────────────────
  function setModAskSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:ASKey:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-ask-source').value);
  }
  function setModAskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:ASKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-ask-rate').value);
  }
  function setModAskAmplCh(ch) {
    genSend(':SOUR' + ch + ':MOD:ASKey:AMPLitude ' + document.getElementById('gen-ch' + ch + '-mod-ask-ampl').value);
  }
  function setModAskPolarityCh(ch) {
    genSend(':SOUR' + ch + ':MOD:ASKey:POLarity ' + document.getElementById('gen-ch' + ch + '-mod-ask-polarity').value);
  }

  // ── FSK sub-parameters ───────────────────────────────────────────────
  function setModFskSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:FSKey:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-fsk-source').value);
  }
  function setModFskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:FSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-fsk-rate').value);
  }
  function setModFskHopFreqCh(ch) {
    genSend(':SOUR' + ch + ':MOD:FSKey ' + document.getElementById('gen-ch' + ch + '-mod-fsk-hopfreq').value);
  }
  function setModFskPolarityCh(ch) {
    genSend(':SOUR' + ch + ':MOD:FSKey:POLarity ' + document.getElementById('gen-ch' + ch + '-mod-fsk-polarity').value);
  }

  // ── PSK sub-parameters ───────────────────────────────────────────────
  function setModPskSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PSKey:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-psk-source').value);
  }
  function setModPskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-psk-rate').value);
  }
  function setModPskPhaseCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PSKey:PHASe ' + document.getElementById('gen-ch' + ch + '-mod-psk-phase').value);
  }
  function setModPskPolarityCh(ch) {
    genSend(':SOUR' + ch + ':MOD:PSKey:POLarity ' + document.getElementById('gen-ch' + ch + '-mod-psk-polarity').value);
  }

  // ── BPSK sub-parameters ──────────────────────────────────────────────
  function setModBpskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:BPSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-bpsk-rate').value);
  }
  function setModBpskPhaseCh(ch) {
    genSend(':SOUR' + ch + ':MOD:BPSKey:PHASe ' + document.getElementById('gen-ch' + ch + '-mod-bpsk-phase').value);
  }
  function setModBpskDataCh(ch) {
    genSend(':SOUR' + ch + ':MOD:BPSKey:DATA ' + document.getElementById('gen-ch' + ch + '-mod-bpsk-data').value);
  }

  // ── QPSK sub-parameters ──────────────────────────────────────────────
  function setModQpskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:QPSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-qpsk-rate').value);
  }
  function setModQpskPhase1Ch(ch) {
    genSend(':SOUR' + ch + ':MOD:QPSKey:PHASe1 ' + document.getElementById('gen-ch' + ch + '-mod-qpsk-phase1').value);
  }
  function setModQpskPhase2Ch(ch) {
    genSend(':SOUR' + ch + ':MOD:QPSKey:PHASe2 ' + document.getElementById('gen-ch' + ch + '-mod-qpsk-phase2').value);
  }
  function setModQpskPhase3Ch(ch) {
    genSend(':SOUR' + ch + ':MOD:QPSKey:PHASe3 ' + document.getElementById('gen-ch' + ch + '-mod-qpsk-phase3').value);
  }
  function setModQpskDataCh(ch) {
    genSend(':SOUR' + ch + ':MOD:QPSKey:DATA ' + document.getElementById('gen-ch' + ch + '-mod-qpsk-data').value);
  }

  // ── 3FSK / 4FSK sub-parameters — indexed hop frequencies, manual
  // syntax "<n>,<frequency>" for the set command. Not bench-confirmed
  // yet, same caveat as the query side in queueModTypeSubParams(). ────
  function setMod3fskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:3FSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-3fsk-rate').value);
  }
  function setMod3fskFreqCh(ch, n) {
    genSend(':SOUR' + ch + ':MOD:3FSKey ' + n + ',' + document.getElementById('gen-ch' + ch + '-mod-3fsk-freq' + n).value);
  }
  function setMod4fskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:4FSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-4fsk-rate').value);
  }
  function setMod4fskFreqCh(ch, n) {
    genSend(':SOUR' + ch + ':MOD:4FSKey ' + n + ',' + document.getElementById('gen-ch' + ch + '-mod-4fsk-freq' + n).value);
  }

  // ── OSK sub-parameters ───────────────────────────────────────────────
  function setModOskSourceCh(ch) {
    genSend(':SOUR' + ch + ':MOD:OSKey:SOURce ' + document.getElementById('gen-ch' + ch + '-mod-osk-source').value);
  }
  function setModOskRateCh(ch) {
    genSend(':SOUR' + ch + ':MOD:OSKey:INTernal:RATE ' + document.getElementById('gen-ch' + ch + '-mod-osk-rate').value);
  }
  function setModOskTimeCh(ch) {
    genSend(':SOUR' + ch + ':MOD:OSKey:TIME ' + document.getElementById('gen-ch' + ch + '-mod-osk-time').value);
  }

  // ── Sweep — Type: row (Spacing) + modifier rows, per channel ───────────
  function setSweepSpacingCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:SPACing ' + document.getElementById('gen-ch' + ch + '-sweep-typesel').value);
    updateSweepStepVisibility(ch);
  }
  // Start, Stop, Center, and Span are four ways of describing the same
  // underlying sweep range — confirmed in the manual: center = (start +
  // stop) / 2, span = stop - start. Same relationship as Ampl/Offset
  // and High/Low elsewhere in this panel, so treated the same way: each
  // Set re-queries itself (in case of clamping) plus the other three,
  // keeping all four in sync regardless of which one was actually
  // changed.
  function setSweepStartFreqCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FREQuency:STARt ' + document.getElementById('gen-ch' + ch + '-sweep-startfreq').value);
    queueGenQuery(ch, 'sweep-startfreq',  ':SOUR' + ch + ':FREQuency:STARt?');
    queueGenQuery(ch, 'sweep-stopfreq',   ':SOUR' + ch + ':FREQuency:STOP?');
    queueGenQuery(ch, 'sweep-centerfreq', ':SOUR' + ch + ':FREQuency:CENTer?');
    queueGenQuery(ch, 'sweep-spanfreq',   ':SOUR' + ch + ':FREQuency:SPAN?');
  }
  function setSweepStopFreqCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FREQuency:STOP ' + document.getElementById('gen-ch' + ch + '-sweep-stopfreq').value);
    queueGenQuery(ch, 'sweep-stopfreq',   ':SOUR' + ch + ':FREQuency:STOP?');
    queueGenQuery(ch, 'sweep-startfreq',  ':SOUR' + ch + ':FREQuency:STARt?');
    queueGenQuery(ch, 'sweep-centerfreq', ':SOUR' + ch + ':FREQuency:CENTer?');
    queueGenQuery(ch, 'sweep-spanfreq',   ':SOUR' + ch + ':FREQuency:SPAN?');
  }
  function setSweepCenterFreqCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FREQuency:CENTer ' + document.getElementById('gen-ch' + ch + '-sweep-centerfreq').value);
    queueGenQuery(ch, 'sweep-centerfreq', ':SOUR' + ch + ':FREQuency:CENTer?');
    queueGenQuery(ch, 'sweep-startfreq',  ':SOUR' + ch + ':FREQuency:STARt?');
    queueGenQuery(ch, 'sweep-stopfreq',   ':SOUR' + ch + ':FREQuency:STOP?');
    queueGenQuery(ch, 'sweep-spanfreq',   ':SOUR' + ch + ':FREQuency:SPAN?');
  }
  function setSweepSpanFreqCh(ch) {
    queueGenQuery(ch, 'ignore', ':SOUR' + ch + ':FREQuency:SPAN ' + document.getElementById('gen-ch' + ch + '-sweep-spanfreq').value);
    queueGenQuery(ch, 'sweep-spanfreq',   ':SOUR' + ch + ':FREQuency:SPAN?');
    queueGenQuery(ch, 'sweep-startfreq',  ':SOUR' + ch + ':FREQuency:STARt?');
    queueGenQuery(ch, 'sweep-stopfreq',   ':SOUR' + ch + ':FREQuency:STOP?');
    queueGenQuery(ch, 'sweep-centerfreq', ':SOUR' + ch + ':FREQuency:CENTer?');
  }
  function setSweepStepCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:STEP ' + document.getElementById('gen-ch' + ch + '-sweep-step').value);
  }
  function setSweepTimeCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:TIME ' + document.getElementById('gen-ch' + ch + '-sweep-time').value);
  }
  function setSweepHTimeStartCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:HTIMe:STARt ' + document.getElementById('gen-ch' + ch + '-sweep-htimestart').value);
  }
  function setSweepHTimeStopCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:HTIMe:STOP ' + document.getElementById('gen-ch' + ch + '-sweep-htimestop').value);
  }
  function setSweepRTimeCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:RTIMe ' + document.getElementById('gen-ch' + ch + '-sweep-rtime').value);
  }
  function setSweepTrigSrcCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:TRIGger:SOURce ' + document.getElementById('gen-ch' + ch + '-sweep-trigsrc').value);
    updateSweepTrigNowVisibility(ch);
  }
  function sweepManualTriggerCh(ch) {
    genSend(':SOUR' + ch + ':SWEep:TRIGger');  // only meaningful when Trig Src = Manual
  }
  function toggleSweepMarkStateCh(ch) {
    const btn = document.getElementById('gen-ch' + ch + '-sweep-markstate-btn');
    const nowOn = btn.dataset.on !== '1';
    btn.textContent = btn.dataset.label + ': ' + (nowOn ? 'ON' : 'OFF');
    btn.dataset.on = nowOn ? '1' : '0';
    genSend(':SOUR' + ch + ':MARKer:STATe ' + (nowOn ? 'ON' : 'OFF'));
  }
  function setSweepMarkFreqCh(ch) {
    genSend(':SOUR' + ch + ':MARKer:FREQuency ' + document.getElementById('gen-ch' + ch + '-sweep-markfreq').value);
  }

  // ── Burst — Type: row (Mode) + modifier rows, per channel ──────────────
  function setBurstModeCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:MODE ' + document.getElementById('gen-ch' + ch + '-burst-typesel').value);
    updateBurstGatePolVisibility(ch);
    updateBurstNonGatedFieldsVisibility(ch);
    updateBurstTrigEdgeVisibility(ch);
  }
  function setBurstNCyclesCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:NCYCles ' + document.getElementById('gen-ch' + ch + '-burst-ncycles').value);
  }
  function setBurstPhaseCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:PHASe ' + document.getElementById('gen-ch' + ch + '-burst-phase').value);
  }
  function setBurstPeriodCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:INTernal:PERiod ' + document.getElementById('gen-ch' + ch + '-burst-period').value);
  }
  function setBurstGatePolCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:GATE:POLarity ' + document.getElementById('gen-ch' + ch + '-burst-gatepol').value);
  }
  function setBurstTrigSrcCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:TRIGger:SOURce ' + document.getElementById('gen-ch' + ch + '-burst-trigsrc').value);
    updateBurstTrigNowVisibility(ch);
    updateBurstTrigEdgeVisibility(ch);
  }
  // Manual confirms TrigOut only actually applies with Internal or
  // Manual trigger source — consistent with it now being hidden
  // whenever Trig Src is External (see updateBurstTrigEdgeVisibility).
  function setBurstTrigOutCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:TRIGger:TRIGOut ' + document.getElementById('gen-ch' + ch + '-burst-trigout').value);
  }
  // Confirmed at the bench: shown only when Trig Src is External. The
  // manual only documents POSitive|NEGative for this command (no OFF
  // keyword) — worth confirming at the bench whether "Off" actually
  // sends successfully via SCPI, or whether the front panel's Off state
  // is doing something slightly different under the hood.
  function setBurstSlopeInCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:TRIGger:SLOPe ' + document.getElementById('gen-ch' + ch + '-burst-slopein').value);
  }
  function burstManualTriggerCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:TRIGger');  // only meaningful when Trig Src = Manual
  }
  function setBurstTDelayCh(ch) {
    genSend(':SOUR' + ch + ':BURSt:TDELay ' + document.getElementById('gen-ch' + ch + '-burst-tdelay').value);
  }
  // ── Connect/disconnect hooks — called from the shared connect() ────────
  function onGenConnect() {
    // Nothing to do directly — resetLinkStatuses() (called just before
    // this, in ws.onopen) always sets linkAlive.funcgen to null at this
    // exact moment, so init here would never actually have anything to
    // decide. The liveness tick sends its first probe within ~1 second
    // regardless, and refreshBothGenChannels()/refreshModeFields() now
    // run from markLinkAlive()'s alive-transition hook instead — see
    // the liveness engine above. This avoids the old bug where a second,
    // redundant probe fired from here could race the tick's own.
  }
  function onGenDisconnect() {
    genQueryQueue = [];
    clearTimeout(genQueryTimeout);
    setGenBusy(false);
  }

  // ═══════════════════════════════════════════════════════════════════════
  // Counter panel (BK1823A) — bench-confirmed command set and response
  // format; see main.cpp's file header for the full writeup, including
  // how the model variant ("1.5(3.0)GHz, U/C") was identified from
  // bench data. NOT SCPI — commands are single letter + single digit
  // (R/F/G/H/D), each terminated with CR only (handled firmware-side).
  // Kept deliberately simpler than the DMM panel per explicit scope
  // decision: live reading + Function/Gate/Hold controls, no Data
  // Logger, no Halt/Resume.
  // ═══════════════════════════════════════════════════════════════════════

  let counterFunc          = 0;      // last function selected (F0-F7, skip 5)
  let counterGate          = 2;      // last gate selected (G0-G3) — default 1s
  let counterHold          = false;
  let counterAutoFetchOn   = false;
  let counterAwaitingRead  = false;
  let counterFetchTimer    = null;
  let counterTimeoutTimer  = null;

  // Gap between successive D-requests once a response comes back. Not
  // paced to the selected gate time — deliberate default for this pass
  // (D's parameter is "don't care" per the manual, and it simply returns
  // whatever the counter's latest completed measurement is; polling
  // faster than the gate time just means occasionally re-displaying the
  // same value, which is harmless). Revisit if that assumption doesn't
  // hold up in practice.
  const COUNTER_POLL_GAP_MS    = 300;
  const COUNTER_READ_TIMEOUT_MS = 3000;  // same scale as the DMM's own timeouts

  function counterSend(cmd) { wsSend('counter', cmd); }

  // ── Connect/disconnect hooks — called from the shared connect() ────────
  function onCounterConnect() {
    // Nothing to do directly — real init now runs only once
    // markLinkAlive('counter') observes a transition to alive; see
    // counterFullInit() below and the liveness engine above.
  }

  function counterFullInit() {
    // Sync the instrument to whatever the UI currently shows, same
    // trade-off the DMM panel already makes on its own transition:
    // this doesn't respect settings from the counter's own front panel
    // or a prior session — every transition-to-alive forces R1 + the
    // UI's current Function/Gate/Hold selections. Confirmed at the
    // bench that sending R1 on open works fine, and that D-requests
    // work regardless of Remote state — but F/G/H are only meaningful
    // once Remote is asserted, so R1 goes first.
    counterSend('R1');
    counterSend('F' + counterFunc);
    counterSend('G' + counterGate);
    counterSend('H' + (counterHold ? 1 : 0));
    if (counterAutoFetchOn) requestCounterReading();
  }

  function onCounterDisconnect() {
    clearTimeout(counterFetchTimer);
    clearTimeout(counterTimeoutTimer);
    counterAwaitingRead = false;
    document.getElementById('counter-reading').textContent = '---';
  }

  // ── Function / Gate / Hold / Auto-Fetch controls ────────────────────────
  function selectCounterFunc(n) {
    counterFunc = n;
    document.querySelectorAll('.ctr-btn[data-func]').forEach(b =>
      b.classList.toggle('active', Number(b.dataset.func) === n));
    counterSend('F' + n);
  }

  function selectCounterGate(n) {
    counterGate = n;
    document.querySelectorAll('.ctr-btn[data-gate]').forEach(b =>
      b.classList.toggle('active', Number(b.dataset.gate) === n));
    counterSend('G' + n);
  }

  function toggleCounterHold() {
    counterHold = !counterHold;
    document.getElementById('counter-hold-btn').textContent =
      'Hold: ' + (counterHold ? 'ON' : 'OFF');
    counterSend('H' + (counterHold ? 1 : 0));
  }

  function toggleCounterAutoFetch() {
    counterAutoFetchOn = !counterAutoFetchOn;
    document.getElementById('counter-autofetch-btn').textContent =
      'Auto-Fetch: ' + (counterAutoFetchOn ? 'ON' : 'OFF');
    if (counterAutoFetchOn) {
      requestCounterReading();
    } else {
      clearTimeout(counterFetchTimer);
      clearTimeout(counterTimeoutTimer);
      counterAwaitingRead = false;
    }
  }

  // ── Fetch loop ───────────────────────────────────────────────────────────
  function scheduleCounterFetch(delayMs) {
    if (!counterAutoFetchOn) return;
    clearTimeout(counterFetchTimer);
    counterFetchTimer = setTimeout(requestCounterReading, delayMs);
  }

  function requestCounterReading() {
    if (!counterAutoFetchOn) return;
    counterAwaitingRead = true;
    counterSend('D1');  // parameter is "don't care" per the manual — D1
                          // matches what was confirmed at the bench
    clearTimeout(counterTimeoutTimer);
    counterTimeoutTimer = setTimeout(() => {
      if (counterAwaitingRead) {
        counterAwaitingRead = false;
        scheduleCounterFetch(COUNTER_POLL_GAP_MS);
      }
    }, COUNTER_READ_TIMEOUT_MS);
  }

  // Called from the shared ws.onmessage for target === 'counter'. Ignores
  // responses that arrive when no fetch is pending (e.g. a manual command
  // typed into the TOOLS tab with Counter selected as target) — this
  // panel's reading display should only reflect its own D-requests.
  function handleCounterResponse(msg) {
    if (!counterAwaitingRead) return;
    counterAwaitingRead = false;
    clearTimeout(counterTimeoutTimer);
    displayCounterReading(msg);
    scheduleCounterFetch(COUNTER_POLL_GAP_MS);
  }

  // Parses per the manual's fixed-width spec (10 bytes decimal incl. dp,
  // then 4 bytes units) rather than splitting on whitespace — msg has
  // already had leading/trailing whitespace stripped firmware-side
  // (pushEvent's trim()), but the number field's own internal right-
  // padding (positions 0-9) survives that untouched, so slicing at a
  // fixed offset is robust regardless of how much trailing padding got
  // stripped off the unit field's end. Falls back to displaying the raw
  // string if it's shorter than expected, rather than guessing.
  function displayCounterReading(raw) {
    let text;
    if (raw.length >= 10) {
      const numPart  = raw.substring(0, 10).trim();
      const unitPart = raw.substring(10).trim();
      text = unitPart ? (numPart + ' ' + unitPart) : numPart;
    } else {
      text = raw.trim();
    }
    document.getElementById('counter-reading').textContent = text || '---';
  }

  // ═══════════════════════════════════════════════════════════════════════
  // DMM panel (BK Precision 5491B) — everything below is unchanged from
  // the original single-instrument project except: (1) every wsSend(cmd)
  // call became dmmSend(cmd), a thin wrapper that fixes target='dmm' and
  // keeps the DMM-specific lastWasQuery/lastQuery bookkeeping the legacy
  // response dispatch needs; (2) termLog() is now a thin wrapper around
  // the shared appLog('dmm', ...); (3) the old private #terminal UI and
  // its connect()/onmessage wiring moved into the shared layer above,
  // calling into onDmmConnect()/onDmmDisconnect() hooks instead.
  // ═══════════════════════════════════════════════════════════════════════

  // ── Unit map: FUNC string → display units ──────────────────────────────────
  const UNITS = {
    'VOLT:DC': 'Vdc',
    'VOLT:AC': 'Vac',
    'CURR:DC': 'Adc',
    'CURR:AC': 'Aac',
    'RES':     '\u03a9',   // Ω
    'FREQ':    'Hz',
    'PER':     's',
    'DIOD':    'V',
    'CONT':    '\u03a9',
  };
  // Human-readable labels, matching each function button's own text —
  // used to keep the top row's label in sync with whichever function it's
  // actually showing, in single-function mode.
  const FUNC_LABELS = {
    'VOLT:DC': 'Voltage DC', 'CURR:DC': 'Current DC',
    'VOLT:AC': 'Voltage AC', 'CURR:AC': 'Current AC',
    'FREQ':    'Frequency',  'PER':     'Period',
    'RES':     'Resistance', 'DIOD':    'Diode', 'CONT': 'Continuity',
  };

  // ── State ──────────────────────────────────────────────────────────────────
  let activeFunc    = '';
  let selectedRangeVal = null;  // numeric range value user picked, 'AUTO', or null (unknown)
  let refDisplayValue = null;   // numeric value for the (REF: <value>) indicator, or null/'ON'
  const customRefByFunc = {};   // remembers last Custom REF value entered, per function
  let lastWasQuery  = false;  // true if last user command ended in ?

  // ── Help button ──────────────────────────────────────────────────────────
  // The target URL lives in secrets.h (firmware-side), not in this file, so
  // it's fetched from a small dedicated endpoint rather than hardcoded here.
  // Fetched once at load and cached; falls back to fetching on click if the
  // button is somehow clicked before that first fetch resolves.
  let helpUrl = null;
  fetch('/help-url').then(r => r.text()).then(t => { helpUrl = t.trim(); }).catch(() => {});
  document.getElementById('help-btn').addEventListener('click', () => {
    if (helpUrl) {
      window.open(helpUrl, '_blank');
    } else {
      fetch('/help-url').then(r => r.text()).then(t => {
        helpUrl = t.trim();
        window.open(helpUrl, '_blank');
      }).catch(() => {});
    }
  });

  // ── WebSocket connect/disconnect hooks ──────────────────────────────────
  // Called by the shared connect()'s ws.onopen/onclose (see the shared
  // layer above) — kept as separate hooks rather than inline in that
  // generic handler so Counter/FuncGen can eventually get their own
  // onXConnect()/onXDisconnect() hooks without onopen/onclose growing
  // per-instrument if/else branches.
  function onDmmConnect() {
    // Nothing to do directly — real init now runs only once
    // markLinkAlive('dmm') observes a transition to alive; see
    // dmmFullInit() below and the liveness engine above.
  }

  function dmmFullInit() {
    // Commands VOLT:DC directly rather than querying FUNC? to sync to
    // whatever the meter already happens to be showing. Trade-off worth
    // knowing: this doesn't respect a function set from the front panel
    // or a prior session — every transition-to-alive forces VOLT:DC.
    // Wrapped in haltForRoutine so the initial RANG/REF/NPLC chain gets
    // the same channel protection as everything else — its snapshot/
    // restore naturally handles starting Auto-Fetch polling too (it
    // defaults to on, so the halt snapshots "on" and restores it once
    // this settles — no separate explicit kick needed anymore).
    haltForRoutine(() => selectSingleFunction('VOLT:DC', { skipAutoFetchOn: true }));
  }

  function onDmmDisconnect() {
    setReading('---');
  }

  // ── Handle incoming responses ──────────────────────────────────────────────
  // Track what the last sent query was so we can route the response
  let lastQuery = '';

  // Thin DMM-scoped wrapper around the shared wsSend(target, cmd) — fixes
  // target='dmm' and keeps the lastWasQuery/lastQuery bookkeeping that
  // handleRespLegacy() below still needs (see its own comments for what's
  // and isn't converted to the arbiter). Logging now happens once, inside
  // the shared wsSend() itself — this wrapper doesn't need its own termLog
  // call.
  function dmmSend(cmd) {
    lastWasQuery = cmd.endsWith('?');
    if (lastWasQuery) lastQuery = cmd.toUpperCase();
    wsSend('dmm', cmd);
  }

  // ── Shared request arbiter ───────────────────────────────────────────────
  // The actual fix for the misattribution bugs found at the bench — one
  // queue, one thing in flight at a time, system-wide (for whatever's
  // been converted to use it). Every sender submits a request and awaits
  // the result; nothing sends directly to wsSend() anymore from a
  // converted call site. Currently wired into single-function selection
  // and Auto-Fetch only — Multi-Function, Meter Settings, RESET's own
  // internals, and Range/Reference/Filter actions still send directly
  // (see handleRespLegacy() and their own code) and are NOT coordinated
  // with this arbiter. Don't exercise those at the same time as testing
  // this, since nothing prevents them from colliding on the wire — the
  // firmware provides no protection of its own anymore (pure relay).
  //
  // Four kinds of request, matching the four timing values:
  //   'func'  — FUNC <name>, no response expected. Hard wait, reuses
  //             cycleDelayMs (the same value Multi-Function's loop uses —
  //             this is the meter's relay-switching settle time, a
  //             physical constraint, not a communication delay).
  //   'set'   — any other non-query (no "?") command. Hard wait, uses
  //             its own separate adjustable value (setWaitMs).
  //   'fetch' — FETCH?. Event-driven timeout, FETCH_TIMEOUT_MS.
  //   'query' — any other query. Event-driven timeout, QUERY_TIMEOUT_MS.
  let setWaitMs = 0;  // adjustable via Advanced menu's "Set Wait"
  const FETCH_TIMEOUT_MS = 3000;
  const QUERY_TIMEOUT_MS = 3000;

  let arbiterBusy = false;
  let arbiterCurrent = null;
  const arbiterQueue = [];

  function arbiterSubmit(req) {
    return new Promise(resolve => {
      req.resolve = resolve;
      arbiterQueue.push(req);
      arbiterTryNext();
    });
  }

  function arbiterTryNext() {
    if (arbiterBusy || !arbiterQueue.length) return;
    arbiterBusy = true;
    const req = arbiterQueue.shift();
    arbiterCurrent = req;
    req.sentAt = Date.now();
    dmmSend(req.cmd);
    if (req.kind === 'func' || req.kind === 'set') {
      const waitMs = req.kind === 'func' ? cycleDelayMs : setWaitMs;
      req.timer = setTimeout(() => arbiterFinish({ ok: true }), waitMs);
    } else {
      const timeoutMs = req.kind === 'fetch' ? FETCH_TIMEOUT_MS : QUERY_TIMEOUT_MS;
      req.timer = setTimeout(() => {
        const elapsed = Date.now() - req.sentAt;  // kept for potential
                                                    // future use — not
                                                    // displayed per request
        termLog('  (timeout)', 'log-none');
        arbiterFinish({ ok: false, timedOut: true });
      }, timeoutMs);
    }
  }

  function arbiterFinish(result) {
    if (!arbiterCurrent) return;
    clearTimeout(arbiterCurrent.timer);
    const req = arbiterCurrent;
    arbiterCurrent = null;
    arbiterBusy = false;
    updateCommStatus(result.ok);
    req.resolve(result);
    arbiterTryNext();
  }

  // Per user request: a plain COMM on/COMM Fail indicator, updated on
  // every arbiter resolution — success or timeout. Only reflects arbiter-
  // covered traffic (single-function, Auto-Fetch, Multi-Function); Meter
  // Settings' timeouts don't feed this yet, consistent with that being a
  // known, already-flagged gap elsewhere.
  function updateCommStatus(ok) {
    const el = document.getElementById('comm-status');
    if (!el) return;
    el.textContent = ok ? 'COMM on' : 'COMM Fail';
    el.classList.toggle('comm-ok', ok);
    el.classList.toggle('comm-fail', !ok);
  }


  function arbiterShapeMatches(shape, msg) {
    if (shape === 'number') return !isNaN(parseFloat(msg));
    if (shape === 'bit') return msg.trim() === '0' || msg.trim() === '1';
    return true;  // no shape check requested for this request
  }

  // Called from ws.onmessage before the old per-mechanism dispatch.
  // Returns true if the arbiter claimed the message (used it, or
  // correctly identified it as a stray and discarded it) — caller should
  // not also pass it to the legacy dispatch in that case.
  function arbiterHandleIncoming(msg) {
    if (!arbiterCurrent || arbiterCurrent.kind === 'func' || arbiterCurrent.kind === 'set') {
      return false;  // nothing awaited, or a hard-wait expects no reply
    }
    if (arbiterShapeMatches(arbiterCurrent.shape, msg)) {
      const elapsed = Date.now() - arbiterCurrent.sentAt;  // kept for
                                                             // potential
                                                             // future use —
                                                             // not
                                                             // displayed
                                                             // per request
      arbiterFinish({ ok: true, value: msg });
    } else {
      // Wrong shape for what's currently awaited — a stray leftover from
      // something earlier. Discard it, keep waiting, timer untouched.
      termLog('  (stray/wrong-shape response, discarded)', 'log-none');
    }
    return true;
  }

  function handleRespLegacy(msg) {
    const q = lastQuery;
    lastQuery = '';
    // Meter Settings and the per-function state-query chain each claim
    // their own response exclusively, ahead of the generic q-based
    // routing below — needed because multiple response chains can
    // otherwise collide.
    if (meterSettingsAwaiting) {
      meterSettingsAwaiting = false;
      const cb = meterSettingsCallback; meterSettingsCallback = null;
      if (cb) cb(msg);
      return;
    }
    if (funcStateQueryAwaiting) {
      funcStateQueryAwaiting = false;
      const cb = funcStateQueryCallback; funcStateQueryCallback = null;
      if (cb) cb(msg);
      return;
    }
    if (q === 'FETCH?') {
      // Both Auto-Fetch and Multi-Function are arbiter-based now — their
      // FETCH? responses get claimed by arbiterHandleIncoming() before
      // this function is even called. The only way q==='FETCH?' reaches
      // here at all is a FETCH? typed manually into the SCPI terminal, so
      // it always belongs on the single-function catch-all display.
      updateReading(msg);
    } else if (q === 'FUNC?') {
      // Meter returns e.g. "VOLT:DC" — normalise and set active function
      const f = msg.trim().toUpperCase();
      setActiveFunc(f);
      // Resume polling — unless the user has explicitly paused it via the
      // Auto-Fetch toggle, in which case respect that (e.g. after RESET).
      if (autoFetchOn) schedulePoll();
    }
  }

  // ── Polling ────────────────────────────────────────────────────────────────
  // Rebuilt on the shared arbiter. The old version fired FETCH? every
  // 500ms unconditionally, with no check for whether the previous one had
  // even resolved yet — bench evidence traced this directly to the busy-
  // gate contention that was silently eating real button clicks all day
  // (piled-up FETCH?s occupying the channel exactly when a click needed
  // it). Now each poll waits for its own result — success or timeout —
  // before the next one is even scheduled.
  let pollTimer = null;
  function schedulePoll() {
    if (pollTimer) clearTimeout(pollTimer);
    pollTimer = setTimeout(pollOnce, 500);
  }
  // Bench-confirmed: an extended outage (WiFi hiccup, meter unresponsive,
  // etc.) can produce a long run of consecutive FETCH? timeouts — one
  // observed case hit 9 in a row (~22s) before the WebSocket itself
  // dropped and reconnected. Auto-Fetch shouldn't keep polling into a
  // connection that's clearly not answering; it stops itself instead,
  // clearly logged, rather than silently retrying forever. Deliberately
  // stays off after a successful reconnect too — this was a real failure,
  // not routine noise, so resuming is a deliberate choice, not automatic.
  const MAX_CONSECUTIVE_TIMEOUTS = 3;
  let consecutiveTimeouts = 0;
  async function pollOnce() {
    const r = await arbiterSubmit({ cmd: 'FETCH?', kind: 'fetch', shape: 'number' });
    if (!autoFetchOn) return;  // turned off while this was in flight
    if (r.ok) {
      consecutiveTimeouts = 0;
      updateReading(r.value);
    } else {
      consecutiveTimeouts++;
      if (consecutiveTimeouts >= MAX_CONSECUTIVE_TIMEOUTS) {
        termLog('-- auto-fetch stopped: ' + consecutiveTimeouts +
                ' consecutive timeouts --', 'log-sys');
        consecutiveTimeouts = 0;
        toggleAutoFetch();  // turns it off, updates the indicator, logs it
        return;              // do not reschedule
      }
    }
    schedulePoll();
  }

  // ── Universal channel halt/resume ───────────────────────────────────────
  // Replaces the old pauseAutoFetchFor() — this is the one mechanism every
  // routine that needs exclusive use of the channel goes through now,
  // manual or automatic. Manual: the HALT/RESUME button. Automatic: any
  // routine wraps its work in haltForRoutine(), which also disables every
  // other clickable path into the channel (function buttons, Range/
  // Reference/Filter, Advanced, the HALT/RESUME button itself) for the
  // routine's full duration — so a manual click can never land mid-
  // routine, and a second automatic routine can never start mid-routine
  // either. With overlap structurally impossible, there's nothing to
  // count or coordinate: each halt just snapshots whichever one of Auto-
  // Fetch/Multi-Function was active (they're mutually exclusive already),
  // turns it off, and restores that exact snapshot when done.
  let haltedPriorState = null;  // null = not halted; else 'autofetch' |
                                 // 'multifunction' | 'none'

  function channelBusy() {
    // True whenever starting something new would collide with something
    // already using the channel — Multi-Function actively cycling
    // (unhalted), any halt (manual or automatic) currently in effect, or
    // the data logger currently running. The logger itself doesn't send
    // anything (see startDataLogger()'s own comment), but Reset/Auto-
    // Fetch-toggle/Meter-Settings/Test-Interconnect all could disrupt the
    // Auto-Fetch/Multi-Function traffic it depends on — this is what
    // keeps those individually disabled while logging, without needing
    // to block Advanced itself (which stays open as the way back to the
    // logger's own Stop button).
    return multiFunctionOn || haltedPriorState !== null || loggerActive;
  }

  function haltChannel() {
    if (haltedPriorState !== null) return;  // already halted
    if (multiFunctionOn) { haltedPriorState = 'multifunction'; toggleMultiFunction(); }
    else if (autoFetchOn) { haltedPriorState = 'autofetch'; toggleAutoFetch(); }
    else { haltedPriorState = 'none'; }
    updateHaltButton();
  }

  function resumeChannel() {
    if (haltedPriorState === null) return;
    const prior = haltedPriorState;
    haltedPriorState = null;
    if (prior === 'multifunction') toggleMultiFunction();
    else if (prior === 'autofetch') toggleAutoFetch();
    updateHaltButton();
  }

  function updateHaltButton() {
    const btn = document.getElementById('halt-resume-btn');
    if (!btn) return;
    const resuming = haltedPriorState !== null;
    btn.textContent = resuming ? 'RESUME' : 'HALT';
    btn.classList.toggle('resuming', resuming);
  }

  function toggleHaltResume() {
    if (haltedPriorState !== null) {
      if (inLocalMode) exitLocalModeAndSync();
      else resumeChannel();
    } else {
      haltChannel();
    }
  }

  // ── Local mode ───────────────────────────────────────────────────────────
  // Halts the channel (same mechanism as the manual HALT button — pauses
  // and remembers Auto-Fetch/Multi-Function) and shows instructions for
  // using the meter's own front panel. There's no separate "exit" action
  // of its own — per the instructions shown, clicking any function button
  // is what returns to remote operation, since that's already the natural
  // next thing a user does. exitLocalModeIfActive() (called at the start
  // of every function-click path) detects that and explicitly resumes
  // BEFORE the click's own halt/resume cycle begins — so the click ends
  // up behaving like a normal, non-nested selection (correctly restoring
  // whatever was running before Local mode started, then giving the newly
  // clicked function its own fresh protected window) rather than getting
  // nesting-guarded into leaving things halted indefinitely.
  let inLocalMode = false;

  // Shared with Test Interconnect's own FUNC? round trip — kept as one
  // list so the two don't drift apart.
  const KNOWN_FUNC_CODES = ['VOLT:DC', 'VOLT:AC', 'CURR:DC', 'CURR:AC',
                             'RES', 'FREQ', 'PER', 'DIOD', 'CONT'];

  // Queries FUNC? and syncs activeFunc to whatever the meter reports —
  // used when leaving Local mode, since the user may have changed
  // functions from the meter's own front panel while in local operation
  // and the app has no way to know that happened otherwise. Returns
  // whether the query itself succeeded; a timeout just means the display
  // may be stale, not that resuming should be blocked.
  async function queryFuncAndSync() {
    const r = await arbiterSubmit({ cmd: 'FUNC?', kind: 'query' });
    if (r.ok) {
      const val = r.value.trim().toUpperCase();
      const matched = KNOWN_FUNC_CODES.find(f => val.indexOf(f) !== -1);
      if (matched) setActiveFunc(matched);
    } else {
      termLog('-- FUNC? query failed while leaving Local mode; ' +
              'function display may be out of sync --', 'log-sys');
    }
    return r.ok;
  }

  function enterLocalMode() {
    haltChannel();
    inLocalMode = true;
    closeSubmenu();
    alert('Press Shift on the instrument to enter local operation.\n\n' +
          'The instrument\'s front panel buttons will then work normally.\n\n' +
          'Click any function button on this page to return to remote operation.');
  }

  function exitLocalModeIfActive() {
    if (!inLocalMode) return;
    inLocalMode = false;
    resumeChannel();
  }

  // The other way out of Local mode — via RESUME or the Advanced menu's
  // Local/Remote toggle — rather than by picking a new function directly.
  // Unlike exitLocalModeIfActive(), this queries FUNC? first (see
  // queryFuncAndSync()) since nothing else is about to overwrite
  // activeFunc immediately afterward the way a function-button click does.
  async function exitLocalModeAndSync() {
    if (!inLocalMode) return;
    closeSubmenu();
    setChannelBusyUIDisabled(true);
    await queryFuncAndSync();
    inLocalMode = false;
    setChannelBusyUIDisabled(false);
    applySubmenuAvailability(activeFunc);
    resumeChannel();
  }

  // Disables every other way to start something new on the channel —
  // used for the full duration of any automatic halt-covered routine.
  function setChannelBusyUIDisabled(disabled) {
    document.querySelectorAll('.func-btn').forEach(b => b.disabled = disabled);
    ['range-menu-btn', 'reference-menu-btn', 'filter-menu-btn',
     'advanced-menu-btn', 'halt-resume-btn'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.disabled = disabled;
    });
  }

  // The universal wrapper every automatic routine uses: halt, lock down
  // every other entry point, run the routine, then unlock and resume —
  // in that order, and via finally so a routine that throws still leaves
  // the channel in a usable state rather than stuck disabled forever.
  async function haltForRoutine(fn) {
    // Nesting-aware: if something already halted the channel before this
    // call started (a manual HALT, or another automatic routine), this
    // call must not halt again or resume at the end — only the truly
    // outermost call owns that lifecycle. Without this, an inner
    // routine's own cleanup would resume an outer manual halt out from
    // under the user the instant the inner routine finished — exactly
    // the bug found at the bench (HALT during Multi-Function briefly
    // dropped to single-function, then silently resumed cycling again).
    const alreadyHalted = haltedPriorState !== null;
    if (!alreadyHalted) haltChannel();
    setChannelBusyUIDisabled(true);
    try {
      await fn();
    } finally {
      setChannelBusyUIDisabled(false);
      applySubmenuAvailability(activeFunc);  // undo setChannelBusyUIDisabled's
                                              // blanket re-enable of Range/
                                              // Reference/Filter — those need
                                              // to reflect activeFunc, not
                                              // just "not busy"
      if (!alreadyHalted) resumeChannel();
    }
  }

  // ── Auto-Fetch — a passive text indicator, not a button. Toggled via
  // the Advanced menu. ────────────────────────────────────────────────────
  let autoFetchOn = false;
  function toggleAutoFetch() {
    autoFetchOn = !autoFetchOn;
    const el = document.getElementById('autofetch-indicator');
    if (autoFetchOn) {
      el.textContent = 'Auto-Fetch: ON';
      termLog('-- auto-fetch resumed --', 'log-sys');
      schedulePoll();
    } else {
      el.textContent = '';
      termLog('-- auto-fetch paused --', 'log-sys');
      if (pollTimer) { clearTimeout(pollTimer); pollTimer = null; }
    }
  }

  // ── Multi-Function — likely permanent, not diagnostic scaffolding.
  // Loops FUNC -> FETCH? across whichever of the six functions are
  // checked (1 to 6), live-adjustable at any time including mid-run.
  // Rebuilt on the shared arbiter, same as single-function mode: each
  // step submits its FUNC (hard wait = cycleDelayMs) then its FETCH?
  // (timeout = FETCH_TIMEOUT_MS) and awaits both in turn — no more manual
  // cycleTimer/cyclingAwaitingFetch bookkeeping, the arbiter's own single-
  // request-in-flight queue already provides that. Stopping (toggle off,
  // or the RES/DIOD/CONT escape hatch) doesn't force anything — whatever
  // step is currently in flight is simply left to finish naturally; a
  // freshly-submitted single-function selection just queues behind it,
  // same as any two requests would. ─────────────────────────────────────
  const CYCLE_FUNCS = ['VOLT:DC', 'CURR:DC', 'VOLT:AC', 'CURR:AC', 'FREQ', 'PER'];
  // Loop membership state per function — starts matching the button
  // defaults (all checked). Source of truth for both loop membership and
  // button color (see onFunctionButtonClick).
  const cycleChecked = { 'VOLT:DC': true, 'CURR:DC': true, 'VOLT:AC': true,
                          'CURR:AC': true, 'FREQ': true, 'PER': true };
  let cycleActive     = false;
  let multiFunctionOn = false;  // starts OFF — single-function mode is
                                 // the initial state
  let cycleDelayMs = 750;  // FUNC hard wait, user-adjustable
  let lastCycleFunc = null;  // search anchor — "find the next checked
                              // function after this one" replaces a fixed
                              // index, so the checked set can change size
                              // at any moment without breaking the loop

  function anyCycleChecked() {
    return CYCLE_FUNCS.some(f => cycleChecked[f]);
  }

  // Finds the next checked function after `afterFunc` in CYCLE_FUNCS
  // order, wrapping around. Returns null if nothing is checked. With
  // exactly one function checked, this naturally lands on that same
  // function every time — true single-function looping, no special case.
  function nextCheckedFunc(afterFunc) {
    const startIdx = afterFunc ? CYCLE_FUNCS.indexOf(afterFunc) : -1;
    for (let i = 1; i <= CYCLE_FUNCS.length; i++) {
      const idx = (startIdx + i + CYCLE_FUNCS.length) % CYCLE_FUNCS.length;
      if (cycleChecked[CYCLE_FUNCS[idx]]) return CYCLE_FUNCS[idx];
    }
    return null;
  }

  // Shows/hides the checkboxes and the five non-VOLT:DC rows based on
  // current mode + checkbox state. Called on load, on every checkbox
  // change, and on every Multi-Function toggle.
  function updateMultiFunctionVisibility() {
    CYCLE_FUNCS.forEach(f => {
      if (f === 'VOLT:DC') {
        // Can't hide this row (it's also the always-visible single-
        // function catch-all), so when it's excluded from the loop
        // during Multi-Function, blank both the label and the value
        // entirely — a persisting label (even dimmed) still read at the
        // bench as "a measurement is missing" rather than "this row
        // isn't in the loop." Fully blank just looks like an empty,
        // unused slot instead.
        const excluded = multiFunctionOn && !cycleChecked['VOLT:DC'];
        const labelEl = document.getElementById('top-row-label');
        const valEl = document.getElementById('cr-VOLT:DC');
        if (labelEl) labelEl.textContent = excluded ? '' : 'Voltage DC:\u00a0\u00a0';
        if (valEl && excluded) {
          valEl.textContent = '';
          valEl.classList.remove('cr-fresh');
        }
        return;
      }
      const row = document.getElementById('row-' + f);
      if (row) row.classList.toggle('shown', multiFunctionOn && cycleChecked[f]);
    });
  }

  // Dual-purpose click handler for the six loop-eligible function
  // buttons — checkboxes removed entirely; the button itself now carries
  // both jobs, since the white/gray coloring already showed loop
  // membership regardless. When Multi-Function is off, a click selects
  // the function normally (same as RES/DIOD/CONT). When it's on, a click
  // toggles loop membership instead — live, works mid-run, same as the
  // checkboxes did.
  function onFunctionButtonClick(func) {
    exitLocalModeIfActive();
    if (!multiFunctionOn) { selectSingleFunctionHalted(func); return; }
    cycleChecked[func] = !cycleChecked[func];
    const btn = document.getElementById('btn-' + func);
    if (btn) btn.classList.toggle('chk-checked', cycleChecked[func]);
    updateMultiFunctionVisibility();
    if (cycleActive && !anyCycleChecked()) {
      termLog('-- multi-function auto-stopped: no functions selected --', 'log-sys');
      turnOffMultiFunction();
    }
  }

  function toggleMultiFunction() {
    if (multiFunctionOn) { turnOffMultiFunction(); return; }
    // No forced eviction needed here anymore — Multi-Function's own steps
    // now go through the same arbiter as everything else, so a lingering
    // single-function selection just gets queued behind naturally, same
    // as any two requests would. (Meter Settings is still a separate,
    // unconverted mechanism — clearing its flag here remains a real
    // precaution, not a leftover.)
    meterSettingsAwaiting = false;
    meterSettingsCallback = null;
    if (!anyCycleChecked()) {
      // Self-heal instead of a dead-end: with checkboxes hidden outside
      // Multi-Function mode, refusing to start here with nothing checked
      // would leave no way to check one again short of a page reload.
      // Default to whichever function is already active (known client-
      // side, no need to query the meter), or VOLT:DC if that's not one
      // of the six.
      const fallback = CYCLE_FUNCS.includes(activeFunc) ? activeFunc : 'VOLT:DC';
      cycleChecked[fallback] = true;
      const btn = document.getElementById('btn-' + fallback);
      if (btn) btn.classList.add('chk-checked');
      termLog('-- no functions were selected, defaulting to ' + fallback + ' --', 'log-sys');
    }
    multiFunctionOn = true;
    const mfBtn = document.getElementById('multifunction-btn');
    mfBtn.classList.add('on');
    mfBtn.textContent = 'Multi-Function (ON)';
    // Top row's label/value are handled by updateMultiFunctionVisibility()
    // below, based on VOLT:DC's actual checked state — not set explicitly
    // here, since it may or may not be checked at this moment.
    // Range and Reference have no stable single "active function" to
    // apply to during cycling (it's constantly changing) — blank their
    // labels entirely rather than leave them grayed-but-still-readable.
    const rangeBtn = document.getElementById('range-menu-btn');
    const refBtn   = document.getElementById('reference-menu-btn');
    if (rangeBtn) rangeBtn.textContent = '\u00a0';
    if (refBtn)   refBtn.textContent   = '\u00a0';
    updateMultiFunctionVisibility();
    cycleActive = true;
    lastCycleFunc = null;  // fresh search start
    // Force Auto-Fetch off — nothing should compete for the channel
    // during the loop, same as the user's manual process.
    if (autoFetchOn) {
      autoFetchOn = false;
      document.getElementById('autofetch-indicator').textContent = '';
      if (pollTimer) { clearTimeout(pollTimer); pollTimer = null; }
      termLog('-- auto-fetch paused (multi-function starting) --', 'log-sys');
    }
    termLog('-- multi-function started, delay=' + cycleDelayMs + 'ms --', 'log-sys');
    setCycleUIDisabled(true);
    cycleStepFunc();
  }

  // Single exit path for Multi-Function mode, used whether triggered by
  // the user clicking the toggle, or by auto-stop when every checkbox
  // gets unchecked mid-run — both end up in exactly the same state.
  // Split into two parts: stopMultiFunctionMode() just cleans up cycling
  // state, without picking a resulting single function — used when
  // RES/DIOD/CONT are clicked directly, so it doesn't waste a redundant
  // FUNC send selecting the topmost-checked function only to immediately
  // override it with whatever was actually clicked.
  function stopMultiFunctionMode() {
    cycleActive = false;
    multiFunctionOn = false;
    termLog('-- multi-function stopped --', 'log-sys');
    const mfBtn = document.getElementById('multifunction-btn');
    mfBtn.classList.remove('on');
    mfBtn.textContent = 'Multi-Function (OFF)';
    const rangeBtn = document.getElementById('range-menu-btn');
    const refBtn   = document.getElementById('reference-menu-btn');
    if (rangeBtn) rangeBtn.textContent = 'Range';
    if (refBtn)   refBtn.textContent   = 'Reference';
    setCycleUIDisabled(false);
    applySubmenuAvailability(activeFunc);  // same fix as haltForRoutine —
                                            // don't let the blanket
                                            // re-enable override what's
                                            // actually applicable to
                                            // whichever function cycling
                                            // last landed on
    updateMultiFunctionVisibility();
    // Resume Auto-Fetch automatically — leaving it off until manually
    // re-enabled via the Advanced menu was not obvious. Must go through
    // toggleAutoFetch() itself, not just set the flag/text directly — a
    // prior version did that and left the indicator saying "ON" while
    // nothing was actually polling, since only toggleAutoFetch() actually
    // calls schedulePoll(). Skipped entirely if something already halted
    // the channel (e.g. Multi-Function stopping as a side effect of the
    // manual HALT button) — springing Auto-Fetch back on here would
    // override that halt; let it own the resume decision instead.
    if (haltedPriorState === null && !autoFetchOn) toggleAutoFetch();
  }

  // Reverting to single-function mode after cycling no longer needs a
  // manual settling delay — that was a workaround for a real bug (the
  // resulting FUNC could get silently dropped by the firmware's old
  // busy-gate if sent too soon after the loop's last command). With
  // Multi-Function's own steps now going through the same arbiter, a
  // freshly-submitted FUNC just queues naturally behind whatever's still
  // in flight — same fix, no artificial wait required anymore.
  function turnOffMultiFunction() {
    stopMultiFunctionMode();
    // Per user request: the topmost checked function becomes the single
    // active function. Falls back to VOLT:DC if somehow nothing is
    // checked (e.g. auto-stop triggered this).
    const topFunc = CYCLE_FUNCS.find(f => cycleChecked[f]) || 'VOLT:DC';
    selectSingleFunctionHalted(topFunc);
  }

  // Clicking Resistance/Diode/Continuity exits Multi-Function mode (if it
  // was on) and selects that function directly, instead of requiring
  // Multi-Function to be turned off first. These three buttons stay
  // clickable during cycling specifically to serve as this escape hatch
  // — see setCycleUIDisabled().
  function selectExclusiveFunction(func) {
    exitLocalModeIfActive();
    if (multiFunctionOn) stopMultiFunctionMode();
    selectSingleFunctionHalted(func);
  }

  async function cycleStepFunc() {
    if (!cycleActive) return;
    const func = nextCheckedFunc(lastCycleFunc);
    if (func === null) {
      // Shouldn't normally happen — onFunctionButtonClick() auto-stops as
      // soon as the last one is toggled off — but guard anyway in case a
      // step was already in flight when that happened.
      termLog('-- multi-function auto-stopped: no functions selected --', 'log-sys');
      turnOffMultiFunction();
      return;
    }
    lastCycleFunc = func;
    setActiveFunc(func);      // immediate optimistic update, same as
                               // single-function mode — nothing better to
                               // sync a silent FUNC command to
    setCycleUIDisabled(true); // re-lock submenu buttons — setActiveFunc()
                               // re-enables them per-function applicability
    await arbiterSubmit({ cmd: 'FUNC ' + func, kind: 'func' });
    if (!cycleActive) return;  // stopped while waiting — finish naturally,
                                // don't touch UI or schedule anything further
    const r = await arbiterSubmit({ cmd: 'FETCH?', kind: 'fetch', shape: 'number' });
    if (!cycleActive) return;  // stopped mid-fetch — finish naturally
    if (r.ok) updateCycleRow(func, r.value);
    // else: timed out — row just keeps its last value, no special
    // handling needed; the arbiter already logged the timeout itself.
    cycleStepFunc();
  }

  // Groups digits in 3s: integer part from the right, fractional part
  // from the left — e.g. "1999.999817" -> "1 999.999 817". Applied only
  // to actual numeric readings, never to placeholder text ("—", "OPEN").
  function groupDigits(numStr) {
    const neg = numStr.startsWith('-');
    if (neg) numStr = numStr.slice(1);
    let [intPart, fracPart] = numStr.split('.');
    intPart = intPart.replace(/\B(?=(\d{3})+(?!\d))/g, ' ');
    if (fracPart) fracPart = fracPart.replace(/(\d{3})(?=\d)/g, '$1 ');
    return (neg ? '-' : '') + intPart + (fracPart ? '.' + fracPart : '');
  }

  function updateCycleRow(func, msg) {
    loggerRecordMultiCell(func, msg);
    const el = document.getElementById('cr-' + func);
    if (!el) return;
    const val = parseFloat(msg);
    el.textContent = isNaN(val) ? '\u2014'
                                 : groupDigits(val.toFixed(6)) + ' ' + (UNITS[func] || '');
    // Green: "newest data actually received" — the only remaining
    // per-step visual signal now that the yellow highlight is gone.
    document.querySelectorAll('.cycle-row .cr-value').forEach(v => v.classList.remove('cr-fresh'));
    el.classList.add('cr-fresh');
  }

  function setCycleUIDisabled(disabled) {
    // The six loop-eligible buttons are no longer disabled during
    // cycling — checkboxes are gone, and these buttons are now the
    // mechanism for toggling loop membership live, mid-run (see
    // onFunctionButtonClick()), same as the checkboxes used to allow.
    // RES/DIOD/CONT were already never disabled — clicking them is the
    // way to exit Multi-Function mode directly (see
    // selectExclusiveFunction()) rather than requiring the toggle to be
    // turned off first.
    // Advanced deliberately NOT included — it stays clickable during
    // cycling now. Most of what's in it is harmless mid-run (Cycle Wait,
    // SCPI, Trigger/Math placeholders, Meter Settings already guards
    // itself). Reset and Auto-Fetch are the two genuinely unsafe items —
    // guarded individually inside the submenu instead of blocking the
    // whole menu (see openSubmenu's 'advanced' branch).
    ['range-menu-btn', 'reference-menu-btn', 'filter-menu-btn'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.disabled = disabled;
    });
  }

  // ── Reading display ────────────────────────────────────────────────────────
  function needsMilliConversion() {
    // Only meaningful once a specific (non-AUTO) sub-1 range is selected —
    // we don't guess a conversion for AUTO/unknown range state.
    return (activeFunc === 'VOLT:DC' || activeFunc === 'VOLT:AC' ||
            activeFunc === 'CURR:DC' || activeFunc === 'CURR:AC') &&
           typeof selectedRangeVal === 'number' && selectedRangeVal < 1;
  }

  function updateUnitsDisplay() {
    const el = document.getElementById('units');
    if (!el) return;  // the big reading display no longer exists — this
                       // element is intentionally gone; Range/Reference
                       // logic that calls this function is left untouched
    let u = UNITS[activeFunc] || '';
    if (needsMilliConversion()) {
      const isVolt = activeFunc === 'VOLT:DC' || activeFunc === 'VOLT:AC';
      u = isVolt ? 'mV' + (activeFunc === 'VOLT:AC' ? 'ac' : 'dc')
                 : 'mA' + (activeFunc === 'CURR:AC' ? 'ac' : 'dc');
    }
    el.textContent = u;
  }

  // The big single reading display is gone. Its former job — showing
  // whatever the currently-active function's latest FETCH? value is, for
  // normal (non-cycling) Auto-Fetch/manual-click use — now writes into
  // the top row of the six-row list (the "Voltage DC" slot), always,
  // regardless of which function is actually active. Not correctly
  // labeled for anything but VOLT:DC — a known, deliberately-accepted
  // placeholder per user request ("details of that are not important"),
  // not a real design. Only reached for non-cycling FETCH? responses —
  // see handleRespLegacy(), which routes cycling FETCH? responses through
  // updateCycleRow() instead so the two paths never collide on this slot.
  function updateReading(raw) {
    loggerRecordSingle(raw);
    const val = parseFloat(raw);
    if (isNaN(val)) { setReading('---'); return; }
    let text;
    if ((activeFunc === 'RES' || activeFunc === 'CONT') && Math.abs(val) >= 50000000) {
      // RES/CONT: meter returns an oddball large value on open circuit —
      // anything >= 50,000,000 isn't a real reading, show OPEN instead.
      text = 'OPEN';
    } else {
      // Sub-1 V/A ranges display in milli-units (matches the range
      // submenu labels) — same value, just fewer leading zeros to read.
      const dispVal = needsMilliConversion() ? val * 1000 : val;
      let u = UNITS[activeFunc] || '';
      if (needsMilliConversion()) {
        const isVolt = activeFunc === 'VOLT:DC' || activeFunc === 'VOLT:AC';
        u = isVolt ? 'mV' + (activeFunc === 'VOLT:AC' ? 'ac' : 'dc')
                   : 'mA' + (activeFunc === 'CURR:AC' ? 'ac' : 'dc');
      }
      text = groupDigits(dispVal.toFixed(6)) + ' ' + u;
    }
    const el = document.getElementById('cr-VOLT:DC');
    if (!el) return;
    document.querySelectorAll('.cycle-row .cr-value').forEach(v => v.classList.remove('cr-fresh'));
    el.textContent = text;
    el.classList.add('cr-fresh');
  }

  function setReading(text) {
    const el = document.getElementById('cr-VOLT:DC');
    if (el) el.textContent = text;
  }

  // ── Function selection ─────────────────────────────────────────────────────
  const NPLC_INAPPLICABLE = ['FREQ', 'PER', 'DIOD', 'CONT'];
  const NO_SUBMENUS = ['DIOD', 'CONT'];  // no Range, Reference, or Filter at all

  // Sets Range/Reference/Filter disabled state for whichever function is
  // current. Split out from setActiveFunc() because it also needs to be
  // re-applied after any blanket re-enable (setChannelBusyUIDisabled(false),
  // setCycleUIDisabled(false)) — those unlock every menu button
  // unconditionally as a routine finishes, which would otherwise stomp on
  // the correct per-function disabled state set here, re-enabling submenus
  // that don't apply (e.g. Range/Reference/Filter for DIOD/CONT) right
  // after they were correctly disabled.
  function applySubmenuAvailability(func) {
    const filterBtn = document.getElementById('filter-menu-btn');
    if (filterBtn) filterBtn.disabled = NPLC_INAPPLICABLE.includes(func);
    const rangeBtn = document.getElementById('range-menu-btn');
    const refBtn   = document.getElementById('reference-menu-btn');
    if (rangeBtn) rangeBtn.disabled = !rangeGroup(func);
    if (refBtn)   refBtn.disabled   = NO_SUBMENUS.includes(func);
  }

  function setActiveFunc(func) {
    // Clear all active buttons
    document.querySelectorAll('.func-btn').forEach(b => b.classList.remove('active'));
    // Set the matching button active — except while cycling is running:
    // during cycling, button color is purely membership-driven (see
    // onFunctionButtonClick), so the green highlight is skipped then.
    // Outside of cycling — including direct single-function clicks on a
    // loop-btn button — green highlighting works exactly as it always
    // has. (This must check "are we cycling right now," not "is this a
    // loop-btn button" — the latter incorrectly suppresses green for
    // normal single-function use too.)
    const btn = document.getElementById('btn-' + func);
    if (btn && !cycleActive) btn.classList.add('active');
    activeFunc = func;
    // Top-row label/value: only touched in single-function mode. During
    // Multi-Function cycling this row is genuinely the VOLT:DC loop row,
    // not the catch-all, so it must not be relabeled or blanked here —
    // toggleMultiFunction()/stopMultiFunctionMode() own that row's label
    // in that mode instead.
    if (!multiFunctionOn) {
      const labelEl = document.getElementById('top-row-label');
      if (labelEl) labelEl.textContent = (FUNC_LABELS[func] || func) + ':\u00a0\u00a0';
      const valEl = document.getElementById('cr-VOLT:DC');
      if (valEl) {
        valEl.textContent = '\u2014 sampling \u2014';
        valEl.classList.remove('cr-fresh');
      }
    }
    // Range/Filter/Reference choices are all function-scoped — old displayed
    // values may no longer apply, and we haven't (yet) queried the meter's
    // actual current state for the new function.
    selectedRangeVal = null;
    document.getElementById('state-range').textContent = rangeGroup(func) ? '?' : '\u2014';
    document.getElementById('state-filter').textContent = NPLC_INAPPLICABLE.includes(func) ? '\u2014' : '?';
    document.getElementById('state-reference').textContent = NO_SUBMENUS.includes(func) ? '\u2014' : '?';
    refDisplayValue = null;
    updateRefIndicator();
    updateUnitsDisplay();
    // Filter (NPLC) doesn't apply to FREQ/PER (different measurement
    // mechanism) or DIOD/CONT (simple pass/fail tests, not precision
    // measurements) — disable rather than let it silently do nothing.
    // Range doesn't apply to FREQ/PER either (no range concept for them,
    // rangeGroup() returns null) — not just DIOD/CONT. Reference still
    // uses NO_SUBMENUS alone, since the manual documents REF for FREQ/PER.
    applySubmenuAvailability(func);
  }

  // ── Per-function state auto-query ───────────────────────────────────────────
  // Whenever a different single function is selected, queries RANGe,
  // REFerence, and NPLC and populates the menu-bar state text with real
  // values, instead of leaving it at "?" until the user opens a submenu.
  // Rebuilt on the shared arbiter — the generation-token machinery this
  // used to need (funcQueryGen, its own dedicated awaiting-flag/callback,
  // a 3s safety timer) is gone; the arbiter already serializes everything
  // and owns its own timeout, so a stale in-flight step just needs to
  // check "is activeFunc still what I was called for" at each resume
  // point, same effect with far less machinery.
  // Deliberately NOT used during Multi-Function cycling — that still uses
  // its own separate, unconverted mechanism (see file header).
  //
  // funcStateQueryAwaiting/funcStateQueryCallback below are now unused by
  // this function — left declared because handleRespLegacy() and the
  // 'none' handler still reference them (always false/no-op now, harmless
  // — those are dead branches until Multi-Function is converted too).
  let funcStateQueryAwaiting = false;
  let funcStateQueryCallback = null;

  // Selecting a function the normal way (button click, initial connect,
  // or reverting from Multi-Function) always means: send FUNC, update
  // the UI immediately as usual, then query RANG/REF/NPLC in sequence.
  // Also turns Auto-Fetch on if it was off — selecting a function to look
  // at implies wanting to see it update.
  async function selectSingleFunction(func, opts = {}) {
    // skipAutoFetchOn: used by callers already running inside a halted
    // routine (see haltForRoutine) — without this, the normal "turn Auto-
    // Fetch on if it was off" behavior below would turn it back on mid-
    // routine, defeating the point of having halted it in the first place.
    if (!opts.skipAutoFetchOn && !autoFetchOn) toggleAutoFetch();
    setActiveFunc(func);  // immediate optimistic UI update — nothing
                           // better to sync a silent FUNC command to
    await arbiterSubmit({ cmd: 'FUNC ' + func, kind: 'func' });
    if (activeFunc !== func) return;  // user moved on again already

    const group = rangeGroup(func);
    const hasSubmenus = !NO_SUBMENUS.includes(func);    const hasNplc = !NPLC_INAPPLICABLE.includes(func);

    if (hasSubmenus && group) {
      const r = await arbiterSubmit({ cmd: func + ':RANG:UPP?', kind: 'query', shape: 'number' });
      if (activeFunc !== func) return;
      if (r.ok) {
        const val = parseFloat(r.value);
        document.getElementById('state-range').textContent = rangeLabel(group, val);
        selectedRangeVal = val;  // keeps mV/mA display conversion in sync
        updateUnitsDisplay();
        updateRefIndicator();
      } else {
        document.getElementById('state-range').textContent = '?';
      }
    }
    if (hasSubmenus) {
      const r = await arbiterSubmit({ cmd: func + ':REF:STAT?', kind: 'query', shape: 'bit' });
      if (activeFunc !== func) return;
      document.getElementById('state-reference').textContent =
        r.ok ? (r.value.trim() === '1' ? 'ON' : 'OFF') : '?';
    }
    if (hasNplc) {
      const r = await arbiterSubmit({ cmd: func + ':NPLC?', kind: 'query', shape: 'number' });
      if (activeFunc !== func) return;
      if (r.ok) {
        const val = parseFloat(r.value);
        let label = r.value;
        if (val === 0.1) label = 'OFF'; else if (val === 1) label = 'Norm'; else if (val === 10) label = 'Max';
        document.getElementById('state-filter').textContent = label;
      } else {
        document.getElementById('state-filter').textContent = '?';
      }
    }
  }

  // Standalone-click entry point — used wherever a function selection is
  // a genuine independent user (or system) action: the six loop-eligible
  // buttons, the RES/DIOD/CONT escape hatch, and reverting to single-
  // function mode when Multi-Function turns off. Forces Auto-Fetch on if
  // it was off *before* halting, so the halt's own snapshot correctly
  // captures "on" and restores it afterward — preserves that existing
  // behavior while adding full halt protection for the RANG/REF/NPLC
  // chain's duration. Distinct from RESET/initial-connect, which
  // deliberately do NOT force this — they just restore whatever state
  // was already there before they started.
  function selectSingleFunctionHalted(func) {
    // Skipped if already nested inside another halt — same reasoning as
    // stopMultiFunctionMode()'s fix: forcing Auto-Fetch on here would
    // override whatever the outer halt is deliberately keeping paused.
    if (haltedPriorState === null && !autoFetchOn) toggleAutoFetch();
    return haltForRoutine(() => selectSingleFunction(func, { skipAutoFetchOn: true }));
  }

  // ── Submenu data (Range / Reference / Filter) ──────────────────────────────
  // NOTE: Range command syntax confirmed (:RANG:UPP <val>, :RANG:AUTO ON).
  // Reference syntax confirmed for VOLT:DC, generalized to other functions
  // per user note ("I believe"). Filter (NPLC) now function-scoped — see
  // project notes, this reverses an earlier "confirmed" global-command result.
  const RANGE_VALUES = {
    'VOLT:DC': [1000, 500, 50, 5, 0.5],
    'VOLT:AC': [750, 500, 50, 5, 0.5],
    CURR: [20, 5, 0.5, 0.05, 0.005],
    RES:  [50000000, 5000000, 500000, 50000, 5000, 500],
  };
  function rangeGroup(func) {
    if (func === 'VOLT:DC' || func === 'VOLT:AC') return func;  // own entry in RANGE_VALUES
    if (func === 'CURR:DC' || func === 'CURR:AC') return 'CURR';
    if (func === 'RES') return 'RES';
    return null;
  }
  function rangeLabel(group, val) {
    if (group === 'RES') {
      if (val >= 1e6) return (val / 1e6) + 'M\u03a9';
      if (val >= 1e3) return (val / 1e3) + 'k\u03a9';
      return val + '\u03a9';
    }
    // Sub-1 volt/amp ranges shown in milli-units — avoids a screen full of
    // leading zeros for novices (e.g. 500mV instead of 0.5V).
    const isVolt = group === 'VOLT:DC' || group === 'VOLT:AC';
    if (val < 1) return Math.round(val * 1000) + (isVolt ? 'mV' : 'mA');
    return val + (isVolt ? 'V' : 'A');
  }
  const FILTER_OPTIONS = [
    { label: 'OFF',  long: 'Off (fast)',                    nplc: '0.1' },
    { label: 'Norm', long: 'Norm (one power line cycle)',   nplc: '1'   },
    { label: 'Max',  long: 'Max (10 power line cycles)',    nplc: '10'  },
  ];
  const REFERENCE_OPTIONS = ['OFF', 'AutoZero', 'Custom'];
  // Per-function REF value bounds, from the manual's SCPI Command Reference.
  // CURR:AC corrected 2026-07-xx: manual states "0 to 20" but bench testing
  // confirmed negative values work identically to CURR:DC (subtraction is
  // pure math, no range-dependent clamping) — bench result wins over manual.
  const REF_RANGES = {
    'VOLT:DC': [-1010, 1010],
    'VOLT:AC': [-757.5, 757.5],
    'CURR:DC': [-20, 20],
    'CURR:AC': [-20, 20],
    'RES':     [0, 20000000],
    'FREQ':    [0, 1000000],
    'PER':     [0, 1],
  };

  function updateRefIndicator() {
    const el = document.getElementById('ref-indicator');
    if (!el) return;  // the big reading display no longer exists — this
                       // element is intentionally gone; Reference/Range
                       // logic that calls this function is left untouched
    if (refDisplayValue === null) { el.textContent = ''; return; }
    if (refDisplayValue === 'ON') { el.textContent = '(REF: ON)'; return; }
    // Same conversion the reading display uses — REF is just another raw
    // float off the wire (no units, no range-awareness on the meter's
    // side), so it should render consistently with whatever's on screen
    // next to it rather than always showing base V/A.
    const shown = needsMilliConversion() ? refDisplayValue * 1000 : refDisplayValue;
    el.textContent = '(REF: ' + shown + ')';
  }

  // ── Submenu popup ──────────────────────────────────────────────────────────
  let currentSubmenu = null;

  function openSubmenu(type, btnEl) {
    currentSubmenu = type;
    const title = document.getElementById('submenu-title');
    const opts  = document.getElementById('submenu-options');
    const warn  = document.getElementById('submenu-warn');
    opts.innerHTML = '';
    warn.textContent = '';

    if (type === 'range') {
      const group = rangeGroup(activeFunc);
      title.textContent = 'RANGE \u2014 ' + activeFunc;
      if (!group) {
        opts.innerHTML = '<span style="color:#aaa">Not applicable to ' + activeFunc + '</span>';
      } else {
        RANGE_VALUES[group].forEach(v => {
          const btn = makeOptBtn(rangeLabel(group, v), () => selectRange(group, v));
          if (selectedRangeVal === v) btn.classList.add('current');
          opts.appendChild(btn);
        });
        const autoBtn = makeOptBtn('AUTO', () => selectRangeAuto());
        if (selectedRangeVal === 'AUTO') autoBtn.classList.add('current');
        opts.appendChild(autoBtn);
      }
    } else if (type === 'reference') {
      title.textContent = 'REFERENCE';
      // Highlights whichever mode is currently active, and shows the
      // real value next to Custom (known exactly — set directly by
      // promptCustomReference()). AutoZero shows "(ON)" rather than a
      // real number: the meter's acquired zero-point is never actually
      // queried back (<FUNC>:REF? would give it) — a known open item,
      // not yet built. Showing "(ON)" is honest about what's actually
      // known here, not a placeholder pretending to be a real value.
      const currentRefLabel = document.getElementById('state-reference').textContent;
      REFERENCE_OPTIONS.forEach(label => {
        let displayLabel = label;
        if (label === currentRefLabel && label !== 'OFF' && refDisplayValue !== null) {
          const shown = (label === 'Custom' && needsMilliConversion())
            ? refDisplayValue * 1000 : refDisplayValue;
          displayLabel = label + ' (' + shown + ')';
        }
        const btn = makeOptBtn(displayLabel, () =>
          label === 'Custom' ? promptCustomReference() : selectReference(label));
        if (label === currentRefLabel) btn.classList.add('current');
        opts.appendChild(btn);
      });
    } else if (type === 'filter') {
      title.textContent = 'FILTER (NPLC)';
      const currentFilterLabel = document.getElementById('state-filter').textContent;
      FILTER_OPTIONS.forEach(o => {
        const btn = makeOptBtn(o.long, () => selectFilter(o));
        if (o.label === currentFilterLabel) btn.classList.add('current');
        opts.appendChild(btn);
      });
      warn.textContent = '';
    } else if (type === 'advanced') {
      // Consolidates what used to be standalone RESET/SCPI/Trigger/Math/
      // Auto-Fetch controls, plus Meter Settings and Cycle Wait. Reset,
      // Auto-Fetch, and Meter Settings are disabled outright whenever the
      // channel is busy (cycling, or any halt in effect) — preferred over
      // alerting after the fact, since it prevents the clash from being
      // possible at all rather than just catching it once attempted.
      title.textContent = 'ADVANCED';
      const busy = channelBusy();
      const resetBtn = makeOptBtn('Reset', () => { confirmReset(); closeSubmenu(); });
      resetBtn.disabled = busy;
      opts.appendChild(resetBtn);
      opts.appendChild(makeOptBtn('SCPI Terminal', () => {
        showTab('scpi');
        setScpiTarget('dmm');
        closeSubmenu();
      }));
      // Trigger and Math are unimplemented placeholders (not yet even
      // stubbed with real SCPI syntax) — disabled outright rather than
      // clickable-but-a-no-op, so it's visually obvious nothing happens.
      const triggerBtn = makeOptBtn('Trigger', () => {});
      triggerBtn.disabled = true;
      opts.appendChild(triggerBtn);
      const mathBtn = makeOptBtn('Math', () => {});
      mathBtn.disabled = true;
      opts.appendChild(mathBtn);
      const autoFetchBtn = makeOptBtn('Auto-Fetch', () => { toggleAutoFetch(); closeSubmenu(); });
      autoFetchBtn.disabled = busy;
      opts.appendChild(autoFetchBtn);
      // Meter Settings deliberately does NOT close the submenu — it stays
      // open, showing "querying..." then the results, reusing this same
      // popup rather than opening a second one.
      const meterSettingsBtn = makeOptBtn('Meter Settings', () => openMeterSettings());
      meterSettingsBtn.disabled = busy;
      opts.appendChild(meterSettingsBtn);
      // Cycle Wait / Set Wait removed from the visible menu per request —
      // promptCycleWait()/promptSetWait() themselves are untouched below,
      // kept in case these come back later. Re-add by uncommenting:
      // opts.appendChild(makeOptBtn('Cycle Wait', () => { promptCycleWait(); closeSubmenu(); }));
      // opts.appendChild(makeOptBtn('Set Wait', () => { promptSetWait(); closeSubmenu(); }));
      // Local's own halt is idempotent (a no-op if something's already
      // halted), so unlike Reset/Auto-Fetch/Meter Settings it's safe to
      // leave enabled regardless of channel state. While in Local mode,
      // this same slot reads "Remote" and exits via exitLocalModeAndSync()
      // instead — same FUNC?-then-resume path as the RESUME button.
      opts.appendChild(makeOptBtn(inLocalMode ? 'Remote' : 'Local', () => {
        if (inLocalMode) exitLocalModeAndSync();
        else enterLocalMode();
      }));
      const testInterconnectBtn = makeOptBtn('Test Interconnect', () => openTestInterconnect());
      testInterconnectBtn.disabled = busy;
      opts.appendChild(testInterconnectBtn);
      opts.appendChild(makeOptBtn('Data Logger', () => { openDataLogger(); closeSubmenu(); }));
    }

    // Position popup left-aligned with the clicked button, just above it.
    const rect = btnEl.getBoundingClientRect();
    const popup = document.getElementById('submenu-popup');
    popup.style.transform = 'none';
    let left = rect.left;
    const maxLeft = window.innerWidth - 330;  // popup width (320) + margin
    if (left > maxLeft) left = Math.max(10, maxLeft);
    popup.style.left = left + 'px';
    popup.style.bottom = (window.innerHeight - rect.top + 6) + 'px';

    document.getElementById('submenu-popup').classList.add('open');
    document.getElementById('submenu-overlay').classList.add('open');
  }

  function closeSubmenu() {
    document.getElementById('submenu-popup').classList.remove('open');
    document.getElementById('submenu-overlay').classList.remove('open');
    currentSubmenu = null;
  }

  function makeOptBtn(label, onClick) {
    const b = document.createElement('button');
    b.className = 'submenu-opt';
    b.textContent = label;
    b.onclick = onClick;
    return b;
  }

  function selectRange(group, val) {
    closeSubmenu();
    haltForRoutine(async () => {
      await arbiterSubmit({ cmd: activeFunc + ':RANG:UPP ' + val, kind: 'set' });
      selectedRangeVal = val;
      document.getElementById('state-range').textContent = rangeLabel(group, val);
      updateUnitsDisplay();
      updateRefIndicator();
    });
  }

  function selectRangeAuto() {
    closeSubmenu();
    haltForRoutine(async () => {
      await arbiterSubmit({ cmd: activeFunc + ':RANG:AUTO ON', kind: 'set' });
      selectedRangeVal = 'AUTO';
      document.getElementById('state-range').textContent = 'AUTO';
      updateUnitsDisplay();
      updateRefIndicator();
    });
  }

  function selectFilter(opt) {
    closeSubmenu();
    haltForRoutine(async () => {
      await arbiterSubmit({ cmd: activeFunc + ':NPLC ' + opt.nplc, kind: 'set' });
      document.getElementById('state-filter').textContent = opt.label;
    });
  }

  function selectReference(label) {
    closeSubmenu();
    haltForRoutine(async () => {
      if (label === 'OFF') {
        await arbiterSubmit({ cmd: activeFunc + ':REF:STAT OFF', kind: 'set' });
        refDisplayValue = null;
      } else {
        // AutoZero: turn reference mode on, then acquire current reading
        // as the zero point. Confirmed sequence for VOLT:DC — generalizing
        // to other functions per user's "I believe" (i.e. still
        // experimental outside VOLT:DC). Toggling OFF does not clear the
        // acquired value; turning back ON reuses it. A fresh :ACQ is
        // needed to re-zero.
        await arbiterSubmit({ cmd: activeFunc + ':REF:STAT ON', kind: 'set' });
        await arbiterSubmit({ cmd: activeFunc + ':REF:ACQ', kind: 'set' });
        // We don't know the acquired numeric value without an extra :REF?
        // query — action item, not built yet (see project notes). Indicator
        // shows ON-only until that's added.
        refDisplayValue = 'ON';
      }
      document.getElementById('state-reference').textContent = label;
      updateRefIndicator();
    });
  }

  function promptCustomReference() {
    const [lo, hi] = REF_RANGES[activeFunc] || [-Infinity, Infinity];
    const prevVal = customRefByFunc[activeFunc] ?? 0;
    const input = prompt('Custom reference for ' + activeFunc +
      ' (' + lo + ' to ' + hi + '):', String(prevVal));
    if (input === null) { closeSubmenu(); return; }  // cancelled
    const n = parseFloat(input);
    if (isNaN(n) || n < lo || n > hi) {
      alert('Enter a number between ' + lo + ' and ' + hi + ' for ' + activeFunc + '.');
      return;  // leave popup open, let them try again
    }
    customRefByFunc[activeFunc] = n;
    closeSubmenu();
    haltForRoutine(async () => {
      // Confirmed order: value first, then enable — the reverse of
      // AutoZero's sequence. Kept as its own distinct pattern rather than
      // reused.
      await arbiterSubmit({ cmd: activeFunc + ':REF ' + n, kind: 'set' });
      await arbiterSubmit({ cmd: activeFunc + ':REF:STAT ON', kind: 'set' });
      refDisplayValue = n;  // known immediately, no query needed
      document.getElementById('state-reference').textContent = 'Custom';
      updateRefIndicator();
    });
  }

  // ── Reset ──────────────────────────────────────────────────────────────────
  function confirmReset() {
    if (!confirm('Send *RST? This clears the meter\'s current setup (range, ' +
                 'reference, trigger, NPLC, etc.) and returns it to power-on ' +
                 'defaults.')) return;
    // *RST's send and quiet window don't go through the arbiter (its
    // required wait — 5s — doesn't match any of the four existing kinds,
    // and this is rare enough not to warrant a fifth). haltForRoutine
    // still fully covers it: halts (pausing Auto-Fetch/Multi-Function and
    // remembering which), disables every other entry point, and restores
    // everything once the whole sequence — including the resulting
    // function selection's own RANG/REF/NPLC chain — actually finishes.
    haltForRoutine(() => new Promise(resolve => {
      dmmSend('*RST');
      // 5-second quiet window, nothing sent — matches bench observation of
      // a human just power-cycling the meter and waiting. Increased from
      // the original 3s guess after bench testing showed BUS SYNTAX ERROR
      // / BUS BAD COMMAND responses to commands sent that early.
      // *RST is known to land on VOLT:DC:AUTO, so command it directly
      // (same "command, don't query" pattern as initial connect) rather
      // than querying FUNC? — then let the state-query chain populate
      // real Range/Reference/Filter values instead of guessing defaults.
      // skipAutoFetchOn: Auto-Fetch's restore is the halt's job (it
      // resumes to whatever was there before *RST) — selecting VOLT:DC
      // here should not independently force it on too.
      setTimeout(async () => {
        await selectSingleFunction('VOLT:DC', { skipAutoFetchOn: true });
        resolve();
      }, 5000);
    }));
  }

  // Advanced menu item: sets cycleDelayMs without starting Multi-Function.
  function promptCycleWait() {
    const input = prompt(
      'Delay between sending FUNC and sending FETCH? during Multi-Function looping, in milliseconds:',
      String(cycleDelayMs));
    if (input === null) return;
    const n = parseInt(input, 10);
    if (isNaN(n) || n < 0) { alert('Enter a non-negative whole number of milliseconds.'); return; }
    cycleDelayMs = n;
    termLog('-- cycle wait set to ' + cycleDelayMs + 'ms --', 'log-sys');
  }

  // Advanced menu item: sets the arbiter's hard wait for non-query (no
  // "?") commands — RANG:UPP, REF:STAT, NPLC, etc. Separate from the FUNC
  // wait (cycleDelayMs) since a plain set command's settle time isn't
  // necessarily the same as the meter's relay-switching time.
  function promptSetWait() {
    const input = prompt(
      'Wait time after sending a non-query command (Range/Reference/Filter, etc.), in milliseconds:',
      String(setWaitMs));
    if (input === null) return;
    const n = parseInt(input, 10);
    if (isNaN(n) || n < 0) { alert('Enter a non-negative whole number of milliseconds.'); return; }
    setWaitMs = n;
    termLog('-- set wait set to ' + setWaitMs + 'ms --', 'log-sys');
  }

  // ── Meter Settings ───────────────────────────────────────────────────────
  // Queries everything the panel knows how to ask about for the currently
  // active function, then lists the results in the Advanced popup (reuses
  // the same popup, doesn't open a second one). Fully sequential, response-
  // or-timeout driven — same pattern as the per-function state query chain,
  // kept as its own separate dedicated flag so the two can never collide
  // even if somehow triggered close together.
  let meterSettingsAwaiting = false;
  let meterSettingsCallback = null;

  // Guard against starting during cycling moved to disabling the menu item
  // itself (see openSubmenu's 'advanced' branch) — preferred over an
  // alert per user request, prevents the clash outright rather than just
  // catching it after the fact.
  function openMeterSettings() {
    const title = document.getElementById('submenu-title');
    const opts  = document.getElementById('submenu-options');
    const warn  = document.getElementById('submenu-warn');
    const func = activeFunc;
    title.textContent = 'METER SETTINGS \u2014 querying\u2026';
    opts.innerHTML = '<span style="color:#aaa">Please wait, this takes several seconds\u2026</span>';
    warn.textContent = '';

    const group = rangeGroup(func);
    const hasSubmenus = !NO_SUBMENUS.includes(func);
    const hasNplc = !NPLC_INAPPLICABLE.includes(func);
    const queries = [];
    if (hasSubmenus && group) {
      queries.push(func + ':RANG:UPP?');
      queries.push(func + ':RANG:AUTO?');
    }
    if (hasSubmenus) {
      queries.push(func + ':REF:STAT?');
      queries.push(func + ':REF?');
    }
    if (hasNplc) queries.push(func + ':NPLC?');
    queries.push('TRIG:SOUR?');

    // Rebuilt on the arbiter — the old custom safety-timer/awaiting-flag
    // machinery this used to need is gone; the arbiter already provides
    // exactly that for every request, so this is now just a plain
    // sequential loop.
    haltForRoutine(async () => {
      const results = ['FUNC (known)  \u2192  ' + func];
      for (const cmd of queries) {
        const r = await arbiterSubmit({ cmd, kind: 'query' });
        results.push(cmd + '  \u2192  ' + (r.ok ? r.value : '(no response)'));
      }
      title.textContent = 'METER SETTINGS \u2014 ' + func;
      opts.innerHTML = '';
      results.forEach(r => {
        const d = document.createElement('div');
        d.style.cssText = 'color:#ddccaa; font-size:0.85em; text-align:left; padding:2px 0;';
        d.textContent = r;
        opts.appendChild(d);
      });
      opts.appendChild(makeOptBtn('Close', () => closeSubmenu()));
    });
  }

  // Tests the two links separately, since one can be up while the other
  // is down: WebSocket to the ESP32 (a local, instant check — no round
  // trip needed, ws.readyState already tells us), then RS-232 to the
  // meter via a genuine FUNC? round trip through the arbiter — this can
  // only succeed if the ESP32 actually relayed it and the meter actually
  // answered, so it's a real end-to-end test, not just "is the socket
  // open." FUNC? was long deliberately avoided elsewhere in the app (in
  // favor of commanding functions directly) — this was its only live use
  // until queryFuncAndSync() (see Local mode) added the one case where a
  // query, not a command, is actually what's needed: resyncing after the
  // meter's front panel may have changed things out from under the app.
  function openTestInterconnect() {
    const title = document.getElementById('submenu-title');
    const opts  = document.getElementById('submenu-options');
    const warn  = document.getElementById('submenu-warn');
    title.textContent = 'TEST INTERCONNECT \u2014 testing\u2026';
    opts.innerHTML = '<span style="color:#aaa">Please wait\u2026</span>';
    warn.textContent = '';

    haltForRoutine(async () => {
      const results = [];
      const espOk = !!ws && ws.readyState === WebSocket.OPEN;
      results.push('ESP32 (WiFi/WebSocket):  ' + (espOk ? 'OK' : 'FAIL \u2014 not connected'));

      if (espOk) {
        const r = await arbiterSubmit({ cmd: 'FUNC?', kind: 'query' });
        if (r.ok) {
          const val = r.value.trim().toUpperCase();
          const known = KNOWN_FUNC_CODES;
          const recognized = known.some(f => val.indexOf(f) !== -1);
          results.push('Meter (RS-232, FUNC?):  ' +
            (recognized ? ('OK \u2014 responded: ' + r.value)
                        : ('RESPONDED, UNRECOGNIZED: ' + r.value)));
        } else {
          results.push('Meter (RS-232, FUNC?):  FAIL \u2014 no response (timeout)');
        }
      } else {
        results.push('Meter (RS-232, FUNC?):  SKIPPED \u2014 ESP32 not connected');
      }

      title.textContent = 'TEST INTERCONNECT \u2014 results';
      opts.innerHTML = '';
      results.forEach(r => {
        const d = document.createElement('div');
        d.style.cssText = 'color:#ddccaa; font-size:0.85em; text-align:left; padding:2px 0;';
        d.textContent = r;
        opts.appendChild(d);
      });
      opts.appendChild(makeOptBtn('Close', () => closeSubmenu()));
    });
  }

  // ── Data Logger ──────────────────────────────────────────────────────────
  // Header line: the function(s) being logged — one name in single-
  // function mode, comma-separated in Multi-Function mode (in CYCLE_FUNCS
  // order, matching whichever were checked when logging started). Each
  // following line is one reading (single mode) or one complete cycle's
  // worth of readings, comma-separated in the same column order as the
  // header (Multi-Function mode) — valid CSV either way.
  //
  // The function/checkbox configuration is frozen for the whole logging
  // session (same disabling as any other halt-covered routine, held for
  // as long as logging runs rather than a bounded routine) — changing
  // what's being measured mid-log would silently break the CSV's column
  // structure, so it's prevented outright rather than left as a footgun.
  //
  // Multi-Function rows are detected by lap boundary, not by "all columns
  // filled" — a cycle step that times out never calls updateCycleRow() at
  // all, so waiting for every column to be present could hang forever.
  // Detecting "we're back at the first logged function" is robust to that
  // — a row gets flushed (blank cells for anything that timed out) the
  // moment the next lap begins. One consequence worth knowing: each row
  // only appears once the *following* lap starts, so there's a one-lap
  // display delay, and the final in-progress lap when you click Stop
  // needs an explicit flush (handled below) or it would be silently lost.
  let loggerOpen = false;
  let loggerActive = false;
  let loggerFuncs = [];
  let loggerCurrentRow = {};
  let loggerCsvLines = [];

  function openDataLogger() {
    loggerOpen = true;
    const el = document.getElementById('datalogger');
    if (el) el.classList.add('open');
    renderDataLoggerDisplay();
  }

  function closeDataLogger() {
    // Deliberately does nothing if logging is active — the Close button
    // itself is disabled for that whole duration (see startDataLogger()),
    // specifically so this can never be reached while running. Without
    // that, closing mid-log would hide the only way back to Stop, while
    // everything it froze stayed frozen — a real dead end found at the
    // bench.
    loggerOpen = false;
    const el = document.getElementById('datalogger');
    if (el) el.classList.remove('open');
  }

  function toggleDataLogger() {
    if (loggerActive) stopDataLogger();
    else startDataLogger();
  }

  function startDataLogger() {
    // No channelBusy() gate here deliberately — the logger doesn't send
    // anything of its own, it only observes whatever Auto-Fetch or
    // Multi-Function is already producing (see the hooks in
    // updateReading()/updateCycleRow()), so there's no collision to
    // prevent by blocking this. Found at the bench: an earlier version
    // wrongly required halting Multi-Function first, which made starting
    // a multi-function log impossible outright (halting turns
    // multiFunctionOn off, so this code would then have logged whatever
    // single function was left active instead of the intended set).
    loggerActive = true;
    loggerCsvLines = [];
    loggerCurrentRow = {};
    // Single-function logging needs Auto-Fetch actually running to have
    // anything to observe — turn it on if it's off, same as selecting a
    // function normally would.
    if (!multiFunctionOn && !autoFetchOn) toggleAutoFetch();
    loggerFuncs = multiFunctionOn ? CYCLE_FUNCS.filter(f => cycleChecked[f]) : [activeFunc];
    loggerCsvLines.push(loggerFuncs.join(','));
    // Freeze the configuration — logging is a passive observer of
    // whatever's already running, so nothing needs to be halted; only the
    // controls that could change WHAT's being measured get locked.
    // Advanced itself is deliberately left enabled (unlike
    // setChannelBusyUIDisabled's usual scope) — it's the only way back to
    // this panel if it's ever reached in a closed state, and "Data
    // Logger" itself is never gated by channelBusy(). The individually
    // risky items inside Advanced (Reset, Auto-Fetch, Meter Settings,
    // Test Interconnect) are still covered — channelBusy() now includes
    // loggerActive, so their own per-item disabling picks this up.
    document.querySelectorAll('.func-btn').forEach(b => b.disabled = true);
    const mfBtn = document.getElementById('multifunction-btn');
    if (mfBtn) mfBtn.disabled = true;
    ['range-menu-btn', 'reference-menu-btn', 'filter-menu-btn', 'halt-resume-btn'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.disabled = true;
    });
    const startStopBtn = document.getElementById('datalogger-startstop');
    if (startStopBtn) startStopBtn.textContent = 'Stop Logging';
    const closeBtn = document.getElementById('datalogger-close');
    if (closeBtn) closeBtn.disabled = true;  // must Stop before Close — see closeDataLogger()
    const dlBtn = document.getElementById('datalogger-download');
    if (dlBtn) dlBtn.disabled = true;
    renderDataLoggerDisplay();
  }

  function stopDataLogger() {
    // Flush whatever partial row was mid-lap when Stop was clicked —
    // otherwise the last, possibly-still-useful lap is silently dropped.
    if (Object.keys(loggerCurrentRow).length > 0) loggerFlushRow();
    loggerActive = false;
    document.querySelectorAll('.func-btn').forEach(b => b.disabled = false);
    const mfBtn = document.getElementById('multifunction-btn');
    if (mfBtn) mfBtn.disabled = false;
    ['range-menu-btn', 'reference-menu-btn', 'filter-menu-btn', 'halt-resume-btn'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.disabled = false;
    });
    const startStopBtn = document.getElementById('datalogger-startstop');
    if (startStopBtn) startStopBtn.textContent = 'Start Logging';
    const closeBtn = document.getElementById('datalogger-close');
    if (closeBtn) closeBtn.disabled = false;
    const dlBtn = document.getElementById('datalogger-download');
    if (dlBtn) dlBtn.disabled = loggerCsvLines.length <= 1;  // header line only = nothing to save
    renderDataLoggerDisplay();
  }

  function loggerFlushRow() {
    const row = loggerFuncs.map(f => loggerCurrentRow[f] !== undefined ? loggerCurrentRow[f] : '');
    loggerCsvLines.push(row.join(','));
    loggerCurrentRow = {};
  }

  function loggerRecordSingle(msg) {
    if (!loggerActive) return;
    const val = parseFloat(msg);
    loggerCsvLines.push(isNaN(val) ? '' : val.toFixed(6));
    if (loggerOpen) renderDataLoggerDisplay();
  }

  function loggerRecordMultiCell(func, msg) {
    if (!loggerActive) return;
    if (loggerFuncs.indexOf(func) === -1) return;  // shouldn't happen, config is frozen — safe either way
    if (func === loggerFuncs[0] && Object.keys(loggerCurrentRow).length > 0) {
      loggerFlushRow();
    }
    const val = parseFloat(msg);
    loggerCurrentRow[func] = isNaN(val) ? '' : val.toFixed(6);
    if (loggerOpen) renderDataLoggerDisplay();
  }

  function renderDataLoggerDisplay() {
    const el = document.getElementById('datalogger-log');
    if (!el) return;
    el.textContent = loggerCsvLines.join('\n');
    el.scrollTop = el.scrollHeight;
  }

  function downloadDataLoggerCsv() {
    if (loggerCsvLines.length === 0) return;
    const blob = new Blob([loggerCsvLines.join('\r\n') + '\r\n'], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'bk5491b_log_' + Date.now() + '.csv';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  // ── DMM terminal logging ──────────────────────────────────────────────────
  // Thin wrapper kept so the ~20 existing termLog() call sites throughout
  // the DMM panel's JS (Multi-Function, Range/Reference/Filter, Reset,
  // Cycle/Set Wait, etc.) didn't all need individual edits — they were
  // already logging DMM-specific state messages, so tagging them 'dmm' and
  // routing into the shared appLog() (the TOOLS tab's unified log) is a
  // drop-in replacement. setWsStatus(), the private #terminal toggle, and
  // manual send/clear all moved to the shared layer at the top of this
  // script (see appLog()/scpiSend()/scpiClear()/setWsStatus() there) — the
  // TOOLS tab is now the one terminal for the whole app, not a DMM-only
  // popup.
  function termLog(text, cls) { appLog('dmm', text, cls); }

  // Every Set button in the Func Gen tab starts disabled, and only
  // becomes clickable while its corresponding field is focused — by
  // explicit request, to prevent an accidental/misdirected click on a
  // Set button you weren't actually looking at.
  //
  // Real bug found at the bench: the original version used
  // row.querySelector('button.gen-set-btn') — singular — assuming
  // exactly one Set button per row, then wired EVERY field in the row
  // to that one button. That held for most rows, but broke once Max
  // moved onto Ampl's row and Min onto Offset's row (each row now has
  // TWO Set buttons and TWO fields): the code grabbed only the first
  // button (Ampl's/Offset's) and wired BOTH fields to it, so focusing
  // Max lit up Ampl's Set button — and clicking it genuinely called
  // setGenAmpl(), not setGenHigh(), regardless of which field you'd
  // actually typed into. Max/Min's own Set buttons were never touched
  // at all.
  //
  // Fixed by walking each row's direct children in DOM order instead of
  // grabbing the first match: fields accumulate into a pending list,
  // and hitting a Set button associates it with only the fields seen
  // since the previous button (then resets for the next one). This
  // correctly handles one-field-per-button rows (most of them),
  // multiple-buttons-per-row (Ampl+Max, Offset+Min), and multiple-
  // fields-sharing-one-button (Harmonic's combined Ampl+Phase row,
  // where the harmonic-index dropdown and both fields all correctly
  // group under the one Set button between them and the next boundary).
  //
  // The mousedown/preventDefault() and deferred-blur mechanics below
  // are unchanged from the earlier fix — see that fix's own history:
  // an even earlier version relied purely on setTimeout ordering to
  // let a click complete before the field's blur could disable the
  // button, which didn't reliably win in practice (the button ended up
  // disabled before its own click could fire, so nothing was ever
  // sent). mousedown/preventDefault() stops the button from stealing
  // focus away from the field in the first place, so blur never fires
  // for that interaction at all; the deferred blur handler remains only
  // as the safety net for genuinely clicking away to something else.
  function initGenSetButtonGating() {
    document.querySelectorAll('#tab-funcgen .gen-field-row').forEach(row => {
      let pendingFields = [];
      Array.from(row.children).forEach(el => {
        if (el.matches('input.gen-field-input, select.gen-mod-select')) {
          pendingFields.push(el);
        } else if (el.matches('button.gen-set-btn')) {
          const setBtn = el;
          const fields = pendingFields;
          pendingFields = [];  // reset — a later Set button in the same
                                 // row (Max after Ampl, Min after Offset)
                                 // must not also claim these same fields
          if (fields.length === 0) return;
          setBtn.disabled = true;
          setBtn.addEventListener('mousedown', e => e.preventDefault());
          fields.forEach(field => {
            genFieldToSetBtn.set(field, setBtn);
            field.addEventListener('focus', () => { setBtn.disabled = false; });
            field.addEventListener('blur', () => {
              setTimeout(() => {
                const stillFocused = fields.some(f => document.activeElement === f);
                if (!stillFocused) setBtn.disabled = true;
              }, 0);
            });
          });
        }
      });
    });
  }

  // ── Start ──────────────────────────────────────────────────────────────────
  // INDEX_HTML_VERSION: tracks this file specifically (browser app),
  // independent of main.cpp's own FW_VERSION constant. Logged first,
  // unconditionally, so it's visible in the TOOLS tab even if the WS
  // connection never succeeds at all — bump the decimal part by 0.1
  // with each new revision of this file.
  const INDEX_HTML_VERSION = '0.56';
  appLog(null, 'index_html.h version ' + INDEX_HTML_VERSION, 'log-sys');
  updateMultiFunctionVisibility();  // matches CSS defaults explicitly
  document.getElementById('autofetch-indicator').textContent =
    autoFetchOn ? 'Auto-Fetch: ON' : '';
  initGenSetButtonGating();
  connect();
</script>
</body>
</html>
)rawhtml";

#endif // INDEX_HTML_H
