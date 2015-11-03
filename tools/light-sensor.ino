/*
Analog input, analog output, serial output
Reads an analog input pin, and T000090 LDR Analog Sensor connected to I0,
maps the result to a range from 0 to 255
and uses the result to set the pulsewidth modulation (PWM) on a T010111
LED Module connected on O0.
Also prints the results to the serial monitor.
created 29 Dec. 2008
Modified 4 Sep 2010
by Tom Igoe
modified 7 dec 2010
by Davide Gomba
This example code is in the public domain.

modified, C. Adjih, oct 2015
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
// These constants won't change. They're used to give names
// to the pins used:
const int analogInPin = I0; // Analog input pin that the LDR is attached to
const int analogOutPin= O0; // Analog output pin that the LED is attached to
const int analogOut2Pin= O1; // Analog output pin that the LED is attached to

int sensorValue = 0; // value read from the pot
int outputValue = 0; // value output to the PWM (analog out)

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
}

void loop() {
  // read the analog in value:
  sensorValue = analogRead(analogInPin); 
  // map it to the range of the analog out:
  outputValue = map(sensorValue, 0, 1023, 0, 255);
  // change the analog out value:
  analogWrite(analogOutPin, outputValue);
    // change the analog out value:
  unsigned long t = millis();
  unsigned long t2 = t >> 3u;
  t2 = t2 & 255;
  analogWrite(analogOut2Pin, t2);
  
  // print the results to the serial monitor:
  Serial.print("sensor = " );
  Serial.print(sensorValue);
  Serial.print("\t output = ");
  Serial.println(outputValue);
  // wait 10 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(10);
}



