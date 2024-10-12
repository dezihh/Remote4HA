# How to setup com ports in devconainers

- Install on Windows usbipd: `winget install usbipd`
- Find all attached devices: `` usbipd list`` 
- Attach your device: ``usbipd.exe bind --busid y-x``
    - or: ``usbipd.exe bind --hardware-id <vid:pid>``
- Attach Device to Subsystem(as Admin): ``usbipd attach --wsl --busid 3-5``

## Keep in mind:
- Add nessecary drivers (cp341, cp210x) in ``/etc/modules-load.d/modules.conf``
    - Testable with modprobe
- IF container rebuild a ``attach -a --wsl --busid 10-3`` for autmatic remapping