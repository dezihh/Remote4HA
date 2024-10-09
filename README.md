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
winget install usbipd
usbipd list
usbipd.exe bind --busid 3-5
usbipd attach --wsl --busid 3-5

modprobe cp341


