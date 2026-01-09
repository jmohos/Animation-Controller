#pragma once

#include <Arduino.h>

typedef enum {
  FAULT_CONSOLE_TASK_FAULT = 0,
  FAULT_CMD_EXEC_TASK_FAULT = 1,
  FAULT_SHOW_TASK_FAULT = 2,
  FAULT_CONFIG_RESTORE_FAULT = 3,
  FAULT_IO_EXPANDER_FAULT = 4,
  FAULT_LCD_DISPLAY_FAULT = 5,
  FAULT_MAX_INDEX = 6
} SYSTEM_FAULT_T;

// Forward reference to master system fault bits in master .ini file.
extern uint32_t system_faults;
extern const char *FAULT_STRING[];

// Fault macros.  Replace x with the fault enumeration name.
#define FAULT_SET(x) system_faults = (system_faults | ((uint32_t)1 << x))
#define FAULT_CLEAR(x) system_faults = (system_faults &= ~((uint32_t)1 << x))
#define FAULT_ACTIVE(x) ((system_faults & (uint32_t)1 << x) != 0)

void print_faults();