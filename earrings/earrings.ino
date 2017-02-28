#define PIN 1 // Gemma
#define N_LEDS 12
#define N_COMET_STATES 5
#define N_VELOCITIES 4
#define N_FRAMES 256
#define N_XFADE_FRAMES 64
#define R 0
#define G 1
#define B 2

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_RGBW + NEO_KHZ800);

struct PatternState {
  char nComets;
  char pingPongInterval;
  char velocity;
  char cometOffset;
} rPrevPat, rCurPat, gPrevPat, gCurPat, bPrevPat, bCurPat;
const char cometStates[] = { 1, 2, 3, 4, 6 };
const char velocities[] = { 1, 5, 7, 11 };
char frame = 0;

char rgbCur[3][N_LEDS];
char rgbPrev[3][N_LEDS];
PatternState rgbCurPat[3];
PatternState rgbPrevPat[3];

char rCur[N_LEDS];
char gCur[N_LEDS];
char bCur[N_LEDS];
char rPrev[N_LEDS];
char gPrev[N_LEDS];
char bPrev[N_LEDS];

void setup() {
  randomSeed(analogRead(0));
  rPrevPat = getPatternState();
  gPrevPat = getPatternState();
  bPrevPat = getPatternState();
  rCurPat = getPatternState();
  gCurPat = getPatternState();
  bCurPat = getPatternState();

  for (char i = 0; i < 3; i++) {
    rgbCurPat[i] = getPatternState();
    rgbPrevPat[i] = getPatternState();
  }

  strip.begin();
  strip.show();
}

char calcCometLen(char nComets) {
  return N_LEDS / nComets;
}

char getRandomVelocity(char nComets) {
  return (flipCoin() ? 1 : -1) * velocities[random(N_VELOCITIES)];
}

PatternState getPatternState() {
  char nComets = cometStates[random(N_COMET_STATES)];
  char pingPongInterval = flipCoin() ? N_FRAMES : random(N_LEDS, N_FRAMES / 2);
  char velocity = getRandomVelocity(nComets);
  char cometOffset = random(N_LEDS);
  return { nComets, pingPongInterval, velocity, cometOffset };
}

void setupAnimationState() {
  rPrevPat = rCurPat;
  gPrevPat = gCurPat;
  bPrevPat = bCurPat;
  if (random(3) == 0) {
    rCurPat = getPatternState();
  }
  if (random(3) == 0) {
    gCurPat = getPatternState();
  }
  if (random(3) == 0) {
    bCurPat = getPatternState();
  }
  for (char i = 0; i < N_LEDS; i++) {
    rPrev[i] = rCur[i];
    gPrev[i] = gCur[i];
    bPrev[i] = bCur[i];
  }

  for (char i = 0; i < 3; i++) {
    rgbPrevPat[i] = rgbCurPat[i];
    if (random(3) == 0) {
      rgbCurPat[i] = getPatternState();
    }
    for (char j = 0; j < N_LEDS; j++) {
      rgbPrev[i][j] = rgbCur[i][j];
    }
  }
}

char computeCometColor(char pixel, PatternState pattern) {
  char nComets = pattern.nComets;
  char cometOffset = pattern.cometOffset;
  char velocity = pattern.velocity;

  char cometLen = calcCometLen(nComets);
  char offset = (pixel + cometOffset + (frame * velocity)) % N_LEDS;
  while (offset < 0) {
    offset += N_LEDS;
  }
  char pos = ((pixel + N_LEDS) - offset) % cometLen;
  char color = ((256 / cometLen) * (cometLen - pos)) - 1;
  if (pos == 0) {
    // Head
  } else {
    // Tail
    // shimmer effect
    color += (flipCoin() ? 1 : -1) * random(128 / cometLen);
  }
  return color;
}

void computeRgb() {
  for (char i = 0; i < N_LEDS; i++) {
    rCur[i] = computeCometColor(i, rCurPat);
    gCur[i] = computeCometColor(i, gCurPat);
    bCur[i] = computeCometColor(i, bCurPat);
    rPrev[i] = computeCometColor(i, rPrevPat);
    gPrev[i] = computeCometColor(i, gPrevPat);
    bPrev[i] = computeCometColor(i, bPrevPat);
  }

  for (char i = 0; i < 3; i++) {
    for (char j = 0; j < N_LEDS; j++) {
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
  for (char i = 0; i < 3; i++) {
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

  int red;
  int green;
  int blue;
  for (char i = 0; i < N_LEDS; i++) {
    red = rCur[i];
    green = gCur[i];
    blue = bCur[i];
    if (frame < N_XFADE_FRAMES) {
      red = (red + rPrev[i]) / 2;
      green = (green + gPrev[i]) / 2;
      blue = (blue + bPrev[i]) / 2;
    }
    strip.setPixelColor(i, red, green, blue, 0 /* white */);
  }

  for (char j = 0; j < N_LEDS; j++) {
    red = rgbCur[R][j];
    green = rgbCur[G][j];
    blue = rgbCur[B][j];
    if (frame < N_XFADE_FRAMES) {
      red = (red + rgbPrev[R][j]) / 2;
      green = (green + rgbPrev[G][j]) / 2;
      blue = (blue + rgbPrev[B][j]) / 2;
    }
    //    strip.setPixelColor(i, red, green, blue, 0 /* white */);
  }
}

void loop() {
  render();
  frame++;
  updatePingPong();
  if (frame == 0) {
    setupAnimationState();
  }
}

bool flipCoin() {
  return random(2) == 0;
}

