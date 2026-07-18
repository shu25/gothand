from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Callable


class DeviceTransport(ABC):
    on_message: Callable[[str], None] | None = None

    @abstractmethod
    async def connect(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    async def disconnect(self) -> None:
        raise NotImplementedError

    @abstractmethod
    async def send(self, data: str) -> None:
        raise NotImplementedError
