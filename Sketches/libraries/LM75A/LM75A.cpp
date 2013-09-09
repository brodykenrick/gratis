/*****************************************************************************
 *
 *   Copyright(C) 2012, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * Embedded Artists AB assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. Embedded Artists AB
 * reserves the right to make changes in the software without
 * notification. Embedded Artists AB also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 *****************************************************************************/

 /******************************************************************************
 * Includes
 *****************************************************************************/
 
#include <Arduino.h>
#include <Wire.h>

#include "LM75A.h"

/******************************************************************************
 * Defines and typedefs
 *****************************************************************************/

#define LM75A_I2C_ADDR 0x49 
#define LM75A_CMD_TEMP 0x00


LM75A_Class::LM75A_Class()
{
  Wire.begin();
}

int LM75A_Class::read()
{
  int t = 0;
  
  Wire.beginTransmission(LM75A_I2C_ADDR);
  Wire.write(LM75A_CMD_TEMP);
  Wire.endTransmission();
  
  Wire.requestFrom(LM75A_I2C_ADDR, 2);
  if (Wire.available() == 2) {
    t = (Wire.read() << 8);
    t += Wire.read();
  }
  
  return (t >> 8);
}


