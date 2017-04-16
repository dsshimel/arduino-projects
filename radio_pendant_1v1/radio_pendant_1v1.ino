#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_NeoPixel.h>

// General
#define PIN_ONBOARD_LED 13
#define MIN_FRAME_DURATION 50
#define MAX_OTHER_DAVIDS 1

// LEDs
#define LED_DATA_PIN 10 // Feather
#define N_LEDS 24
#define N_FRAMES 256
#define N_XFADE_FRAMES 64
#define COMET_DOMAIN_SIZE 6
#define N_VELOCITIES 4
#define MAX_COLOR 255
#define R 0
#define G 1
#define B 2

// Radio
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RFM95_FREQ 433.0
#define TX_POWER 10 // TX_POWER can range from 5 to 23
#define PING_INTERVAL 64 // == N_FRAMES / 4
#define PING_PACKET_LENGTH 29 // header (3), me, frame, rgb patterns (3 * 7), footer (3)


// Structures
struct Pattern {
  long nComets;
  long cometOffset;
  long velocityIndex;
  long pingPongDur;
  boolean mirror;
  boolean slave;
  long reduction;
};

struct David {
  char david;
  char frame;
  struct Pattern patterns[3];
  // Parameterize the white pattern that the other david does
  long index;
  long white;
};

// General variables
char me;
long frame = 0;
struct David them[MAX_OTHER_DAVIDS];
long nOthers = 0;

// Radio variables
RH_RF95 rf95(RFM95_CS, RFM95_INT);
const char HEADER[] = { 0x87, 0x12, 0x22 };
const char FOOTER[] = { 0x89, 0x06, 0x14 };
bool hasRadio = false;
uint8_t pingBuf[PING_PACKET_LENGTH];
uint8_t pingBufLen = sizeof(pingBuf);
uint8_t receiveBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t receiveBufLen = sizeof(receiveBuf);

// LED variables
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_DATA_PIN, NEO_GRBW + NEO_KHZ800);
struct Pattern patterns[3];
struct Pattern patterns0[3];
long rgb[3][N_LEDS];
long rgb0[3][N_LEDS];

// Initialization
void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(9600);
  randomSeed(analogRead(0));

  me = random(1, 256);
  Serial.print("david is ");
  Serial.println(me);

  hasRadio = initRfm();
  initStrip();
}

bool initRfm() {
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    return false;
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RFM95_FREQ)) {
    Serial.println("setFrequency failed");
    return false;
  }
  Serial.print("Set freq to ");
  Serial.println(RFM95_FREQ);

  rf95.setTxPower(TX_POWER, false);
  return true;
}

void initStrip() {
  // startup animation
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

  reset();
}

void buildPingBuf() {
  long i = 0;
  pingBuf[i++] = HEADER[0];
  pingBuf[i++] = HEADER[1];
  pingBuf[i++] = HEADER[2];
  pingBuf[i++] = me;
  pingBuf[i++] = frame;
  while (i < PING_PACKET_LENGTH - 3) {
    for (int c = 0; c < 3; c++) {
      pingBuf[i++] = (char) patterns[c].nComets;
      pingBuf[i++] = (char) patterns[c].cometOffset;
      pingBuf[i++] = (char) patterns[c].velocityIndex;
      pingBuf[i++] = (char) patterns[c].pingPongDur;
      pingBuf[i++] = patterns[c].mirror ? 1 : 0;
      pingBuf[i++] = patterns[c].slave ? 1 : 0;
      pingBuf[i++] = (char) patterns[c].reduction;
    }
  }
  pingBuf[PING_PACKET_LENGTH - 3] = FOOTER[0];
  pingBuf[PING_PACKET_LENGTH - 2] = FOOTER[1];
  pingBuf[PING_PACKET_LENGTH - 1] = FOOTER[2];
}

// Radio methods
void ping() {
  buildPingBuf();
  rf95.send((uint8_t *) pingBuf, pingBufLen);
  rf95.waitPacketSent();
}

bool isValidPacket() {
  return
    receiveBuf[0] == HEADER[0] &&
    receiveBuf[1] == HEADER[1] &&
    receiveBuf[2] == HEADER[2] &&
    receiveBuf[PING_PACKET_LENGTH - 1] == FOOTER[2] &&
    receiveBuf[PING_PACKET_LENGTH - 2] == FOOTER[1] &&
    receiveBuf[PING_PACKET_LENGTH - 3] == FOOTER[0];
}

char receivePacket() {
  if (!rf95.recv(receiveBuf, &receiveBufLen)) {
    Serial.println("Receive failed");
    return 0;
  }

  if (isValidPacket()) {
    char otherDavid = receiveBuf[3];
    char otherFrame = receiveBuf[4];

    // Signal strength, [-100, -15]
    int8_t sigStr = rf95.lastRssi();
    Serial.print("signal strength: ");
    Serial.println(sigStr);
    // TODO: do something with signal strength

    if (hasOthers()) {
      // TODO: support multiple davids
      // check if other david is this one
      if (them[0].david == otherDavid) {
        // update other
        // no need to get the patterns since I should alraedy have them
        // TODO: something with otherFrame
        // TODO: change the parameters of the pattern?
        them[0].white = MAX_COLOR;
      } else {
        // no room for david
        Serial.print("no room for other david: ");
        Serial.println(otherDavid);
      }
    } else {
      // new david this round!
      nOthers = 1; // TODO: not supporting more than 1 david right now

      them[0].david = otherDavid;
      them[0].frame = otherFrame;
      long i = 5;
      while (i < PING_PACKET_LENGTH - 3) {
        for (int c = 0; c < 3; c++) {
          them[0].patterns[c].nComets = (long) receiveBuf[i++];
          them[0].patterns[c].cometOffset = (long) receiveBuf[i++];
          them[0].patterns[c].velocityIndex = (long) receiveBuf[i++];
          them[0].patterns[c]. pingPongDur = (long) receiveBuf[i++];
          them[0].patterns[c].mirror = receiveBuf[i++] == 1 ? true : false;
          them[0].patterns[c].slave = receiveBuf[i++] == 1 ? true : false;
          them[0].patterns[c].reduction = (long) receiveBuf[i++];
        }
      }

      // initialie other david white pattern params
      them[0].index = random(N_LEDS);
      them[0].white = MAX_COLOR;
    }

    return otherDavid;
  } else {
    Serial.println("Invalid packet");
    return 0;
  }
}

void maybeDoRadio() {
  if (!hasRadio) {
    return;
  }

  if (frame % PING_INTERVAL == 0) {
    ping();
  }

  if (rf95.available()) {
    receivePacket();
  }
}

long cometDomains[COMET_DOMAIN_SIZE] = { 1, 2, 3, 4, 6, 8};
long velocities[N_VELOCITIES] = { 1, 2, N_LEDS - 2, N_LEDS - 1 };

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

void updateOtherDavidWhiteParams() {
  if (hasOthers()) {
    them[0].white -= 4;
    if (them[0].white < 0) {
      them[0].white = 0;
    }

    them[0].index = (them[0].index + (1 + N_LEDS / 2)) % N_LEDS;
  }
}

void setPixelColors() {
  long red, green, blue, white;
  for (long i = 0; i < N_LEDS; i++) {
    red = rgb[R][i];
    green = rgb[G][i];
    blue = rgb[B][i];
    if (frame < N_XFADE_FRAMES) {
      red = getXfadeValue(red, rgb0[R][i]);
      green = getXfadeValue(green, rgb0[G][i]);
      blue = getXfadeValue(blue, rgb0[B][i]);
    }

    if (hasOthers()) {
      if (i == them[0].index) {
        white = them[0].white;
      } else {
        white = 0;
      }
    } else {
      white = 0;
    }

    strip.setPixelColor(i, red, green, blue, white);
  }

  updateOtherDavidWhiteParams();
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
}

void copyRedPattern(long c) {
  patterns[c].nComets = patterns[R].nComets;
  patterns[c].cometOffset = patterns[R].cometOffset;
  patterns[c].velocityIndex = patterns[R].velocityIndex;
  patterns[c].pingPongDur = patterns[R].pingPongDur;
  patterns[c].mirror = patterns[R].mirror;
  // don't copy slave
  // don't copy reduction
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

  setPixelColors();

  strip.show();
  // delay(MIN_FRAME_DURATION); // doing this at the end of loop() now
}

long getVelocityIndex(boolean slave, long slaveIndex, long velocityIndex) {
  if (slave) {
    return slaveIndex;
  } else {
    return getNewRandomLong(velocityIndex, N_VELOCITIES);
  }
}

bool hasOthers() {
  return nOthers > 0;
}

long getRandomNComets() {
  long nComets = cometDomains[random(COMET_DOMAIN_SIZE)];
  while (random(nComets) != 0) {
    nComets = cometDomains[random(COMET_DOMAIN_SIZE)];
  }
  return nComets;
}

long getGeneticLong(long randomVal, long myVal, long theirVal) {
  return random(4) == 0 ? randomVal : flipCoin() ? myVal : theirVal;
}

long getGeneticBool(bool randomVal, bool myVal, bool theirVal) {
  return random(4) == 0 ? randomVal : flipCoin() ? myVal : theirVal;
}

void reset() {
  frame = 0;

  boolean slaveG = flipCoin();
  boolean slaveB = flipCoin();
  if (slaveG && slaveB) {
    slaveG = flipCoin();
    slaveB = !slaveG;
  }

  for (long c = 0; c < 3; c++) {
    backupPattern(c);

    // maximize the amount of variation by always changing all rgb pattern params
    if (hasOthers()) {
      // TODO: support multiple davids? crossbreeding algo would get trickier for maybe not much gain
      Pattern otherPattern = them[0].patterns[c];
      patterns[c].nComets = getGeneticLong(getRandomNComets(), patterns[c].nComets, otherPattern.nComets);
      patterns[c].cometOffset = getGeneticLong(random(N_LEDS), patterns[c].cometOffset, otherPattern.cometOffset);
      patterns[c].velocityIndex = getGeneticLong(random(N_VELOCITIES), patterns[c].velocityIndex, otherPattern.velocityIndex);
      patterns[c].pingPongDur = getGeneticLong(getPingPongDur(), patterns[c].pingPongDur, otherPattern.pingPongDur);
      patterns[c].mirror = getGeneticBool(flipCoin(), patterns[c].mirror, otherPattern.mirror);
    } else {
      patterns[c].nComets = getRandomNComets();
      patterns[c].cometOffset = random(N_LEDS);
      patterns[c].velocityIndex = random(N_VELOCITIES);
      patterns[c].pingPongDur = getPingPongDur();
      patterns[c].mirror = flipCoin();
    }

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

  nOthers = 0;
}

void loop() {
  unsigned long frameStartTime = millis();

  render();

  maybeDoRadio();

  frame++;

  for (long c = 0; c < 3; c++) {
    if (frame % patterns[c].pingPongDur == 0) {
      patterns[c].velocityIndex = getVelocityIndex(patterns[c].slave, patterns[R].velocityIndex, patterns[c].velocityIndex);
    }
  }

  if (frame >= N_FRAMES) {
    reset();
  }

  unsigned long frameDuration = millis() - frameStartTime;
  if (frameDuration < MIN_FRAME_DURATION) {
    long millisToDelay = MIN_FRAME_DURATION - frameDuration;
    if (millisToDelay < 0) {
      millisToDelay = 0;
    }
    if (millisToDelay > MIN_FRAME_DURATION) {
      millisToDelay = MIN_FRAME_DURATION;
    }
    delay(millisToDelay);
  }
}
