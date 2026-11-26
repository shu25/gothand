from __future__ import annotations

import asyncio

from bleak import BleakClient, BleakScanner

from transport.base import DeviceTransport

SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab"
CMD_UUID = "12345678-1234-1234-1234-1234567890ac"
STATE_UUID = "12345678-1234-1234-1234-1234567890ad"


class BleTransport(DeviceTransport):
    def __init__(self, device_name: str = "Godhand Controller"):
        self.device_name = device_name
        self.client: BleakClient | None = None
        self._send_lock = asyncio.Lock()

    async def connect(self) -> bool:
        devices = await BleakScanner.discover(timeout=5)
        target = None
        for device in devices:
            uuids = []
            metadata = getattr(device, "metadata", None)
            if metadata is not None and hasattr(metadata, "get"):
                uuids = metadata.get("uuids", [])
            service_match = any(str(uuid).lower() == SERVICE_UUID.lower() for uuid in uuids)
            if service_match or device.name == self.device_name or (device.name and "Godhand" in device.name):
                target = device
                break
        if target is None:
            return False
        self.client = BleakClient(target.address)
        await self.client.connect()
        await self.client.start_notify(STATE_UUID, self._handle_notify)
        try:
            value = await self.client.read_gatt_char(STATE_UUID)
            self._handle_notify(0, bytearray(value))
        except Exception:
            pass
        return True

    async def disconnect(self) -> None:
        if self.client is not None and self.client.is_connected:
            await self.client.disconnect()

    async def send(self, data: str) -> None:
        if self.client is None or not self.client.is_connected:
            return
        async with self._send_lock:
            try:
                await self.client.write_gatt_char(CMD_UUID, data.encode("utf-8"), response=False)
            except Exception:
                await self.client.write_gatt_char(CMD_UUID, data.encode("utf-8"), response=True)

    def _handle_notify(self, sender: int, data: bytearray) -> None:
        del sender
        if self.on_message is not None:
            self.on_message(data.decode("utf-8", errors="ignore"))
