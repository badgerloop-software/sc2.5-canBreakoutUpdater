#!/usr/bin/env python3
import argparse
import socket
import struct
import sys
import time

REQUEST_BLOCK_SIZE = 0
FILE_SIZE = 1
SENDING_FILE = 2
FILE_SENT = 3

DEFAULT_IMAGE = None

def chunk_image(data: bytes, pad: int, block: int):
    for off in range(0, len(data), block):
        piece = data[off:off + block]
        if len(piece) < block:
            piece = piece + bytes([pad]) * (block - len(piece))
        yield off, piece


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("image", nargs="?", default=DEFAULT_IMAGE, help=f"signed image")
    ap.add_argument("--ip", default="192.168.1.1", help="board address (MCU_IP_ADDR)")
    ap.add_argument("--port", type=int, default=12345, help="board port (LOCAL_PORT)")
    ap.add_argument("--bind", default=None, help="local IP to send from, e.g. 192.168.1.2")
    ap.add_argument("--pad", type=lambda s: int(s, 0), default=0xFF, help="fill byte for the final chunk (default 0xFF)")
    ap.add_argument("--warmup", type=float, default=1.5, help="seconds to wait before the handshake (default 1.5)")
    ap.add_argument("--ack-timeout", type=float, default=15.0, help="seconds to wait for each reply/ACK (default 15)")
    ap.add_argument("--dry-run", action="store_true", help="read the image and exit, send nothing")
    args = ap.parse_args()

    if (args.image is None):
        sys.exit("Image path not valid")
    
    try:
        with open(args.image, "rb") as f:
            image = f.read()
    except OSError as e:
        sys.exit(f"cannot read image: {e}")

    if not image:
        sys.exit("image is empty")
    if not (0 <= args.pad <= 0xFF):
        sys.exit("--pad must be a single byte 0..255")

    print(f"image      : {args.image}")
    print(f"size       : {len(image)} bytes")
    print(f"dest       : {args.ip}:{args.port}" + (f"  from {args.bind}" if args.bind else ""))

    if args.dry_run:
        print("\ndry-run: nothing sent (block size is queried from the board at run time)")
        return 0

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    bind_ip = args.bind if args.bind else ""
    sock.bind((bind_ip, args.port))
    sock.settimeout(args.ack_timeout)
    dest = (args.ip, args.port)

    def send(command: int, data: bytes = b""):
        sock.sendto(bytes([command]) + data, dest)

    def recv(what: str) -> bytes:
        try:
            reply, _ = sock.recvfrom(8)
        except socket.timeout:
            sys.exit(f"\nno reply after {what} (>{args.ack_timeout:.0f}s) - aborting.")
        return reply

    def wait_ack(what: str):
        reply = recv(what)
        if reply and reply[0] != 0x00:
            sys.exit(f"\nboard reported error 0x{reply[0]:02x} after {what} - aborting.")

    if args.warmup > 0:
        print(f"\nwarmup {args.warmup:.1f}s ...")
        time.sleep(args.warmup)

    t0 = time.time()

    send(REQUEST_BLOCK_SIZE)
    reply = recv("REQUEST_BLOCK_SIZE")
    if len(reply) < 4:
        sys.exit(f"\nbad block-size reply ({len(reply)} bytes) - aborting.")
    block = int.from_bytes(reply[:4], "little")
    if not (1 <= block <= 8192):
        sys.exit(f"\nimplausible block size {block} - aborting.")
    print(f"<- block   {block} B")

    packets = list(chunk_image(image, args.pad, block))
    print(f"packets    : {len(packets)} x {block} B")

    send(FILE_SIZE, struct.pack("<I", len(image)))
    print(f"-> FILE_SIZE  {len(image)} (waiting for erase-done ACK, up to {args.ack_timeout:.0f}s)")
    wait_ack("FILE_SIZE (erase)")
    print("<- ACK     erase done, streaming")

    for i, (off, pkt) in enumerate(packets):
        send(SENDING_FILE, pkt)
        wait_ack(f"SENDING_FILE {i}")
        if i % 32 == 0 or i == len(packets) - 1:
            print(f"-> data {i:4d}/{len(packets)}  off 0x{off:06x}")

    send(FILE_SENT)
    try:
        sock.recvfrom(8)
    except socket.timeout:
        pass

    dt = time.time() - t0
    print(f"\nsent {len(image)} bytes in {dt:.1f}s. Board finalizes and reboots.")
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
