#include <SPI.h>
#include <RH_RF95.h>

#define PIN_ONBOARD_LED 13

// for the feather
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RFM95_FREQ 433.0
#define TX_POWER 23
#define RECEIVE_TIMEOUT 1000 // TODO: make this number smaller
#define IS_TRANSMITTER true

RH_RF95 rf95(RFM95_CS, RFM95_INT);

const int packetLength = 20;

void resetRfm95() {
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}

void initRfm() {
  resetRfm95();

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RFM95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set freq to ");
  Serial.println(RFM95_FREQ);

  // TX_POWER can range from 5 to 23
  rf95.setTxPower(TX_POWER, false);
}

void setup() {
  pinMode(PIN_ONBOARD_LED, OUTPUT);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // wait until serial console is open, remove if not tethered to computer
  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Feather LoRa TX Test!");
  initRfm();
}

void doBlink(int duration) {
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  delay(duration);
  digitalWrite(PIN_ONBOARD_LED, LOW);
}

void sendPacket() {
  Serial.println("Sending to rf95_server");
  char radiopacket[packetLength] = "Hello";
  Serial.print("Sending ");
  Serial.println(radiopacket);
  radiopacket[packetLength - 1] = 0; // null terminate the string

  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *) radiopacket, packetLength);

  Serial.println("Waiting for packet to complete.");
  delay(10);
  rf95.waitPacketSent();
}

bool receivePacket() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.recv(buf, &len)) {
    RH_RF95::printBuffer("Received: ", buf, len);
    Serial.print("Got message: ");
    Serial.println((char *) buf);
    Serial.print("RSSI: ");
    // Signal strength, [-100, -15]
    Serial.println(rf95.lastRssi(), DEC);
    return true;
  } else {
    Serial.println("Receive failed");
    return false;
  }
}

void tryReceivePacket() {
  Serial.println("Waiting for reply...");
  delay(10);
  if (rf95.waitAvailableTimeout(RECEIVE_TIMEOUT)) {
    receivePacket();
  } else {
    Serial.println("No reply, is there a listener around?");
  }
}

void loop() {
  doBlink(1000);
  delay(1000);

  if (IS_TRANSMITTER) {
    sendPacket();
    tryReceivePacket();
  } else {
    Serial.println("I'm a receiver");
    if (rf95.available() && receivePacket()) {
      delay(10);
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
    } else {
      Serial.println("Didn't get anything");
    }
  }
}
