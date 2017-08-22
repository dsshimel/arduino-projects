#include <SPI.h>
#include <Adafruit_WS2801.h>

void render() __attribute__((__optimize__("O2")));
void setPixelColors() __attribute__((__optimize__("O2")));

// General
#define MIN_FRAME_DURATION 50
#define DISABLE 0

// LEDs
// There are actually 50 LEDs but we're going to copy them
#define N_LEDS 25
#define N_FRAMES 400
#define N_XFADE_FRAMES 200
#define COMET_DOMAIN_SIZE 3
#define N_VELOCITIES 4
#define N_STRIPINGS 3
#define MAX_COLOR 255
#define R 0
#define G 1
#define B 2

// Structures
struct Pattern {
  long nComets;
  long cometOffset;
  long velocityIndex;
  long pingPongDur;
  boolean mirror;
  boolean slave;
  long reduction;
  long striping;
};

// General variables
long frame = 0;

// LED variables
Adafruit_WS2801 strip = Adafruit_WS2801(N_LEDS * 2, 2, 3);
struct Pattern patterns[3];
struct Pattern patterns0[3];
long rgb[3][N_LEDS];
long rgb0[3][N_LEDS];
boolean mimics[3];
boolean mimics0[3];
boolean flipPixel = random(2) == 0;
int numDrawingStrategies = 4;
int drawingStrategy = random(numDrawingStrategies);

// Initialization
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));

  initStrip();
}

void initStrip() {
  strip.begin();

  if (DISABLE == 1) {
    for (long i = 0; i < N_LEDS; i++) {
      strip.setPixelColor(i, 0, 0, 0, 0);
    }
    strip.show();
    return;
  }

  // startup animation
  for (int c = 0; c < 7; c++) {
    for (long j = 0; j < N_LEDS * 2; j++) {
      long i = (j + (4 * c)) % (N_LEDS * 2);
      long color = (MAX_COLOR / (j + 1));
      switch (c) {
        case 0:
          strip.setPixelColor(i, color, 0, 0, 0);
          break;
        case 1:
          strip.setPixelColor(i, color, color, 0, 0);
          break;
        case 2:
          strip.setPixelColor(i, 0, color, 0, 0);
          break;
        case 3:
          strip.setPixelColor(i, 0, color, color, 0);
          break;
        case 4:
          strip.setPixelColor(i, 0, 0, color, 0);
          break;
        case 5:
          strip.setPixelColor(i, color, 0, color, 0);
          break;
        case 6:
          strip.setPixelColor(i, color, color, color, color);
          break;
      }
    }
    strip.show();
    delay(100);
  }

  for (long i = 0; i < (N_LEDS * 2); i++) {
    strip.setPixelColor(i, 0, 0, 0, 0);
  }
  strip.show();
  delay(200);

  for (long c = 0; c < 3; c++) {
    patterns[c].nComets = 1;
    patterns[c].cometOffset = 0;
    patterns[c].velocityIndex = 0;
    patterns[c].pingPongDur = N_FRAMES;
    patterns[c].mirror = false;
    patterns[c].slave = false;
    patterns[c].reduction = 1;
    patterns[c].striping = 0;
  }

  reset();
}

long cometDomains[COMET_DOMAIN_SIZE] = { 1, 2, 3 };
long velocities[N_VELOCITIES] = { 1, 1, N_LEDS - 1, N_LEDS - 1 };
long stripings[N_STRIPINGS] = { 1, 6, 13 };

// LED methods
long getIntensity(long cometLength, long i, long cometOffset, long velocity) {
  long pixel = (i + (frame * velocity) + cometOffset) % cometLength;
  long intensityFraction = cometLength - pixel;
  long intensity = (intensityFraction * MAX_COLOR) / cometLength;
  if (pixel > 0) {
    intensity /= 2;
  } else {
    intensity = MAX_COLOR;
  }
  return intensity < 0 ? 0 : intensity;
}

long getMirrorIndex(long i) {
  return (i + (N_LEDS / 2)) % N_LEDS;
}

long getCometLength(long nComets) {
  return (N_LEDS / nComets);
}

void setPixelColors() {
  long red, green, blue, white;
  for (long i = 0; i < N_LEDS; i++) {
    long iR = (i * stripings[patterns[R].striping]) % N_LEDS;
    long iG = (i * stripings[patterns[G].striping]) % N_LEDS;
    long iB = (i * stripings[patterns[B].striping]) % N_LEDS;
    red = rgb[R][iR];
    green = rgb[G][iG];
    blue = rgb[B][iB];
    if (frame < N_XFADE_FRAMES) {
      long iR0 = (i * stripings[patterns0[R].striping]) % N_LEDS;
      long iG0 = (i * stripings[patterns0[G].striping]) % N_LEDS;
      long iB0 = (i * stripings[patterns0[B].striping]) % N_LEDS;
      red = getXfadeValue(red, rgb0[R][iR0]);
      green = getXfadeValue(green, rgb0[G][iG0]);
      blue = getXfadeValue(blue, rgb0[B][iB0]);
    }

    long p = flipPixel ? N_LEDS - i - 1 : i;
    switch (drawingStrategy) {
      case 0:
        strip.setPixelColor(2 * p, red, green, blue);
        strip.setPixelColor((N_LEDS * 2) - (2 * p) - 1, red, green, blue);
        break;
      case 1:
        strip.setPixelColor(p, red, green, blue);
        strip.setPixelColor((N_LEDS * 2) - 1 - p, red, green, blue);
        break;
      case 2:
        strip.setPixelColor(p, red, green, blue);
        strip.setPixelColor(N_LEDS + p, red, green, blue);
        break;
      case 3:
        strip.setPixelColor(2 * p, red, green, blue);
        strip.setPixelColor((2 * p) + 1, red, green, blue);
        break;
    }
  }
}

long getXfadeValue(long c, long c0) {
  c = (c * frame) / N_XFADE_FRAMES;
  c0 = (c0 * (N_XFADE_FRAMES - 1 - frame)) / N_XFADE_FRAMES;
  return c + c0;
}

boolean flipCoin() {
  return random(2) == 0;
}

void backupPattern(long c) {
  patterns0[c].nComets = patterns[c].nComets;
  patterns0[c].cometOffset = patterns[c].cometOffset;
  patterns0[c].velocityIndex = patterns[c].velocityIndex;
  patterns0[c].pingPongDur = patterns[c].pingPongDur;
  patterns0[c].mirror = patterns[c].mirror;
  patterns0[c].slave = patterns[c].slave;
  patterns0[c].reduction = patterns[c].reduction;
  patterns0[c].striping = patterns[c].striping;
}

void copyPattern(long src, long dest) {
  if (src == dest) {
    return;
  }
  patterns[dest].nComets = patterns[src].nComets;
  patterns[dest].cometOffset = patterns[src].cometOffset;
  patterns[dest].velocityIndex = patterns[src].velocityIndex;
  patterns[dest].pingPongDur = patterns[src].pingPongDur;
  patterns[dest].mirror = patterns[src].mirror;
  // don't copy slave
  // don't copy reduction
  patterns[dest].striping = patterns[src].striping;
}

long getPingPongDur() {
  return flipCoin() ? N_FRAMES : N_LEDS * random(1, N_FRAMES / N_LEDS / 2);
}

long getNewRandomLong(long val, long maxVal) {
  long temp = val;
  while (temp == val) {
    temp = random(maxVal);
  }
  return temp;
}

void render() {
  long i;
  for (long i0 = 0; i0 < N_LEDS; i0++) {
    i = (i0 + frame) % N_LEDS;
    long mirrorIndex = getMirrorIndex(i);

    for (long c = 0; c < 3; c++) {
      long cometLength = getCometLength(patterns[c].nComets);
      long velocity = velocities[patterns[c].velocityIndex];
      rgb[c][i] = getIntensity(cometLength, i, patterns[c].cometOffset, velocity) / patterns[c].reduction;

      long cometLength0 = getCometLength(patterns0[c].nComets);
      long velocity0 = velocities[patterns0[c].velocityIndex];
      rgb0[c][i] = getIntensity(cometLength0, i, patterns0[c].cometOffset, velocity0) / patterns0[c].reduction;

      if (patterns[c].mirror) {
        rgb[c][mirrorIndex] = rgb[c][i];
      }
      if (patterns0[c].mirror) {
        rgb0[c][mirrorIndex] = rgb0[c][i];
      }
    }
  }

  setPixelColors();

  strip.show();
}

long getVelocityIndex(boolean slave, long slaveIndex, long velocityIndex) {
  if (slave) {
    return slaveIndex;
  } else {
    return getNewRandomLong(velocityIndex, N_VELOCITIES);
  }
}

long getRandomNComets() {
  return getRandomNComets(COMET_DOMAIN_SIZE);
}

long getRandomNComets(long cometDomainSize) {
  long nComets = cometDomains[random(cometDomainSize)];
  while (random(nComets) != 0) {
    nComets = cometDomains[random(cometDomainSize)];
  }
  return nComets;
}

void reset() {
  frame = 0;
  drawingStrategy += 1;
  if (drawingStrategy >= numDrawingStrategies) {
    drawingStrategy = 0;
    flipPixel = flipCoin();
  } /*else {
    flipPixel = !flipPixel;
  }*/

  for (long c = 0; c < 3; c++) {
    mimics0[c] = mimics[c];
    mimics[c] = flipCoin();
  }
  long leader = -1;
  long follower = -1;
  for (long c = 0; c < 3; c++) {
    if (mimics[c]) {
      if (leader < 0) {
        leader = c;
      } else if (follower < 0) {
        follower = c;
      }
      else {
        leader = -1;
        follower = -1;
      }
    }
  }

  for (long c = 0; c < 3; c++) {
    backupPattern(c);

    patterns[c].nComets = getRandomNComets();
    patterns[c].cometOffset = random(N_LEDS);
    patterns[c].velocityIndex = random(N_VELOCITIES);
    patterns[c].pingPongDur = getPingPongDur();
    patterns[c].mirror = flipCoin();
    patterns[c].striping = flipCoin() ? 0 : random(N_STRIPINGS);

    if (c == follower) {
      copyPattern(follower, c);
    }

    patterns[c].reduction = 1 + random(3);
    if (c == G) {
      while (patterns[R].reduction == patterns[c].reduction) {
        patterns[c].reduction = 1 + random(3);
      }
    } else if (c == B) {
      while (patterns[R].reduction == patterns[c].reduction || patterns[G].reduction == patterns[c].reduction) {
        patterns[c].reduction = 1 + random(3);
      }
    }

    if (patterns[c].reduction == 2) {
      patterns[c].nComets = getRandomNComets(COMET_DOMAIN_SIZE - 3);
    }
    if (patterns[c].reduction == 3) {
      patterns[c].nComets = getRandomNComets(COMET_DOMAIN_SIZE - 2);
    }
  }
}

long minDelay = 60;
void loop() {
  if (DISABLE == 1) {
    return;
  }

  unsigned long frameStartTime = millis();

  render();

  frame++;

  for (long c = 0; c < 3; c++) {
    if (frame % patterns[c].pingPongDur == 0) {
      patterns[c].velocityIndex = getVelocityIndex(patterns[c].slave, patterns[R].velocityIndex, patterns[c].velocityIndex);
    }
  }

  if (frame >= N_FRAMES) {
    reset();
  }

  unsigned long frameEndTime = millis();
  long frameDuration = frameEndTime - frameStartTime;
  if (frameDuration > minDelay) {
    minDelay = frameDuration;
  }

  long delayTime = minDelay - frameDuration;

  delay(delayTime > 0 ? delayTime : 0);
}

