#pragma once

#include <cstddef>
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
  static std::size_t bufferedLineCount();
  static std::size_t copyBufferedLine(std::size_t index, char* destination,
                                      std::size_t capacity);
  static std::uint32_t bufferRevision();
  static void clearBuffer();

 private:
  static constexpr std::size_t kBufferedLineCount = 64U;
  static constexpr std::size_t kBufferedLineBytes = 192U;

  static const char* levelName(LogLevel level);
  static void appendToBuffer(const char* line);

  static LogLevel currentLevel_;
  static char* bufferedLines_;
  static std::size_t firstBufferedLine_;
  static std::size_t bufferedLineCount_;
  static std::uint32_t bufferRevision_;
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

