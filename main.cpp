/*
 * main.cpp — Multi-Instrument Remote Panel
 *
 * Remote web control panel for THREE bench instruments sharing one
 * ESP32-S3 and one browser-based UI:
 *   - BK Precision 5491B DMM       — RS-232, UART1 (GPIO17 TX / GPIO18 RX)
 *   - BK1823A Frequency Counter    — RS-232, UART2 (GPIO6  TX / GPIO5  RX)
 *   - Rigol DG4062 Function Gen    — USB-TMC, native USB-OTG (host mode)
 *
 * This began as a merge of two previously separate sketches (a DMM-only
 * WiFi/HTTP/WS panel, and a standalone raw ESP-IDF USB Host TMC client),
 * plus a new UART2 relay for the counter. All three links, and the full
 * browser panel for all three instruments, are now bench-confirmed
 * against real hardware — see README.md for current feature status per
 * instrument and what's still open.
 *
 * ── Two-file structure (this file + index_html.h) ──
 * Arduino IDE's automatic function-prototype generator scans source
 * files with a naive text pattern-matcher that doesn't understand raw
 * string literal boundaries, and misidentifies JS syntax inside an
 * inline HTML string as C++ needing a prototype. Keeping the browser
 * app in index_html.h (which the scanner never touches) makes that bug
 * structurally impossible. This project builds under PlatformIO, which
 * doesn't have that particular auto-prototype behavior — but the split
 * is kept regardless, since it also just keeps ~7000 lines of embedded
 * JS/HTML/CSS out of the C++ translation unit. Do not merge
 * index_html.h back into this file.
 *
 * ── Every WS message is tagged with a "target" ──
 * With three instruments sharing one WebSocket, every message — both
 * directions — carries a JSON envelope: {"target":"dmm"|"counter"|
 * "funcgen","cmd":"...","seq":N} outgoing (seq is Func Gen-only — see
 * its own queue comment below), {"event":"resp"|"err"|"sys",
 * "target":"...","msg":"...","seq":N} incoming ("sys" messages omit
 * target — firmware-level, not instrument-specific). Parsing/building
 * this JSON uses ArduinoJson (v7) rather than hand-rolled string
 * slicing — a parsing bug here doesn't just misformat a reading, it
 * could send a command to the wrong instrument.
 *
 * ── DMM and Counter: a dumb pipe, multiplexed by target ──
 * For "dmm" and "counter", the firmware has zero SCPI knowledge and
 * zero notion of "in flight" or "timeout" — a WS message with that
 * target goes straight to that instrument's UART TX, unmodified; a
 * complete line from that UART goes straight back over WS, tagged with
 * that same target, the instant it's assembled. All pacing, timeouts,
 * retry, and response validation live entirely in the browser's JS
 * (one arbiter per target — see index_html.h).
 *
 * ── Function Gen: NOT a dumb pipe — a real, accepted deviation ──
 * USB-TMC bulk transfers have no equivalent to "just relay UART bytes."
 * Every read requires the host to send a REQUEST_DEV_DEP_MSG_IN header
 * and then synchronously wait for the bulk IN transfer to complete —
 * there is no way to "relay" that; the firmware has to speak enough of
 * the TMC transaction shape to drive it. tmcQuery()/tmcSendCommand()
 * block for up to TMC_QUERY_GAP_MS plus transfer completion.
 *
 * This synchronous blocking caused a real, confirmed reliability bug
 * early on: a single "refresh both channels" UI action fires 25+ Func
 * Gen queries back-to-back, and with handleFuncgenCommand() called
 * directly from onWsEvent() (AsyncTCP's own task), that meant a
 * sustained 20-30+ second window where the WebSocket connection
 * couldn't be serviced at all — leading to disconnects, which
 * immediately re-triggered the same expensive query burst on
 * reconnect, producing a self-perpetuating disconnect loop. Fixed:
 * onWsEvent() now only enqueues Func Gen commands (fast, non-blocking)
 * onto s_funcgen_queue; a dedicated funcgenWorkerTask drains it and
 * calls the same handleFuncgenCommand() from its own task context
 * instead — see the queue declaration's own comment for the full
 * writeup, including the sequence-number system layered on top of it
 * once FIFO ordering alone turned out not to be a sufficient guarantee
 * in every real scenario.
 *
 * ── Debug output: WS "sys" events, not Serial ──
 * Native USB is committed to USB-TMC host mode
 * (ARDUINO_USB_MODE=0 / ARDUINO_USB_CDC_ON_BOOT=0, set via
 * platformio.ini's build_flags), so there is no USB Serial Monitor
 * available in this build. All debug output goes through wsLog()
 * calls that broadcast {"event":"sys","msg":"..."} to every connected
 * browser, visible in the app's TOOLS tab terminal log. A small ring
 * buffer captures early boot messages (WiFi connect, USB enumeration)
 * that happen before any browser has connected, and flushes them to
 * the first client that does. Genuinely early boot failures (e.g. WiFi
 * never connecting) are still invisible until/unless a client
 * connects — worth knowing if debugging a dead board with no network
 * to reach.
 *
 * ── USB-TMC platform requirement: pioarduino, not stock espressif32 ──
 * Confirmed at the bench: the stock PlatformIO `espressif32` platform
 * package (3.20017.241212 / IDF 5.3.x at the time this was found) has
 * a bug that makes usb_host_interface_claim() always return 0x106,
 * regardless of what's actually connected — the DG4062 would never
 * enumerate no matter how correct the rest of this file's USB-TMC code
 * was. Switching platformio.ini's `platform` line to the pioarduino
 * fork resolved it outright. See platformio.ini for the actual pinned
 * version. Also confirmed: external 5V on VBUS is NOT required — the
 * DG4062 enumerates and runs fine with VBUS disconnected (an earlier
 * theory blamed missing VBUS for non-enumeration; that theory was
 * wrong, the platform bug above was the real cause).
 *
 * ── BK1823A Counter protocol (not SCPI) ──
 * Confirmed end-to-end at the bench on the real instrument: UART2 on
 * GPIO5(RX)/GPIO6(TX), 9600 8N1, via MAX232 #2. Command set confirmed
 * against the manufacturer's manual (RS-232 output/command format
 * section) and cross-checked against bench behavior: R0/R1 (Remote
 * OFF/ON), F0-F7 (function select — this specific unit is the
 * "1.5(3.0)GHz, U/C" variant: F0=FreqA, F1=FreqB, F2=FreqC, F3=Period,
 * F4=Total, F5=unused/NC on this variant, F6=Ratio A/B, F7=Time
 * Interval A->B), G0-G3 (gate 0.01/0.1/1/10 sec), H0/H1/H2 (Hold
 * OFF/ON/TOGGLE), D<anything> (data request — manual confirms the
 * parameter is "DON'T CARE"). Response format confirmed fixed-width:
 * 10 bytes decimal (incl. decimal point) + 4 bytes units + CR — parsed
 * by that spec, not by splitting on whitespace, so it stays robust if
 * significant-digit count changes. Command terminator confirmed CR
 * only (0DH), not CRLF, for every command. Both readDmmLine() and
 * readCounterLine() correctly handle CR-only, LF-only, or CRLF framing
 * (the DMM and Counter don't share the same convention — an early bug
 * here, applying the DMM's CRLF assumption to the counter's CR-only
 * framing, caused responses to run together until the RX buffer
 * force-truncated mid-value; both readers are now framing-aware).
 *
 * ── DG4062 device identification ──
 * VID:PID filter (1AB1:0641) gates interface claiming — a TMC device
 * with a different VID:PID connects but is deliberately left
 * unclaimed, logged as a sys warning, so a different TMC instrument
 * plugged into the same port doesn't get silently treated as the
 * DG4062.
 *
 * Board: ESP32-S3 Dev Module (developed on a Hosyond N16R8 — 16MB
 *        flash / 8MB OPI PSRAM; single native USB-OTG port, no
 *        separate USB-UART bridge chip — this is why Serial Monitor
 *        isn't available once USB is committed to host mode)
 * Built with PlatformIO — see platformio.ini for the pinned platform
 *        (pioarduino, not stock espressif32 — see above), board
 *        overrides, USB-mode build flags, and library dependencies.
 * UART1 wiring (via MAX232 #1 — DMM):
 *   GPIO17 = TX   (to MAX232 T1IN)
 *   GPIO18 = RX   (from MAX232 R1OUT)
 * UART2 wiring (via MAX232 #2 — Counter):
 *   GPIO6  = TX   (to MAX232 T1IN)
 *   GPIO5  = RX   (from MAX232 R1OUT)
 */

#include <Arduino.h>
// PlatformIO note: unlike Arduino IDE's .ino build (which auto-includes
// Arduino.h and auto-generates function prototypes by scanning the
// file), building this as main.cpp under PlatformIO does neither. The
// explicit #include above and the forward-declaration block below make
// this a normal, order-independent C++ translation unit instead of
// relying on definitions happening to already be in dependency order.
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "usb/usb_host.h"

// ─── Firmware Version ────────────────────────────────────────────────────────
const char* FW_VERSION = "v2.1.3";
// ─────────────────────────────────────────────────────────────────────────────

// ─── WiFi Credentials ────────────────────────────────────────────────────────
#include "secrets.h"
// ─────────────────────────────────────────────────────────────────────────────

// ─── RS-232 / UART1 Configuration — DMM ──────────────────────────────────────
#define DMM_UART         Serial1
#define UART1_TX_PIN     17
#define UART1_RX_PIN     18
#define UART1_BAUD       9600
#define DMM_TERM         "\r\n"     // BK 5491B DMM

// ─── RS-232 / UART2 Configuration — Counter ──────────────────────────────────
// CONFIRMED at the bench against real BK1823A hardware (9600 8N1, GPIO5/6,
// CR-only command terminator) — see file header for the full bench-confirmed
// command set.
#define COUNTER_UART     Serial2
#define UART2_TX_PIN     6
#define UART2_RX_PIN     5          // non-strapping general-purpose pin
#define UART2_BAUD       9600
#define COUNTER_TERM     "\r"      // CONFIRMED — BK1823A manual: "Terminate
                                    // Code: CR (0DH)" for every command
                                    // (H/G/D/F/R). Previously sent CRLF
                                    // (matching the DMM) — apparently
                                    // tolerated, but non-compliant; fixed
                                    // to match spec exactly now that it's
                                    // known.

// How often to ping WebSocket clients to keep connections alive (milliseconds)
#define WS_PING_INTERVAL_MS  20000

// Receive buffers — one per RS-232 UART, kept fully independent
#define RX_BUF_SIZE  256
static char dmmRxBuf[RX_BUF_SIZE];
static int  dmmRxLen = 0;
static char ctrRxBuf[RX_BUF_SIZE];
static int  ctrRxLen = 0;

static unsigned long lastPingMs = 0;
// ─────────────────────────────────────────────────────────────────────────────

// ─── USB-TMC Configuration — Function Gen ────────────────────────────────────
// Ported from USBTMC_Host.ino. See that file's original header for the full
// USB-TMC protocol primer; comments here focus on what changed for this merge.

#define TMC_MSGID_DEV_DEP_OUT          1
#define TMC_MSGID_REQUEST_DEV_DEP_IN   2
#define TMC_HEADER_SIZE               12

#define USB_DESC_TYPE_INTERFACE   0x04
#define USB_DESC_TYPE_ENDPOINT    0x05

#define TMC_CLASS      0xFE
#define TMC_SUBCLASS   0x03

#define TMC_OUT_PAYLOAD_MAX    256
#define TMC_IN_PAYLOAD_MAX     512

// Gap between OUT command and REQUEST_IN — instrument prep time.
// Increase if funcgen queries return timeout errors.
#define TMC_QUERY_GAP_MS       1000

#define TMC_XFER_TIMEOUT_MS   10000

// Rigol DG4062 — from the original project's instrument inventory comment.
// Interface claim is gated on this match; see file header.
#define DG4062_VID  0x1AB1
#define DG4062_PID  0x0641

typedef enum {
    DEV_STATE_IDLE,
    DEV_STATE_OPENED,
    DEV_STATE_READY
} dev_state_t;

#define MAX_CLAIM_ATTEMPTS  100   // 100 x ~20ms loop = ~2 seconds max

static volatile bool            s_tmc_ready        = false;
static usb_device_handle_t      s_dev_hdl          = nullptr;
static usb_host_client_handle_t s_client_hdl       = nullptr;
static uint8_t                  s_bulk_out_ep      = 0;
static uint8_t                  s_bulk_in_ep       = 0;
static uint8_t                  s_intf_num         = 0;
static uint16_t                 s_vid              = 0;
static uint16_t                 s_pid              = 0;
static volatile uint8_t         s_dev_addr_pending = 0;
static dev_state_t              s_dev_state        = DEV_STATE_IDLE;
static int                      s_claim_attempts   = 0;

static uint8_t s_btag = 1;

static SemaphoreHandle_t s_xfer_sem   = nullptr;
static esp_err_t         s_xfer_err   = ESP_OK;
static size_t            s_xfer_bytes = 0;

// ─── Func Gen command queue ───────────────────────────────────────────────
// Added after a real, observed reliability bug: handleFuncgenCommand() was
// previously called SYNCHRONOUSLY from onWsEvent(), which runs in
// AsyncTCP's own task. A single browser-side "refresh both channels"
// action fires ~30+ Func Gen queries back-to-back, each blocking for
// TMC_QUERY_GAP_MS (1s) or more — a sustained 30-40+ second window where
// the AsyncTCP task couldn't service WebSocket pings, TCP ACKs, or
// anything else on the connection. The browser (or an intermediate
// router) reasonably concluded the connection was dead and dropped it —
// which immediately triggered a reconnect, which immediately re-fired
// the same expensive query burst, producing a self-perpetuating
// disconnect loop. Confirmed at the bench (see project notes).
//
// Fix: onWsEvent() now only enqueues the command (fast, non-blocking)
// instead of processing it inline. A dedicated funcgenWorkerTask drains
// the queue and calls the SAME handleFuncgenCommand() as before,
// unchanged — just from its own task context, where blocking on a slow
// USB-TMC transfer no longer affects the WebSocket connection's own
// responsiveness. The queue is strictly FIFO and single-consumer, so
// the ordering guarantee the browser's JS relies on (responses arrive
// back in the same order queries were sent — see index_html.h's Func
// Gen section) is fully preserved; no JS changes were needed for this.
struct FuncgenQueueItem {
  char cmd[64];
  long seq;  // browser-assigned sequence number, echoed back with the
              // response so the browser can verify its own FIFO
              // assumption is actually holding — see index_html.h's own
              // seq-tracking comment for the full story of why this
              // exists. -1 means "no seq provided" (shouldn't normally
              // happen once the browser side is updated, but handled
              // gracefully either way).
};
static QueueHandle_t s_funcgen_queue = nullptr;
#define FUNCGEN_QUEUE_DEPTH 64  // comfortably covers one full
                                 // refreshBothGenChannels() burst
                                 // (~34 queries) with headroom

// ─── Forward declarations ─────────────────────────────────────────────────
// See PlatformIO note above — this block replaces what Arduino IDE's .ino
// builder would have auto-generated.
void         wsLog(const String& msg);
void         flushBootLog(AsyncWebSocketClient* client);
void         sendToDmm(const String& cmd);
bool         readDmmLine();
void         sendToCounter(const String& cmd);
bool         readCounterLine();
void         pushEvent(const String& target, const String& msg, long seq = -1);
void         pushErr(const String& target, const String& msg, long seq = -1);
static uint8_t    nextTag();
static void       xferCallback(usb_transfer_t *xfer);
static bool       findTmcInterface(const usb_config_desc_t *cfg);
static esp_err_t  sendSetConfiguration();
static void       usbClientEventCb(const usb_host_client_event_msg_t *msg, void *arg);
static void       doClaimInterface();
static void       usbLibTask(void *arg);
static void       usbClientTask(void *arg);
static esp_err_t  tmcSendCommand(const char *cmd);
static esp_err_t  tmcRequestIn(uint8_t *outBuf, size_t maxLen, size_t *gotLen);
static void       handleFuncgenCommand(const String& cmd, long seq);
static void       funcgenWorkerTask(void *arg);
void         onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                        AwsEventType type, void* arg, uint8_t* data, size_t len);
// ─────────────────────────────────────────────────────────────────────────────

// ─── Embedded HTML ────────────────────────────────────────────────────────────
#include "index_html.h"
// ─────────────────────────────────────────────────────────────────────────────

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ─── Debug: WS "sys" events replace Serial ───────────────────────────────────
// No target field — these are firmware-level messages, not tied to one
// instrument. A small ring buffer captures messages logged before any
// browser is connected (WiFi connect sequence, USB enumeration at boot)
// and flushes them to the first client that connects, so that early
// window isn't silently lost — see file header for the tradeoff this
// replaces (a wired Serial console).
#define BOOT_LOG_RING_SIZE  24
static String bootLogRing[BOOT_LOG_RING_SIZE];
static int    bootLogCount = 0;   // number of entries written so far (caps at ring size)
static int    bootLogHead  = 0;   // next write index
static bool   everConnected = false;

void wsLog(const String& msg) {
  if (!everConnected) {
    bootLogRing[bootLogHead] = msg;
    bootLogHead = (bootLogHead + 1) % BOOT_LOG_RING_SIZE;
    if (bootLogCount < BOOT_LOG_RING_SIZE) bootLogCount++;
  }
  JsonDocument doc;
  doc["event"] = "sys";
  doc["msg"]   = msg;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

void flushBootLog(AsyncWebSocketClient* client) {
  // Oldest-first: if the ring wrapped, start from bootLogHead (oldest);
  // otherwise start from 0.
  int start = (bootLogCount < BOOT_LOG_RING_SIZE) ? 0 : bootLogHead;
  for (int i = 0; i < bootLogCount; i++) {
    int idx = (start + i) % BOOT_LOG_RING_SIZE;
    JsonDocument doc;
    doc["event"] = "sys";
    doc["msg"]   = bootLogRing[idx];
    String out;
    serializeJson(doc, out);
    client->text(out);
  }
}
// ─────────────────────────────────────────────────────────────────────────────

// ─── RS-232 helpers — DMM (target "dmm") ─────────────────────────────────────
void sendToDmm(const String& cmd) {
  DMM_UART.print(cmd);
  DMM_UART.print(DMM_TERM);
}

// Tracks the terminator char that most recently ended a line, so a CRLF
// pair's second character can be swallowed instead of firing a spurious
// empty line. 0 = no pending terminator.
static char dmmLastTerm = 0;

bool readDmmLine() {
  while (DMM_UART.available()) {
    char c = (char)DMM_UART.read();
    if (c == '\r' || c == '\n') {
      if (dmmRxLen > 0) {
        dmmRxBuf[dmmRxLen] = '\0';
        dmmRxLen = 0;
        dmmLastTerm = c;
        return true;
      }
      if (dmmLastTerm != 0 && c != dmmLastTerm) {
        dmmLastTerm = 0;  // this was the second half of a CRLF pair
        continue;
      }
      continue;  // stray/duplicate terminator with nothing buffered
    }
    dmmLastTerm = 0;
    if (dmmRxLen >= RX_BUF_SIZE - 1) {
      dmmRxBuf[dmmRxLen] = '\0';
      dmmRxLen = 0;
      return true;
    }
    dmmRxBuf[dmmRxLen++] = c;
  }
  return false;
}

// ─── RS-232 helpers — Counter (target "counter") ─────────────────────────────
// This instrument does NOT use SCPI — commands are short tokens (F0/F1,
// G0/G1, D0/D1, R1, etc.) confirmed at the bench. Framing fix below was
// prompted by a real bench observation: many D1 responses arrived
// concatenated into one giant blob. The old code (matching readDmmLine's
// original CRLF-only assumption) discarded every '\r' unconditionally —
// correct for the DMM's actual CRLF termination, but wrong if this
// instrument terminates with CR only. This version handles CR-only,
// LF-only, or CRLF correctly, same as the DMM path above.
void sendToCounter(const String& cmd) {
  COUNTER_UART.print(cmd);
  COUNTER_UART.print(COUNTER_TERM);
}

static char ctrLastTerm = 0;

bool readCounterLine() {
  while (COUNTER_UART.available()) {
    char c = (char)COUNTER_UART.read();
    if (c == '\r' || c == '\n') {
      if (ctrRxLen > 0) {
        ctrRxBuf[ctrRxLen] = '\0';
        ctrRxLen = 0;
        ctrLastTerm = c;
        return true;
      }
      if (ctrLastTerm != 0 && c != ctrLastTerm) {
        ctrLastTerm = 0;
        continue;
      }
      continue;
    }
    ctrLastTerm = 0;
    if (ctrRxLen >= RX_BUF_SIZE - 1) {
      ctrRxBuf[ctrRxLen] = '\0';
      ctrRxLen = 0;
      return true;
    }
    ctrRxBuf[ctrRxLen++] = c;
  }
  return false;
}

// ─── Outgoing WS event helper (tagged) ───────────────────────────────────────
void pushEvent(const String& target, const String& msg, long seq) {
  String safe = msg;
  safe.trim();
  JsonDocument doc;
  doc["event"]  = "resp";
  doc["target"] = target;
  doc["msg"]    = safe;   // ArduinoJson handles quote-escaping itself —
                           // no manual replace("\"","'") needed anymore.
  if (seq >= 0) doc["seq"] = seq;  // only Func Gen passes a real seq —
                                     // DMM/Counter calls are unaffected,
                                     // this field is simply omitted for them
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

void pushErr(const String& target, const String& msg, long seq) {
  JsonDocument doc;
  doc["event"]  = "err";
  doc["target"] = target;
  doc["msg"]    = msg;
  if (seq >= 0) doc["seq"] = seq;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}
// ─────────────────────────────────────────────────────────────────────────────

// ─── USB-TMC layer (ported from USBTMC_Host.ino; Console.printf -> wsLog) ────

static uint8_t nextTag() {
    if (++s_btag == 0) s_btag = 1;
    return s_btag;
}

static void xferCallback(usb_transfer_t *xfer) {
    s_xfer_err   = (xfer->status == USB_TRANSFER_STATUS_COMPLETED)
                   ? ESP_OK : ESP_FAIL;
    s_xfer_bytes = (size_t)xfer->actual_num_bytes;
    xSemaphoreGive(s_xfer_sem);
}

static bool findTmcInterface(const usb_config_desc_t *cfg) {
    wsLog("[USB] Interfaces=" + String(cfg->bNumInterfaces) +
          " Config=" + String(cfg->bConfigurationValue) +
          " MaxPower=" + String(cfg->bMaxPower * 2) + "mA");

    const uint8_t *p   = (const uint8_t *)cfg;
    const uint8_t *end = p + cfg->wTotalLength;

    bool    in_tmc   = false;
    uint8_t tmc_intf = 0;
    uint8_t ep_out   = 0, ep_in = 0;

    while (p < end) {
        uint8_t len  = p[0];
        uint8_t type = p[1];
        if (len < 2 || (p + len) > end) break;

        if (type == USB_DESC_TYPE_INTERFACE) {
            in_tmc = false;
            ep_out = ep_in = 0;
            if (p[5] == TMC_CLASS && p[6] == TMC_SUBCLASS) {
                in_tmc   = true;
                tmc_intf = p[2];
                wsLog("[USB] TMC intf #" + String(p[2]) + " found");
            }
        } else if (type == USB_DESC_TYPE_ENDPOINT && in_tmc) {
            uint8_t  addr      = p[2];
            uint8_t  attr      = p[3];
            uint8_t  xfer_type = attr & 0x03;
            if (xfer_type == 0x02) {           // bulk only
                if (addr & 0x80) ep_in  = addr;
                else             ep_out = addr;
            }
            if (ep_in && ep_out) {
                s_bulk_in_ep  = ep_in;
                s_bulk_out_ep = ep_out;
                s_intf_num    = tmc_intf;
                return true;
            }
        }
        p += len;
    }
    return false;
}

static esp_err_t sendSetConfiguration() {
    usb_transfer_t *ctrl = nullptr;
    esp_err_t err = usb_host_transfer_alloc(8, 0, &ctrl);
    if (err != ESP_OK) return err;

    uint8_t *s = ctrl->data_buffer;
    s[0] = 0x00; s[1] = 0x09; s[2] = 0x01; s[3] = 0x00;
    s[4] = 0x00; s[5] = 0x00; s[6] = 0x00; s[7] = 0x00;

    ctrl->device_handle    = s_dev_hdl;
    ctrl->bEndpointAddress = 0x00;
    ctrl->callback         = xferCallback;
    ctrl->context          = nullptr;
    ctrl->num_bytes        = 8;
    ctrl->timeout_ms       = 1000;

    xSemaphoreTake(s_xfer_sem, 0);
    err = usb_host_transfer_submit_control(s_client_hdl, ctrl);
    if (err == ESP_OK) {
        uint32_t start = millis();
        while (xSemaphoreTake(s_xfer_sem, 0) != pdTRUE) {
            if (millis() - start > 2000) { err = ESP_ERR_TIMEOUT; break; }
            usb_host_client_handle_events(s_client_hdl, 1);
        }
        if (err == ESP_OK) err = s_xfer_err;
    }
    usb_host_transfer_free(ctrl);
    return err;
}

static void usbClientEventCb(const usb_host_client_event_msg_t *msg, void *arg) {
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        uint8_t addr = msg->new_dev.address;

        if (usb_host_device_open(s_client_hdl, addr, &s_dev_hdl) != ESP_OK) {
            wsLog("[USB] device_open failed"); return;
        }

        const usb_device_desc_t *dev_desc = nullptr;
        usb_host_get_device_descriptor(s_dev_hdl, &dev_desc);
        s_vid = dev_desc->idVendor;
        s_pid = dev_desc->idProduct;
        wsLog("[USB] VID:PID = " + String(s_vid, HEX) + ":" + String(s_pid, HEX));

        // Gate on the DG4062's known VID:PID — see file header. A different
        // TMC instrument connecting here is deliberately left unclaimed
        // rather than silently adopted as "the function gen."
        if (s_vid != DG4062_VID || s_pid != DG4062_PID) {
            wsLog("[USB] WARNING: connected device is not the DG4062 "
                  "(expected " + String(DG4062_VID, HEX) + ":" +
                  String(DG4062_PID, HEX) + ") — not claiming.");
            usb_host_device_close(s_client_hdl, s_dev_hdl);
            s_dev_hdl = nullptr;
            return;
        }

        const usb_config_desc_t *cfg_desc = nullptr;
        usb_host_get_active_config_descriptor(s_dev_hdl, &cfg_desc);

        if (!findTmcInterface(cfg_desc)) {
            wsLog("[USB] No TMC interface found.");
            usb_host_device_close(s_client_hdl, s_dev_hdl);
            s_dev_hdl = nullptr; return;
        }

        esp_err_t err = usb_host_interface_claim(s_client_hdl, s_dev_hdl,
                                                  s_intf_num, 0);
        if (err == ESP_OK) {
            wsLog("[USB] DG4062 TMC interface claimed — ready.");
            s_tmc_ready = true;
            s_dev_state = DEV_STATE_READY;
        } else {
            s_dev_state = DEV_STATE_OPENED;
            s_claim_attempts = 0;
        }

    } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        wsLog("[USB] Function gen disconnected.");
        s_tmc_ready = false;
        s_dev_state = DEV_STATE_IDLE;
        // Same reasoning as the WS_EVT_DISCONNECT fix in onWsEvent(), a
        // second distinct trigger for the same underlying problem:
        // whatever's still queued but unanswered when the DG4062 itself
        // goes away doesn't get flushed just because the WebSocket
        // connection is still up — the browser session never dropped
        // here, only the USB device did. Confirmed at the bench:
        // power-cycling the Rigol mid-session left AM modulation
        // sub-parameter queries (from settings configured before the
        // power-cycle) still queued; once the device came back, the
        // firmware worked through that stale backlog and its answers
        // bled into the middle of an otherwise-fresh, unrelated refresh
        // — the same class of corruption, just a different trigger than
        // a WebSocket-level disconnect.
        if (s_funcgen_queue) xQueueReset(s_funcgen_queue);
        if (s_dev_hdl) {
            usb_host_interface_release(s_client_hdl, s_dev_hdl, s_intf_num);
            usb_host_device_close(s_client_hdl, s_dev_hdl);
            s_dev_hdl = nullptr;
            s_bulk_in_ep = s_bulk_out_ep = 0;
        }
    }
}

static void doClaimInterface() {
    esp_err_t err = usb_host_interface_claim(s_client_hdl, s_dev_hdl,
                                             s_intf_num, 0);
    s_claim_attempts++;
    if (err == ESP_OK) {
        wsLog("[USB] DG4062 TMC interface claimed — ready.");
        s_tmc_ready = true;
        s_dev_state = DEV_STATE_READY;
        return;
    }
    if (s_claim_attempts >= MAX_CLAIM_ATTEMPTS) {
        wsLog("[USB] interface_claim failed after " + String(s_claim_attempts) +
              " attempts: 0x" + String(err, HEX));
        usb_host_device_close(s_client_hdl, s_dev_hdl);
        s_dev_hdl    = nullptr;
        s_bulk_in_ep = s_bulk_out_ep = 0;
        s_dev_state  = DEV_STATE_IDLE;
    }
}

static void usbLibTask(void *arg) {
    while (true) {
        uint32_t flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_uninstall();
            break;
        }
    }
    vTaskDelete(nullptr);
}

static void usbClientTask(void *arg) {
    usb_host_client_config_t client_cfg = {};
    client_cfg.is_synchronous               = false;
    client_cfg.max_num_event_msg            = 8;
    client_cfg.async.client_event_callback  = usbClientEventCb;
    client_cfg.async.callback_arg           = nullptr;

    if (usb_host_client_register(&client_cfg, &s_client_hdl) != ESP_OK) {
        wsLog("[USB] client_register failed");
        vTaskDelete(nullptr);
        return;
    }
    wsLog("[USB] Client registered — waiting for DG4062.");

    while (true) {
        usb_host_client_handle_events(s_client_hdl, 10);
        switch (s_dev_state) {
            case DEV_STATE_IDLE:
                break;  // s_dev_addr_pending path unused — NEW_DEV handled
                        // directly in usbClientEventCb(), same as the
                        // original USBTMC_Host.ino (see that file's own
                        // comments — this Phase-1 path is a documented
                        // fallback that in practice is never reached).
            case DEV_STATE_OPENED:
                doClaimInterface();
                break;
            case DEV_STATE_READY:
                break;
        }
    }
    vTaskDelete(nullptr);
}

static esp_err_t tmcSendCommand(const char *cmd) {
    size_t cmdLen = strlen(cmd);
    size_t padLen = (cmdLen + 3) & ~3u;
    size_t total  = TMC_HEADER_SIZE + padLen;

    usb_transfer_t *xfer = nullptr;
    esp_err_t err = usb_host_transfer_alloc(total, 0, &xfer);
    if (err != ESP_OK) return err;

    uint8_t tag = nextTag();
    uint8_t *b  = xfer->data_buffer;

    b[0]  = TMC_MSGID_DEV_DEP_OUT;
    b[1]  = tag;
    b[2]  = (uint8_t)(~tag);
    b[3]  = 0x00;
    b[4]  = (uint8_t)(cmdLen);
    b[5]  = (uint8_t)(cmdLen >> 8);
    b[6]  = (uint8_t)(cmdLen >> 16);
    b[7]  = (uint8_t)(cmdLen >> 24);
    b[8]  = 0x01;
    b[9]  = b[10] = b[11] = 0x00;

    memcpy(b + TMC_HEADER_SIZE, cmd, cmdLen);
    if (padLen > cmdLen)
        memset(b + TMC_HEADER_SIZE + cmdLen, 0x00, padLen - cmdLen);

    xfer->device_handle    = s_dev_hdl;
    xfer->bEndpointAddress = s_bulk_out_ep;
    xfer->callback         = xferCallback;
    xfer->context          = nullptr;
    xfer->num_bytes        = (int)total;
    xfer->timeout_ms       = TMC_XFER_TIMEOUT_MS;

    xSemaphoreTake(s_xfer_sem, 0);
    err = usb_host_transfer_submit(xfer);
    if (err == ESP_OK) {
        xSemaphoreTake(s_xfer_sem, portMAX_DELAY);
        err = s_xfer_err;
    }
    usb_host_transfer_free(xfer);
    return err;
}

static esp_err_t tmcRequestIn(uint8_t *outBuf, size_t maxLen, size_t *gotLen) {
    *gotLen = 0;

    usb_transfer_t *req = nullptr;
    esp_err_t err = usb_host_transfer_alloc(TMC_HEADER_SIZE, 0, &req);
    if (err != ESP_OK) return err;

    uint8_t tag = nextTag();
    uint8_t *rb = req->data_buffer;
    rb[0]  = TMC_MSGID_REQUEST_DEV_DEP_IN;
    rb[1]  = tag;
    rb[2]  = (uint8_t)(~tag);
    rb[3]  = 0x00;
    rb[4]  = (uint8_t)(maxLen);
    rb[5]  = (uint8_t)(maxLen >> 8);
    rb[6]  = (uint8_t)(maxLen >> 16);
    rb[7]  = (uint8_t)(maxLen >> 24);
    rb[8]  = 0x02;
    rb[9]  = '\n';
    rb[10] = rb[11] = 0x00;

    req->device_handle    = s_dev_hdl;
    req->bEndpointAddress = s_bulk_out_ep;
    req->callback         = xferCallback;
    req->context          = nullptr;
    req->num_bytes        = TMC_HEADER_SIZE;
    req->timeout_ms       = TMC_XFER_TIMEOUT_MS;

    xSemaphoreTake(s_xfer_sem, 0);
    err = usb_host_transfer_submit(req);
    if (err == ESP_OK) {
        xSemaphoreTake(s_xfer_sem, portMAX_DELAY);
        err = s_xfer_err;
    }
    usb_host_transfer_free(req);
    if (err != ESP_OK) return err;

    size_t inTotal = ((TMC_HEADER_SIZE + maxLen + 63) / 64) * 64;
    usb_transfer_t *rsp = nullptr;
    err = usb_host_transfer_alloc(inTotal, 0, &rsp);
    if (err != ESP_OK) return err;

    rsp->device_handle    = s_dev_hdl;
    rsp->bEndpointAddress = s_bulk_in_ep;
    rsp->callback         = xferCallback;
    rsp->context          = nullptr;
    rsp->num_bytes        = (int)inTotal;
    rsp->timeout_ms       = TMC_XFER_TIMEOUT_MS;

    xSemaphoreTake(s_xfer_sem, 0);
    err = usb_host_transfer_submit(rsp);
    if (err == ESP_OK) {
        xSemaphoreTake(s_xfer_sem, portMAX_DELAY);
        err = s_xfer_err;
    }

    if (err == ESP_OK && s_xfer_bytes > (size_t)TMC_HEADER_SIZE) {
        uint8_t *d = rsp->data_buffer;
        size_t reported = (uint32_t)d[4] | ((uint32_t)d[5] << 8)
                        | ((uint32_t)d[6] << 16) | ((uint32_t)d[7] << 24);
        *gotLen = (reported < maxLen) ? reported : maxLen;
        memcpy(outBuf, d + TMC_HEADER_SIZE, *gotLen);
    }

    usb_host_transfer_free(rsp);
    return err;
}

// ── High-level SCPI query/send for the function gen ──────────────────────
// Called synchronously from the WS handler — see file header for why this
// (unlike the DMM/Counter UART paths) can't be a pure non-blocking relay.
static void handleFuncgenCommand(const String& cmd, long seq) {
    if (!s_tmc_ready) {
        pushErr("funcgen", "No DG4062 connected", seq);
        return;
    }
    // indexOf, not endsWith: AMPL?/PHASe? are the first queries in this
    // project that take a parameter AFTER the '?' (e.g. the harmonic
    // index — ":SOUR1:HARM:AMPL? 2"), so the command's last character
    // is the parameter digit, not '?'. endsWith("?") silently
    // misclassified these as SET commands, skipping the actual TMC
    // read-back and returning a fabricated "OK" instead of the real
    // value — confirmed at the bench (Ampl/Phase Offset both showed
    // literal "OK" instead of a number). Every other query in this
    // project has '?' as its literal last character, so this is a
    // strict widening, not a behavior change for anything else.
    bool isQuery = cmd.indexOf('?') >= 0;
    esp_err_t err = tmcSendCommand(cmd.c_str());
    if (err != ESP_OK) {
        pushErr("funcgen", "SEND ERROR 0x" + String(err, HEX), seq);
        return;
    }
    if (!isQuery) {
        pushEvent("funcgen", "OK", seq);  // no response expected; ack for
                                            // the browser arbiter's 'set'-
                                            // kind wait — see index_html.h
        return;
    }
    delay(TMC_QUERY_GAP_MS);
    static uint8_t rxBuf[TMC_IN_PAYLOAD_MAX + 1];
    size_t gotLen = 0;
    err = tmcRequestIn(rxBuf, TMC_IN_PAYLOAD_MAX, &gotLen);
    if (err != ESP_OK) {
        pushErr("funcgen", "RECV ERROR 0x" + String(err, HEX), seq);
        return;
    }
    rxBuf[gotLen] = '\0';
    pushEvent("funcgen", String((char*)rxBuf), seq);
}

// Drains s_funcgen_queue one item at a time, calling the same
// handleFuncgenCommand() logic as before — just decoupled from
// AsyncTCP's task. xQueueReceive with portMAX_DELAY blocks efficiently
// (no CPU spin) when the queue is empty.
static void funcgenWorkerTask(void *arg) {
    FuncgenQueueItem item;
    while (true) {
        if (xQueueReceive(s_funcgen_queue, &item, portMAX_DELAY) == pdTRUE) {
            handleFuncgenCommand(String(item.cmd), item.seq);
        }
    }
}
// ─────────────────────────────────────────────────────────────────────────────

// ─── WebSocket event handler ──────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket* server,
               AsyncWebSocketClient* client,
               AwsEventType type,
               void* arg,
               uint8_t* data,
               size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      everConnected = true;
      client->text("{\"event\":\"sys\",\"msg\":\"Multi-Instrument Panel ready\"}");
      flushBootLog(client);
      break;

    case WS_EVT_DISCONNECT:
      // Flush any Func Gen commands still queued but not yet processed.
      // This firmware-side queue has no awareness of WebSocket-level
      // disconnects at all — without this, a command queued by a now-
      // dead browser session (e.g. a liveness probe's *IDN? that hadn't
      // been answered yet) would still get processed and answered even
      // after a brand new client connects, and that stray response
      // would land on whatever's at the front of the NEW session's own
      // response queue, corrupting its field-matching. Confirmed at the
      // bench: a log showing repeated stray "Rigol Technologies..."
      // (*IDN? response) responses in a fresh session, with no
      // corresponding *IDN? send anywhere in that session's own log —
      // leftover answers to a PREVIOUS session's probes. One residual,
      // smaller edge case this doesn't cover: a command already
      // mid-transfer (not just queued) when the disconnect happens will
      // still complete and could produce one stray response — reset
      // only clears what hasn't started yet, not what's already
      // in-flight on the USB side.
      if (s_funcgen_queue) xQueueReset(s_funcgen_queue);
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        JsonDocument doc;
        DeserializationError jerr = deserializeJson(doc, data, len);
        if (jerr) {
          wsLog("[WS] Bad JSON from client: " + String(jerr.c_str()));
          break;
        }
        const char* target = doc["target"] | "";
        const char* cmdRaw = doc["cmd"] | "";
        String cmd = String(cmdRaw);
        cmd.trim();
        if (cmd.length() == 0) break;
        long seq = doc["seq"] | -1L;  // only funcgen commands carry a
                                        // real seq — see index_html.h

        if (strcmp(target, "dmm") == 0) {
          sendToDmm(cmd);
        } else if (strcmp(target, "counter") == 0) {
          sendToCounter(cmd);
        } else if (strcmp(target, "funcgen") == 0) {
          // __RESYNC__ is a sentinel, never a real SCPI command — the
          // browser sends this specifically when ITS OWN query timeout
          // gives up waiting on something (see index_html.h's
          // armGenQueryTimeout()). Confirmed at the bench: the earlier
          // queue-flush fixes only covered WS/USB disconnect — but a
          // long, continuous session with neither ever happening can
          // still leave stale commands in this firmware-side queue if
          // the BROWSER gives up on one first. Those stale entries then
          // get answered anyway, arbitrarily later, misapplied to
          // whatever the browser is asking about by then. This closes
          // that gap: whenever the browser's own timeout fires, it
          // tells the firmware to give up too, not just itself.
          if (cmd == "__RESYNC__") {
            if (s_funcgen_queue) xQueueReset(s_funcgen_queue);
            break;
          }
          // Diagnostic, not a fix — added by explicit request to settle
          // whether repeated *IDN? queries genuinely come from one
          // client sending them more than once, or from more than one
          // client (e.g. a stale connection alongside the current one),
          // before committing to a fix direction based on a guess.
          if (cmd == "*IDN?") {
            wsLog("[GEN] *IDN? received from client #" + String(client->id()) + ", seq=" + String(seq));
          }
          // Enqueue only — see the funcgen queue comment near its
          // declaration for why this can no longer call
          // handleFuncgenCommand() directly from here.
          FuncgenQueueItem item;
          strncpy(item.cmd, cmd.c_str(), sizeof(item.cmd) - 1);
          item.cmd[sizeof(item.cmd) - 1] = '\0';
          item.seq = seq;
          if (xQueueSend(s_funcgen_queue, &item, 0) != pdTRUE) {
            // Include the seq here too — if the browser never learns
            // which seq got dropped, it would wait forever for a reply
            // that's never coming and misflag every subsequent response
            // as a mismatch.
            pushErr("funcgen", "queue full, command dropped", seq);
          }
        } else {
          wsLog("[WS] Unknown target: '" + String(target) + "'");
        }
      }
      break;
    }

    case WS_EVT_ERROR:
      break;

    default: break;
  }
}
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  // No Serial.begin() — native USB is committed to TMC host mode; see
  // file header. All logging goes out over WS as "sys" events instead.

  // ── WiFi ───────────────────────────────────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wsLog(String("Connecting to ") + WIFI_SSID + " ...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  wsLog("WiFi connected. IP: " + WiFi.localIP().toString());

  // ── mDNS ───────────────────────────────────────────────────────────────────
  if (MDNS.begin(LOCAL_URL)) {
    MDNS.addService("http", "tcp", 80);
    wsLog(String("mDNS started: http://") + LOCAL_URL);
  } else {
    wsLog("mDNS failed to start — use the IP address instead");
  }

  // ── WebSocket + HTTP ───────────────────────────────────────────────────────
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "text/html",
      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        size_t total = sizeof(INDEX_HTML) - 1;
        if (index >= total) return 0;
        size_t len = min(maxLen, total - index);
        memcpy_P(buffer, INDEX_HTML + index, len);
        return len;
      });
    request->send(response);
  });
  server.on("/help-url", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", HELP_URL);
  });
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
  wsLog("HTTP server started");

  // ── UART1 RS-232 (DMM) ───────────────────────────────────────────────────
  DMM_UART.begin(UART1_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
  delay(100);
  while (DMM_UART.available()) DMM_UART.read();  // drain startup noise
  wsLog("UART1 (DMM) ready at " + String(UART1_BAUD) + " baud");

  // ── UART2 RS-232 (Counter) ───────────────────────────────────────────────
  COUNTER_UART.begin(UART2_BAUD, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
  delay(100);
  while (COUNTER_UART.available()) COUNTER_UART.read();
  wsLog("UART2 (Counter) ready at " + String(UART2_BAUD) + " baud "
        "[confirmed at the bench against real BK1823A hardware]");

  // ── USB-TMC host (Function Gen) ──────────────────────────────────────────
  s_xfer_sem = xSemaphoreCreateBinary();
  usb_host_config_t host_cfg = {};
  host_cfg.skip_phy_setup = false;
  host_cfg.intr_flags     = ESP_INTR_FLAG_LEVEL1;
  if (usb_host_install(&host_cfg) != ESP_OK) {
    wsLog("[USB] host_install failed");
  } else {
    wsLog("[USB] Host installed — waiting for DG4062");
    xTaskCreatePinnedToCore(usbLibTask,    "usb_lib",    4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(usbClientTask, "usb_client", 4096, nullptr, 2, nullptr, 0);
  }

  // Decouples Func Gen command processing from the WS event handler —
  // see the queue declaration's comment for the reliability bug this
  // fixes. Pinned to core 1 (separate from the USB host tasks above,
  // which need core 0) — the actual unblocking signal for a TMC
  // transfer comes from usbClientTask's own event processing regardless
  // of which core this worker runs on, so core 1 just avoids adding
  // more load to core 0.
  s_funcgen_queue = xQueueCreate(FUNCGEN_QUEUE_DEPTH, sizeof(FuncgenQueueItem));
  if (!s_funcgen_queue) {
    wsLog("[GEN] ERROR: failed to create command queue");
  } else {
    xTaskCreatePinnedToCore(funcgenWorkerTask, "funcgen_worker", 4096, nullptr, 2, nullptr, 1);
  }

  wsLog("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
  wsLog("INDEX_HTML compiled size: " + String((unsigned)sizeof(INDEX_HTML)) + " bytes");
  wsLog(String("=== Multi-Instrument Panel ") + FW_VERSION + " ready ===");
}

void loop() {
  ws.cleanupClients();

  unsigned long now = millis();
  if (now - lastPingMs >= WS_PING_INTERVAL_MS) {
    lastPingMs = now;
    ws.pingAll();
  }

  // ── Pure relay — DMM ──────────────────────────────────────────────────────
  while (readDmmLine()) {
    pushEvent("dmm", String(dmmRxBuf));
  }

  // ── Pure relay — Counter ─────────────────────────────────────────────────
  while (readCounterLine()) {
    pushEvent("counter", String(ctrRxBuf));
  }

  // Function gen has no loop()-side relay — its request/response cycle is
  // fully synchronous inside handleFuncgenCommand(), called directly from
  // the WS event handler. See file header.
}
