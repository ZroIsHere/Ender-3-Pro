/*
  config.h - ESP3D configuration file

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
  License along with This code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _ESP3D_CONFIG_H
#define _ESP3D_CONFIG_H
#include <Arduino.h>

#include "../include/esp3d_defines.h"

#include "../../configuration.h"
#define ESP3D_CODE_BASE "ESP3D"

#include "../core/esp3d_hal.h"
#include "../core/esp3d_log.h"
#include "../include/esp3d_pins.h"
#include "../include/esp3d_sanity.h"
#include "../include/esp3d_version.h"

#if defined(ARDUINO_ARCH_ESP8266)
/************************************
 *
 * SSL Client
 *
 * **********************************/
// Using BearSSL need to decrease size of packet to not be OOM on ESP8266
#define BEARSSL_MFLN_SIZE 512
#define BEARSSL_MFLN_SIZE_FALLBACK 4096
#endif  // ARDUINO_ARCH_ESP8266

/************************************
 *
 * Additional Flags
 *
 * **********************************/

// Make Flag more generic
#if (defined(PIN_RESET_FEATURE) && defined(ESP3D_RESET_PIN) && \
     ESP3D_RESET_PIN != -1) ||                                 \
    defined(SD_RECOVERY_FEATURE)
#define RECOVERY_FEATURE
#endif  // PIN_RESET_FEATURE || SD_RECOVERY_FEATURE

#if defined(DISPLAY_DEVICE) || defined(SENSOR_DEVICE) ||   \
    defined(RECOVERY_FEATURE) || defined(BUZZER_DEVICE) || \
    defined(CAMERA_DEVICE) || defined(SD_DEVICE)
#define CONNECTED_DEVICES_FEATURE
#endif  // DISPLAY_DEVICE || SENSOR_DEVICE , etc...

#endif  //_ESP3D_CONFIG_H
