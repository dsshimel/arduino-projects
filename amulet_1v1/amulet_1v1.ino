#include <SPI.h>
#include <RH_RF95.h>

#define PIN_ONBOARD_LED 13

// for the feather
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RFM95_FREQ 433.0
#define TX_POWER 23

#define MAX_FRAME 256
#define MIN_FRAME_DURATION 47
#define PING_FRAME 20
#define PING_PACKET_LENGTH 10
#define DAVID_EVICTION_MILLIS 4000

#define NUM_LEDS = 24;

RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint8_t pingBuf[PING_PACKET_LENGTH];
uint8_t pingBufLen = sizeof(pingBuf);
uint8_t receiveBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t receiveBufLen = sizeof(receiveBuf);

char otherDavid = 0;
char otherFrame = -1;
char otherFrameGen = -1;
int otherMinSigStr = 0;
int otherMaxSigStr = 0;
unsigned long otherLastSeen = -1;
bool hasOtherDavid = false;

bool hasRadio = false;
unsigned long minFrameDuration = MIN_FRAME_DURATION;
unsigned long extraTime = 0;

const char HEADER[] = { 0x87, 0x12, 0x22 };
char david = 0;
char frame = 0;
char frameGen = 0;
const char FOOTER[] = { 0x89, 0x06, 0x14 };

void resetRfm95() {
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}

bool initRfm() {
  resetRfm95();

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

  // TX_POWER can range from 5 to 23
  rf95.setTxPower(TX_POWER, false);
  return true;
}

void setupRadio() {
  hasRadio = initRfm();
  minFrameDuration = MIN_FRAME_DURATION;
}

void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  //  while (!Serial);
  Serial.begin(9600);
  randomSeed(analogRead(0));

  david = random(1, 256);
  Serial.print("david is ");
  Serial.println(david);

  delay(100); // TODO: do I need this?

  setupRadio();
}

void ping(char responseTo) {
  pingBuf[0] = HEADER[0];
  pingBuf[1] = HEADER[1];
  pingBuf[2] = HEADER[2];
  pingBuf[3] = david;
  pingBuf[4] = frameGen;
  pingBuf[5] = frame;
  pingBuf[6] = responseTo;
  pingBuf[7] = FOOTER[0];
  pingBuf[8] = FOOTER[1];
  pingBuf[9] = FOOTER[2];
//  Serial.print("Sending as ");
//  Serial.println(david, HEX);
  if (responseTo == 0x00) {
    //    RH_RF95::printBuffer("Pinging everyone: ", pingBuf, pingBufLen);
    Serial.println("pinging world");
  } else {
    //    RH_RF95::printBuffer("Acking: ", pingBuf, pingBufLen);
    Serial.print("acking ");
    Serial.println(responseTo, HEX);
  }
  //  Serial.println(pingBuf);

  rf95.send((uint8_t *) pingBuf, pingBufLen);
  rf95.waitPacketSent();
}

int getNumDavids() {
  return hasOtherDavid ? 2 : 1;
}

void handleOtherDavid(
  char newDavid, char newFrameGen, char newFrame, char responseTo, int8_t newSigStr) {
  unsigned long now = millis();

  // update cases
  if (hasOtherDavid && otherDavid == newDavid) {
    otherDavid = newDavid;
    otherMinSigStr = min(otherMinSigStr, newSigStr);
    otherMaxSigStr = max(otherMaxSigStr, newSigStr);
    otherFrameGen = newFrameGen;
    otherFrame = newFrame;
    otherLastSeen = now;
  }
  // didn't find someone to update, so check if I'm a new player
  else if (!hasOtherDavid) {
    otherDavid = newDavid;
    otherMinSigStr = newSigStr;
    otherMaxSigStr = newSigStr;
    otherFrameGen = newFrameGen;
    otherFrame = newFrame;
    hasOtherDavid = true;
    otherLastSeen = now;
  }

  if (hasOtherDavid) {
    if (otherMinSigStr == otherMaxSigStr) {
      // no good
      Serial.println("no signal strength range");
      Serial.print("scaled sig str ");
      Serial.println(255, DEC);
    } else {
      unsigned long otherRange = (otherMaxSigStr - otherMinSigStr + 1);
      unsigned long sigStr = 255 * (newSigStr - otherMinSigStr + 1);
      char scaledSigStr = sigStr / otherRange;
      
//      float sigStrPercent = (float) (newSigStr - otherMinSigStr) / (float) otherRange;
      Serial.print("scaled sig str ");
      Serial.println(scaledSigStr, DEC);
    }
  }

  if (responseTo == david) {
    Serial.print("receiving response from ");
    Serial.println(otherDavid, HEX);
  } else {
    Serial.print("detecting ping from ");
    Serial.println(otherDavid, HEX);
  }
}

char receivePacket() {
  if (rf95.recv(receiveBuf, &receiveBufLen)) {
    //    RH_RF95::printBuffer("Received: ", receiveBuf, receiveBufLen);
    if (receiveBuf[0] == HEADER[0] &&
        receiveBuf[1] == HEADER[1] &&
        receiveBuf[2] == HEADER[2] &&
        receiveBuf[PING_PACKET_LENGTH - 1] == FOOTER[2] &&
        receiveBuf[PING_PACKET_LENGTH - 2] == FOOTER[1] &&
        receiveBuf[PING_PACKET_LENGTH - 3] == FOOTER[0]) {
      char newDavid = receiveBuf[3];
      char newFrameGen = receiveBuf[4];
      char newFrame = receiveBuf[5];
      char responseTo = receiveBuf[6];
      // Signal strength, [-100, -15]
      int8_t newSigStr = rf95.lastRssi();

      handleOtherDavid(newDavid, newFrameGen, newFrame, responseTo, newSigStr);

      return newDavid;
    } else {
      Serial.println("Invalid packet");
      return 0;
    }
  } else {
    Serial.println("Receive failed");
    return 0;
  }
}

void checkMail() {
  unsigned long now = millis();
  if (hasOtherDavid && now - otherLastSeen > DAVID_EVICTION_MILLIS) {
    hasOtherDavid = false;
    Serial.print("Evicted ");
    Serial.println(otherDavid, HEX);
    Serial.print("Did not see him for ");
    Serial.println(now - otherLastSeen, DEC);
  }

  if (rf95.available()) {
    char newDavid = receivePacket();
    if (newDavid > 0) {
      ping(newDavid);
    }
  }
}

void doRadio() {
  if (frame % PING_FRAME == 0) {
    ping(0x00 /* responseTo */);
  }

  checkMail();
}

void maybeDoRadio() {
  if (hasRadio) {
    doRadio();
  }
}

void render() {

}

void loop() {
  unsigned long frameStartTime = millis();

  maybeDoRadio();

  unsigned long radioEndTime = millis();

  render();

  frame++;
  if (frame == 0) {
    frameGen++;
  }

  unsigned long frameEndTime = millis();
  unsigned long frameDuration = frameEndTime - frameStartTime;
  if (frameDuration > 0) { }

  if (frameDuration <= minFrameDuration) {
    extraTime = minFrameDuration - frameDuration;
    delay(extraTime);
    if (minFrameDuration > MIN_FRAME_DURATION) {
      minFrameDuration -= 1;
    }
  } else {
    minFrameDuration = frameDuration;
    Serial.print("Min frame duration now ");
    Serial.println(minFrameDuration);
  }
}
