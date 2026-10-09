#include "diagnostics/Logger.h"

#include <Arduino.h>

#include <array>
#include <cstdio>

namespace keezer::diagnostics {

LogLevel Logger::currentLevel_ = LogLevel::Info;

void Logger::begin(const std::uint32_t baudRate) {
  Serial.begin(baudRate);
}

void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }

LogLevel Logger::level() { return currentLevel_; }

void Logger::log(const LogLevel level, const char* const tag,
                 const char* const format, ...) {
  if (static_cast<std::uint8_t>(level) >
      static_cast<std::uint8_t>(currentLevel_)) {
    return;
  }

  Serial.printf("[%10lu] %-5s %-10s ",
                static_cast<unsigned long>(millis()), levelName(level), tag);

  va_list arguments;
  va_start(arguments, format);
  std::array<char, 256U> message{};
  std::vsnprintf(message.data(), message.size(), format, arguments);
  va_end(arguments);
  Serial.print(message.data());
  Serial.println();
}

const char* Logger::levelName(const LogLevel level) {
  switch (level) {
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Debug:
      return "DEBUG";
  }
  return "?";
}

}  // namespace keezer::diagnostics
