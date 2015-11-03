/*
 * [Cedric Adjih, Oct 2015] Relay control with TinkerKit, starting from TinkerKit example,
 * Initial copyight:
based on Blink, Arduino's "Hello World!"
Turns on an LED on for one second, then off for one second, repeatedly.
The Tinkerkit Led Modules (T010110-7) is hooked up on O0
This example code is in the public domain.
*/
#define O0 11
#define O1 10
#define O2 9
#define O3 6
#define O4 5
#define O5 3

#define I0 A0
#define I1 A1
#define I2 A2
#define I3 A3
#define I4 A4
#define I5 A5


void setOff()
{
   digitalWrite(O0, LOW); // set the LED on
   digitalWrite(O4, LOW);
   digitalWrite(O5, HIGH);
}

void setOn()
{
   digitalWrite(O0, HIGH); // set the LED on
   digitalWrite(O4, HIGH);
   digitalWrite(O5, LOW);
}



void setup() {
  Serial.begin(9600);
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(O0, OUTPUT);
  pinMode(O1, OUTPUT);
  pinMode(O2, OUTPUT);
  pinMode(O4, OUTPUT);
  pinMode(O5, OUTPUT);
  setOff();
  digitalWrite(O1, HIGH);
}

void loop() {
  if (Serial.available() > 0) {
    char data = Serial.read();
    if (data == '1' || data == '+') {
      setOn();
      Serial.println("ON");
    } else {
      setOff();
      Serial.println("OFF");
    }
  }

#if 0
  digitalWrite(O0, HIGH); // set the LED on
  delay(1000); // wait for a second
  //digitalWrite(O0, LOW); // set the LED off
  delay(1000); // wait for a second

  for (;;) {
    digitalWrite(O1, HIGH);
    //digitalWrite(O2, HIGH);
    delay(1000); // wait for a second
    digitalWrite(O1, LOW); // set the LED off
    //digitalWrite(O2, LOW);
    delay(1000); // wait for a second
  }
#endif
}

