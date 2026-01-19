#pragma once
#include <Arduino.h>
#include "Ui.h"
#include "MenuActions.h"

using MenuCallback = void (*)(App &app);

enum class MenuAction : uint8_t {
  OpenEndpoints,
  OpenEndpointConfig,
  OpenEdit,
  OpenSettings,
  OpenDiagnostics,
  OpenRoboClawStatus,
  SaveConfig,
  ResetConfig,
  SdTest,
  Reboot
};

struct MenuItem {
  const char *label;
  MenuAction action;
  bool opensScreen;
  UiScreen targetScreen;
  MenuCallback callback;
};

template <size_t N>
constexpr uint8_t menuCount(const MenuItem (&)[N]) {
  return static_cast<uint8_t>(N);
}

constexpr MenuItem ScreenItem(const char *label, MenuAction action, UiScreen screen) {
  return MenuItem{label, action, true, screen, nullptr};
}

constexpr MenuItem ActionItem(const char *label, MenuAction action, MenuCallback callback) {
  return MenuItem{label, action, false, UiScreen::Menu, callback};
}

static constexpr MenuItem MENU_ITEMS[] = {
  ScreenItem("Endpoints", MenuAction::OpenEndpoints, UiScreen::Endpoints),
  ScreenItem("Endpoint Config", MenuAction::OpenEndpointConfig, UiScreen::EndpointConfig),
  ActionItem("Edit Sequence", MenuAction::OpenEdit, MenuActionEditSequence),
  ScreenItem("Settings", MenuAction::OpenSettings, UiScreen::Settings),
  ScreenItem("Diagnostics", MenuAction::OpenDiagnostics, UiScreen::Diagnostics)
};

static constexpr MenuItem SETTINGS_ITEMS[] = {
  ActionItem("Save Config", MenuAction::SaveConfig, MenuActionSaveConfig),
  ActionItem("Reset Config", MenuAction::ResetConfig, MenuActionResetConfig)
};

static constexpr MenuItem DIAGNOSTICS_ITEMS[] = {
  ScreenItem("RoboClaw Status", MenuAction::OpenRoboClawStatus, UiScreen::RoboClawStatus),
  ActionItem("SD Test", MenuAction::SdTest, MenuActionSdTest),
  ActionItem("Reboot", MenuAction::Reboot, MenuActionReboot)
};

static constexpr uint8_t MENU_ITEM_COUNT = menuCount(MENU_ITEMS);
static constexpr uint8_t SETTINGS_ITEM_COUNT = menuCount(SETTINGS_ITEMS);
static constexpr uint8_t DIAGNOSTICS_ITEM_COUNT = menuCount(DIAGNOSTICS_ITEMS);
