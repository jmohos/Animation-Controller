#include "MenuActions.h"
#include "App.h"

void MenuActionSaveConfig(App &app) {
  app.actionSaveConfig();
}

void MenuActionResetConfig(App &app) {
  app.actionResetConfig();
}

void MenuActionSdTest(App &app) {
  app.actionSdTest();
}

void MenuActionReboot(App &app) {
  app.actionReboot();
}
