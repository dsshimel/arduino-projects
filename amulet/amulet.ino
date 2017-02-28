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
#define PING_FRAME 25
#define PING_PACKET_LENGTH 10
#define DAVID_EVICTION_MILLIS 4000

#define NUM_LEDS = 24;

RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint8_t pingBuf[PING_PACKET_LENGTH];
uint8_t pingBufLen = sizeof(pingBuf);
uint8_t receiveBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t receiveBufLen = sizeof(receiveBuf);

struct David {
  char david;
  char frame;
  int minSigStr;
  int maxSigStr;
  unsigned long lastSeen;
} playerOne, playerTwo;
bool hasPlayerOne = false;
bool hasPlayerTwo = false;

bool hasRadio = false;
unsigned long minFrameDuration = 0;

const char HEADER[] = { 0x87, 0x12, 0x22 };
char david = 0;
char frame = 0;
char frameGenerations = 0;
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
  minFrameDuration = 0;
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
  pingBuf[4] = frameGenerations;
  pingBuf[5] = frame;
  pingBuf[6] = responseTo;
  pingBuf[7] = FOOTER[0];
  pingBuf[8] = FOOTER[1];
  pingBuf[9] = FOOTER[2];
  //  Serial.print("Pinging ");
  Serial.print("Sending as ");
  Serial.println(david, HEX);
  if (responseTo == 0x00) {
    //    RH_RF95::printBuffer("Pinging everyone: ", pingBuf, pingBufLen);
    Serial.println("*** SYN ***");
  } else {
    //    RH_RF95::printBuffer("Acking: ", pingBuf, pingBufLen);
    Serial.print("*** ACK ***\t\t");
    Serial.println(responseTo, HEX);
  }
  //  Serial.println(pingBuf);

  rf95.send((uint8_t *) pingBuf, pingBufLen);
  rf95.waitPacketSent();
}

void doBlink(int duration) {
  //  digitalWrite(PIN_ONBOARD_LED, HIGH);
  //  delay(duration);
  //  digitalWrite(PIN_ONBOARD_LED, LOW);
  //  delay(duration);
}

int getNumDavids() {
  int result = 1;
  if (hasPlayerOne) {
    result += 1;
  }
  if (hasPlayerTwo) {
    result += 1;
  }
  return result;
}

void handleOtherDavid(
  char otherDavid, char otherFrameGen, char otherFrame, char responseTo, int8_t otherSigStr) {
  unsigned long now = millis();

  int minSigStr = otherSigStr;
  int maxSigStr = otherSigStr;
  // update cases
  if (hasPlayerOne && playerOne.david == otherDavid) {
    minSigStr = min(playerOne.minSigStr, minSigStr);
    maxSigStr = max(playerOne.maxSigStr, maxSigStr);
    playerOne = { otherDavid, otherFrame, minSigStr, maxSigStr, now };
  } else if (hasPlayerTwo && playerTwo.david == otherDavid) {
    minSigStr = min(playerTwo.minSigStr, minSigStr);
    maxSigStr = max(playerTwo.maxSigStr, maxSigStr);
    playerTwo = { otherDavid, otherFrame, minSigStr, maxSigStr, now };
  }
  // didn't find someone to update, so check if I'm a new player
  else if (!hasPlayerOne) {
    playerOne = { otherDavid, otherFrame, minSigStr, maxSigStr, now };
    hasPlayerOne = true;
  } else if (!hasPlayerTwo) {
    playerTwo = { otherDavid, otherFrame, minSigStr, maxSigStr, now };
    hasPlayerTwo = true;
  }

  if (responseTo == david) {
    Serial.print("Ack from ");
    Serial.println(otherDavid, HEX);
    //    boolean hesOlder = false;
    //    if (otherFrameGen > frameGenerations) {
    //      hesOlder = true;
    //    }
    //    if (otherFrameGen == frameGenerations && otherFrame > frame) {
    //      hesOlder = true;
    //    }
  } else {
    Serial.print("Ping from ");
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
      char otherDavid = receiveBuf[3];
      char otherFrameGen = receiveBuf[4];
      char otherFrame = receiveBuf[5];
      char responseTo = receiveBuf[6];
      // Signal strength, [-100, -15]
      int8_t otherSigStr = rf95.lastRssi();

      handleOtherDavid(otherDavid, otherFrameGen, otherFrame, responseTo, otherSigStr);
      
      //      for (int i = 0; i < getNumDavids(); i++) {
      //        doBlink(5);
      //      }

      return otherDavid;
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
  if (hasPlayerOne && now - playerOne.lastSeen > DAVID_EVICTION_MILLIS) {
    hasPlayerOne = false;
    Serial.print("Evicted ");
    Serial.println(playerOne.david, HEX);
    Serial.print("Duration ");
    Serial.println(now - playerOne.lastSeen, DEC);
  }

  if (hasPlayerTwo && now - playerTwo.lastSeen > DAVID_EVICTION_MILLIS) {
    hasPlayerTwo = false;
    Serial.print("Evicted ");
    Serial.println(playerTwo.david, HEX);
  }

  if (rf95.available()) {
    char otherDavid = receivePacket();
    if (otherDavid > 0) {
      ping(otherDavid);
    }
  }
}

void doRadio() {
  if (frame % PING_FRAME == 0) {
    ping(0x00 /* responseTo */);
    doBlink(5);
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

  //  Serial.print("frame: ");
  //  Serial.println(frame, DEC);

  maybeDoRadio();

  render();

  frame++;
  if (frame == 0) {
    frameGenerations++;
  }

  unsigned long frameEndTime = millis();
  unsigned long frameDuration = frameEndTime - frameStartTime;
  if (frameDuration > 0) {
    Serial.print("frame duration ");
    Serial.println(frameDuration, DEC);
  }

  if (frameDuration <= minFrameDuration) {
    Serial.print("extra time ");
    unsigned long extraTime = minFrameDuration - frameDuration;
    Serial.println(extraTime, DEC);
    if (rf95.waitAvailableTimeout(extraTime / 2)) {
      checkMail();
    }
    delay(extraTime / 2);
    if (minFrameDuration > 0) {
      minFrameDuration -= 1;
    }
  } else {
    minFrameDuration = frameDuration;
    Serial.print("Min frame duration now ");
    Serial.println(minFrameDuration);
  }
}
