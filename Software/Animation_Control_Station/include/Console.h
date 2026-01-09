#pragma once

#include <Arduino.h>

// ==== Tunables (raise if you need more) ====
#ifndef CON_MAX_ARGS
#define CON_MAX_ARGS 8 // max number of arguments after the command
#endif
#ifndef CON_MAX_TOK
#define CON_MAX_TOK 32 // max length of each token (bytes, incl. '\0')
#endif
#ifndef CON_MAX_CMD
#define CON_MAX_CMD 32 // max length of the command token
#endif
#ifndef CON_MAX_LINE
#define CON_MAX_LINE 128 // max length of an input line (bytes, incl. '\0')
#endif

// One parsed console message: command + argv[], all as plain C strings.
struct CommandMsg
{
  char cmd[CON_MAX_CMD]; // "trigger", "mode", ...
  uint8_t argc = 0;
  char argv[CON_MAX_ARGS][CON_MAX_TOK]; // argument tokens as-is
};


class Console {
public:
  using DispatchCommandFn = void (*)(const CommandMsg &msg);

  /**
   * Description: Initialize console input handling.
   * Inputs: None.
   * Outputs: Prepares console state for polling.
   */
  void begin();

  /**
   * Description: Poll the console serial port and dispatch commands.
   * Inputs: None.
   * Outputs: Parses complete lines and invokes the command handler.
   */
  void poll();

  /**
   * Description: Register the command handler used for dispatch.
   * Inputs:
   * - handler: function pointer for command dispatch (nullptr to clear).
   * Outputs: Stores the handler for future dispatch.
   */
  void setDispatchCommand(DispatchCommandFn handler);

private:
  /**
   * Description: Check whether a character is whitespace.
   * Inputs:
   * - c: character to test.
   * Outputs: Returns true if the character is space or tab.
   */
  bool isWhitespace(char c) const;

  /**
   * Description: Tokenize a line into a command message.
   * Inputs:
   * - line: input line to parse (null-terminated).
   * - out: parsed output structure.
   * Outputs: Fills the CommandMsg with cmd and argv tokens.
   */
  void tokenizeLine(const char *line, CommandMsg &out) const;

  /**
   * Description: Dispatch a parsed command message if a handler is registered.
   * Inputs:
   * - msg: parsed command message.
   * Outputs: Calls the registered handler when available.
   */
  void dispatchCommand(const CommandMsg &msg) const;

  DispatchCommandFn _dispatchCommand = nullptr;
  char _lineBuffer[CON_MAX_LINE] = {};
  size_t _lineLength = 0;
};

