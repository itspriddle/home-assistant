"""Read-only media players, one per Emby client device.

Entities are created the first time a device plays something and restored from
the entity registry on restart, so devices can be assigned to areas in the UI.
"""

from __future__ import annotations

from datetime import datetime
from typing import Any

from homeassistant.components.media_player import (
    DOMAIN as MEDIA_PLAYER_DOMAIN,
    MediaPlayerEntity,
    MediaPlayerEntityFeature,
    MediaPlayerState,
    MediaType,
)
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers import device_registry as dr, entity_registry as er
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from . import EmbyConfigEntry
from .const import DOMAIN, EVENT_PLAYBACK_STARTED
from .coordinator import EmbySession, EmbySessionsCoordinator

CONTENT_TYPES = {
    "movie": MediaType.MOVIE,
    "episode": MediaType.TVSHOW,
    "audio": MediaType.MUSIC,
}


async def async_setup_entry(
    hass: HomeAssistant,
    entry: EmbyConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    data = entry.runtime_data
    coordinator = data.sessions
    server_id = data.server_id
    prefix = f"{server_id}_"
    device_reg = dr.async_get(hass)
    known: set[str] = set()

    def device_info(device_id: str, session: EmbySession | None) -> DeviceInfo:
        identifier = (DOMAIN, f"{prefix}{device_id}")
        info = DeviceInfo(
            identifiers={identifier},
            manufacturer="Emby",
            via_device_id=data.server_device_id,
        )
        if session is not None:
            info["name"] = session.device
            info["model"] = session.client
            info["sw_version"] = session.app_version
        elif (device := device_reg.async_get_device_by_identifier(identifier, entry.entry_id)) is not None:
            info["name"] = device.name
        return info

    # Restore devices seen before.
    restored: list[EmbyMediaPlayer] = []
    for reg_entry in er.async_entries_for_config_entry(er.async_get(hass), entry.entry_id):
        if reg_entry.domain != MEDIA_PLAYER_DOMAIN or not reg_entry.unique_id.startswith(prefix):
            continue
        device_id = reg_entry.unique_id.removeprefix(prefix)
        known.add(device_id)
        restored.append(
            EmbyMediaPlayer(
                coordinator,
                reg_entry.unique_id,
                device_id,
                device_info(device_id, coordinator.data.get(device_id)),
                announce_current=False,
            )
        )
    async_add_entities(restored)

    @callback
    def add_new_devices() -> None:
        new = [s for device_id, s in coordinator.data.items() if device_id not in known]
        if not new:
            return
        known.update(s.device_id for s in new)
        async_add_entities(
            EmbyMediaPlayer(
                coordinator,
                f"{prefix}{s.device_id}",
                s.device_id,
                device_info(s.device_id, s),
                announce_current=True,
            )
            for s in new
        )

    add_new_devices()
    entry.async_on_unload(coordinator.async_add_listener(add_new_devices))


class EmbyMediaPlayer(CoordinatorEntity[EmbySessionsCoordinator], MediaPlayerEntity):
    """Shows what an Emby client is playing. Has no controls."""

    _attr_has_entity_name = True
    _attr_name = None
    _attr_supported_features = MediaPlayerEntityFeature(0)
    # Proxy artwork through HA so clients never talk to Emby directly.
    _attr_media_image_remotely_accessible = False
    _unrecorded_attributes = frozenset({"overview"})

    def __init__(
        self,
        coordinator: EmbySessionsCoordinator,
        unique_id: str,
        device_id: str,
        device_info: DeviceInfo,
        *,
        announce_current: bool,
    ) -> None:
        super().__init__(coordinator)
        self._device_id = device_id
        self._attr_unique_id = unique_id
        self._attr_device_info = device_info
        # Restored entities don't re-announce whatever was already playing
        # before HA restarted.
        session = self._session
        self._announced_item_id = None if announce_current or session is None else session.item_id

    @property
    def _session(self) -> EmbySession | None:
        return self.coordinator.data.get(self._device_id)

    async def async_added_to_hass(self) -> None:
        await super().async_added_to_hass()
        self._announce_new_item()

    @callback
    def _handle_coordinator_update(self) -> None:
        self._announce_new_item()
        super()._handle_coordinator_update()

    @callback
    def _announce_new_item(self) -> None:
        session = self._session
        item_id = session.item_id if session else None
        if item_id == self._announced_item_id:
            return
        self._announced_item_id = item_id
        if session is None:
            return
        self.hass.bus.async_fire(
            EVENT_PLAYBACK_STARTED,
            {
                "entity_id": self.entity_id,
                "emby_device_id": session.device_id,
                "device": session.device,
                "client": session.client,
                "user": session.user,
                "type": session.type,
                "item_id": session.item_id,
                "title": session.title,
                "full_title": session.full_title,
            },
        )

    @property
    def state(self) -> MediaPlayerState:
        if (session := self._session) is None:
            return MediaPlayerState.IDLE
        return MediaPlayerState.PAUSED if session.state == "paused" else MediaPlayerState.PLAYING

    @property
    def app_name(self) -> str | None:
        return session.client if (session := self._session) else None

    @property
    def media_content_id(self) -> str | None:
        return session.item_id if (session := self._session) else None

    @property
    def media_content_type(self) -> MediaType | str | None:
        if (session := self._session) is None:
            return None
        return CONTENT_TYPES.get(session.type, MediaType.VIDEO)

    @property
    def media_title(self) -> str | None:
        return session.title if (session := self._session) else None

    @property
    def media_series_title(self) -> str | None:
        return session.series if (session := self._session) else None

    @property
    def media_season(self) -> str | None:
        if (session := self._session) is None or session.season is None:
            return None
        return str(session.season)

    @property
    def media_episode(self) -> str | None:
        if (session := self._session) is None or session.episode is None:
            return None
        return str(session.episode)

    @property
    def media_artist(self) -> str | None:
        return session.artist if (session := self._session) else None

    @property
    def media_album_artist(self) -> str | None:
        return session.artist if (session := self._session) else None

    @property
    def media_album_name(self) -> str | None:
        return session.album if (session := self._session) else None

    @property
    def media_track(self) -> int | None:
        return session.track if (session := self._session) else None

    @property
    def media_duration(self) -> int | None:
        return session.duration if (session := self._session) else None

    @property
    def media_position(self) -> int | None:
        return session.position if (session := self._session) else None

    @property
    def media_position_updated_at(self) -> datetime | None:
        return session.position_updated_at if (session := self._session) else None

    @property
    def media_image_url(self) -> str | None:
        return session.image if (session := self._session) else None

    @property
    def extra_state_attributes(self) -> dict[str, Any]:
        if (session := self._session) is None:
            return {}
        return {
            "full_title": session.full_title,
            "user": session.user,
            "play_method": session.play_method,
            "year": session.year,
            "url": session.url,
            "overview": session.overview,
        }
