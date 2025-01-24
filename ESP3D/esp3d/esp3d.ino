/*
  esp3d

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "src/core/esp3d.h"
// global variable
Esp3D myesp3d;

int filamentRunOutPin = 14;
bool filamentRunOut = false;

//Setup
void setup()
{
    pinMode(filamentRunOutPin, INPUT);
    myesp3d.begin();
}
//main loop
void loop()
{
    if (digitalRead(filamentRunOutPin) == LOW && !filamentRunOut){
      filamentRunOut = true;
      Serial.println("M412 H1");
    }
    if (digitalRead(filamentRunOutPin) == HIGH && filamentRunOut){
      filamentRunOut = false;
      delay(10000);
      Serial.println("M412 H1");
    }
    myesp3d.handle();
}
