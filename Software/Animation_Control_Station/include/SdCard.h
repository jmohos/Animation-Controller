#pragma once
#include <Arduino.h>
#include <SdFat.h>

struct AppConfig;

class SdCardManager {
public:
  static constexpr const char* kEndpointConfigPath = "/endpoints.csv";
  static constexpr const char* kAnimationFilePath = "/animation.csv";

  /**
   * Description: Initialize the SD card interface.
   * Inputs: None.
   * Outputs: Returns true when the SD card is ready.
   */
  bool begin();

  /**
   * Description: Check SD card readiness.
   * Inputs: None.
   * Outputs: Returns true when initialized successfully.
   */
  bool isReady() const { return _ready; }

  /**
   * Description: List files in a directory.
   * Inputs:
   * - path: directory path to list ("/" for root).
   * - out: output stream to write listing.
   * Outputs: Returns true on success.
   */
  bool listDir(const char *path, Stream &out);

  /**
   * Description: Dump the contents of a file to a stream.
   * Inputs:
   * - path: file path to read.
   * - out: output stream for file contents.
   * Outputs: Returns true on success.
   */
  bool readFile(const char *path, Stream &out);

  /**
   * Description: Run a simple read/write diagnostic test.
   * Inputs:
   * - out: output stream for status.
   * Outputs: Returns true on success.
   */
  bool testReadWrite(Stream &out);

  /**
   * Description: Load endpoint configuration from SD card.
   * Inputs:
   * - cfg: configuration structure to update.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool loadEndpointConfig(AppConfig &cfg, Stream &out);

  /**
   * Description: Save endpoint configuration to SD card.
   * Inputs:
   * - cfg: configuration structure to persist.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool saveEndpointConfig(const AppConfig &cfg, Stream &out);

  /**
   * Description: Legacy endpoint loader (animation files now only hold sequences).
   * Inputs:
   * - path: animation file path.
   * - cfg: configuration structure to update.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool loadAnimationConfig(const char *path, AppConfig &cfg, Stream &out);

  /**
   * Description: Create a sequence-only animation file with a [sequence] section.
   * Inputs:
   * - path: animation file path.
   * - cfg: configuration structure to persist.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool saveAnimationConfig(const char *path, const AppConfig &cfg, Stream &out);

  /**
   * Description: Write the built-in default animation sequence.
   * Inputs:
   * - path: animation file path.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool saveDefaultAnimation(const char *path, Stream &out);

  /**
   * Description: Ensure an animation file exists (sequence-only).
   * Inputs:
   * - path: animation file path.
   * - cfg: configuration structure to persist.
   * - out: output stream for status messages.
   * Outputs: Returns true on success.
   */
  bool updateAnimationConfig(const char *path, const AppConfig &cfg, Stream &out);

  /**
   * Description: Open a file for read access.
   * Inputs:
   * - path: file path to open.
   * - file: output file handle.
   * Outputs: Returns true on success.
   */
  bool openFile(const char *path, FsFile &file);

  /**
   * Description: Open a file for write access (truncate/create).
   * Inputs:
   * - path: file path to open.
   * - file: output file handle.
   * Outputs: Returns true on success.
   */
  bool openFileWrite(const char *path, FsFile &file);

private:
  SdFs _sd;
  bool _ready = false;

  bool readLine(FsFile &file, char *buf, size_t len);
  bool parseInt(const char *token, int32_t &value);
  bool parseUint(const char *token, uint32_t &value);
  bool isSectionLine(const char *line, const char *section) const;
  void stripInlineComment(char *line) const;
  void writeEndpointsSection(FsFile &file, const AppConfig &cfg);
};
