import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import time as time_ns
from esphome.components.font import Font

DEPENDENCIES = ["lvgl"]

CONF_TIME_ID = "time_id"
CONF_FONT_LARGE = "font_large"
CONF_FONT_MEDIUM = "font_medium"
CONF_FONT_SMALL = "font_small"
CONF_FONT_ICON = "font_icon"

cyd_ui_ns = cg.esphome_ns.namespace("cyd_ui")
CydUi = cyd_ui_ns.class_("CydUi", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CydUi),
        cv.Required(CONF_TIME_ID): cv.use_id(time_ns.RealTimeClock),
        cv.Optional(CONF_FONT_LARGE): cv.use_id(Font),
        cv.Optional(CONF_FONT_MEDIUM): cv.use_id(Font),
        cv.Optional(CONF_FONT_SMALL): cv.use_id(Font),
        cv.Optional(CONF_FONT_ICON): cv.use_id(Font),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))

    has_font = False
    for conf_key, setter in [
        (CONF_FONT_LARGE, "set_font_large"),
        (CONF_FONT_MEDIUM, "set_font_medium"),
        (CONF_FONT_SMALL, "set_font_small"),
        (CONF_FONT_ICON, "set_font_icon"),
    ]:
        if conf_key in config:
            font_var = await cg.get_variable(config[conf_key])
            cg.add(getattr(var, setter)(font_var))
            has_font = True

    if has_font:
        cg.add_define("USE_LVGL_FONT")
