/*
sd_sdfat2_esp32.cpp - ESP3D sd support class

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
#include "../../../include/esp3d_config.h"
#if defined(ARDUINO_ARCH_ESP32) && defined(SD_DEVICE)
#if (SD_DEVICE == ESP_SDFAT2)

#include <SdFat.h>
#include <sdios.h>

#include <stack>

#include "../../../core/esp3d_settings.h"
#include "../esp_sd.h"

#if SDFAT_FILE_TYPE == 1
typedef File32 File;
#elif SDFAT_FILE_TYPE == 2
typedef ExFile File;
#elif SDFAT_FILE_TYPE == 3
typedef FsFile File;
#else  // SDFAT_FILE_TYPE
#error Invalid SDFAT_FILE_TYPE
#endif  // SDFAT_FILE_TYPE

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG \
  SdSpiConfig((ESP_SD_CS_PIN == -1) ? SS : ESP_SD_CS_PIN, DEDICATED_SPI)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG \
  SdSpiConfig((ESP_SD_CS_PIN == -1) ? SS : ESP_SD_CS_PIN, SHARED_SPI)
#endif  // HAS_SDIO_CLASS

extern File tSDFile_handle[ESP_MAX_SD_OPENHANDLE];

// Max Freq Working
#define FREQMZ 40
SdFat SD;
#undef FILE_WRITE
#undef FILE_READ
#undef FILE_APPEND
#define FILE_WRITE (O_RDWR | O_CREAT | O_AT_END)
#define FILE_READ O_RDONLY
#define FILE_APPEND (O_RDWR | O_AT_END)

void dateTime(uint16_t* date, uint16_t* dtime) {
  struct tm tmstruct;
  time_t now;
  time(&now);
  localtime_r(&now, &tmstruct);
  *date = FAT_DATE((tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1,
                   tmstruct.tm_mday);
  *dtime = FAT_TIME(tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
}

time_t getDateTimeFile(File& filehandle) {
  static time_t dt = 0;
#ifdef SD_TIMESTAMP_FEATURE
  struct tm timefile;
  uint16_t date;
  uint16_t time;
  if (filehandle) {
    if (filehandle.getModifyDateTime(&date, &time)) {
      timefile.tm_year = FS_YEAR(date) - 1900;
      timefile.tm_mon = FS_MONTH(date) - 1;
      timefile.tm_mday = FS_DAY(date);
      timefile.tm_hour = FS_HOUR(time);
      timefile.tm_min = FS_MINUTE(time);
      timefile.tm_sec = FS_SECOND(time);
      timefile.tm_isdst = -1;
      dt = mktime(&timefile);
      if (dt == -1) {
        esp3d_log_e("mktime failed");
      }
    } else {
      esp3d_log_e("stat file failed");
    }
  } else {
    esp3d_log_e("check file for stat failed");
  }
#endif  // SD_TIMESTAMP_FEATURE
  return dt;
}

uint8_t ESP_SD::getState(bool refresh) {
#if defined(ESP_SD_DETECT_PIN) && ESP_SD_DETECT_PIN != -1
  // no need to go further if SD detect is not correct
  if (!((digitalRead(ESP_SD_DETECT_PIN) == ESP_SD_DETECT_VALUE) ? true
                                                                : false)) {
    _state = ESP_SDCARD_NOT_PRESENT;
    return _state;
  }
#endif  // ESP_SD_DETECT_PIN
  // if busy doing something return state
  if (!((_state == ESP_SDCARD_NOT_PRESENT) || _state == ESP_SDCARD_IDLE)) {
    return _state;
  }
  if (!refresh) {
    return _state;  // to avoid refresh=true + busy to reset SD and waste time
  } else {
    _sizechanged = true;
  }
  // SD is idle or not detected, let see if still the case
  _state = ESP_SDCARD_NOT_PRESENT;
  esp3d_log("Spi : CS: %d,  Miso: %d, Mosi: %d, SCK: %d",
            ESP_SD_CS_PIN != -1 ? ESP_SD_CS_PIN : SS,
            ESP_SD_MISO_PIN != -1 ? ESP_SD_MISO_PIN : MISO,
            ESP_SD_MOSI_PIN != -1 ? ESP_SD_MOSI_PIN : MOSI,
            ESP_SD_SCK_PIN != -1 ? ESP_SD_SCK_PIN : SCK);
  // refresh content if card was removed
  if (SD.begin((ESP_SD_CS_PIN == -1) ? SS : ESP_SD_CS_PIN,
               SD_SCK_MHZ(FREQMZ / _spi_speed_divider))) {
      _state = ESP_SDCARD_IDLE;
  }

  return _state;
}

bool ESP_SD::begin() {
  pinMode(ESP_SD_CS_PIN, OUTPUT);
  _started = true;
  _state = ESP_SDCARD_NOT_PRESENT;
  _spi_speed_divider = ESP3DSettings::readByte(ESP_SD_SPEED_DIV);
  // sanity check
  if (_spi_speed_divider <= 0) {
    _spi_speed_divider = 1;
  }
#ifdef SD_TIMESTAMP_FEATURE
  // set callback to get time on files on SD
  SdFile::dateTimeCallback(dateTime);
#endif  // SD_TIMESTAMP_FEATURE
// Setup pins
#if defined(ESP_SD_DETECT_PIN) && ESP_SD_DETECT_PIN != -1
  pinMode(ESP_SD_DETECT_PIN, INPUT);
#endif  // ESP_SD_DETECT_PIN
#if SD_DEVICE_CONNECTION == ESP_SHARED_SD
#if defined(ESP_FLAG_SHARED_SD_PIN) && ESP_FLAG_SHARED_SD_PIN != -1
  pinMode(ESP_FLAG_SHARED_SD_PIN, OUTPUT);
  digitalWrite(ESP_FLAG_SHARED_SD_PIN, !ESP_FLAG_SHARED_SD_VALUE);
#endif  // ESP_FLAG_SHARED_SD_PIN
#endif  // SD_DEVICE_CONNECTION  == ESP_SHARED_SD
#if (ESP_SD_CS_PIN != -1) || (ESP_SD_MISO_PIN != -1) || \
    (ESP_SD_MOSI_PIN != -1) || (ESP_SD_SCK_PIN != -1)
  SPI.begin(ESP_SD_SCK_PIN, ESP_SD_MISO_PIN, ESP_SD_MOSI_PIN, ESP_SD_CS_PIN);
#endif  // SPI

  return _started;
}

void ESP_SD::end() {
  _state = ESP_SDCARD_NOT_PRESENT;
  _started = false;
}

void ESP_SD::refreshStats(bool force) {
  if (force || _sizechanged) {
    usedBytes(true);
  }
  _sizechanged = false;
}

uint64_t ESP_SD::totalBytes(bool refresh) {
  static uint64_t _totalBytes = 0;
  if (!SD.volumeBegin()) {
    return 0;
  }
  if (refresh || _totalBytes == 0) {
    _totalBytes = SD.clusterCount();
    uint8_t sectors = SD.sectorsPerCluster();
    _totalBytes = _totalBytes * sectors * 512;
  }
  return _totalBytes;
}

uint64_t ESP_SD::usedBytes(bool refresh) {
  return totalBytes(refresh) - freeBytes(refresh);
}

uint64_t ESP_SD::freeBytes(bool refresh) {
  static uint64_t _freeBytes = 0;
  if (!SD.volumeBegin()) {
    return 0;
  }
  if (refresh || _freeBytes == 0) {
    _freeBytes = SD.freeClusterCount();
    uint8_t sectors = SD.sectorsPerCluster();
    _freeBytes = _freeBytes * sectors * 512;
  }
  return _freeBytes;
}

uint ESP_SD::maxPathLength() { return 255; }

bool ESP_SD::rename(const char* oldpath, const char* newpath) {
  return SD.rename(oldpath, newpath);
}

bool ESP_SD::format() {
  uint32_t const ERASE_SIZE = 262144L;
  uint32_t cardSectorCount = 0;
  uint8_t sectorBuffer[512];
  // SdCardFactory constructs and initializes the appropriate card.
  SdCardFactory cardFactory;
  // Pointer to generic SD card.
  SdCard* m_card = nullptr;
  if (ESP_SD::getState(true) == ESP_SDCARD_IDLE) {
    // prepare
    m_card = cardFactory.newCard(SD_CONFIG);
    if (!m_card || m_card->errorCode()) {
      esp3d_log_e("card init failed.");
      return false;
    }

    cardSectorCount = m_card->sectorCount();
    if (!cardSectorCount) {
      esp3d_log_e("Get sector count failed.");
      return false;
    }

    esp3d_log("Capacity detected :%d GB", cardSectorCount * 5.12e-7);
  } else {
    esp3d_log_e("SD not ready");
    return false;
  }

  uint32_t firstBlock = 0;
  uint32_t lastBlock;
  uint16_t n = 0;
  do {
    lastBlock = firstBlock + ERASE_SIZE - 1;
    if (lastBlock >= cardSectorCount) {
      lastBlock = cardSectorCount - 1;
    }
    if (!m_card->erase(firstBlock, lastBlock)) {
      esp3d_log_e("erase failed");
      return false;
    }

    firstBlock += ERASE_SIZE;
    if ((n++) % 64 == 63) {
      ESP3DHal::wait(0);
    }
  } while (firstBlock < cardSectorCount);

  if (!m_card->readSector(0, sectorBuffer)) {
    esp3d_log("readBlock");
  }

  ExFatFormatter exFatFormatter;
  FatFormatter fatFormatter;

  // Format exFAT if larger than 32GB.
  bool rtn = cardSectorCount > 67108864
                 ? exFatFormatter.format(m_card, sectorBuffer, nullptr)
                 : fatFormatter.format(m_card, sectorBuffer, nullptr);

  if (!rtn) {
    esp3d_log_e("erase failed");
    return false;
  }

  return true;
}

ESP_SDFile ESP_SD::open(const char* path, uint8_t mode) {
  esp3d_log("open %s, %d", path, mode);
  // do some check
  if (((strcmp(path, "/") == 0) &&
       ((mode == ESP_FILE_WRITE) || (mode == ESP_FILE_APPEND))) ||
      (strlen(path) == 0)) {
    _sizechanged = true;
    esp3d_log_e("reject  %s", path);
    return ESP_SDFile();
  }
  // path must start by '/'
  if (path[0] != '/') {
    esp3d_log_e("%s is invalid path", path);
    return ESP_SDFile();
  }
  if (mode != ESP_FILE_READ) {
    // check container exists
    String p = path;
    p.remove(p.lastIndexOf('/') + 1);
    if (!exists(p.c_str())) {
      esp3d_log_e("Error opening: %s", path);
      return ESP_SDFile();
    }
  }
  File tmp = SD.open(path, (mode == ESP_FILE_READ)    ? FILE_READ
                           : (mode == ESP_FILE_WRITE) ? FILE_WRITE
                                                      : FILE_WRITE);
  if (tmp) {
    ESP_SDFile esptmp(&tmp, strcmp(path, "/") == 0 ? true : tmp.isDir(),
                      (mode == ESP_FILE_READ) ? false : true, path);
    esp3d_log("%s is a %s", path, tmp.isDir() ? "Dir" : "File");
    return esptmp;
  } else {
    esp3d_log_e("open %s failed", path);
    return ESP_SDFile();
  }
}

bool ESP_SD::exists(const char* path) {
  bool res = false;
  // root should always be there if started
  if (strcmp(path, "/") == 0) {
    return _started;
  }
  res = SD.exists(path);
  if (!res) {
    ESP_SDFile root = ESP_SD::open(path, ESP_FILE_READ);
    if (root) {
      res = root.isDirectory();
    }
  }
  return res;
}

bool ESP_SD::remove(const char* path) {
  _sizechanged = true;
  return SD.remove(path);
}

bool ESP_SD::mkdir(const char* path) { return SD.mkdir(path); }

bool ESP_SD::rmdir(const char* path) {
  String p = path;
  if (!p.endsWith("/")) {
    p += '/';
  }
  if (!p.startsWith("/")) {
    p = '/' + p;
  }
  if (!exists(p.c_str())) {
    return false;
  }
  bool res = true;
  std::stack<String> pathlist;
  pathlist.push(p);
  while (pathlist.size() > 0 && res) {
    File dir = SD.open(pathlist.top().c_str());
    dir.rewindDirectory();
    File f = dir.openNextFile();
    bool candelete = true;
    while (f && res) {
      if (f.isDir()) {
        candelete = false;
        String newdir;
        char tmp[255];
        f.getName(tmp, 254);
        newdir = pathlist.top() + tmp;
        newdir += "/";
        pathlist.push(newdir);
        f.close();
        f = File();
      } else {
        char tmp[255];
        f.getName(tmp, 254);
        _sizechanged = true;
        String filepath = pathlist.top() + tmp;
        f.close();
        if (!SD.remove(filepath.c_str())) {
          res = false;
        }
        f = dir.openNextFile();
      }
    }
    if (candelete) {
      if (pathlist.top() != "/") {
        res = SD.rmdir(pathlist.top().c_str());
      }
      pathlist.pop();
    }
    dir.close();
  }
  p = String();
  esp3d_log("count %d has error %d\n", pathlist.size(), res);
  return res;
}

void ESP_SD::closeAll() {
  for (uint8_t i = 0; i < ESP_MAX_SD_OPENHANDLE; i++) {
    tSDFile_handle[i].close();
    tSDFile_handle[i] = File();
  }
}

bool ESP_SDFile::seek(uint32_t pos, uint8_t mode) {
  if (mode == ESP_SEEK_END) {
    return tSDFile_handle[_index].seek(-pos);  // based on SDFS comment
  }
  return tSDFile_handle[_index].seek(pos);
}

ESP_SDFile::ESP_SDFile(void* handle, bool isdir, bool iswritemode,
                       const char* path) {
  _isdir = isdir;
  _dirlist = "";
  _index = -1;
  _filename = "";
  _name = "";
  _lastwrite = 0;
  _iswritemode = iswritemode;
  _size = 0;
  if (!handle) {
    return;
  }
  bool set = false;
  for (uint8_t i = 0; (i < ESP_MAX_SD_OPENHANDLE) && !set; i++) {
    if (!tSDFile_handle[i]) {
      tSDFile_handle[i] = *((File*)handle);
      // filename
      char tmp[255];
      tSDFile_handle[i].getName(tmp, 254);
      _filename = path;
      // name
      _name = tmp;
      if (_name.endsWith("/")) {
        _name.remove(_name.length() - 1, 1);
        _isdir = true;
      }
      if (_name[0] == '/') {
        _name.remove(0, 1);
      }
      int pos = _name.lastIndexOf('/');
      if (pos != -1) {
        _name.remove(0, pos + 1);
      }
      if (_name.length() == 0) {
        _name = "/";
      }
      // size
      _size = tSDFile_handle[i].size();
      // time
      if (!_isdir && !iswritemode) {
        _lastwrite = getDateTimeFile(tSDFile_handle[i]);

      } else {
        // no need date time for directory
        _lastwrite = 0;
      }
      _index = i;
      // esp3d_log("Opening File at index %d",_index);
      set = true;
    }
  }
}
// todo need also to add short filename
const char* ESP_SDFile::shortname() const {
#if SDFAT_FILE_TYPE != 1
  return _name.c_str();
#else
  static char sname[13];
  File ftmp = SD.open(_filename.c_str());
  if (ftmp) {
    ftmp.getSFN(sname, 12);
    ftmp.close();
    if (strlen(sname) == 0) {
      return _name.c_str();
    }
    return sname;
  } else {
    return _name.c_str();
  }
#endif
}

void ESP_SDFile::close() {
  if (_index != -1) {
    // esp3d_log("Closing File at index %d", _index);
    tSDFile_handle[_index].close();
    // reopen if mode = write
    // udate size + date
    if (_iswritemode && !_isdir) {
      File ftmp = SD.open(_filename.c_str());
      if (ftmp) {
        _size = ftmp.size();
        _lastwrite = getDateTimeFile(ftmp);
        ftmp.close();
      }
    }
    tSDFile_handle[_index] = File();
    // esp3d_log("Closing File at index %d",_index);
    _index = -1;
  }
}

ESP_SDFile ESP_SDFile::openNextFile() {
  if ((_index == -1) || !_isdir) {
    esp3d_log("openNextFile failed");
    return ESP_SDFile();
  }
  File tmp = tSDFile_handle[_index].openNextFile();
  if (tmp) {
    char tmps[255];
    tmp.getName(tmps, 254);
    esp3d_log("tmp name :%s %s", tmps, (tmp.isDir()) ? "isDir" : "isFile");
    String s = _filename;
    if (s != "/") {
      s += "/";
    }
    s += tmps;
    ESP_SDFile esptmp(&tmp, tmp.isDir(), false, s.c_str());
    esptmp.close();
    return esptmp;
  }
  return ESP_SDFile();
}

const char* ESP_SD::FilesystemName() { return "SDFat - " SD_FAT_VERSION_STR; }

#endif  // SD_DEVICE == ESP_SDFAT2
#endif  // ARCH_ESP32 && SD_DEVICE
