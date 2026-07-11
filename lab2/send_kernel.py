#!/usr/bin/env python3

import os
import struct
import sys
import termios

MAGIC = 0x544F4F42
MAX_SIZE = 16 * 1024 * 1024


def write_all(fd, data):
    view = memoryview(data)

    while len(view) != 0:
        sent = os.write(fd, view)

        if sent <= 0:
            raise RuntimeError("serial write failed")

        view = view[sent:]


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <serial-device> <payload.bin>")
        sys.exit(1)

    serial_device = sys.argv[1]
    image_path = sys.argv[2]

    with open(image_path, "rb") as image_file:
        image = image_file.read()

    if len(image) == 0:
        raise RuntimeError("payload is empty")

    if len(image) > MAX_SIZE:
        raise RuntimeError(
            f"payload is too large: {len(image)} bytes "
            f"(maximum {MAX_SIZE} bytes)"
        )

    header = struct.pack("<II", MAGIC, len(image))
    packet = header + image

    print(f"Sending {len(image)} bytes to {serial_device}...")

    fd = os.open(serial_device, os.O_WRONLY | os.O_NOCTTY)

    try:
        write_all(fd, packet)
        termios.tcdrain(fd)
    finally:
        os.close(fd)

    print("Transfer completed.")


if __name__ == "__main__":
    main()
