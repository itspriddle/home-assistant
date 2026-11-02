#include "lvgl_screenshot.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#ifdef USE_LVGL
#include "esphome/components/lvgl/lvgl_esphome.h"
#endif

namespace esphome {
namespace lvgl_screenshot {

static const char *const TAG = "lvgl_screenshot";

void LvglScreenshot::setup() {
  if (this->web_server_ != nullptr) {
    this->web_server_->add_handler(this);
    ESP_LOGCONFIG(TAG, "Screenshot endpoint registered at /screenshot");
  }
}

void LvglScreenshot::dump_config() {
  ESP_LOGCONFIG(TAG, "LVGL Screenshot:");
  ESP_LOGCONFIG(TAG, "  Endpoint: /screenshot");
}

bool LvglScreenshot::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() != HTTP_GET)
    return false;
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  return request->url_to(url_buf) == "/screenshot";
}

void LvglScreenshot::handleRequest(AsyncWebServerRequest *request) {
#ifdef USE_LVGL
  // Runs on the HTTP server task. LVGL isn't thread-safe, so ask loop() to
  // capture on the main task and wait for the result.
  std::unique_lock<std::mutex> lock(this->capture_mutex_);
  this->capture_done_ = false;
  this->capture_requested_ = true;
  bool done = this->capture_cv_.wait_for(lock, std::chrono::seconds(5), [this] { return this->capture_done_; });
  this->capture_requested_ = false;
  uint8_t *bmp = this->capture_bmp_;
  size_t size = this->capture_size_;
  this->capture_bmp_ = nullptr;
  lock.unlock();

  if (!done || bmp == nullptr) {
    ESP_LOGE(TAG, "Capture %s", done ? "failed" : "timed out");
    request->send(500, "text/plain", done ? "Capture failed" : "Capture timed out");
    return;
  }

  ESP_LOGI(TAG, "Sending BMP (%u bytes)", (unsigned) size);
  AsyncWebServerResponse *response = request->beginResponse(200, "image/bmp", bmp, size);
  response->addHeader("Content-Disposition", "inline; filename=\"screenshot.bmp\"");
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
  free(bmp);
#else
  request->send(500, "text/plain", "LVGL not enabled");
#endif
}

#ifdef USE_LVGL

void LvglScreenshot::loop() {
  if (!this->capture_requested_)
    return;

  size_t size = 0;
  uint8_t *bmp = this->render_bmp_(size);

  std::lock_guard<std::mutex> lock(this->capture_mutex_);
  if (this->capture_requested_) {
    this->capture_bmp_ = bmp;
    this->capture_size_ = size;
    this->capture_done_ = true;
    this->capture_requested_ = false;
    this->capture_cv_.notify_one();
  } else {
    // The request timed out while we were capturing
    free(bmp);
  }
}

void LvglScreenshot::write_bmp_header_(uint8_t *buffer, int32_t width, int32_t height) {
  int32_t row_size = ((width * 3 + 3) / 4) * 4;
  int32_t pixel_data_size = row_size * height;
  int32_t file_size = 54 + pixel_data_size;

  // BMP File Header (14 bytes)
  buffer[0] = 'B';
  buffer[1] = 'M';
  buffer[2] = file_size & 0xFF;
  buffer[3] = (file_size >> 8) & 0xFF;
  buffer[4] = (file_size >> 16) & 0xFF;
  buffer[5] = (file_size >> 24) & 0xFF;
  buffer[6] = 0;
  buffer[7] = 0;
  buffer[8] = 0;
  buffer[9] = 0;
  buffer[10] = 54;
  buffer[11] = 0;
  buffer[12] = 0;
  buffer[13] = 0;

  // BITMAPINFOHEADER (40 bytes)
  buffer[14] = 40;
  buffer[15] = 0;
  buffer[16] = 0;
  buffer[17] = 0;
  buffer[18] = width & 0xFF;
  buffer[19] = (width >> 8) & 0xFF;
  buffer[20] = (width >> 16) & 0xFF;
  buffer[21] = (width >> 24) & 0xFF;
  // Negative height for top-down DIB
  int32_t neg_height = -height;
  buffer[22] = neg_height & 0xFF;
  buffer[23] = (neg_height >> 8) & 0xFF;
  buffer[24] = (neg_height >> 16) & 0xFF;
  buffer[25] = (neg_height >> 24) & 0xFF;
  buffer[26] = 1;
  buffer[27] = 0;
  buffer[28] = 24;
  buffer[29] = 0;
  buffer[30] = 0;
  buffer[31] = 0;
  buffer[32] = 0;
  buffer[33] = 0;
  buffer[34] = pixel_data_size & 0xFF;
  buffer[35] = (pixel_data_size >> 8) & 0xFF;
  buffer[36] = (pixel_data_size >> 16) & 0xFF;
  buffer[37] = (pixel_data_size >> 24) & 0xFF;
  for (int i = 38; i < 54; i++) {
    buffer[i] = 0;
  }
}

uint8_t *LvglScreenshot::render_bmp_(size_t &size) {
  lv_obj_t *scr = lv_screen_active();
  if (scr == nullptr) {
    ESP_LOGE(TAG, "No active screen");
    return nullptr;
  }

  // Render the active screen into its own buffer at logical (rotated) size.
  // RGB888 is stored B, G, R in memory, which is what BMP wants.
  lv_draw_buf_t *snap = lv_snapshot_take(scr, LV_COLOR_FORMAT_RGB888);
  if (snap == nullptr) {
    ESP_LOGE(TAG, "Snapshot failed");
    return nullptr;
  }

  int32_t width = snap->header.w;
  int32_t height = snap->header.h;
  uint32_t stride = snap->header.stride;
  ESP_LOGI(TAG, "Taking screenshot %dx%d", (int) width, (int) height);

  int32_t row_size = ((width * 3 + 3) / 4) * 4;
  int32_t file_size = 54 + row_size * height;

  // Allocate BMP buffer in PSRAM
  uint8_t *bmp_buffer = (uint8_t *)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (bmp_buffer == nullptr) {
    bmp_buffer = (uint8_t *)malloc(file_size);
  }
  if (bmp_buffer == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate BMP buffer (%d bytes)", (int) file_size);
    lv_draw_buf_destroy(snap);
    return nullptr;
  }

  this->write_bmp_header_(bmp_buffer, width, height);

  uint8_t *dst = bmp_buffer + 54;
  for (int32_t y = 0; y < height; y++) {
    memcpy(dst, snap->data + y * stride, width * 3);
    memset(dst + width * 3, 0, row_size - width * 3);
    dst += row_size;
  }
  lv_draw_buf_destroy(snap);

  size = file_size;
  return bmp_buffer;
}

#endif  // USE_LVGL

}  // namespace lvgl_screenshot
}  // namespace esphome
