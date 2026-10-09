#include <Arduino.h>

#include "app/AppController.h"

namespace {

keezer::app::AppController app;

}  // namespace

void setup() { app.begin(); }

void loop() { app.update(); }

