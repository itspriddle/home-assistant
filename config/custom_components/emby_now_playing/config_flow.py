"""Config flow for Emby Now Playing."""

from __future__ import annotations

import logging
from typing import Any

import voluptuous as vol

from homeassistant.config_entries import ConfigFlow, ConfigFlowResult
from homeassistant.const import CONF_API_KEY, CONF_URL, CONF_VERIFY_SSL
from homeassistant.helpers.aiohttp_client import async_get_clientsession

from .api import EmbyAuthError, EmbyClient, EmbyError
from .const import DOMAIN

_LOGGER = logging.getLogger(__name__)

STEP_USER_SCHEMA = vol.Schema(
    {
        vol.Required(CONF_URL): str,
        vol.Required(CONF_API_KEY): str,
        vol.Required(CONF_VERIFY_SSL, default=True): bool,
    }
)


class EmbyNowPlayingConfigFlow(ConfigFlow, domain=DOMAIN):
    """Set up a connection to an Emby server."""

    VERSION = 1

    async def async_step_user(self, user_input: dict[str, Any] | None = None) -> ConfigFlowResult:
        errors: dict[str, str] = {}
        if user_input is not None:
            user_input[CONF_URL] = user_input[CONF_URL].rstrip("/")
            client = EmbyClient(
                async_get_clientsession(self.hass, verify_ssl=user_input[CONF_VERIFY_SSL]),
                user_input[CONF_URL],
                user_input[CONF_API_KEY],
            )
            try:
                info = await client.system_info()
            except EmbyAuthError:
                errors["base"] = "invalid_auth"
            except EmbyError:
                _LOGGER.debug("Could not connect to Emby", exc_info=True)
                errors["base"] = "cannot_connect"
            else:
                await self.async_set_unique_id(info["Id"])
                self._abort_if_unique_id_configured(updates=user_input)
                return self.async_create_entry(title=info.get("ServerName", "Emby"), data=user_input)

        return self.async_show_form(
            step_id="user",
            data_schema=self.add_suggested_values_to_schema(STEP_USER_SCHEMA, user_input),
            errors=errors,
        )
