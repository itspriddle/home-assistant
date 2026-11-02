#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "esphome/core/component.h"
#include "esphome/components/web_server_base/web_server_base.h"

#ifdef USE_LVGL
#include "lvgl.h"
#endif

namespace esphome {
namespace lvgl_screenshot {

class LvglScreenshot : public Component, public AsyncWebHandler {
 public:
  LvglScreenshot() = default;

  void set_web_server(web_server_base::WebServerBase *web_server) { this->web_server_ = web_server; }

  void setup() override;
  void dump_config() override;
#ifdef USE_LVGL
  void loop() override;
#endif
  float get_setup_priority() const override { return setup_priority::LATE; }

  // AsyncWebHandler interface
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  web_server_base::WebServerBase *web_server_{nullptr};

#ifdef USE_LVGL
  // Captures the active screen as a malloc'd BMP; must run on the main task
  uint8_t *render_bmp_(size_t &size);
  void write_bmp_header_(uint8_t *buffer, int32_t width, int32_t height);

  // Hand-off between handleRequest() (HTTP task) and loop() (main task)
  std::mutex capture_mutex_;
  std::condition_variable capture_cv_;
  std::atomic<bool> capture_requested_{false};
  bool capture_done_{false};
  uint8_t *capture_bmp_{nullptr};
  size_t capture_size_{0};
#endif
};

}  // namespace lvgl_screenshot
}  // namespace esphome
