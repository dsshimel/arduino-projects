#define PIN 10 // Feather
#define N_LEDS 24
#define N_FRAMES 144
#define N_XFADE_FRAMES 36
#define COMET_DOMAIN_SIZE 6
#define N_VELOCITIES 4
#define MAX_COLOR 255
#define R 0
#define G 1
#define B 2

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

struct Pattern {
  long nComets;
  long cometOffset;
  long velocityIndex;
  long pingPongDur;
  boolean mirror;
  boolean slave;
  long reduction;
};

struct Pattern patterns[3];
struct Pattern patterns0[3];
long rgb[3][N_LEDS];
long rgb0[3][N_LEDS];

void initStrip() {
  strip.begin();
  for (long i = 0; i < N_LEDS; i++) {
    strip.setPixelColor(i, 32, 32, 32, 32);
  }
  strip.show();
  delay(500);
  for (long i = 0; i < N_LEDS; i++) {
    strip.setPixelColor(i, 0, 0, 0, 0);
  }
  strip.show();

  for (long c = 0; c < 3; c++) {
    patterns[c].nComets = 1;
    patterns[c].cometOffset = 0;
    patterns[c].velocityIndex = 0;
    patterns[c].pingPongDur = N_FRAMES;
    patterns[c].mirror = false;
    patterns[c].slave = false;
    patterns[c].reduction = 1;
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  initStrip();
  reset();
}

long frame = 0;
long cometDomains[COMET_DOMAIN_SIZE] = { 1, 2, 3, 4, 6, 8};
long velocities[N_VELOCITIES] = { 1, 2, N_LEDS - 2, N_LEDS - 1 };

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

void render() {
  long i;
  for (long i0 = 0; i0 < N_LEDS; i0++) {
    i = (i0 + frame) % N_LEDS;
    long mirrorIndex = getMirrorIndex(i);

    for (long c = 0; c < 3; c++) {
      long cometLength = getCometLength(patterns[c].nComets);
      long velocity = velocities[patterns[c].velocityIndex];
      rgb[c][i] = getIntensity(cometLength, i, patterns[c].cometOffset, velocity) / ((3 * patterns[c].reduction) - 1);

      long cometLength0 = getCometLength(patterns0[c].nComets);
      long velocity0 = velocities[patterns0[c].velocityIndex];
      rgb0[c][i] = getIntensity(cometLength0, i, patterns0[c].cometOffset, velocity0) / ((3 * patterns0[c].reduction) - 1);

      if (patterns[c].mirror) {
        rgb[c][mirrorIndex] = rgb[c][i];
      }
      if (patterns0[c].mirror) {
        rgb0[c][mirrorIndex] = rgb0[c][i];
      }
    }
  }

  setPixelColors(
  );

  strip.show();
  delay(50);
}

void setPixelColors() {
  long red, green, blue;
  for (long i = 0; i < N_LEDS; i++) {
    red = rgb[R][i];
    green = rgb[G][i];
    blue = rgb[B][i];
    if (frame < N_XFADE_FRAMES) {
      red = getXfadeValue(red, rgb0[R][i]);
      green = getXfadeValue(green, rgb0[G][i]);
      blue = getXfadeValue(blue, rgb0[B][i]);
    }
    strip.setPixelColor(i, red, green, blue, 0);
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

void reset() {
  frame = 0;

  boolean slaveG = flipCoin();
  boolean slaveB = flipCoin();
  if (slaveG && slaveB) {
    slaveG = flipCoin();
    slaveB = !slaveG;
  }

  boolean changeAll = flipCoin();
  for (long c = 0; c < 3; c++) {
    patterns0[c].nComets = patterns[c].nComets;
    patterns0[c].cometOffset = patterns[c].cometOffset;
    patterns0[c].velocityIndex = patterns[c].velocityIndex;
    patterns0[c].pingPongDur = patterns[c].pingPongDur;
    patterns0[c].mirror = patterns[c].mirror;
    patterns0[c].slave = patterns[c].slave;
    patterns0[c].reduction = patterns[c].reduction;

    if (changeAll || flipCoin()) {
      long nComets = cometDomains[random(COMET_DOMAIN_SIZE)];
      while (random(nComets) != 0) {
        nComets = cometDomains[random(COMET_DOMAIN_SIZE)];
      }
      patterns[c].nComets = nComets;
      patterns[c].cometOffset = random(N_LEDS);
      patterns[c].velocityIndex = random(N_VELOCITIES);
      patterns[c].pingPongDur = getPingPongDur();
      patterns[c].mirror = flipCoin();
      if (c == R) {
        patterns[c].slave = false;
      } else if (c == G) {
        patterns[c].slave = slaveG;
        if (slaveG) {
          copyRedPattern(G);
        }
      } else if (c == B) {
        patterns[c].slave = slaveB;
        if (slaveB) {
          copyRedPattern(B);
        }
      }
    }
    patterns[c].reduction = 1 + random(3);
    if (c == G) {
      patterns[c].reduction = 1 + getNewRandomLong(patterns[R].reduction - 1, 3);
    } else if (c == B) {
      patterns[c].reduction = 1 + random(3);
      while (patterns[R].reduction == patterns[c].reduction || patterns[G].reduction == patterns[c].reduction) {
        patterns[c].reduction = 1 + random(3);
      }
    }
  }
}

void copyRedPattern(long c) {
  patterns[c].nComets = patterns[R].nComets;
  patterns[c].cometOffset = patterns[R].cometOffset;
  patterns[c].velocityIndex = patterns[R].velocityIndex;
  patterns[c].pingPongDur = patterns[R].pingPongDur;
  patterns[c].mirror = patterns[R].mirror;
}

long getPingPongDur() {
  return flipCoin() ? N_FRAMES : N_LEDS * random(1, N_FRAMES / N_LEDS / 2);
}

void loop() {
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
}

long getVelocityIndex(boolean slave, long slaveIndex, long velocityIndex) {
  if (slave) {
    return slaveIndex;
  } else {
    return getNewRandomLong(velocityIndex, N_VELOCITIES);
  }
}

long getNewRandomLong(long val, long maxVal) {
  long temp = val;
  while (temp == val) {
    temp = random(maxVal);
  }
  return temp;
}

