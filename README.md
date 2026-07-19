# Godhand Wearable Controller

ESP32-C3 SuperMiniとRaspberry Pi Pico 2を組み合わせた、腕装着型マウス兼音楽コントローラーの開発プロジェクトです。

ESP32-C3側は`esp32/`、Pico 2側は`pico2/`、PC側は`pc/`に分けています。

ESP32-C3 firmwareにはBLE通信の初期実装も入り、PCからの`VOL`、`TIME`、`DISP`、`CLOCK`、`FFT`、`B:F:...`コマンド受信と、ESP32入力イベントのNotify送信に対応しています。

## 必要ライブラリ

PlatformIOが`esp32/platformio.ini`から自動取得します。

- Adafruit GFX Library
- Adafruit SSD1306

## ビルド方法

```powershell
cd esp32
pio run
```

`pio`がPATHにない環境では以下でも実行できます。

```powershell
cd esp32
python -m platformio run
```

## PCアプリ

新しいPC側アプリは`pc/`に分割実装しています。PySide6 GUI、設定JSON、ログ、FFT解析、音量制御、メディアキー送信、BLE transportの土台を含みます。

依存関係:

```powershell
cd pc
python -m pip install -r requirements.txt
```

起動:

```powershell
cd pc
python main.py
```

既存の`pc_controller/`は旧CLI/Tkinter実装として残しています。

## Pico 2 firmware

Pico 2側は`pico2/`に追加しています。USB HID Mouse、トラックボール入力抽象、スクロールエンコーダ、10ボタン入力、ESP32-C3向けUARTプロトコル、RF managerスタブを含みます。

```powershell
cd pico2
python -m platformio run
```

詳細は[pico2/README.md](pico2/README.md)を参照してください。

書き込み:

```powershell
cd esp32
pio run -t upload
```

シリアルモニター:

```powershell
cd esp32
pio device monitor
```

## 配線

初期ピン設定は[esp32/src/config.h](esp32/src/config.h)に集約しています。ESP32-C3 SuperMiniの実ボード配線に合わせて必要に応じて変更してください。

| 機能 | GPIO |
| --- | --- |
| OLED SDA | GPIO6 |
| OLED SCL | GPIO7 |
| OLED I2C Address | 0x3C |
| Rotary Encoder A | GPIO8 |
| Rotary Encoder B | GPIO9 |
| Encoder Switch | GPIO10 |
| Button 1-5 | GPIO0, GPIO1, GPIO2, GPIO3, GPIO4 |

入力は`INPUT_PULLUP`前提です。ボタンやエンコーダスイッチは押下時にGNDへ落ちる配線にしてください。

## 実装済み機能

- SSD1306 OLED制御
- ロータリーエンコーダによる音量変更
- エンコーダ押し込みによるPlay/Pause切り替え
- 5ボタン入力
- `AppState`による状態管理
- CLOCK画面
- VOLUME画面
- FFT画面の仮描画
- STATUS画面
- `millis()`ベースの画面更新とVOLUME画面の自動復帰
- `BleManager`、`PcProtocol`、`PicoProtocol`の将来用インターフェース

## ファイル構成

| ファイル | 役割 |
| --- | --- |
| `esp32/src/main.cpp` | 初期化と非ブロッキングメインループ |
| `esp32/src/config.h` | ピン、画面サイズ、タイムアウト、ボタン割り当て |
| `esp32/src/AppState.*` | 表示モード、音量、接続状態、時計、FFT状態の管理 |
| `esp32/src/DisplayUi.*` | SSD1306描画処理 |
| `esp32/src/Inputs.*` | ロータリーエンコーダと5ボタンの入力処理 |
| `esp32/src/BleManager.*` | Bluetooth実装予定のインターフェース |
| `esp32/src/PcProtocol.*` | PC通信プロトコル実装予定のインターフェース |
| `esp32/src/PicoProtocol.*` | Pico 2 UART連携実装予定のインターフェース |

## 今後の実装予定

- Phase 2: ESP32-C3とPC間のBLE通信
- Phase 3: PC側音声キャプチャ、FFT解析、FFT送信の実機接続
- Phase 4: 音声状態に応じたCLOCK/FFT自動切り替え
- Phase 5: Pico 2側トラックボール、クリック、スクロール
- Phase 6: ESP32-C3とPico 2間のUART通信
- Phase 7: 低遅延RFドングルとUSB HIDマウス化
