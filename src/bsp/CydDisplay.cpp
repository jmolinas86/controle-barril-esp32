#include "bsp/CydDisplay.h"

#include "BuildConfig.h"

namespace keezer::bsp {

CydDisplay::CydDisplay() {
  {
    auto cfg = bus_.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    // Short on-board traces allow a faster write clock. Phase 18 keeps this
    // value centralized so visual artifacts can be reverted independently.
    cfg.freq_write = config::kDisplayWriteClockHz;
    cfg.freq_read = 16'000'000;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = pins::kTftSclk;
    cfg.pin_mosi = pins::kTftMosi;
    cfg.pin_miso = pins::kTftMiso;
    cfg.pin_dc = pins::kTftDc;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();
    cfg.pin_cs = pins::kTftCs;
    cfg.pin_rst = pins::kTftReset;
    cfg.pin_busy = -1;
    cfg.memory_width = 240;
    cfg.memory_height = 320;
    cfg.panel_width = 240;
    cfg.panel_height = 320;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.dummy_read_pixel = 16;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    // Bench comparison against the approved dark reference confirmed that
    // this panel revision must keep hardware color inversion disabled.
    cfg.invert = false;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    panel_.config(cfg);
  }

  {
    auto cfg = light_.config();
    cfg.pin_bl = pins::kTftBacklight;
    cfg.invert = false;
    cfg.freq = 5'000;
    cfg.pwm_channel = 7;
    light_.config(cfg);
    panel_.setLight(&light_);
  }

  {
    auto cfg = touch_.config();
    cfg.x_min = 240;
    cfg.x_max = 3800;
    cfg.y_min = 3700;
    cfg.y_max = 200;
    cfg.pin_int = pins::kTouchIrq;
    cfg.bus_shared = false;
    // The XPT2046 orientation differs on the USB-C + micro-USB revision.
    cfg.offset_rotation = 2;

    // -1 selects LovyanGFX software SPI for XPT2046. This preserves the
    // second hardware SPI controller for the microSD bus.
    cfg.spi_host = -1;
    cfg.freq = 1'000'000;
    cfg.pin_sclk = pins::kTouchSclk;
    cfg.pin_mosi = pins::kTouchMosi;
    cfg.pin_miso = pins::kTouchMiso;
    cfg.pin_cs = pins::kTouchCs;
    touch_.config(cfg);
    panel_.setTouch(&touch_);
  }

  setPanel(&panel_);
}

}  // namespace keezer::bsp
