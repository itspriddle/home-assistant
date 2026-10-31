"""Data coordinators for Emby Now Playing."""

from __future__ import annotations

from dataclasses import asdict, dataclass, replace
from datetime import datetime
import logging
import re
from typing import Any
from urllib.parse import urlencode

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed
from homeassistant.util import dt as dt_util

from .api import EmbyClient, EmbyError
from .const import LIBRARY_SCAN_INTERVAL, POSITION_TOLERANCE_SECONDS, SESSIONS_SCAN_INTERVAL

_LOGGER = logging.getLogger(__name__)

TICKS_PER_SECOND = 10_000_000


@dataclass(frozen=True, slots=True)
class EmbySession:
    """A playing (or paused) Emby session, flattened for HA."""

    session_id: str
    device_id: str
    device: str
    client: str
    app_version: str | None
    user: str | None
    ip: str | None
    play_method: str | None
    state: str  # "playing" | "paused"
    type: str  # "movie" | "episode" | "audio" | lowercased Emby type
    item_id: str
    title: str
    full_title: str
    series: str | None
    season: int | None
    episode: int | None
    artist: str | None
    album: str | None
    track: int | None
    year: int | None
    date: str | None
    overview: str | None
    duration: int | None
    position: int
    position_updated_at: datetime
    image: str | None
    url: str
    series_url: str | None
    season_url: str | None
    album_url: str | None
    artist_url: str | None
    imdb_url: str | None
    tmdb_url: str | None
    tvdb_url: str | None

    def as_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["position_updated_at"] = self.position_updated_at.isoformat()
        return data


@dataclass(frozen=True, slots=True)
class EmbyLibrary:
    """Library item counts and links."""

    movies: int
    series: int
    episodes: int
    movies_url: str | None
    tv_url: str | None


def _ticks_to_seconds(ticks: int | None) -> int | None:
    return None if ticks is None else round(ticks / TICKS_PER_SECOND)


def _web_url(base: str, item_id: str | None, server_id: str) -> str | None:
    if not item_id:
        return None
    return f"{base}/web/index.html#!/item?{urlencode({'id': item_id, 'serverId': server_id})}"


def _image_url(base: str, item_id: str | None, tag: str | None) -> str | None:
    if not item_id or not tag:
        return None
    query = urlencode({"maxHeight": 450, "tag": tag, "quality": 90})
    return f"{base}/emby/Items/{item_id}/Images/Primary?{query}"


def _external_url(item: dict[str, Any], name: str) -> str | None:
    return next((u.get("Url") for u in item.get("ExternalUrls") or [] if u.get("Name") == name), None)


def parse_session(raw: dict[str, Any], base_url: str, now: datetime) -> EmbySession | None:
    """Flatten an Emby /Sessions entry. Returns None if nothing is playing."""
    item = raw.get("NowPlayingItem")
    device_id = raw.get("DeviceId")
    if not item or not device_id:
        return None

    server_id = raw.get("ServerId", "")
    play_state = raw.get("PlayState") or {}
    item_id = str(item.get("Id", ""))
    media_type = (item.get("Type") or "").lower()
    title = item.get("Name") or "Unknown"

    series = season = episode = artist = album = track = None
    series_url = season_url = album_url = artist_url = None

    if media_type == "episode":
        series = item.get("SeriesName")
        season = item.get("ParentIndexNumber")
        episode = item.get("IndexNumber")
        code = ""
        if season is not None and episode is not None:
            code = f"S{season:02d}E{episode:02d}"
        full_title = " · ".join(p for p in (series, code, title) if p)
        image = _image_url(base_url, item.get("SeriesId"), item.get("SeriesPrimaryImageTag"))
        series_url = _web_url(base_url, item.get("SeriesId"), server_id)
        season_url = _web_url(base_url, item.get("SeasonId"), server_id)
    elif media_type == "audio":
        artist = item.get("AlbumArtist") or next(iter(item.get("Artists") or []), None)
        album = item.get("Album")
        track = item.get("IndexNumber")
        full_title = " · ".join(p for p in (artist, album, title) if p)
        image = _image_url(base_url, item_id, (item.get("ImageTags") or {}).get("Primary")) or _image_url(
            base_url, item.get("AlbumId"), item.get("AlbumPrimaryImageTag")
        )
        album_url = _web_url(base_url, item.get("AlbumId"), server_id)
        artist_id = next(
            (a.get("Id") for a in item.get("ArtistItems") or [] if a.get("Name") == artist), None
        )
        artist_url = _web_url(base_url, artist_id, server_id)
    else:
        full_title = title
        image = _image_url(base_url, item_id, (item.get("ImageTags") or {}).get("Primary"))

    premiere = item.get("PremiereDate") or item.get("DateCreated")
    client = raw.get("Client") or "Unknown"

    return EmbySession(
        session_id=raw.get("Id", ""),
        device_id=device_id,
        device=raw.get("DeviceName") or client,
        # Infuse reports itself as Infuse-Library, Infuse-Direct, ...
        client="Infuse" if re.search(r"Infuse", client) else client,
        app_version=raw.get("ApplicationVersion"),
        user=raw.get("UserName"),
        ip=raw.get("RemoteEndPoint"),
        play_method=play_state.get("PlayMethod"),
        state="paused" if play_state.get("IsPaused") else "playing",
        type=media_type,
        item_id=item_id,
        title=title,
        full_title=full_title,
        series=series,
        season=season,
        episode=episode,
        artist=artist,
        album=album,
        track=track,
        year=item.get("ProductionYear"),
        date=premiere[:10] if premiere else None,
        overview=None if media_type == "audio" else item.get("Overview"),
        duration=_ticks_to_seconds(item.get("RunTimeTicks")),
        position=_ticks_to_seconds(play_state.get("PositionTicks")) or 0,
        position_updated_at=now,
        image=image,
        url=_web_url(base_url, item_id, server_id) or base_url,
        series_url=series_url,
        season_url=season_url,
        album_url=album_url,
        artist_url=artist_url,
        imdb_url=_external_url(item, "IMDb"),
        tmdb_url=_external_url(item, "TheMovieDb") or _external_url(item, "MovieDb"),
        tvdb_url=_external_url(item, "TheTVDB"),
    )


def _carry_position(new: EmbySession, old: EmbySession | None, now: datetime) -> EmbySession:
    """Keep the previous position/timestamp while playback is on schedule.

    Consumers extrapolate from position + position_updated_at, so there's no
    need to publish a new position every poll. Only seeks, pauses, resumes and
    item changes produce a new position.
    """
    if old is None or old.item_id != new.item_id or old.state != new.state:
        return new
    expected = old.position
    if old.state == "playing":
        expected += (now - old.position_updated_at).total_seconds()
    if abs(expected - new.position) > POSITION_TOLERANCE_SECONDS:
        return new
    return replace(new, position=old.position, position_updated_at=old.position_updated_at)


class EmbySessionsCoordinator(DataUpdateCoordinator[dict[str, EmbySession]]):
    """Polls /Sessions. Data is active sessions keyed by Emby DeviceId."""

    config_entry: ConfigEntry

    def __init__(self, hass: HomeAssistant, entry: ConfigEntry, client: EmbyClient) -> None:
        super().__init__(
            hass,
            _LOGGER,
            config_entry=entry,
            name="Emby sessions",
            update_interval=SESSIONS_SCAN_INTERVAL,
            # Only notify entities when something actually changed.
            always_update=False,
        )
        self.client = client

    async def _async_update_data(self) -> dict[str, EmbySession]:
        try:
            raw_sessions = await self.client.sessions()
        except EmbyError as err:
            raise UpdateFailed(str(err)) from err

        now = dt_util.utcnow()
        previous = self.data or {}
        sessions: dict[str, EmbySession] = {}
        for raw in raw_sessions:
            if (session := parse_session(raw, self.client.url, now)) is None:
                continue
            sessions[session.device_id] = _carry_position(
                session, previous.get(session.device_id), now
            )
        return dict(sorted(sessions.items(), key=lambda kv: kv[1].device.lower()))


class EmbyLibraryCoordinator(DataUpdateCoordinator[EmbyLibrary]):
    """Polls library counts occasionally."""

    config_entry: ConfigEntry

    def __init__(
        self, hass: HomeAssistant, entry: ConfigEntry, client: EmbyClient, server_id: str
    ) -> None:
        super().__init__(
            hass,
            _LOGGER,
            config_entry=entry,
            name="Emby library",
            update_interval=LIBRARY_SCAN_INTERVAL,
            always_update=False,
        )
        self.client = client
        self.server_id = server_id

    def _library_url(self, folders: list[dict[str, Any]], collection_type: str) -> str | None:
        folder_id = next(
            (f.get("ItemId") or f.get("Id") for f in folders if f.get("CollectionType") == collection_type),
            None,
        )
        if folder_id is None:
            return None
        query = urlencode({"serverId": self.server_id, "parentId": folder_id})
        return f"{self.client.url}/web/index.html#!/videos?{query}"

    async def _async_update_data(self) -> EmbyLibrary:
        try:
            counts = await self.client.item_counts()
            folders = await self.client.virtual_folders()
        except EmbyError as err:
            raise UpdateFailed(str(err)) from err

        return EmbyLibrary(
            movies=counts.get("MovieCount", 0),
            series=counts.get("SeriesCount", 0),
            episodes=counts.get("EpisodeCount", 0),
            movies_url=self._library_url(folders, "movies"),
            tv_url=self._library_url(folders, "tvshows"),
        )
