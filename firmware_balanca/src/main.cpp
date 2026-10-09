#include <Arduino.h>

#include "app/ScaleApplication.h"

namespace {

balanca::app::ScaleApplication application;

}  // namespace

void setup() { application.begin(); }

void loop() { application.update(); }
