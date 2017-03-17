#define PIN 10 // Feather
#define N_LEDS 24
#define N_FRAMES 288
#define N_XFADE_FRAMES 36
#define COMET_DOMAIN_SIZE 4
#define N_VELOCITIES 4
#define MAX_COLOR 63
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
long cometDomains[COMET_DOMAIN_SIZE] = { 1, 2, 3, 4};
long velocities[N_VELOCITIES] = { 1, 2, N_LEDS - 2, N_LEDS - 1 };
long nCometsR = 1;
long nCometsG = 1;
long nCometsB = 1;
long cometOffsetR = 0;
long cometOffsetG = 0;
long cometOffsetB = 0;
long velocityIndexR = 0;
long velocityIndexG = 0;
long velocityIndexB = 0;
long pingPongDurR = N_FRAMES;
long pingPongDurG = N_FRAMES;
long pingPongDurB = N_FRAMES;
long valsR[N_LEDS];
long valsG[N_LEDS];
long valsB[N_LEDS];
boolean mirrorR = false;
boolean mirrorG = false;
boolean mirrorB = false;
boolean slaveG = false;
boolean slaveB = false;
long divR = 1;
long divG = 1;
long divB = 1;

long nCometsR0 = 1;
long nCometsG0 = 1;
long nCometsB0 = 1;
long cometOffsetR0 = 0;
long cometOffsetG0 = 0;
long cometOffsetB0 = 0;
long velocityIndexR0 = 0;
long velocityIndexG0 = 0;
long velocityIndexB0 = 0;
long pingPongDurR0 = N_FRAMES;
long pingPongDurG0 = N_FRAMES;
long pingPongDurB0 = N_FRAMES;
long valsR0[N_LEDS];
long valsG0[N_LEDS];
long valsB0[N_LEDS];
boolean mirrorR0 = false;
boolean mirrorG0 = false;
boolean mirrorB0 = false;
boolean slaveG0 = false;
boolean slaveB0 = false;
long divR0 = 1;
long divG0 = 1;
long divB0 = 1;

long getIntensity(long cometLength, long i, long frame, long cometOffset, long velocity) {
  long pixel = (i + (frame * velocity) + cometOffset) % cometLength;
  long intensityFraction = cometLength - pixel;
  long intensity = (intensityFraction * MAX_COLOR) / cometLength;
  if (pixel > 0) {
    //    intensity = (intensity / 8) * 8;
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
  long red, green, blue, white;
  long velocityR = velocities[velocityIndexR];
  long velocityG = velocities[velocityIndexG];
  long velocityB = velocities[velocityIndexB];
  long cometLengthR = getCometLength(nCometsR);
  long cometLengthG = getCometLength(nCometsG);
  long cometLengthB = getCometLength(nCometsB);

  long red0, green0, blue0, white0;
  long velocityR0 = velocities[velocityIndexR0];
  long velocityG0 = velocities[velocityIndexG0];
  long velocityB0 = velocities[velocityIndexB0];
  long cometLengthR0 = getCometLength(nCometsR0);
  long cometLengthG0 = getCometLength(nCometsG0);
  long cometLengthB0 = getCometLength(nCometsB0);

  long i;
  for (long i0 = 0; i0 < N_LEDS; i0++) {
    i = (i0 + frame) % N_LEDS;
    red = getIntensity(cometLengthR, i, frame, cometOffsetR, velocityR);
    green = getIntensity(cometLengthG, i, frame, cometOffsetG, velocityG);
    blue = getIntensity(cometLengthB, i, frame, cometOffsetB, velocityB);
    red0 = getIntensity(cometLengthR0, i, frame, cometOffsetR0, velocityR0);
    green0 = getIntensity(cometLengthG0, i, frame, cometOffsetG0, velocityG0);
    blue0 = getIntensity(cometLengthB0, i, frame, cometOffsetB0, velocityB0);

    long mirrorIndex = getMirrorIndex(i);
    valsR[i] = red / (2 * divR - 1);
    if (mirrorR) {
      valsR[mirrorIndex] = red;
    }
    valsG[i] = green / (2 * divG - 1);
    if (mirrorG) {
      valsG[mirrorIndex] = green;
    }
    valsB[i] = blue / (2 * divB - 1);
    if (mirrorB) {
      valsB[mirrorIndex] = blue;
    }
    valsR0[i] = red0 / (2 * divR0 - 1);
    if (mirrorR0) {
      valsR0[mirrorIndex] = red0;
    }
    valsG0[i] = green0 / (2 * divG0 - 1);
    if (mirrorG0) {
      valsG0[mirrorIndex] = green0;
    }
    valsB0[i] = blue0 / (2 * divB0 - 1);
    if (mirrorB0) {
      valsB0[mirrorIndex] = blue0;
    }

    for (long c = 0; c < 3; c++) {
      long cometLength = getCometLength(patterns[c].nComets);
      long velocity = velocities[patterns[c].velocityIndex];
      rgb[c][i] = getIntensity(cometLength, i, frame, patterns[c].cometOffset, velocity) / ((2 * patterns[c].reduction) - 1);

      long cometLength0 = getCometLength(patterns0[c].nComets);
      long velocity0 = velocities[patterns0[c].velocityIndex];
      rgb0[c][i] = getIntensity(cometLength0, i, frame, patterns0[c].cometOffset, velocity0) / ((2 * patterns0[c].reduction) - 1);

      if (patterns[c].mirror) {
        rgb[c][mirrorIndex] = rgb[c][i];
      }
      if (patterns0[c].mirror) {
        rgb0[c][mirrorIndex] = rgb0[c][i];
      }
    }
    if (valsR[i] != rgb[R][i]) {
      Serial.println("bad");
    }
    if (valsG[i] != rgb[G][i]) {
      Serial.println("bad");
    }
    if (valsB[i] != rgb[B][i]) {
      Serial.println("bad");
    }
  }

  for (long i0 = 0; i0 < N_LEDS; i0++) {
    long i = i0;

    red = valsR[i];
    green = valsG[i];
    blue = valsB[i];
    red0 = valsR0[i];
    green0 = valsG0[i];
    blue0 = valsB0[i];
    if (frame < N_XFADE_FRAMES) {
      red = (red * frame) / N_XFADE_FRAMES;
      green = (green * frame) / N_XFADE_FRAMES;
      blue = (blue * frame) / N_XFADE_FRAMES;
      red0 = (red0 * (N_XFADE_FRAMES - 1 - frame)) / N_XFADE_FRAMES;
      green0 = (green0 * (N_XFADE_FRAMES - 1 - frame)) / N_XFADE_FRAMES;
      blue0 = (blue0 * (N_XFADE_FRAMES - 1 - frame)) / N_XFADE_FRAMES;
      red += red0;
      green += green0;
      blue += blue0;
    }

    if (frame < N_XFADE_FRAMES) {
      strip.setPixelColor(i, getXfadeValue(rgb[R][i], rgb0[R][i]), getXfadeValue(rgb[G][i], rgb0[G][i]), getXfadeValue(rgb[B][i], rgb0[B][i]), 0);
    } else {
      strip.setPixelColor(i, rgb[R][i], rgb[G][i], rgb[B][i], 0);
    }

    //    strip.setPixelColor(i, red, green, blue, 0);
    //        strip.setPixelColor(i, red, 0, 0, 0);
  }

  strip.show();
  delay(50);
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

  nCometsR0 = nCometsR;
  cometOffsetR0 = cometOffsetR;
  velocityIndexR0 = velocityIndexR;
  pingPongDurR0 = pingPongDurR;
  mirrorR0 = mirrorR;
  divR0 = divR;

  nCometsG0 = nCometsG;
  cometOffsetG0 = cometOffsetG;
  velocityIndexG0 = velocityIndexG;
  pingPongDurG0 = pingPongDurG;
  mirrorG0 = mirrorG;
  slaveG0 = slaveG;
  divG0 = divG;

  nCometsB0 = nCometsB;
  cometOffsetB0 = cometOffsetB;
  velocityIndexB0 = velocityIndexB;
  pingPongDurB0 = pingPongDurB;
  mirrorB0 = mirrorB;
  slaveB0 = slaveB;
  divB0 = divB;

  slaveG = flipCoin();
  slaveB = flipCoin();
  if (slaveG && slaveB) {
    slaveG = flipCoin();
    slaveB = !slaveG;
  }

  for (long c = 0; c < 3; c++) {
    patterns0[c].nComets = patterns[c].nComets;
    patterns0[c].cometOffset = patterns[c].cometOffset;
    patterns0[c].velocityIndex = patterns[c].velocityIndex;
    patterns0[c].pingPongDur = patterns[c].pingPongDur;
    patterns0[c].mirror = patterns[c].mirror;
    patterns0[c].slave = patterns[c].slave;
    patterns0[c].reduction = patterns[c].reduction;

    patterns[c].nComets = cometDomains[random(COMET_DOMAIN_SIZE)];
    patterns[c].cometOffset = random(N_LEDS);
    patterns[c].velocityIndex = random(N_VELOCITIES);
    patterns[c].pingPongDur = getPingPongDur();
    patterns[c].mirror = flipCoin();
    patterns[c].reduction = 1 + random(3);
    if (c == R) {
      patterns[c].slave = false;
    } else if (c == G) {
      patterns[c].slave = slaveG;
      if (slaveG) {
        copyRedPattern(G);
      }
      patterns[c].reduction = 1 + getNewRandomLong(patterns[R].reduction - 1, 3);
    } else if (c == B) {
      patterns[c].slave = slaveB;
      if (slaveB) {
        copyRedPattern(B);
      }
      patterns[c].reduction = 1 + random(3);
      while (patterns[R].reduction == patterns[c].reduction || patterns[G].reduction == patterns[c].reduction) {
        patterns[c].reduction = 1 + random(3);
      }
    }
  }

  nCometsR = cometDomains[random(COMET_DOMAIN_SIZE)];
  cometOffsetR = random(N_LEDS);
  velocityIndexR = random(N_VELOCITIES);
  pingPongDurR = getPingPongDur();
  mirrorR = flipCoin();
  divR = 1 + random(3);

  if (slaveG) {
    nCometsG = nCometsR;
    cometOffsetG = flipCoin() ? random(N_LEDS) : cometOffsetR;
    velocityIndexG = velocityIndexR;
    pingPongDurG = pingPongDurR;
    mirrorG = mirrorR;
  } else {
    nCometsG = cometDomains[random(COMET_DOMAIN_SIZE)];
    cometOffsetG = random(N_LEDS);
    velocityIndexG = random(N_VELOCITIES);
    pingPongDurG = getPingPongDur();
    mirrorG = flipCoin();
  }
  divG = 1 + getNewRandomLong(divR - 1, 3);

  if (slaveB) {
    nCometsB = nCometsR;
    cometOffsetB = flipCoin() ? random(N_LEDS) : cometOffsetR;
    velocityIndexB = velocityIndexR;
    pingPongDurB = pingPongDurR;
    mirrorB = mirrorR;
  } else {
    nCometsB = cometDomains[random(COMET_DOMAIN_SIZE)];
    cometOffsetB = random(N_LEDS);
    velocityIndexB = random(N_VELOCITIES);
    pingPongDurB = getPingPongDur();
    mirrorB = flipCoin();
  }
  divB = 1 + random(3);
  while (divR == divB || divG == divB) {
    divB = 1 + random(3);
  }

  patterns[R].nComets = nCometsR;
  patterns[R].cometOffset = cometOffsetR;
  patterns[R].velocityIndex = velocityIndexR;
  patterns[R].pingPongDur = pingPongDurR;
  patterns[R].mirror = mirrorR;
  patterns[R].slave = false;
  patterns[R].reduction = divR;

  patterns[G].nComets = nCometsG;
  patterns[G].cometOffset = cometOffsetG;
  patterns[G].velocityIndex = velocityIndexG;
  patterns[G].pingPongDur = pingPongDurG;
  patterns[G].mirror = mirrorG;
  patterns[G].slave = slaveG;
  patterns[G].reduction = divG;

  patterns[B].nComets = nCometsB;
  patterns[B].cometOffset = cometOffsetB;
  patterns[B].velocityIndex = velocityIndexB;
  patterns[B].pingPongDur = pingPongDurB;
  patterns[B].mirror = mirrorB;
  patterns[B].slave = slaveB;
  patterns[B].reduction = divB;
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

  if (frame % pingPongDurR == 0) {
    velocityIndexR = getNewRandomLong(velocityIndexR, N_VELOCITIES);;
  }
  if (frame % pingPongDurG == 0) {
    velocityIndexG = getVelocityIndex(slaveG, velocityIndexR, velocityIndexG);
  }
  if (frame % pingPongDurB == 0) {
    velocityIndexB = getVelocityIndex(slaveB, velocityIndexR, velocityIndexB);
  }

  for (long c = 0; c < 3; c++) {
    patterns[c].velocityIndex = getVelocityIndex(patterns[c].slave, patterns[R].velocityIndex, patterns[c].velocityIndex);
  }

  patterns[R].velocityIndex = velocityIndexR;
  patterns[G].velocityIndex = velocityIndexG;
  patterns[B].velocityIndex = velocityIndexB;

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

