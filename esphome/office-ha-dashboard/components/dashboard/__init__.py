import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import font, light, time
from esphome.components.lvgl.defines import add_lv_use
from esphome.const import CONF_ID, CONF_TIME_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["lvgl"]
AUTO_LOAD = ["font"]

dashboard_ns = cg.esphome_ns.namespace("dashboard")
Dashboard = dashboard_ns.class_("Dashboard", cg.Component)

# Font roles, see theme.h for what each one is used for
FONT_ROLES = [
    "caption",  # 18px regular
    "small",  # 20px regular
    "body",  # 22px regular
    "label",  # 26px semibold
    "value",  # 34px medium
    "title",  # 40px semibold
    "large",  # 96px regular, digits only
    "display",  # 140px regular, digits only
    "clock",  # 168px medium, digits only
    "clock_xl",  # 260px medium, digits only (clock screen)
    "icon_small",  # 34px MDI
    "icon_medium",  # 44px MDI
    "icon_large",  # 88px MDI
]

CONF_FONTS = "fonts"
CONF_IDLE_TIMEOUT = "idle_timeout"
CONF_BACKLIGHT_ID = "backlight_id"
CONF_CLOCK_TIMEOUT = "clock_timeout"
CONF_DIM_TIMEOUT = "dim_timeout"
CONF_DIM_BRIGHTNESS = "dim_brightness"

# (yaml key, C++ setter, trigger template args, variable names)
TRIGGERS = {
    "on_light_toggle": ("add_on_light_toggle", [cg.std_string], ["entity_id"]),
    "on_light_brightness": (
        "add_on_light_brightness",
        [cg.std_string, cg.int_],
        ["entity_id", "brightness_pct"],
    ),
    "on_lights_off": ("add_on_lights_off", [], []),
    "on_scene": ("add_on_scene", [cg.std_string], ["entity_id"]),
    "on_climate_temperature": ("add_on_climate_temperature", [cg.float_], ["temperature"]),
    "on_climate_mode": ("add_on_climate_mode", [cg.std_string], ["mode"]),
}


def _require_lvgl_features(config):
    # Widgets are created in C++, so LVGL can't see what we use. Enable it
    # here (also makes font::Font::get_lv_font() available).
    add_lv_use("font", "label", "flex", "arc", "img", "line")
    return config


CONFIG_SCHEMA = cv.All(cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Dashboard),
        cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_FONTS): cv.Schema(
            {cv.Required(role): cv.use_id(font.Font) for role in FONT_ROLES}
        ),
        cv.Optional(
            CONF_IDLE_TIMEOUT, default="60s"
        ): cv.positive_time_period_milliseconds,
        # Inactivity before the clock screen, and before dimming the
        # backlight (both from the last touch / presence; 0 disables)
        cv.Optional(
            CONF_CLOCK_TIMEOUT, default="10min"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(
            CONF_DIM_TIMEOUT, default="20min"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_BACKLIGHT_ID): cv.use_id(light.LightState),
        cv.Optional(CONF_DIM_BRIGHTNESS, default="1%"): cv.percentage,
        **{
            cv.Optional(key): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        automation.Trigger.template(*args)
                    )
                }
            )
            for key, (_, args, _) in TRIGGERS.items()
        },
    }
).extend(cv.COMPONENT_SCHEMA), _require_lvgl_features)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_time(await cg.get_variable(config[CONF_TIME_ID])))
    cg.add(var.set_idle_timeout(config[CONF_IDLE_TIMEOUT]))
    cg.add(var.set_clock_timeout(config[CONF_CLOCK_TIMEOUT]))
    cg.add(var.set_dim_timeout(config[CONF_DIM_TIMEOUT]))
    cg.add(var.set_dim_brightness(config[CONF_DIM_BRIGHTNESS]))
    if CONF_BACKLIGHT_ID in config:
        cg.add(var.set_backlight(await cg.get_variable(config[CONF_BACKLIGHT_ID])))

    for role, font_id in config[CONF_FONTS].items():
        cg.add(var.set_font(role, await cg.get_variable(font_id)))

    for key, (setter, args, names) in TRIGGERS.items():
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
            cg.add(getattr(var, setter)(trigger))
            await automation.build_automation(
                trigger, list(zip(args, names)), conf
            )
