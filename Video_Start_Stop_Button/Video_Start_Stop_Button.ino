#include <Bounce.h>

// edit usb_desc.h to set VENDOR_ID to 16C1

Bounce button1 = Bounce(1, 20);
Bounce button2 = Bounce(2, 20);
elapsedMillis timeout;

void setup() {
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  //Serial.begin(9600);
  //Serial.println(F("RawHID Video Record Button"));
}

void loop() {
  byte buffer[64];
  int n = RawHID.recv(buffer, 0); // 0 timeout = do not wait
  if (n > 0) {
    // the computer sent a message.  Display the bits
    // of the first byte on pin 0 to 7.  Ignore the
    // other 63 bytes!
    //Serial.print(F("Received packet, first byte: "));
    //Serial.println((int)buffer[0]);
    digitalWrite(3, (buffer[0] & 0x80) ? HIGH : LOW);
    digitalWrite(4, (buffer[0] & 0x01) ? HIGH : LOW);
    digitalWrite(5, (buffer[0] & 0x02) ? HIGH : LOW);
    digitalWrite(6, (buffer[0] & 0x04) ? HIGH : LOW);
    digitalWrite(7, (buffer[0] & 0x08) ? HIGH : LOW);
    timeout = 0;
  }
  button1.update();
  button2.update();
  if (button1.fallingEdge()) {
    //Serial.println("Button 1 press (Go)");
    memset(buffer, 0, sizeof(buffer));
    strcpy((char *)buffer, "Go Button Press");
    RawHID.send(buffer, 100);
  }
  if (button2.fallingEdge()) {
    //Serial.println("Button 2 press (Stop)");
    memset(buffer, 0, sizeof(buffer));
    strcpy((char *)buffer, "Stop Button Press");
    RawHID.send(buffer, 100);
  }
  if (timeout > 1500) {
    digitalWrite(3, LOW); // Software on computer no longer running
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
  }
}
