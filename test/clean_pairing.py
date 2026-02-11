import asyncio
from dbus_fast import Variant
from bluez_peripheral.adapter import Adapter
from bluez_peripheral.util import get_message_bus
from bluez_peripheral.agent import NoIoAgent
from bluez_peripheral.advert import Advertisement

async def ensure_adapter_ready():
    bus = await get_message_bus()
    adapter_path = "/org/bluez/hci0"
    introspection = await bus.introspect("org.bluez", adapter_path)
    proxy = bus.get_proxy_object("org.bluez", adapter_path, introspection)
    props_iface = proxy.get_interface("org.freedesktop.DBus.Properties")

    await props_iface.call_set("org.bluez.Adapter1", "Powered", Variant("b", True))
    await props_iface.call_set("org.bluez.Adapter1", "DiscoverableTimeout", Variant("u", 0))
    await props_iface.call_set("org.bluez.Adapter1", "Discoverable", Variant("b", True))
    await props_iface.call_set("org.bluez.Adapter1", "PairableTimeout", Variant("u", 0))
    await props_iface.call_set("org.bluez.Adapter1", "Pairable", Variant("b", True))
    try:
        await props_iface.call_set("org.bluez.Adapter1", "Privacy", Variant("s", "off"))
    except Exception:
        pass
    print("Adapter bereit, discoverable und pairable.")

async def advertise_keyboard():
    bus = await get_message_bus()
    adapter = await Adapter.get_first(bus)

    # WICHTIG: UUID-Liste wie im echten BleKeyboard-Projekt!
    advert = Advertisement(
        "Virtual Keyboard", [
            "0000180F-0000-1000-8000-00805F9B34FB", # Battery
            "0000180A-0000-1000-8000-00805F9B34FB", # Device Info
            "00001812-0000-1000-8000-00805F9B34FB"  # HID
        ],
        appearance=0x03C1,
        timeout=0 # unbegrenzt, bis manuell gestoppt!
    )
    agent = NoIoAgent()
    await agent.register(bus)
    await advert.register(bus, adapter=adapter)
    print("BLE Advertising aktiv! Jetzt kann sich der Client pairen/verbinden.")

async def main():
    await ensure_adapter_ready()
    await advertise_keyboard()
    print("Pairen/Verbinden von Client aus starten. Danach Skript neu starten (wie nach Reboot).")
    while True:
        await asyncio.sleep(60)

if __name__ == "__main__":
    asyncio.run(main())
