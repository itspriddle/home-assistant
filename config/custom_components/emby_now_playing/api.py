"""Minimal read-only Emby REST client."""

from __future__ import annotations

from typing import Any

import aiohttp

REQUEST_TIMEOUT = aiohttp.ClientTimeout(total=10)


class EmbyError(Exception):
    """Base error talking to Emby."""


class EmbyAuthError(EmbyError):
    """The API key was rejected."""


class EmbyConnectionError(EmbyError):
    """Emby could not be reached or returned an unexpected response."""


class EmbyClient:
    """Fetch sessions, server info and library counts from Emby."""

    def __init__(self, session: aiohttp.ClientSession, url: str, api_key: str) -> None:
        self._session = session
        self._api_key = api_key
        self.url = url.rstrip("/")

    async def _get(self, path: str) -> Any:
        try:
            async with self._session.get(
                f"{self.url}/emby{path}",
                headers={"X-Emby-Token": self._api_key, "Accept": "application/json"},
                timeout=REQUEST_TIMEOUT,
            ) as resp:
                if resp.status in (401, 403):
                    raise EmbyAuthError(f"Emby rejected the API key ({resp.status})")
                if resp.status != 200:
                    raise EmbyConnectionError(f"GET {path} returned {resp.status}")
                return await resp.json()
        except (aiohttp.ClientError, TimeoutError, ValueError) as err:
            raise EmbyConnectionError(f"GET {path} failed: {err}") from err

    async def system_info(self) -> dict[str, Any]:
        return await self._get("/System/Info")

    async def sessions(self) -> list[dict[str, Any]]:
        return await self._get("/Sessions")

    async def item_counts(self) -> dict[str, Any]:
        return await self._get("/Items/Counts")

    async def virtual_folders(self) -> list[dict[str, Any]]:
        return await self._get("/Library/VirtualFolders")
