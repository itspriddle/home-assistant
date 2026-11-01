import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import time as time_ns
from esphome.components.font import Font

DEPENDENCIES = ["lvgl"]
AUTO_LOAD = ["json"]

CONF_TIME_ID = "time_id"
CONF_FONT_TITLE = "font_title"
CONF_FONT_BODY = "font_body"
CONF_FONT_SMALL = "font_small"
CONF_FONT_ICON = "font_icon"

emby_ui_ns = cg.esphome_ns.namespace("emby_ui")
EmbyUi = emby_ui_ns.class_("EmbyUi", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(EmbyUi),
        cv.Required(CONF_TIME_ID): cv.use_id(time_ns.RealTimeClock),
        cv.Required(CONF_FONT_TITLE): cv.use_id(Font),
        cv.Required(CONF_FONT_BODY): cv.use_id(Font),
        cv.Required(CONF_FONT_SMALL): cv.use_id(Font),
        cv.Required(CONF_FONT_ICON): cv.use_id(Font),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))

    for conf_key, setter in [
        (CONF_FONT_TITLE, "set_font_title"),
        (CONF_FONT_BODY, "set_font_body"),
        (CONF_FONT_SMALL, "set_font_small"),
        (CONF_FONT_ICON, "set_font_icon"),
    ]:
        font_var = await cg.get_variable(config[conf_key])
        cg.add(getattr(var, setter)(font_var))

    cg.add_define("USE_LVGL_FONT")
