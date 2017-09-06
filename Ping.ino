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

int16_t to_next_ping = 0;
int16_t from_last_success = 0;

bool ping_was_sended = false;
bool ping_ok = true;

byte beep_phase = 0;
byte cur_color = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // max address for ethernet shield
byte ip[] = {192, 168, 100, 11}; // ip address for ethernet shield
IPAddress pingAddr(192, 168, 100, 10); // ip address to ping

SOCKET pingSocket = 0;

char buffer [256];
ICMPPing ping(pingSocket, (uint16_t)random(0, 255));

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
  static ICMPEchoReply echoReply;

  /*
      if (! ping.asyncStart(pingAddr, 3, echoResult))
    {
      Serial.print("Couldn't even send our ping request?? Status: ");
      Serial.println((int)echoResult.status);
      delay(500);
      return;
      }

    while (! ping.asyncComplete(echoResult))
    {
    // we have critical stuff we wish to handle
    // while we wait for ping to come back
    someCriticalStuffThatCantWait();
    }

    // async is done!  let's see how it worked out...
    if (echoResult.status != SUCCESS)
    {
    // failure... but whyyyy?
    sprintf(buffer, "Echo request failed; %d", echoResult.status);
    } else {
    // huzzah
    lastPingSucceeded = true;
    sprintf(buffer,
            "Reply[%d] from: %d.%d.%d.%d: bytes=%d time=%ldms TTL=%d",
            echoResult.data.seq,
            echoResult.addr[0],
            echoResult.addr[1],
            echoResult.addr[2],
            echoResult.addr[3],
            REQ_DATASIZE,
            millis() - echoResult.data.time,
            echoResult.ttl);
    }

  */

  if (ping_was_sended) {
    if (ping.asyncComplete(echoReply)) {
      // ping request completed
      if (echoReply.status == SUCCESS) {
        from_last_success = 0;
        ping_ok = true;
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
        to_next_ping = ALARM_PING_PERIOD;
        ping_ok = false;
        sprintf(buffer, "Echo request failed; %d", echoReply.status);
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
        ping_ok = false;
      }
    }
  }


  if (ping_ok) {
    byte g_col = cur_color & 0x7F;
    byte b_col = 64 - g_col;

    pixels.setPixelColor(0, 0, g_col, b_col); // RGB
    if (cur_color & 0x80) {
      cur_color--;
      if (cur_color == 0x80) cur_color = 0;
    } else {
      cur_color++;
      if (cur_color >= 64) cur_color = 0x80 + 64;
    }

    digitalWrite(BUZZ_PIN, 0);
  } else {
    byte r_col;
    if (beep_phase & 0x08) {
      r_col = 255;
      digitalWrite(BUZZ_PIN, 1);
    } else {
      r_col = 0;
      digitalWrite(BUZZ_PIN, 0);
    }
    pixels.setPixelColor(0, r_col, 0, 0); // RGB
    beep_phase++;
  }

  pixels.show(); // This sends the updated pixel color to the hardware.

  delay(30);
  if (to_next_ping > 0) to_next_ping -= 30;
}










