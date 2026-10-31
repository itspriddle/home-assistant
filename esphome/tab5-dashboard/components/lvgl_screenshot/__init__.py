import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import web_server_base
from esphome.components.lvgl.defines import add_lv_use
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID

DEPENDENCIES = ["web_server_base", "lvgl"]
AUTO_LOAD = ["web_server_base"]

CONF_LVGL_ID = "lvgl_id"

lvgl_screenshot_ns = cg.esphome_ns.namespace("lvgl_screenshot")
LvglScreenshot = lvgl_screenshot_ns.class_("LvglScreenshot", cg.Component)



def _require_lvgl_features(config):
    # Captures are taken with lv_snapshot_take()
    add_lv_use("snapshot")
    return config


CONFIG_SCHEMA = cv.All(cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LvglScreenshot),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
            web_server_base.WebServerBase
        ),
    }
).extend(cv.COMPONENT_SCHEMA), _require_lvgl_features)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    web_server_base_var = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    cg.add(var.set_web_server(web_server_base_var))
