#pragma once
#include <Arduino.h>
#include <SdFat.h>

struct AppConfig;

class SdCardManager {
public:
  static constexpr const char* kEndpointConfigPath = "/endpoints.csv";
  static constexpr const char* kAnimationFilePath = "/animation.csv";

  bool begin();
  bool isReady() const { return _ready; }
  bool listDir(const char *path, Stream &out);
  bool readFile(const char *path, Stream &out);
  bool testReadWrite(Stream &out);
  bool loadEndpointConfig(AppConfig &cfg, Stream &out);
  bool saveEndpointConfig(const AppConfig &cfg, Stream &out);
  bool loadAnimationConfig(const char *path, AppConfig &cfg, Stream &out);
  bool saveAnimationConfig(const char *path, const AppConfig &cfg, Stream &out);
  bool saveDefaultAnimation(const char *path, Stream &out);
  bool updateAnimationConfig(const char *path, const AppConfig &cfg, Stream &out);
  bool openFile(const char *path, FsFile &file);
  bool openFileWrite(const char *path, FsFile &file);

private:
  SdFs _sd;
  bool _ready = false;

  bool readLine(FsFile &file, char *buf, size_t len);
  bool parseInt(const char *token, int32_t &value);
  bool parseUint(const char *token, uint32_t &value);
  void stripInlineComment(char *line) const;
  void writeEndpointsSection(FsFile &file, const AppConfig &cfg);
};
