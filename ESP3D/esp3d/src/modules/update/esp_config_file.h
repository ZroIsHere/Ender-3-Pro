/*
  esp_config_file.h - ESP3D configuration file support class

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

#ifndef _ESP_CONFIG_FILE_H
#define _ESP_CONFIG_FILE_H
#include <Arduino.h>
typedef std::function<bool(const char *, const char *, const char *)>
    TProcessingFunction;

class ESP_ConfigFile {
 public:
  ESP_ConfigFile(const char *path, TProcessingFunction fn);
  ~ESP_ConfigFile();
  char *trimSpaces(char *line, uint8_t maxsize = 0);
  bool isComment(char *line);
  bool isSection(char *line);
  bool isValue(char *line);
  char *getSectionName(char *line);
  char *getKeyName(char *line);
  char *getValue(char *line);
  bool processFile();
  bool revokeFile();

 private:
  bool isScrambleKey(const char *key, const char *str);
  char *_filename;
  TProcessingFunction _pfunction;
};

#endif  //_ESP_CONFIG_FILE_H
