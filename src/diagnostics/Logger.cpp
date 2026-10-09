#include "diagnostics/Logger.h"

#include <Arduino.h>

#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace keezer::diagnostics {

LogLevel Logger::currentLevel_ = LogLevel::Info;
char* Logger::bufferedLines_ = nullptr;
std::size_t Logger::firstBufferedLine_ = 0U;
std::size_t Logger::bufferedLineCount_ = 0U;
std::uint32_t Logger::bufferRevision_ = 0U;

void Logger::begin(const std::uint32_t baudRate) {
  Serial.begin(baudRate);
  if (bufferedLines_ == nullptr) {
    bufferedLines_ = static_cast<char*>(
        std::calloc(kBufferedLineCount, kBufferedLineBytes));
    if (bufferedLines_ == nullptr) {
      Serial.println("[LOGGER] Web log buffer allocation failed");
    }
  }
}

void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }

LogLevel Logger::level() { return currentLevel_; }

void Logger::log(const LogLevel level, const char* const tag,
                 const char* const format, ...) {
  if (static_cast<std::uint8_t>(level) >
      static_cast<std::uint8_t>(currentLevel_)) {
    return;
  }

  va_list arguments;
  va_start(arguments, format);
  std::array<char, 256U> message{};
  std::vsnprintf(message.data(), message.size(), format, arguments);
  va_end(arguments);

  std::array<char, 320U> line{};
  std::snprintf(line.data(), line.size(), "[%10lu] %-5s %-10s %s",
                static_cast<unsigned long>(millis()), levelName(level), tag,
                message.data());
  Serial.println(line.data());
  appendToBuffer(line.data());
}

std::size_t Logger::bufferedLineCount() { return bufferedLineCount_; }

std::size_t Logger::copyBufferedLine(const std::size_t index,
                                     char* const destination,
                                     const std::size_t capacity) {
  if (destination == nullptr || capacity == 0U ||
      index >= bufferedLineCount_ || bufferedLines_ == nullptr) {
    return 0U;
  }
  const std::size_t sourceIndex =
      (firstBufferedLine_ + index) % kBufferedLineCount;
  const char* const source =
      bufferedLines_ + (sourceIndex * kBufferedLineBytes);
  const std::size_t length = std::min(std::strlen(source), capacity - 1U);
  std::memcpy(destination, source, length);
  destination[length] = '\0';
  return length;
}

std::uint32_t Logger::bufferRevision() { return bufferRevision_; }

void Logger::clearBuffer() {
  firstBufferedLine_ = 0U;
  bufferedLineCount_ = 0U;
  ++bufferRevision_;
}

void Logger::appendToBuffer(const char* const line) {
  if (line == nullptr || bufferedLines_ == nullptr) return;
  std::size_t targetIndex = 0U;
  if (bufferedLineCount_ < kBufferedLineCount) {
    targetIndex =
        (firstBufferedLine_ + bufferedLineCount_) % kBufferedLineCount;
    ++bufferedLineCount_;
  } else {
    targetIndex = firstBufferedLine_;
    firstBufferedLine_ = (firstBufferedLine_ + 1U) % kBufferedLineCount;
  }
  char* const destination =
      bufferedLines_ + (targetIndex * kBufferedLineBytes);
  std::snprintf(destination, kBufferedLineBytes, "%s", line);
  ++bufferRevision_;
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
