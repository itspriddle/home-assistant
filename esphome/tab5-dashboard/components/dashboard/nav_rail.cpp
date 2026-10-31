#include "dashboard.h"
#include "screen.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

static constexpr int NAV_BTN = 72;

struct NavItem {
  ScreenId id;
  const char *glyph;
  uint32_t active_fg;
  uint32_t active_bg;
};

static const NavItem NAV_ITEMS[] = {
    {ScreenId::HOME, icon::HOME, color::TEXT, color::SURFACE_3},
    {ScreenId::LIGHTS, icon::LIGHTBULB, color::AMBER, color::AMBER_TINT},
    {ScreenId::CLIMATE, icon::THERMOMETER, color::HEAT, color::HEAT_TINT},
};

lv_obj_t *build_nav_rail(lv_obj_t *root, Dashboard *dash, ScreenId active) {
  lv_obj_t *rail = box(root, color::RAIL);
  lv_obj_set_size(rail, RAIL_W, SCREEN_H);
  lv_obj_align(rail, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_pad_ver(rail, 36, 0);
  flex_col(rail, 16, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

  for (const auto &item : NAV_ITEMS) {
    bool is_active = item.id == active;
    lv_obj_t *btn = box(rail, is_active ? static_cast<int64_t>(item.active_bg) : -1, 22);
    lv_obj_set_size(btn, NAV_BTN, NAV_BTN);
    lv_obj_t *ic = label(btn, fonts.icon_small, is_active ? item.active_fg : color::MUTED, item.glyph);
    lv_obj_center(ic);
    ScreenId target = item.id;
    on_click(btn, [dash, target]() { dash->show(target); });
  }

  // Spacer pushes the clock to the bottom
  lv_obj_t *spacer = box(rail);
  lv_obj_set_width(spacer, 1);
  lv_obj_set_flex_grow(spacer, 1);

  lv_obj_t *clock = label(rail, fonts.body, color::MUTED, "--:--");
  lv_obj_set_user_data(rail, clock);
  return rail;
}

void update_nav_rail(lv_obj_t *rail, Dashboard *dash) {
  auto *clock = static_cast<lv_obj_t *>(lv_obj_get_user_data(rail));
  if (clock != nullptr)
    set_text(clock, dash->clock_text());
}

}  // namespace dashboard
}  // namespace esphome
