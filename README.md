Remotecontainer Skeleton: https://github.com/prenone/platformio-vscode-devcontainer?tab=readme-ov-file

<b>Start: </b><br>
Install Dev Containers<br>
F1 -> Dev Containers: Clone Repository in Named Container Volume<br>

## BOM:
NodeMcu ESP32 Wroom 32 https://www.amazon.de/dp/B0CLD28SHQ?ref=ppx_yo2ov_dt_b_fed_asin_title<br>
HC-Typ-05 Bluetooth RF Transceiver https://www.amazon.de/gp/product/B0DGXCGD8N/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1<br>
Hailege USB Host Shield 2.0 ADK USB Host https://www.amazon.de/dp/B07Y83TJ47?ref=ppx_yo2ov_dt_b_fed_asin_title <br>

<b>Modules:</b><br>
https://registry.platformio.org/libraries/felis/USB-Host-Shield-20<br>

lib_deps = <br>
    tzapu/WiFiManager<br>
    tanakamasayuki/EspUsbHost<br>

<b>WSL4 COM:</b><br>
winget install usbipd<br>
usbipd list<br>
usbipd.exe bind --busid 3-5<br>
usbipd attach --wsl --busid 3-5<br>

modprobe cp341<br>

<br>Plug Any USB Device on an ESP8266: https://www.hackster.io/usini/plug-any-usb-device-on-an-esp8266-e0ca8a
<br>USB to USB keyboard converter build log: https://geekhack.org/index.php?topic=80421.0
<br>OSC did not start" keeps showing up in the Serial Monitor: https://forum.arduino.cc/t/osc-did-not-start-keeps-showing-up-in-the-serial-monitor/516403
<br>Arduino Shield – USB 2.0 Host shield Hw: http://domoticx.com/arduino-shield-usb-2-0-host-shield/
<br>Big Foto of Shield: https://forum.pjrc.com/index.php?threads/teensy-with-mini-usb-host-shield-chineese.43357/
<br>SoftUSB: https://github.com/tobozo/ESP32-USB-Soft-Host/tree/main
<br>Esp32-Jumpstart (Host, Pinout) https://github.com/touchgadget/esp32-usb-host-demos

<br> Use Webserial! https://github.com/ayushsharma82/WebSerial/tree/master

## BLE
<br>https://github.com/blackketter/ESP32-BLE-Combo/tree/master
<br>kleiner: https://github.com/T-vK/ESP32-BLE-Keyboard/blob/master/BleKeyboard.h
<br>commands-table: https://github.com/asterics/esp32_mouse_keyboard
<br> BLE Advertising Article https://stackoverflow.com/questions/69921353/how-can-i-clone-a-non-paired-ble-signal-from-a-remote-to-trigger-a-device
<br> Google Keycodes https://developer.android.com/reference/android/view/KeyEvent

## Filesystem
<br>Partition Tables: https://github.com/espressif/arduino-esp32/tree/master/tools/partitions
<br>ESP Readme: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
<br>Blog: https://community.platformio.org/t/platformio-esp32-partitions/33792


## Keycodes:
<br>[Flirc HID codes and Android keycode counterparts](https://forum.flirc.tv/index.php?/topic/3569-flirc-hid-codes-and-android-keycode-counterparts/)
<br>[Android to HID Mappings](https://source.android.com/docs/core/interaction/input/keyboard-devices?hl=de#hid-consumer-page-0x0c)(Bible)
<br>[HID Usage Tables](https://usb.org/sites/default/files/hut1_21.pdf) (Official PDF Document)
<br>[HID Table](https://www.freebsddiary.org/APC/usb_hid_usages.php) Better sorted
<br>[Android keycode Display for Developer](https://play.google.com/store/apps/details?id=jp.ne.neko.freewing.KeyCodeDisp&hl=gsw)
<br>[Android Key Events Developer Page](https://developer.android.com/reference/android/view/KeyEvent)
<br>[Other Ideas to HID Mnagement](https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2)

<br>[Markdwon Editor Online](https://dillinger.io/)
<br>[Markdown Syntax Help](https://www.markdownguide.org/basic-syntax/)


### ESP32 S3 DevKitC 1 N16R8 ESP32 S3 WROOM1
<br>Memory: 384kb
<br>Flash: 16MB und 8MB PSRAM

## Vscode Twaeaks
<br> `pio pkg update -g -p espressif32` - Update Platformio



