# Platform plan

Godhand should keep the ESP32 firmware protocol small and stable, then put OS-specific behavior in the companion app.

## Shared BLE protocol

- `VOL:<0-100>`: set device display volume and mirror host volume.
- `PLAY` / `PAUSE`: update display play state.
- `TIME:HH:MM:SS`: sync the OLED clock from the host.
- `DISP:volume|fft`: switch display mode.
- `B:F:<16 packed chars>`: send FFT bars in a short BLE-safe packet.
- `B:W:<16 packed chars>`: send waveform points in a short BLE-safe packet.
- `FFT:v1,...,v16`: legacy sound bars.
- `EVENT:<action>`: firmware-to-host button and encoder events.

This should stay usable from Windows, macOS, Android, and iOS clients.

## Host app backends

Each platform should implement the same high-level operations:

- `set_system_volume(value)`
- `send_media_key(action)`
- `capture_system_audio_bars()`
- `load_control_profile()` / `save_control_profile()`
- later: `load_mouse_profile()` / `save_mouse_profile()`

## Platform notes

- Windows: Python is OK for the first app. Use pycaw endpoint volume for the default output device, native media keys with pyautogui as a fallback, and soundcard loopback from the default speaker for FFT/waveform data.
- macOS: media keys can be sent from a native app or automation bridge. System audio capture usually needs a virtual audio device or a native ScreenCaptureKit-style path.
- Android: build a native app. BLE, media session controls, and volume are available, but capturing other apps' audio is restricted.
- iOS: build a native app. BLE is fine, but global media control and other-app audio capture are heavily restricted. Treat sound bars as app-provided audio or skip them.

## Control profiles

Keep button, encoder, and future mouse customization in host-side profiles instead of hardcoding behavior in firmware. The firmware should report input events; the app decides whether the event means media control, volume, display mode, or mouse behavior.
