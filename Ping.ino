/*
  Ping Example

  This example sends an ICMP pings every 500 milliseconds, sends the human-readable
  result over the serial port.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created 30 Sep 2010
  by Blake Foster

*/

#include <SPI.h>
#include <Ethernet.h>
#include <ICMPPing.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#ifndef ICMPPING_ASYNCH_ENABLE
#error "Asynchronous functions only available if ICMPPING_ASYNCH_ENABLE is defined -- see ICMPPing.h"
#endif

#define NEO_PIN 6
#define NEO_NUMPIXELS 1
#define BUZZ_PIN 7

// ping period in ms
#define PING_PERIOD 2000
#define ALARM_PING_PERIOD 100

// PING_REQUEST_TIMEOUT_MS -- timeout in ms.  between 1 and 65000 or so
// save values: 1000 to 5000, say.
#define PING_REQUEST_TIMEOUT_MS     500

#define YELLOW_ALARM_TIMEOUT        30000
#define RED_ALARM_TIMEOUT           60000
#define BUZZER_ALARM_TIMEOUT        120000

int16_t to_next_ping = 0;
int32_t from_last_success = 0;
int32_t from_first_fail = 0;

bool ping_was_sended = false;
byte beep_phase = 0;
byte cur_color = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // max address for ethernet shield
byte ip[] = {192, 168, 100, 11}; // ip address for ethernet shield
IPAddress pingAddr(192, 168, 100, 10); // ip address to ping

SOCKET pingSocket = 0;

char buffer [256];
ICMPPing ping(pingSocket, (uint16_t)random(0, 255));

ICMPEchoReply echoReply;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NEO_NUMPIXELS, NEO_PIN, NEO_RGB + NEO_KHZ800);

void setup()
{
  // start Ethernet
  Ethernet.begin(mac, ip);
  Serial.begin(115200);

  pinMode(BUZZ_PIN, OUTPUT);

  // increase the default time-out, if needed, assuming a bad
  // connection or whatever.
  ICMPPing::setTimeout(PING_REQUEST_TIMEOUT_MS);

  pixels.begin(); // This initializes the NeoPixel library.
}

void loop()
{
  if (ping_was_sended) {
    if (ping.asyncComplete(echoReply)) {
      // ping request completed
      if (echoReply.status == SUCCESS) {
        from_last_success = 0;
        from_first_fail = 0;
        sprintf(buffer,
                "Reply[%d] from: %d.%d.%d.%d: bytes=%d time=%ldms TTL=%d",
                echoReply.data.seq,
                echoReply.addr[0],
                echoReply.addr[1],
                echoReply.addr[2],
                echoReply.addr[3],
                REQ_DATASIZE,
                millis() - echoReply.data.time,
                echoReply.ttl);
      }
      else
      {
        if (from_first_fail == 0) from_first_fail = 1;
        to_next_ping = ALARM_PING_PERIOD;
        sprintf(buffer, "Echo request failed; %d %d", echoReply.status, from_first_fail);
      }
      ping_was_sended = false;
      Serial.println(buffer);
    }
  } else { // ping was not sended
    if (to_next_ping <= 0) {
      if (ping.asyncStart(pingAddr, 1, echoReply)) { // ping sended successfully
        ping_was_sended = true;
        to_next_ping = PING_PERIOD;
      }
      else
      {
        Serial.print("Couldn't even send ping request with status: ");
        Serial.println((int)echoReply.status);
        to_next_ping = ALARM_PING_PERIOD;
        if (from_first_fail == 0) from_first_fail = 1;
      }
    }
  }

  byte r_col = 0, g_col = 0, b_col = 0;

  if (from_first_fail > RED_ALARM_TIMEOUT) {
    if (beep_phase & 0x08) r_col = 255;
  } else {
    if (from_first_fail > YELLOW_ALARM_TIMEOUT) {
      if (beep_phase & 0x08) g_col = r_col = 128;
    } else {
      g_col = cur_color & 0x7F;
      b_col = 64 - g_col;
      if (cur_color & 0x80) {
        cur_color--;
        if (cur_color == 0x80) cur_color = 0;
      } else {
        cur_color++;
        if (cur_color >= 64) cur_color = 0x80 + 64;
      }
    }
  }
  pixels.setPixelColor(0, r_col, g_col, b_col); // RGB
  pixels.show(); // This sends the updated pixel color to the hardware.

  if (from_first_fail > BUZZER_ALARM_TIMEOUT) {
    if (beep_phase & 0x08) {
      digitalWrite(BUZZ_PIN, 1);
    } else {
      digitalWrite(BUZZ_PIN, 0);
    }
  } else {
    digitalWrite(BUZZ_PIN, 0);
  }

  beep_phase++;

  delay(30);

  if (to_next_ping > 0) to_next_ping -= 30;
  if (from_last_success < 0x40000000) from_last_success += 30;
  if ((from_first_fail != 0) && (from_first_fail < 0x40000000)) from_first_fail += 30;
}










