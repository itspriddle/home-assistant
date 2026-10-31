"""Logbook entries for Emby playback history."""

from __future__ import annotations

from collections.abc import Callable
from typing import Any

from homeassistant.components.logbook import (
    LOGBOOK_ENTRY_ENTITY_ID,
    LOGBOOK_ENTRY_MESSAGE,
    LOGBOOK_ENTRY_NAME,
)
from homeassistant.core import Event, HomeAssistant, callback

from .const import DOMAIN, EVENT_PLAYBACK_STARTED


@callback
def async_describe_events(
    hass: HomeAssistant,
    async_describe_event: Callable[[str, str, Callable[[Event], dict[str, Any]]], None],
) -> None:
    @callback
    def describe_playback_started(event: Event) -> dict[str, Any]:
        data = event.data
        return {
            LOGBOOK_ENTRY_NAME: data["device"],
            LOGBOOK_ENTRY_MESSAGE: f"started playing {data['full_title']}",
            LOGBOOK_ENTRY_ENTITY_ID: data["entity_id"],
        }

    async_describe_event(DOMAIN, EVENT_PLAYBACK_STARTED, describe_playback_started)
