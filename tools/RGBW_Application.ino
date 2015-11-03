/*

 Designer Systems DS-RGBW Shield Arduino Demonstrator
 
 Requires Arduino Duemilanove or MEGA or NANO boards
 
 DS-RGBW.S [A0 & A1 jumpers ON] connected to I2C interface (ANALOG IN 4 [SDA] & 5 [SCL])
 
 RGBW_Application.ino	Date: 24/6/13	Version: 1.00 

Modified: C. Adjih, oct 2015

 */

#include <Wire.h>

#define RGBW 0x70					// RGBW I2C Address

// Setup code
void setup()
{

  Wire.begin();						// Start I2C comms
  Serial.begin(9600); 
}

#define DELAY 1000

// Main code
void loop(){

  byte Data;
  
  // All off
  Wire.beginTransmission(RGBW);
  Wire.write(2);				      
  Wire.write(0);				      
  Wire.write(0);				      
  Wire.write(0);				      
  Wire.write(0);				      
  Wire.endTransmission();

  // Wait 1 second
  delay(DELAY);

  //for(;;)
  {

  // Fade WHITE LED from 0 to 100%
  for (Data = 0; Data < 100; Data++) {
    WriteRegister(2, Data);
    delay(10);
  }

  // Fade WHITE LED from 100% to 0%
  for (Data = 99; Data > 0; Data--) {
    WriteRegister(2, Data);
    delay(10);
  }


  }


  for (;;) {
    int i1,i2,i3;
    for (i1=0;i1<2;i1++) 
    for (i2=0;i2<2;i2++) 
    for (i3=0;i3<2;i3++) 
    for (Data = 0; Data < 255; Data++) {
      WriteRGBRegisters(i1*Data, i2*Data, i3*Data);
      WriteRegister(2, 0);
      delay(5);
    }



    // Send RGB colour 255,88,67 [Pink]
    //WriteRGBRegisters(255,88,67);
  
  }
  
  // Wait 1 second
  delay(DELAY);
  
  // Send RGB colour 255,88,67 [Pink]
  WriteRGBRegisters(255,88,67);
  
  // Wait 1 second
  delay(DELAY);

  // Send RGB colour 48,187,80 [Light Green]
  WriteRGBRegisters(48,187,80);
  
  // Wait 1 second
  delay(DELAY);

  // Send RGB colour 243,99,20 [Orange]
  WriteRGBRegisters(243,99,20);
  
  // Wait 1 second
  delay(DELAY);

  // Send RGB colour 86,54,80 [Dark Purple]
  WriteRGBRegisters(86,54,80);
  
  // Wait 1 second
  delay(DELAY);
  
  // Set colour wheel to start colour
  WriteHSBRegisters(0,255,255);
  
  // Rotate through complete colour wheel by changing HSB Hue value
  for (Data = 0; Data < 255; Data++) {
    WriteRegister(6, Data);
    delay(80);
  }
 
  // Wait 1 second
  delay(DELAY);
  
}

/****************************************************************************
  Function:
    void WriteRegister(byte Register, byte Value)

  Description:
    This routine writes a byte value to an I2C register

  Precondition:
    Wire.Begin() has been called

  Parameters:
    Register = Register address
    Value = Byte value to write to register address

  Returns:
    None
    
  Remarks:
    None
  ***************************************************************************/
void WriteRegister(byte Register, byte Value) {
  Wire.beginTransmission(RGBW);
  Wire.write(Register);				      
  Wire.write(Value);				      
  Wire.endTransmission();
}

/****************************************************************************
  Function:
    void WriteRGBRegisters(byte Red, byte Green, byte Blue)

  Description:
    This routine writes an RGB value to the DS-RGB.S RGB registers

  Precondition:
    Wire.Begin() has been called

  Parameters:
    Red = RED value
    Green = GREEN value
    Blue = BLUE value

  Returns:
    None
    
  Remarks:
    None
  ***************************************************************************/
void WriteRGBRegisters(byte Red, byte Green, byte Blue) {
  Wire.beginTransmission(RGBW);
  Wire.write(3);				      
  Wire.write(Red);				      
  Wire.write(Green);				      
  Wire.write(Blue);				      
  Wire.endTransmission();
}

/****************************************************************************
  Function:
    void WriteHSBRegisters(byte Hue, byte Saturation, byte Brightness)

  Description:
    This routine writes an HSB value to the DS-RGB.S HSB registers

  Precondition:
    Wire.Begin() has been called

  Parameters:
    Hue = HUE value
    Saturation = Saturation value
    Brightness = Brightness value

  Returns:
    None
    
  Remarks:
    None
  ***************************************************************************/
void WriteHSBRegisters(byte Hue, byte Saturation, byte Brightness) {
  Wire.beginTransmission(RGBW);
  Wire.write(6);				      
  Wire.write(Hue);				      
  Wire.write(Saturation);				      
  Wire.write(Brightness);				      
  Wire.endTransmission();
}

