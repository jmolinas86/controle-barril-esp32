#pragma once

#include <cstddef>
#include <cstdint>

#ifndef KEEZER_ENABLE_COMPRESSOR_GPIO
#define KEEZER_ENABLE_COMPRESSOR_GPIO 0
#endif

#ifndef KEEZER_PHASE1_SD_SMOKE_TEST
#define KEEZER_PHASE1_SD_SMOKE_TEST 0
#endif

#ifndef KEEZER_FORCE_TOUCH_CALIBRATION
#define KEEZER_FORCE_TOUCH_CALIBRATION 0
#endif

#ifndef KEEZER_DEMO_MODE
#define KEEZER_DEMO_MODE 1
#endif

#ifndef KEEZER_DEMO_AUTOPLAY
#define KEEZER_DEMO_AUTOPLAY 1
#endif

#ifndef KEEZER_SCALE_USE_HTTP
#define KEEZER_SCALE_USE_HTTP 0
#endif

namespace keezer::config {

inline constexpr std::uint32_t kSerialBaud = 115200U;
inline constexpr std::uint16_t kDisplayWidth = 240U;
inline constexpr std::uint16_t kDisplayHeight = 320U;
inline constexpr std::uint16_t kDisplayRenderLines = 24U;
inline constexpr std::size_t kDisplayBufferPixels =
    static_cast<std::size_t>(kDisplayWidth) * kDisplayRenderLines;
inline constexpr std::uint8_t kDisplayBrightness = 180U;
inline constexpr std::uint32_t kDisplayWriteClockHz = 80'000'000U;
inline constexpr bool kDisplayScanlineSyncEnabled = true;
inline constexpr std::uint16_t kDisplayScanlineGuardLines = 10U;
inline constexpr std::uint16_t kDisplayScanlineWindowLines = 32U;
inline constexpr std::uint32_t kDisplayScanlineSyncTimeoutUs = 20'000U;

inline constexpr std::uint32_t kUiTelemetryPeriodMs = 30'000U;
inline constexpr std::uint32_t kUiDataRefreshPeriodMs = 250U;
inline constexpr std::uint32_t kLvglMaximumHandlerIntervalMs = 5U;
inline constexpr std::uint8_t kTouchScrollLimitPixels = 5U;
inline constexpr std::uint8_t kTouchScrollThrowPercent = 8U;
inline constexpr std::uint32_t kPerformanceTelemetryPeriodMs = 30'000U;
inline constexpr std::uint32_t kSlowLoopThresholdUs = 10'000U;
inline constexpr std::uint32_t kSdClockHz = 4'000'000U;
inline constexpr std::uint64_t kSdMinimumFreeBytes = 4U * 1024U;
inline constexpr std::uint32_t kScaleOnlineTimeoutMs = 30'000U;
inline constexpr std::uint32_t kScaleOfflineTimeoutMs = 300'000U;
inline constexpr std::uint32_t kScaleHttpFastPollPeriodMs = 500U;
inline constexpr std::uint32_t kScaleHttpStablePollPeriodMs = 2'000U;
inline constexpr std::uint32_t kScaleHttpIdlePollPeriodMs = 5'000U;
inline constexpr std::uint32_t kScaleHttpFastModeHoldMs = 10'000U;
inline constexpr std::int32_t kScaleHttpActivityThresholdGrams = 20;
inline constexpr std::int32_t kScaleHttpEmptyThresholdGrams = 1'000;
inline constexpr std::uint32_t kScaleHttpResponseTimeoutMs = 1'000U;
inline constexpr std::uint32_t kScaleHttpResolveRetryPeriodMs = 5'000U;
inline constexpr std::uint32_t kScaleMdnsQueryTimeoutMs = 250U;
inline constexpr std::uint32_t kScaleWifiRetryPeriodMs = 10'000U;
inline constexpr std::uint32_t kNetworkConnectionTimeoutMs = 20'000U;
inline constexpr std::uint32_t kNetworkInitialRetryMs = 5'000U;
inline constexpr std::uint32_t kNetworkMaximumRetryMs = 60'000U;
inline constexpr std::uint32_t kNetworkTelemetryRefreshMs = 2'000U;
inline constexpr std::int32_t kScaleMaximumWeightGrams = 100'000;
inline constexpr std::uint8_t kWeightFilterWindowSize = 5U;
inline constexpr std::int32_t kWeightFilterDeadbandGrams = 20;
inline constexpr std::int32_t kSignificantWeightChangeGrams = 2'000;
inline constexpr std::int16_t kTemperatureSetpointCentiCelsius = 200;
inline constexpr std::int16_t kTemperatureMinimumSetpointCentiCelsius = -500;
inline constexpr std::int16_t kTemperatureMaximumSetpointCentiCelsius = 1'500;
inline constexpr std::int16_t kTemperatureSetpointStepCentiCelsius = 10;
inline constexpr std::uint16_t kTemperatureHysteresisCentiCelsius = 100U;
inline constexpr std::uint32_t kCompressorMinimumOffTimeMs = 180'000U;
inline constexpr std::uint32_t kCompressorMinimumOnTimeMs = 60'000U;
inline constexpr std::uint32_t kTemperatureSensorTimeoutMs = 10'000U;
inline constexpr std::int16_t kTemperatureMinimumValidCentiCelsius = -2'000;
inline constexpr std::int16_t kTemperatureMaximumValidCentiCelsius = 5'000;
inline constexpr std::uint8_t kTemperatureRecoveryValidSamples = 3U;
// Accelerated timings are used only by an explicit demo build. Production
// hardware always uses the compressor protection timings above.
inline constexpr std::uint32_t kDemoCompressorMinimumOffTimeMs = 10'000U;
inline constexpr std::uint32_t kDemoCompressorMinimumOnTimeMs = 5'000U;
inline constexpr std::uint32_t kDemoTemperatureSamplePeriodMs = 1'000U;
inline constexpr std::uint32_t kDemoTemperatureCycleMs = 60'000U;
// Phase 7 short cycle lets a bench test observe ONLINE, STALE and OFFLINE
// without waiting five minutes. Real transports use the production timeouts.
inline constexpr std::uint32_t kScaleSimulatorReportPeriodMs = 1'000U;
inline constexpr std::uint32_t kScaleSimulatorSendingMs = 12'000U;
inline constexpr std::uint32_t kScaleSimulatorCycleMs = 25'000U;
inline constexpr std::uint32_t kScaleSimulatorOnlineTimeoutMs = 3'000U;
inline constexpr std::uint32_t kScaleSimulatorOfflineTimeoutMs = 7'000U;
inline constexpr std::uint8_t kNfcMinimumConsecutiveReadings = 3U;
inline constexpr std::uint32_t kNfcStabilizationMs = 1'500U;
inline constexpr std::uint32_t kNfcAmbiguityWindowMs = 1'500U;
inline constexpr std::uint32_t kManualWeighingTimeoutMs = 60'000U;
inline constexpr std::int32_t kKegRemovalWeightThresholdGrams = 1'000;
inline constexpr std::int32_t kKegRemovalSignificantDropGrams = 2'000;
inline constexpr std::uint32_t kKegRemovalStrongConfirmationMs = 5'000U;
inline constexpr std::uint32_t kKegRemovalModerateConfirmationMs = 15'000U;
inline constexpr std::int32_t kAutoTrackingMinimumConsumptionGrams = 100;
inline constexpr std::uint32_t kAutoTrackingStableConfirmationMs = 3'000U;
inline constexpr std::uint32_t kAutoTrackingMinimumRecordIntervalMs = 30'000U;
inline constexpr std::int32_t kAutoTrackingSuspiciousIncreaseGrams = 300;
inline constexpr std::int32_t kAutoTrackingSuspiciousChangeGrams = 2'000;
// KTC3 invalidates calibration after changing from landscape to portrait.
inline constexpr std::uint32_t kTouchCalibrationMagic = 0x4B544333U;

inline constexpr bool kEnableCompressorGpio =
    KEEZER_ENABLE_COMPRESSOR_GPIO != 0;
inline constexpr bool kRunSdSmokeTest = KEEZER_PHASE1_SD_SMOKE_TEST != 0;
inline constexpr bool kForceTouchCalibration =
    KEEZER_FORCE_TOUCH_CALIBRATION != 0;
inline constexpr bool kDemoMode = KEEZER_DEMO_MODE != 0;
inline constexpr bool kDemoAutoplay = KEEZER_DEMO_AUTOPLAY != 0;
inline constexpr bool kScaleUseHttp = KEEZER_SCALE_USE_HTTP != 0;
inline constexpr std::uint16_t kKegCatalogSchemaVersion = 1U;

}  // namespace keezer::config
