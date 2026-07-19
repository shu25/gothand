# Godhand Pico 2 Mouse Controller

Raspberry Pi Pico 2向けのマウスコントローラーfirmwareです。Arduino-Pico coreを使い、USB HID MouseとしてPCへ入力を送ります。

## 実装済み

- Pico 2 / RP2350向けPlatformIO設定
- トラックボール抽象`PointingDevice`
- 2軸quadrature trackball仮実装
- スクロール用ロータリーエンコーダ
- 10ボタン入力
- USB HID mouse移動、左/右/中クリック、ホイール
- ESP32-C3向けUARTテキストプロトコル
- RF managerスタブ
- `millis()`ベースの非ブロッキングメインループ

## ビルド

初回はArduino-Pico toolchainの取得に時間がかかります。

```powershell
cd pico2
python -m platformio run
```

書き込み:

```powershell
cd pico2
python -m platformio run -t upload
```

## 配線

初期ピンは[src/config.h](src/config.h)にあります。

| 機能 | GPIO |
| --- | --- |
| ESP UART TX | GP0 |
| ESP UART RX | GP1 |
| Trackball X A/B | GP2 / GP3 |
| Trackball Y A/B | GP4 / GP5 |
| Scroll Encoder A/B | GP6 / GP7 |
| Buttons 1-10 | GP8-GP17 |

UART接続:

```text
Pico GP0 TX -> ESP32 RX
Pico GP1 RX <- ESP32 TX
GND         <-> GND
```

ESP32側の初期UARTピンは`esp32/src/config.h`の`PICO_UART_RX_PIN` / `PICO_UART_TX_PIN`で変更できます。

## プロトコル

PicoからESP32へ:

```text
PICO:READY
MOUSE:10,-5
SCROLL:1
BTN:1:DOWN
BTN:1:UP
```

ESP32からPicoへ:

```text
VOL:75
PLAY
PAUSE
NEXT
PREV
```

現時点ではESP32から受けたコマンドはログ表示のみです。次の段階でモード切替、マクロ、RF report制御に接続します。
