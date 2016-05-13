/*     ---------------------------------------------------------
 *     |  Blinking Bracelet Project for Coachella              |
 *     ---------------------------------------------------------
 *
 *  Uses the adafruit libraries for wearable LED arrays as well as accelerometer code
 */

#include <Adafruit_NeoPixel.h>

#define PIN 17
#define TPIXEL 10 //The total amount of pixel's/led's in your connected strip/stick (Default is 60)
#define BUTTONPIN 4 // the number of the pushbutton pin
#define MIC_PIN   A4  // Microphone is attached to this analog pin
#define DC_OFFSET  -172  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   60  // Length of buffer for dynamic level adjustment
#define TOP       (TPIXEL + 2) // Allow dot to go slightly off scale
enum FadeDirection {FORWARD, REVERSE};

//color mode settings. These define the number of total number of color modes and the order that they are selected.
#define COLORFADE_MODE 0 //fade between two colors
//#define COLORWIPE_MODE 1 //color wipe mode 
//#define RAINBOW_MODE 2 // rainbow mode
#define RAINBOWCYCLE_MODE 1 //rainbow cycle mode
#define THEATERCHASE_MODE 2 //
#define VUDOUBLE_MODE 3 // double vu mode constant
#define NUM_MODES 4 // sets the maximum number of color modes

const int longPressDuration = 750; // duration of a long press in milliseconds
const int brightnessLevels = 3; //defines how many brightness levels will be used
const int minBrightness = 80; //defines the lowest brightness level from 0-255
const int maxBrightness = 180; //defines the maximum brightness level from 0-255

// variables will change:
int
buttonState = 0,             // variable for reading the pushbutton status
lastButtonState = 0,     // variable for tracking the last button status
currentBrightness = 80;        // keeps track of the current color that is displayed for fading purposes

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long
buttonStart = 0,         // marks when the button was first pushed
lastDebounceTime = 0,  // the last time the output pin was toggled
debounceDelay = 50;    // the debounce time; increase if the output flickers
uint32_t currentColor = Adafruit_NeoPixel::Color(190,210,210),
fadeColor1 = Adafruit_NeoPixel::Color(190,210,210),
fadeColor2 = Adafruit_NeoPixel::Color(0,0,255);  // set the first color to black;

FadeDirection fadeDir = FORWARD;

volatile int lightMode = 0; // how many times the button has been pressed

uint16_t
WaitTime = 80;

unsigned long previousMillis;

//vuMeter variables
byte
peak      = 0,      // Used for falling dot
dotCount  = 0,      // Frame counter for delaying dot-falling speed
volCount  = 0,      // Frame counter for storing past volume data
count1 = 0,
count2 = 0;

int
vol[SAMPLES],       // Collection of prior volume samples
    lvl       = 10,      // Current "dampened" audio level
    minLvlAvg = 0,      // For dynamic adjustment of graph low & high
    maxLvlAvg = 512;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(TPIXEL, PIN, NEO_GRB + NEO_KHZ800);
// our RGB -> eye-recognized gamma color
byte gammatable[256];

void setup() {
  pinMode(BUTTONPIN, INPUT); //Set the switch pin as input
  pinMode(PIN, OUTPUT);
  lastButtonState = digitalRead(BUTTONPIN);
  buttonState = lastButtonState;
  randomSeed(analogRead(0));
  strip.setBrightness(currentBrightness); //adjust brightness here
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  debounceButton();
  if (lightMode == VUDOUBLE_MODE) {
    vuDoubleMeterUpdate();
  }
  else {
    if (millis() - previousMillis >= WaitTime) {
      switch (lightMode) {
        case COLORFADE_MODE:
         colorFadeUpdate();
         break;
 //       case COLORWIPE_MODE:
   //       colorWipeUpdate();
     //     break;
 //       case RAINBOW_MODE:
  //        rainbowUpdate();
   //       break;
        case RAINBOWCYCLE_MODE:
          rainbowCycleUpdate();
          break;
        case THEATERCHASE_MODE:
          theaterChaseUpdateTrail();
      }
      previousMillis = millis();
    }
  }
}

void debounceButton() {
  int reading = digitalRead(BUTTONPIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        buttonStart = millis();
      }
      else {
        int pressDuration = millis() - buttonStart; //calculate how long the button was pressed for
        if (pressDuration < longPressDuration) {
          swap(); //change modes if the press was short
        }
        else {
          currentBrightness = currentBrightness + ((maxBrightness - minBrightness) / brightnessLevels); //step up our brightness, using 20 as our lowest light level
          if (currentBrightness > maxBrightness) {
            currentBrightness = minBrightness;
          }
          strip.setBrightness(currentBrightness); //fade gradually to a random color if it was a long press
        }
      }
    }
  }
  lastButtonState = reading;
}

void swap() {
  if (lightMode++ == NUM_MODES - 1) {
    lightMode = 0;
  }
  currentColor = getRandomColor();
  count1 = 0;
  count2 = 0;
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

uint32_t getRandomColor() {
  return Wheel(lowByte(random(0, 255)));
}

void colorWipeUpdate() {
  strip.setPixelColor(count1++, currentColor);
  strip.show();
  if (count1 == strip.numPixels()) {
    count1 = 0;
    currentColor = getRandomColor();
  }
}

void colorFadeUpdate() {
  byte TotalSteps = 50;
  int redColor, greenColor, blueColor;
        uint8_t red = ((Red(fadeColor1) * (TotalSteps - count1)) + (Red(fadeColor2) * count1)) / TotalSteps;
        uint8_t green = ((Green(fadeColor1) * (TotalSteps - count1)) + (Green(fadeColor2) * count1)) / TotalSteps;
        uint8_t blue = ((Blue(fadeColor1) * (TotalSteps - count1)) + (Blue(fadeColor2) * count1)) / TotalSteps;        
  for (byte i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, red, green, blue);
  }
  strip.show();
  if (fadeDir == FORWARD) {
    if (++count1 >= TotalSteps) {
      fadeDir = REVERSE;
    }
  }
  else {
    if (--count1 <= 0) {
      fadeDir = FORWARD;
    }
  }
}

void rainbowUpdate() {
  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i + count1) & 255));
    strip.show();
    count1++;
    if (count1 == 256) {
      count1 = 0;
    }
  }
}
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycleUpdate() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + count1++) & 255));
  }
  strip.show();
  if (count1 == 256 * 5) {
    count1 = 0;
  }
}

//Theatre-style crawling lights.
void theaterChaseUpdate() {
  for (int i = 0; i < strip.numPixels(); i = i + 4) {
    strip.setPixelColor(i + count1, currentColor);  //turn every third pixel on
  }
  strip.show();

  for (int i = 0; i < strip.numPixels(); i = i + 4) {
    strip.setPixelColor(i + count1, 0);      //turn every third pixel off
  }
  if (++count1 == 4) {
    count1 = 0;
  }
}

//Theatre-style crawling lights.
void theaterChaseUpdateTrail() {
  for (int i = 0; i < strip.numPixels(); i = i + 4) {
    strip.setPixelColor(i + count1, currentColor);  //turn every third pixel on
    strip.setPixelColor(i + count1 - 1, getHalfBrightness(currentColor));
  }
  strip.show();

  for (int i = 0; i < strip.numPixels(); i = i + 4) {
    strip.setPixelColor(i + count1, 0);      //turn every third pixel off
    strip.setPixelColor(i + count1 - 1, 0);
  }
  if (++count1 == 4) {
    count1 = 0;
  }
}
//VU Style Sound Meter with two bars
void vuDoubleMeterUpdate() {
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
  
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP / 2 * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP / 2) height = TOP / 2;


  // Color pixels based on rainbow gradient
  for (i = 0; i < TPIXEL / 2; i++) {
    if (i >= height)   {
      strip.setPixelColor(i,   0,   0, 0);
      strip.setPixelColor(TPIXEL - i - 1, 0, 0, 0);
    }
    else {
      strip.setPixelColor(i, Wheel(map(i, 0, strip.numPixels() / 2 - 1, 100, 230)));
      strip.setPixelColor(TPIXEL - i - 1, Wheel(map(i, 0, strip.numPixels() / 2 - 1, 100, 230)));
    }
  }

  strip.show(); // Update strip


  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

}

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
uint32_t getHalfBrightness (uint32_t color) {
  return (color & 0xfcfcfc) >> 1;
}

