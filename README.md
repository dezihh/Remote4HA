Remotecontainer Skeleton: https://github.com/prenone/platformio-vscode-devcontainer?tab=readme-ov-file

<b>Start: </b><br>
Install Dev Containers<br>
F1 -> Dev Containers: Clone Repository in Named Container Volume<br>

<b>BOM:</b><br>
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

<br><b>BLE</b>
<br>https://github.com/blackketter/ESP32-BLE-Combo/tree/master
<br<kleiner: https://github.com/T-vK/ESP32-BLE-Keyboard/blob/master/BleKeyboard.h
<br>commands-table: https://github.com/asterics/esp32_mouse_keyboard



