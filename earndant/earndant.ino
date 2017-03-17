//#define PIN 1 // Gemma--
#define PIN 10 // Feather
#define N_LEDS 24
#define N_COMET_STATES 7
#define N_VELOCITIES 8
#define N_FRAMES 256
#define N_XFADE_FRAMES 64
#define R 0
#define G 1
#define B 2

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

struct PatternState {
  long nComets;
  long pingPongInterval;
  long velocity;
  long cometOffset;
} rPrevPat, rCurPat, gPrevPat, gCurPat, bPrevPat, bCurPat;
const long cometStates[] = { 1, 2, 3, 4, 6, 8, 12 };
const long velocities[] = { 1, 5, 7, 11, 13, 17, 19, 23 };
long frame = 0;

long rgbCur[3][N_LEDS];
long rgbPrev[3][N_LEDS];
PatternState rgbCurPat[3];
PatternState rgbPrevPat[3];

long rCur[N_LEDS];
long gCur[N_LEDS];
long bCur[N_LEDS];
long rPrev[N_LEDS];
long gPrev[N_LEDS];
long bPrev[N_LEDS];

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  rPrevPat = getPatternState();
  gPrevPat = getPatternState();
  bPrevPat = getPatternState();
  rCurPat = getPatternState();
  gCurPat = getPatternState();
  bCurPat = getPatternState();

  for (long i = 0; i < 3; i++) {
    rgbCurPat[i] = getPatternState();
    rgbPrevPat[i] = getPatternState();
  }

  strip.begin();
  for (long i = 0; i < N_LEDS; i++) {
    strip.setPixelColor(i, 64, 64, 64, 64);
  }
  strip.show();
}

long calcCometLen(long nComets) {
  return N_LEDS / nComets;
}

long getRandomVelocity(long nComets) {
  return (flipCoin() ? 1 : -1) * velocities[random(N_VELOCITIES)];
}

PatternState getPatternState() {
  long nComets = 1;//cometStates[random(N_COMET_STATES)];
  long pingPongInterval = N_FRAMES;//flipCoin() ? N_FRAMES : random(N_LEDS, N_FRAMES / 2);
  long velocity = 1;//getRandomVelocity(nComets);
  long cometOffset = 0;//random(N_LEDS);
  return { nComets, pingPongInterval, velocity, cometOffset };
}

void setupAnimationState() {
//  rPrevPat = rCurPat;
//  gPrevPat = gCurPat;
//  bPrevPat = bCurPat;
  if (random(3) == 0) {
    rCurPat = getPatternState();
  }
  if (random(3) == 0) {
    gCurPat = getPatternState();
  }
  if (random(3) == 0) {
    bCurPat = getPatternState();
  }
//  for (long i = 0; i < N_LEDS; i++) {
//    rPrev[i] = rCur[i];
//    gPrev[i] = gCur[i];
//    bPrev[i] = bCur[i];
//  }

//  for (long i = 0; i < 3; i++) {
//    rgbPrevPat[i] = rgbCurPat[i];
//    if (random(3) == 0) {
//      rgbCurPat[i] = getPatternState();
//    }
//    for (long j = 0; j < N_LEDS; j++) {
//      rgbPrev[i][j] = rgbCur[i][j];
//    }
//  }
}

long computeCometColor(long pixel, PatternState pattern) {
  long nComets = pattern.nComets;
  long cometOffset = pattern.cometOffset;
  long velocity = pattern.velocity;

  long cometLen = calcCometLen(nComets);
  long offset = (pixel + cometOffset + (frame * velocity)) % N_LEDS;
  while (offset < 0) {
    offset += N_LEDS;
  }
  long pos = ((pixel + N_LEDS) - offset) % cometLen;
  long color = ((256 / cometLen) * (cometLen - pos)) - 1;
  if (pos == 0) {
    // Head
  } else {
    // Tail
    // shimmer effect
    color += (flipCoin() ? 1 : -1) * random(128 / cometLen);
  }
    Serial.print("pixel: ");
    Serial.print(pixel);
    Serial.print(", color: ");
    Serial.println(color);
  return color;
}

void computeRgb() {
  for (long i = 0; i < N_LEDS; i++) {
    rCur[i] = computeCometColor(i, rCurPat);
    gCur[i] = computeCometColor(i, gCurPat);
    bCur[i] = computeCometColor(i, bCurPat);
    rPrev[i] = computeCometColor(i, rPrevPat);
    gPrev[i] = computeCometColor(i, gPrevPat);
    bPrev[i] = computeCometColor(i, bPrevPat);
  }

  for (long i = 0; i < 3; i++) {
    for (long j = 0; j < N_LEDS; j++) {
      rgbCur[i][j] = computeCometColor(j, rgbCurPat[i]);
      rgbPrev[i][j] = computeCometColor(j, rgbPrevPat[i]);
    }
  }
}

void updatePingPong() {
  if (frame % rCurPat.pingPongInterval == 0) {
    rCurPat = { rCurPat.nComets, rCurPat.pingPongInterval, -1 * rCurPat.velocity, rCurPat.cometOffset };
  }
  if (frame % gCurPat.pingPongInterval == 0) {
    gCurPat = { gCurPat.nComets, gCurPat.pingPongInterval, -1 * gCurPat.velocity, gCurPat.cometOffset };
  }
  if (frame % bCurPat.pingPongInterval == 0) {
    bCurPat = { bCurPat.nComets, bCurPat.pingPongInterval, -1 * bCurPat.velocity, bCurPat.cometOffset };
  }
  if (frame % rPrevPat.pingPongInterval == 0) {
    rPrevPat = { rPrevPat.nComets, rPrevPat.pingPongInterval, -1 * rPrevPat.velocity, rPrevPat.cometOffset };
  }
  if (frame % gPrevPat.pingPongInterval == 0) {
    gPrevPat = { gPrevPat.nComets, gPrevPat.pingPongInterval, -1 * gPrevPat.velocity, gPrevPat.cometOffset };
  }
  if (frame % bPrevPat.pingPongInterval == 0) {
    bPrevPat = { bPrevPat.nComets, bPrevPat.pingPongInterval, -1 * bPrevPat.velocity, bPrevPat.cometOffset };
  }

  PatternState rgbCurPatState;
  PatternState rgbPrevPatState;
  for (long i = 0; i < 3; i++) {
    rgbCurPatState = rgbCurPat[i];
    if (frame % rgbCurPatState.pingPongInterval == 0) {
      rgbCurPat[i] = { rgbCurPatState.nComets, rgbCurPatState.pingPongInterval, -1 * rgbCurPatState.velocity, rgbCurPatState.cometOffset };
    }
    rgbPrevPatState = rgbPrevPat[i];
    if (frame % rgbPrevPatState.pingPongInterval == 0) {
      rgbPrevPat[i] = { rgbPrevPatState.nComets, rgbPrevPatState.pingPongInterval, -1 * rgbPrevPatState.velocity, rgbPrevPatState.cometOffset };
    }
  }
}

void render() {
  computeRgb();

  long red;
  long green;
  long blue;
  for (long i = 0; i < N_LEDS; i++) {
    red = rCur[i];
    green = gCur[i];
    blue = bCur[i];
//    if (frame < N_XFADE_FRAMES) {
//      red = (red + rPrev[i]) / 2;
//      green = (green + gPrev[i]) / 2;
//      blue = (blue + bPrev[i]) / 2;
//    }
    Serial.println("setting pixel color");
    Serial.print("i: ");
    Serial.print(i);
    Serial.print(", red: ");
    Serial.println(red);
//    strip.setPixelColor(i, red, green, blue, 0 /* white */);
    strip.setPixelColor(i, red, 0, 0, 0 /* white */);
  }

  for (long j = 0; j < N_LEDS; j++) {
    red = rgbCur[R][j];
    green = rgbCur[G][j];
    blue = rgbCur[B][j];
    if (frame < N_XFADE_FRAMES) {
      red = (red + rgbPrev[R][j]) / 2;
      green = (green + rgbPrev[G][j]) / 2;
      blue = (blue + rgbPrev[B][j]) / 2;
    }
    green = 0;
    blue = 0;
//    strip.setPixelColor(j, red, green, blue, 0 /* white */);
  }

  strip.show();
}

void loop() {
  render();
  delay(100);
  frame++;
  if (frame >= 256) {
    frame = 0;
  }
  updatePingPong();
  if (frame == 0) {
    setupAnimationState();
  }
}

bool flipCoin() {
  return random(2) == 0;
}

