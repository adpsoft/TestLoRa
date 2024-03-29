#include "heltec.h"
#include "images.h"

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6


byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to

unsigned int msgCount = 0;            // count of outgoing messages
unsigned int randomValue = 0;
unsigned int loss = 0;
unsigned int interval = 1000;
unsigned long previousMillis = 0;

void sendMessage(const String &outgoing);

void onReceive(int packetSize);

void logo();


void __unused setup() {
    //WIFI Kit series V1 not support Vext control
    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/,
                 true /*PABOOST Enable*/, BAND /*long BAND*/);

    Heltec.display->init();
    Heltec.display->flipScreenVertically();
    Heltec.display->setFont(ArialMT_Plain_10);
    logo();
    delay(1500);
    Heltec.display->clear();

    Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
    Heltec.display->drawString(0, 10, "Wait for incoming data...");
    Heltec.display->display();
    delay(1000);
    Serial.println("Heltec.LoRa init succeeded.");

    //LoRa.onReceive(onReceive);
    //LoRa.receive();
}

void logo() {
    Heltec.display->clear();
    Heltec.display->drawXbm(0, 5, logo_width, logo_height, logo_bits);
    Heltec.display->display();
}

void DisplayData(String rssi, String packet, String id, String recvRnd) {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 15, "Received Seq " + id);
    Heltec.display->drawStringMaxWidth(0, 26, 128, packet);
    Heltec.display->drawStringMaxWidth(0, 37, 128, "My random (" + String(randomValue) + ")");
    Heltec.display->drawStringMaxWidth(0, 48, 128, "His random (" + recvRnd + ")");
    Heltec.display->setColor(BLACK);
    Heltec.display->fillRect(0, 0, 128, 10);
    Heltec.display->setColor(WHITE);
    Heltec.display->drawString(0, 0, rssi);
    Heltec.display->display();
}

void DisplayLoss() {
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->setColor(BLACK);
    Heltec.display->fillRect(0, 0, 128, 10);
    Heltec.display->setColor(WHITE);
    Heltec.display->drawString(0, 0, "Lost packets: " + String(++loss));
    Heltec.display->display();
}

void __unused loop() {

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        String message = "Hello World RND(" + String(random(1000)) + ")!";   // send a message

        randomValue = random(10000, 100000);

        sendMessage(message);
        Serial.println("Sending " + message);

        interval = random(1500,3000);
    }

    int pktSize;
    if (pktSize = LoRa.parsePacket()) {
        onReceive(pktSize);
    } else {
        DisplayLoss();
    }
}

void sendMessage(const String &outgoing) {
    LoRa.beginPacket();                   // start packet
    LoRa.write(destination);              // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(reinterpret_cast<const uint8_t *>(&randomValue), sizeof(unsigned int));                 // add message ID
    LoRa.write(reinterpret_cast<const uint8_t *>(&msgCount), sizeof(unsigned int));                 // add message ID
    LoRa.write(outgoing.length());        // add payload length
    LoRa.print(outgoing);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;          // if there's no packet, return

    // read packet header bytes:
    int recipient = LoRa.read();          // recipient address
    byte sender = LoRa.read();            // sender address

    unsigned int rndValue;
    LoRa.readBytes(reinterpret_cast<uint8_t *>(&rndValue), sizeof(unsigned int));
    unsigned int incomingMsgId;
    LoRa.readBytes(reinterpret_cast<uint8_t *>(&incomingMsgId), sizeof(unsigned int));
    byte incomingLength = LoRa.read();    // incoming msg length

    String incoming = "";                 // payload of packet

    while (LoRa.available())             // can't use readString() in callback
    {
        incoming += (char) LoRa.read();      // add bytes one by one
    }

    if (incomingLength != incoming.length())   // check length for error
    {
        Serial.println("error: message length does not match length");
        return;                             // skip rest of function
    }

    // if the recipient isn't this device or broadcast,
    if (recipient != localAddress && recipient != 0xFF) {
        Serial.println("This message is not for me.");
        return;                             // skip rest of function
    }

    // if message is for this device, or broadcast, print details:
    Serial.println("Received from: 0x" + String(sender, HEX));
    Serial.println("Sent to: 0x" + String(recipient, HEX));
    Serial.println("Message ID: " + String(incomingMsgId));
    Serial.println("Message length: " + String(incomingLength));
    Serial.println("Message: " + incoming);
    Serial.println("RSSI: " + String(LoRa.packetRssi()));
    Serial.println("Snr: " + String(LoRa.packetSnr()));
    Serial.println();

    DisplayData("RSSI: " + String(LoRa.packetRssi()), incoming, String(incomingMsgId), String(rndValue));
}
