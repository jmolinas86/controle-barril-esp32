#pragma once

#include <cstdarg>
#include <cstdint>

namespace keezer::diagnostics {

enum class LogLevel : std::uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
};

class Logger final {
 public:
  static void begin(std::uint32_t baudRate);
  static void setLevel(LogLevel level);
  static LogLevel level();
  static void log(LogLevel level, const char* tag, const char* format, ...)
      __attribute__((format(printf, 3, 4)));

 private:
  static const char* levelName(LogLevel level);
  static LogLevel currentLevel_;
};

}  // namespace keezer::diagnostics

#define KEEZER_LOG_ERROR(tag, format, ...)                                  \
  ::keezer::diagnostics::Logger::log(                                       \
      ::keezer::diagnostics::LogLevel::Error, tag, format, ##__VA_ARGS__)
#define KEEZER_LOG_WARN(tag, format, ...)                                   \
  ::keezer::diagnostics::Logger::log(                                       \
      ::keezer::diagnostics::LogLevel::Warn, tag, format, ##__VA_ARGS__)
#define KEEZER_LOG_INFO(tag, format, ...)                                   \
  ::keezer::diagnostics::Logger::log(                                       \
      ::keezer::diagnostics::LogLevel::Info, tag, format, ##__VA_ARGS__)
#define KEEZER_LOG_DEBUG(tag, format, ...)                                  \
  ::keezer::diagnostics::Logger::log(                                       \
      ::keezer::diagnostics::LogLevel::Debug, tag, format, ##__VA_ARGS__)

