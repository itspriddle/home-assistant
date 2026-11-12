import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, spi
from esphome.components import time as time_ns
from esphome.components.font import Font
from esphome.const import CONF_DC_PIN, CONF_ID, CONF_NAME, CONF_TIME_ID

DEPENDENCIES = ["spi"]
CODEOWNERS = []

CONF_DISPLAY_ID = "display_id"
CONF_LABEL_FONT = "label_font"
CONF_FACES = "faces"
CONF_FONT = "font"
CONF_COLOR = "color"
CONF_COLOR_BOTTOM = "color_bottom"
CONF_ACCENT = "accent"
CONF_Y_OFFSET = "y_offset"
CONF_PANEL_POWER_PIN = "panel_power_pin"
CONF_SELECT_DATA_PIN = "select_data_pin"
CONF_SELECT_CLOCK_PIN = "select_clock_pin"
CONF_SELECT_LATCH_PIN = "select_latch_pin"

elekstube_ui_ns = cg.esphome_ns.namespace("elekstube_ui")
ElekstubeUi = elekstube_ui_ns.class_("ElekstubeUi", cg.Component, spi.SPIDevice)

_HEX_COLOR = re.compile(r"^#?([0-9A-Fa-f]{6})$")


def hex_color(value):
    """A 24-bit colour as "#RRGGBB" or "RRGGBB"."""
    match = _HEX_COLOR.match(cv.string_strict(value))
    if match is None:
        raise cv.Invalid(f"Expected a colour like '#FFB347', got '{value}'")
    return int(match.group(1), 16)


FACE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string_strict,
        cv.Required(CONF_FONT): cv.use_id(Font),
        cv.Required(CONF_COLOR): hex_color,
        # Different from color = vertical gradient from color (top) to this.
        cv.Optional(CONF_COLOR_BOTTOM): hex_color,
        # Seconds bar and labels; defaults to color.
        cv.Optional(CONF_ACCENT): hex_color,
        cv.Optional(CONF_Y_OFFSET, default=0): cv.int_range(min=-100, max=100),
    }
)


def _unique_face_names(faces):
    names = [face[CONF_NAME] for face in faces]
    if len(names) != len(set(names)):
        raise cv.Invalid("Face names must be unique")
    return faces


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ElekstubeUi),
            cv.Required(CONF_DISPLAY_ID): cv.use_id(display.Display),
            cv.Required(CONF_TIME_ID): cv.use_id(time_ns.RealTimeClock),
            cv.Required(CONF_LABEL_FONT): cv.use_id(Font),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_PANEL_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SELECT_DATA_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SELECT_CLOCK_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SELECT_LATCH_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_FACES): cv.All(
                cv.ensure_list(FACE_SCHEMA), cv.Length(min=1), _unique_face_names
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=False))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config, write_only=True)

    cg.add(var.set_display(await cg.get_variable(config[CONF_DISPLAY_ID])))
    cg.add(var.set_time(await cg.get_variable(config[CONF_TIME_ID])))
    cg.add(var.set_label_font(await cg.get_variable(config[CONF_LABEL_FONT])))
    cg.add(var.set_dc_pin(await cg.gpio_pin_expression(config[CONF_DC_PIN])))
    cg.add(
        var.set_bus_pins(
            await cg.gpio_pin_expression(config[CONF_SELECT_DATA_PIN]),
            await cg.gpio_pin_expression(config[CONF_SELECT_CLOCK_PIN]),
            await cg.gpio_pin_expression(config[CONF_SELECT_LATCH_PIN]),
            await cg.gpio_pin_expression(config[CONF_PANEL_POWER_PIN]),
        )
    )

    for face in config[CONF_FACES]:
        top = face[CONF_COLOR]
        cg.add(
            var.add_face(
                await cg.get_variable(face[CONF_FONT]),
                top,
                face.get(CONF_COLOR_BOTTOM, top),
                face.get(CONF_ACCENT, top),
                face[CONF_Y_OFFSET],
            )
        )
