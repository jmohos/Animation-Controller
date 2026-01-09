#include "Console.h"
#include <cstring>

/**
 * Description: Initialize the console serial input capture.
 * Inputs: None.
 * Outputs: None.
 */
void Console::begin() {
  _lineLength = 0;
  _lineBuffer[0] = '\0';
}

/**
 * Description: Register the command handler used for dispatch.
 * Inputs:
 * - handler: function pointer for command dispatch (nullptr to clear).
 * Outputs: Stores the handler for future dispatch.
 */
void Console::setDispatchCommand(DispatchCommandFn handler) {
  _dispatchCommand = handler;
}

/**
 * Description: Poll the console serial inputs and process commands.
 * Inputs: None.
 * Outputs: Parses complete lines and dispatches commands.
 */
void Console::poll() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      if (_lineLength > 0) {
        _lineBuffer[_lineLength] = '\0';
        CommandMsg message;
        tokenizeLine(_lineBuffer, message);
        if (message.cmd[0] != '\0') {
          dispatchCommand(message);
        }
      }
      _lineLength = 0;
      _lineBuffer[0] = '\0';
    } else {
      // simple backspace handling
      if ((ch == 0x08 || ch == 0x7F)) {
        if (_lineLength > 0) {
          _lineLength--;
          _lineBuffer[_lineLength] = '\0';
        }
      } else if (isPrintable(ch)) {
        if (_lineLength < (CON_MAX_LINE - 1)) {
          _lineBuffer[_lineLength++] = ch;
          _lineBuffer[_lineLength] = '\0';
        }
      }
    }
  }
}

/**
 * Description: Check whether a character is whitespace.
 * Inputs:
 * - c: character to test.
 * Outputs: Returns true if the character is space or tab.
 */
bool Console::isWhitespace(char c) const {
  return c == ' ' || c == '\t';
}

/**
 * Description: Tokenize a line into a command message.
 * Inputs:
 * - line: input line to parse.
 * - out: parsed output structure.
 * Outputs: Fills the CommandMsg with cmd and argv tokens.
 */
void Console::tokenizeLine(const char *line, CommandMsg &out) const {
  out.cmd[0] = '\0';
  out.argc = 0;

  // Walk the string and extract tokens separated by spaces/tabs
  bool isFirst = true;
  const char *cursor = line;
  while (*cursor != '\0') {
    while (*cursor != '\0' && isWhitespace(*cursor)) {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }

    const char *start = cursor;
    size_t tokenLength = 0;
    while (*cursor != '\0' && !isWhitespace(*cursor)) {
      cursor++;
      tokenLength++;
    }

    if (tokenLength == 0) {
      continue;
    }

    if (isFirst) {
      const size_t copyLength = (tokenLength < (CON_MAX_CMD - 1)) ? tokenLength : (CON_MAX_CMD - 1);
      memcpy(out.cmd, start, copyLength);
      out.cmd[copyLength] = '\0';
      isFirst = false;
    } else if (out.argc < CON_MAX_ARGS) {
      const size_t copyLength = (tokenLength < (CON_MAX_TOK - 1)) ? tokenLength : (CON_MAX_TOK - 1);
      memcpy(out.argv[out.argc], start, copyLength);
      out.argv[out.argc][copyLength] = '\0';
      out.argc++;
    }
  }
}

/**
 * Description: Dispatch a parsed command message if a handler is registered.
 * Inputs:
 * - msg: parsed command message.
 * Outputs: Calls the registered handler when available.
 */
void Console::dispatchCommand(const CommandMsg &msg) const {
  if (_dispatchCommand) {
    _dispatchCommand(msg);
  }
}
