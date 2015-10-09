#include "util.h"

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  dump_eeprom();
  Serial.print("UID: ");
  Serial.print(get_uid(), HEX);
  for (;;)
    ;
}
