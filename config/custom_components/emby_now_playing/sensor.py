"""Sensors for Emby Now Playing."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

from homeassistant.components.sensor import (
    SensorEntity,
    SensorEntityDescription,
    SensorStateClass,
)
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from . import EmbyConfigEntry
from .coordinator import EmbyLibrary, EmbyLibraryCoordinator, EmbySessionsCoordinator


async def async_setup_entry(
    hass: HomeAssistant,
    entry: EmbyConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    data = entry.runtime_data
    async_add_entities(
        [
            EmbyNowPlayingSensor(data.sessions, entry),
            *(EmbyLibrarySensor(data.library, entry, desc) for desc in LIBRARY_SENSORS),
        ]
    )


class EmbyNowPlayingSensor(CoordinatorEntity[EmbySessionsCoordinator], SensorEntity):
    """Number of active sessions, with every session as an attribute.

    The `sessions` attribute is what the Lovelace card renders.
    """

    _attr_has_entity_name = True
    _attr_translation_key = "now_playing"
    _attr_icon = "mdi:play-box-multiple"
    _attr_state_class = SensorStateClass.MEASUREMENT
    # Large and churns with every change; not useful in history.
    _unrecorded_attributes = frozenset({"sessions"})

    def __init__(self, coordinator: EmbySessionsCoordinator, entry: EmbyConfigEntry) -> None:
        super().__init__(coordinator)
        self._attr_unique_id = f"{entry.runtime_data.server_id}_now_playing"
        self._attr_device_info = entry.runtime_data.server_device

    @property
    def native_value(self) -> int:
        return len(self.coordinator.data)

    @property
    def extra_state_attributes(self) -> dict[str, Any]:
        return {"sessions": [s.as_dict() for s in self.coordinator.data.values()]}


@dataclass(frozen=True, kw_only=True)
class EmbyLibrarySensorDescription(SensorEntityDescription):
    value_fn: Callable[[EmbyLibrary], int]
    url_fn: Callable[[EmbyLibrary], str | None]


LIBRARY_SENSORS: tuple[EmbyLibrarySensorDescription, ...] = (
    EmbyLibrarySensorDescription(
        key="movies",
        translation_key="movies",
        icon="mdi:movie-open",
        value_fn=lambda lib: lib.movies,
        url_fn=lambda lib: lib.movies_url,
    ),
    EmbyLibrarySensorDescription(
        key="series",
        translation_key="series",
        icon="mdi:television-classic",
        value_fn=lambda lib: lib.series,
        url_fn=lambda lib: lib.tv_url,
    ),
    EmbyLibrarySensorDescription(
        key="episodes",
        translation_key="episodes",
        icon="mdi:television-play",
        value_fn=lambda lib: lib.episodes,
        url_fn=lambda lib: lib.tv_url,
    ),
)


class EmbyLibrarySensor(CoordinatorEntity[EmbyLibraryCoordinator], SensorEntity):
    """Item count for a library type. `url` links to the library in Emby."""

    _attr_has_entity_name = True
    _attr_state_class = SensorStateClass.MEASUREMENT
    entity_description: EmbyLibrarySensorDescription

    def __init__(
        self,
        coordinator: EmbyLibraryCoordinator,
        entry: EmbyConfigEntry,
        description: EmbyLibrarySensorDescription,
    ) -> None:
        super().__init__(coordinator)
        self.entity_description = description
        self._attr_unique_id = f"{entry.runtime_data.server_id}_{description.key}"
        self._attr_device_info = entry.runtime_data.server_device

    @property
    def native_value(self) -> int:
        return self.entity_description.value_fn(self.coordinator.data)

    @property
    def extra_state_attributes(self) -> dict[str, Any]:
        return {"url": self.entity_description.url_fn(self.coordinator.data)}
