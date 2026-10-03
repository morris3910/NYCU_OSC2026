#!/usr/bin/env python3

import os
import struct
import sys
import termios
import tty

MAGIC = 0x544F4F42
MAX_SIZE = 16 * 1024 * 1024


# ensure we sent the whole kernel
def write_all(fd, data):
    view = memoryview(data)

    while len(view) != 0:
        sent = os.write(fd, view)

        if sent <= 0:
            raise RuntimeError("serial write failed")

        view = view[sent:] # cut the part we had sent 


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <serial-device|--stdout> <payload.bin>")
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

    # "<II" means two little endian 32-bit unsigned integer
    # format : [4 bytes MAGIC][4 bytes payload size][payload binary]
    header = struct.pack("<II", MAGIC, len(image))
    packet = header + image

    # screen's `exec .!.` forwards stdout to its existing serial connection.
    # Keep status messages on stderr so they cannot corrupt the BOOT packet.
    if serial_device == "--stdout":
        fd = sys.stdout.fileno()
        old_settings = termios.tcgetattr(fd) if os.isatty(fd) else None
        try:
            # screen connects the subprocess through a PTY. Disable newline
            # conversion and flow control before sending arbitrary binary data.
            if old_settings is not None:
                tty.setraw(fd, when=termios.TCSANOW)
            print(f"Sending {len(image)} bytes through screen...", file=sys.stderr)
            write_all(fd, packet)
        finally:
            if old_settings is not None:
                termios.tcsetattr(fd, termios.TCSANOW, old_settings)
        print("Transfer completed.", file=sys.stderr)
        return

    print(f"Sending {len(image)} bytes to {serial_device}...")

    # O_NOCTTY means don't set the serial device to this process's controlling terminal
    fd = os.open(serial_device, os.O_WRONLY | os.O_NOCTTY)

    try:
        write_all(fd, packet)
        # wait for the data sent out the serial hardware, cause write_all only put the data in the serial output buffer 
        #termios.tcdrain(fd) 
    finally:
        os.close(fd)

    print("Transfer completed.")


if __name__ == "__main__":
    main()
