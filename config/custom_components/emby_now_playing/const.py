"""Constants for Emby Now Playing."""

from datetime import timedelta
from typing import Final

DOMAIN: Final = "emby_now_playing"

SESSIONS_SCAN_INTERVAL: Final = timedelta(seconds=5)
LIBRARY_SCAN_INTERVAL: Final = timedelta(minutes=30)

# Reported positions within this many seconds of the extrapolated position
# are treated as unchanged, so steady playback doesn't write state every poll.
POSITION_TOLERANCE_SECONDS: Final = 5

# Fired when a device starts playing a new item. Shown in the logbook.
EVENT_PLAYBACK_STARTED: Final = f"{DOMAIN}_playback_started"
