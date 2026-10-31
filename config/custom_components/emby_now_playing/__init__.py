"""Emby Now Playing: read-only view of what Emby clients are playing."""

from __future__ import annotations

from dataclasses import dataclass

from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_API_KEY, CONF_URL, CONF_VERIFY_SSL, Platform
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import ConfigEntryNotReady
from homeassistant.helpers import device_registry as dr
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.device_registry import DeviceInfo

from .api import EmbyClient, EmbyError
from .const import DOMAIN
from .coordinator import EmbyLibraryCoordinator, EmbySessionsCoordinator

PLATFORMS: list[Platform] = [Platform.MEDIA_PLAYER, Platform.SENSOR]


@dataclass
class EmbyData:
    """Runtime data for a config entry."""

    client: EmbyClient
    server_id: str
    server_device: DeviceInfo
    server_device_id: str
    sessions: EmbySessionsCoordinator
    library: EmbyLibraryCoordinator


type EmbyConfigEntry = ConfigEntry[EmbyData]


async def async_setup_entry(hass: HomeAssistant, entry: EmbyConfigEntry) -> bool:
    client = EmbyClient(
        async_get_clientsession(hass, verify_ssl=entry.data.get(CONF_VERIFY_SSL, True)),
        entry.data[CONF_URL],
        entry.data[CONF_API_KEY],
    )
    try:
        info = await client.system_info()
    except EmbyError as err:
        raise ConfigEntryNotReady(str(err)) from err

    server_id: str = info["Id"]
    sessions = EmbySessionsCoordinator(hass, entry, client)
    library = EmbyLibraryCoordinator(hass, entry, client, server_id)
    await sessions.async_config_entry_first_refresh()
    await library.async_config_entry_first_refresh()

    server_device = DeviceInfo(
        identifiers={(DOMAIN, server_id)},
        # Entity IDs derive from this: sensor.emby_now_playing, sensor.emby_movies
        name="Emby",
        manufacturer="Emby",
        model="Emby Server",
        sw_version=info.get("Version"),
        configuration_url=f"{client.url}/web/index.html",
    )
    # Registered up front so client devices can link to it with via_device_id.
    server_device_entry = dr.async_get(hass).async_get_or_create(
        config_entry_id=entry.entry_id, **server_device
    )

    entry.runtime_data = EmbyData(
        client=client,
        server_id=server_id,
        server_device=server_device,
        server_device_id=server_device_entry.id,
        sessions=sessions,
        library=library,
    )
    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    return True


async def async_unload_entry(hass: HomeAssistant, entry: EmbyConfigEntry) -> bool:
    return await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
