#include <FastLED.h>
#include <WiFi.h>
#include <NTPClient.h>
#include "time.h"
#include <WiFiUdp.h>

#define LED_PIN 13
#define COLOR_ORDER GRB
#define CHIPSET WS2812
#define BRIGHTNESS 5

#define MY_NTP_SERVER "at.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define STASSID "1LustigerName"
#define STAPSK "1LustigesPasswort"

const char *ssid = "1LustigerName";
const char *password = "1LustigesPasswort";

time_t now; // this is the epoch
tm tm;

// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP);

// Params for width and height
const uint8_t kMatrixWidth = 8;
const uint8_t kMatrixHeight = 22;
const uint8_t PanelAnzahl = 3;
const CRGB StundenFarbe = CRGB::DeepPink; 
const CRGB MinutenFarbe = CRGB::Yellow;
const CRGB MarkerFarbe = CRGB::MidnightBlue;

// Param for different pixel layouts
// const bool kMatrixSerpentineLayout = true;
// const bool kMatrixVertical = false;

uint8_t internalX = 0;
uint8_t internalY = 0;
uint8_t PanelIndex = 0;

#define NUM_LEDS (kMatrixWidth * kMatrixHeight * PanelAnzahl)
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);

int xyToIdx(int x, int y)
{
  int panelIdx = y / 8;

  int internalX = x;
  int internalY = (x % 2 == 0) ? (y % 8) : (7 - (y % 8));

  return panelIdx * 8 * 22 + internalX * 8 + internalY;
}

void zeichneLinie(int x0, int y0, int x1, int y1, CRGB color) {
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    if (x0>=0 && x0<22 && y0>=0 && y0<24)
      leds[xyToIdx(x0, y0)] = color;
    if (x0==x1 && y0==y1) break;
    int e2 = 2*err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
void setzeStunde(int stunde, int minute) {
  float rad = ((stunde % 12) * 30.0f + minute * 0.5f) * (float)M_PI / 180.0f;
  const int r = 8;
  int ex = (int)roundf(10.0f + sinf(rad) * r);
  int ey = (int)roundf(10.0f - cosf(rad) * r);
  // perpendicular offset: for near-vertical lines offset right (+X), for near-horizontal offset down (+Y)
  int px = (fabsf(cosf(rad)) >= fabsf(sinf(rad))) ? 1 : 0;
  int py = (fabsf(cosf(rad)) >= fabsf(sinf(rad))) ? 0 : 1;
  zeichneLinie(10, 10, ex, ey, StundenFarbe);
  zeichneLinie(10+px, 10+py, ex+px, ey+py, StundenFarbe);
  leds[xyToIdx(10, 10)] = StundenFarbe;
  leds[xyToIdx(11, 10)] = StundenFarbe;
  leds[xyToIdx(10, 11)] = StundenFarbe;
  leds[xyToIdx(11, 11)] = StundenFarbe;
}

void setzeMinute(int minute) {
  float rad = minute * 6.0f * (float)M_PI / 180.0f;
  const int r = 9;
  int ex = (int)roundf(10.0f + sinf(rad) * r);
  int ey = (int)roundf(10.0f - cosf(rad) * r);
  zeichneLinie(10, 10, ex, ey, MinutenFarbe);
}
void setzeMarker()
{
  leds[xyToIdx(10, 1)] = MarkerFarbe; // Oben
  leds[xyToIdx(11, 1)] = MarkerFarbe;
  leds[xyToIdx(20, 10)] = MarkerFarbe; //rechts
  leds[xyToIdx(20, 11)] = MarkerFarbe;
  leds[xyToIdx(10, 20)] = MarkerFarbe; //unten
  leds[xyToIdx(11, 20)] = MarkerFarbe;
  leds[xyToIdx(1, 10)] = MarkerFarbe; //links
  leds[xyToIdx(1, 11)] = MarkerFarbe;
}

void SekundeBlink()
{
  leds[xyToIdx(10, 23)] = MarkerFarbe;
  leds[xyToIdx(11, 23)] = MarkerFarbe;
}

void showTime()
{
  Serial.print("year:");
  Serial.print(tm.tm_year + 1900); // years since 1900
  Serial.print("\tmonth:");
  Serial.print(tm.tm_mon + 1); // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(tm.tm_mday); // day of month
  Serial.print("\thour:");
  Serial.print(tm.tm_hour); // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min); // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec); // seconds after the minute  0-61*
  Serial.print("\twday");
  Serial.print(tm.tm_wday); // days since Sunday 0-6
  if (tm.tm_isdst == 1)     // Daylight Saving Time flag
    Serial.print("\tDST");
  else
    Serial.print("\tstandard");
  Serial.println();
}

void setup()
{
  Serial.begin(9600);
  configTime(0, 0, MY_NTP_SERVER); // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TZ, 1);          // Set environment variable with your time zone
  tzset();

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop()
{

  time(&now);             // read the current time
  localtime_r(&now, &tm); // update the structure tm with the current time
//showTime();     // prints time to console

  if (tm.tm_hour < 9 || tm.tm_hour > 21) //Nightmode
  {
    FastLED.setBrightness(1);
  }
  else
    FastLED.setBrightness(BRIGHTNESS);

  setzeMarker();
  setzeStunde(tm.tm_hour, tm.tm_min);
  setzeMinute(tm.tm_min); // Minute on top, deswegen danach

  if (tm.tm_sec % 2 == 0)
  {
    SekundeBlink();
  }

  FastLED.show();
  FastLED.clear();
  delay(500);
}
