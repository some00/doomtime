import argparse
import dbus
import sys
from pathlib import Path
from collections import namedtuple


def import_doomtime_client():
    sys.path.append(str(Path(__file__).parent))
    import doomtime_client as rv
    return rv


def import_rgb_to_packed_rgb444():
    sys.path.append("@DOOMTIME_SCRIPTS@")
    from rgb_to_packed_rgb444 import rgb_to_packed_rgb444 as rv
    return rv


Characteristics = namedtuple("Characteristics",
                             ["frames", "reset", "palettes"])
doomtime_client = import_doomtime_client()
rgb_to_packed_rgb444 = import_rgb_to_packed_rgb444()
DBUS_OM_IFACE = "org.freedesktop.DBus.ObjectManager"
BLUEZ_SERVICE_NAME = "org.bluez"
GATT_CHRC_IFACE = "org.bluez.GattCharacteristic1"
DBUS_PROP_IFACE = "org.freedesktop.DBus.Properties"
UUIDS = {
    "765ffe6c-5b87-4541-a2ef-eb3bdd46a462": "frames",
    "061746ad-f829-4669-a1f7-2f3dec24cf00": "reset",
    "03978775-348d-4956-aa1c-7885468f1be8": "palettes",
}


def acquire_write(iface):
    fd, mtu = iface.AcquireWrite({})
    rv = fd.take()
    return rv


def find_service() -> Characteristics:
    bus = dbus.SystemBus()
    om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, "/"), DBUS_OM_IFACE)
    objects = om.GetManagedObjects()
    paths = {}
    for path, interfaces in objects.items():
        for interface, _ in interfaces.items():
            if interface == GATT_CHRC_IFACE:
                chrc = dbus.Interface(
                    bus.get_object(BLUEZ_SERVICE_NAME, path), DBUS_PROP_IFACE)
                uuid = chrc.Get(GATT_CHRC_IFACE, "UUID")
                name = UUIDS.get(uuid)
                if name:
                    paths[name] = dbus.Interface(
                        bus.get_object(BLUEZ_SERVICE_NAME, path), interface)
    if len(UUIDS) != len(paths):
        print(paths)
        exit("bluetooth device not found")
    return Characteristics(**paths)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--sp", type=float, default=1.0,
                        help="Set point for the throughput controller")
    parser.add_argument("-p", "--kp", type=float, default=-108.0,
                        help="Proportional gain for throughput controller")
    parser.add_argument("-i", "--ki", type=float, default=-40.0,
                        help="Integral gain for throughput controller")
    parser.add_argument("-d", "--kd", type=float, default=-108.0,
                        help="Derivative gain for throughput controller")
    parser.add_argument("-f", "--initial-fps", type=float, default=35.0,
                        help="FPS for the first second")
    parser.add_argument("-t", "--initial-stack-size", type=int, default=140)
    parser.add_argument("-c", "--constant-fps", action="store_true",
                        help=("Disables the throughput controller and"
                              " keeps the initial fps"))
    args = parser.parse_args()
    chrs = find_service()
    chrs.reset.WriteValue(b"1", {})
    doomtime_client.run(
        fd=acquire_write(chrs.frames),
        pal_fd=acquire_write(chrs.palettes),
        sp=args.sp,
        kp=args.kp,
        ki=args.ki,
        kd=args.kd,
        initial_fps=args.initial_fps,
        initial_stack_size=args.initial_stack_size,
        use_pid=not args.constant_fps,
        to_packed_rgb_444=lambda x: rgb_to_packed_rgb444(x).tobytes(),
    )


if __name__ == "__main__":
    main()
