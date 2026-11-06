# sc2.5-canBreakoutUpdater

Zephyr firmware for the CAN breakout boards, with firmware
updates delivered over Ethernet.

## What it does

The board runs an application on top of MCUboot with two image slots. A
background task brings up the Ethernet interface and listens on UDP for a new
firmware image. When a host pushes an image, the task writes it into the spare
slot, asks MCUboot to boot it, and reboots; on the next boot the new image runs,
confirms itself, and becomes permanent (a bad image is rolled back automatically
by MCUboot). The peripheral drivers (CAN, ADC, DAC, PID, INA281, thermistor)
come from the `embedded-pio` submodule, and the board definitions from the
`boards` submodule. Target boards are the Nucleo H753ZI and L432KC BSP variants.

`updateBoard.py` is the host-side tool that pushes a signed image to a board
over that UDP protocol.

## Using `updateBoard.py`

Pushes an MCUboot-signed image to a running board over UDP, waiting for an
acknowledgement after every packet.

### Network setup

The board and host must be on the same subnet with these fixed addresses:

| | Address | Port |
|---|---|---|
| Board (MCU) | `192.168.1.1` | `12345` |
| Host (PC)   | `192.168.1.2` | `12345` |

The board only accepts packets **from** `192.168.1.2:12345`, so the host must
send from that exact address and port — pass `--bind 192.168.1.2` (the script
binds the same port it sends to). Give the host NIC the static IP
`192.168.1.2 / 255.255.255.0`. No gateway is needed.

### Assigning the host IP (Windows)

The script does **not** configure your network adapter — it only sends from an
address that must already exist on it. You have to assign `192.168.1.2` to the
Ethernet adapter yourself before running the script, or the `--bind` will fail
with an "address not available" error. Do this once per machine.

**Using Settings (Windows 11):**

1. Plug the board into the host's Ethernet port (directly, or through the same
   switch/USB-Ethernet adapter).
2. Open **Settings → Network & internet → Ethernet**.
3. Click the adapter the board is on, find **IP assignment**, and click **Edit**.
4. Choose **Manual**, turn **IPv4** on, and enter:
   - IP address: `192.168.1.2`
   - Subnet mask: `255.255.255.0`
   - Gateway / DNS: leave blank
5. Save.

**Or using PowerShell (run as Administrator):**

```powershell
# find the adapter name (e.g. "Ethernet", "Ethernet 2")
Get-NetAdapter

# assign the static IP (replace "Ethernet" with your adapter's name)
New-NetIPAddress -InterfaceAlias "Ethernet" -IPAddress 192.168.1.2 -PrefixLength 24
```

**Verify** the board is reachable before updating:

```powershell
ping 192.168.1.1
```

A reply means the link and addressing are correct and you can run the update. If
it times out, re-check the cable, the adapter you configured, and that the board
is powered and running. (A USB-Ethernet adapter must also have its driver
installed and not be set to a fixed speed/duplex that the board can't match.)

To undo the static IP later, set the adapter's IP assignment back to
**Automatic (DHCP)** in Settings, or run
`Remove-NetIPAddress -InterfaceAlias "Ethernet" -IPAddress 192.168.1.2`.

### Which file to send

Send the **signed** application image for the target, not the raw `.bin`:

```
build/sc2.5-canBreakoutUpdater/zephyr/zephyr.signed.bin
```

(or the equivalent path under whatever `--build-dir` you built into). The image
path is required — there is no default.

### Command

```bash
py updateBoard.py build/sc2.5-canBreakoutUpdater/zephyr/zephyr.signed.bin --bind 192.168.1.2
```

### Arguments

| Argument | Default | Meaning |
|---|---|---|
| `image` | *(required)* | Path to the signed image to send. |
| `--ip` | `192.168.1.1` | Board address. |
| `--port` | `12345` | Board UDP port (host binds the same port). |
| `--bind` | *(none)* | Host IP to send from; set to `192.168.1.2`. |
| `--pad` | `0xFF` | Fill byte for the final chunk's trailing bytes. |
| `--warmup` | `1.5` | Seconds to wait before the handshake, so the board has armed its receiver. |
| `--ack-timeout` | `15` | Seconds to wait for each reply/ACK. Must exceed the board's slot erase. |
| `--dry-run` | off | Read the image and print its size, send nothing. |

### What it does on the wire

1. **REQUEST_BLOCK_SIZE** — asks the board for its flash block size and chunks
   the image to match, so one packet fills one flash block on any target.
2. **FILE_SIZE** — sends the image length; the board erases the spare slot and
   acknowledges when the erase is done (this is why `--ack-timeout` must be
   generous).
3. **SENDING_FILE** — streams the image one block per packet, waiting for an ACK
   before sending the next (so it never outruns the board's flash writes).
4. **FILE_SENT** — tells the board the transfer is complete; the board finalizes
   the update and reboots into the new image.

A typical run prints the queried block size, a per-packet progress line, and a
final summary. If the board reports an error or stops acknowledging, the script
aborts with a message rather than leaving a half-written image (MCUboot will
keep running the current firmware in that case).

### Expected output

```
image      : build/sc2.5-canBreakoutUpdater/zephyr/zephyr.signed.bin
size       : 133288 bytes
dest       : 192.168.1.1:12345  from 192.168.1.2

warmup 1.5s ...
<- block   512 B
packets    : 261 x 512 B
-> FILE_SIZE  133288 (waiting for erase-done ACK, up to 15s)
<- ACK     erase done, streaming
-> data    0/261  off 0x000000
...
-> data  260/261  off 0x020800

sent 133288 bytes in 5.2s. Board finalizes and reboots.
```

## TODO

- **Update over CAN** — deliver firmware images to the breakout board over the CAN bus in addition to Ethernet, for targets without an Ethernet connection.
- **Wi-Fi shield support** — accept over-the-air updates through a Wi-Fi shield, so the board can be updated wirelessly rather than over a wired link.
- **Backup image in a third slot** — keep the previous MCU image in a 3rd slot as a backup, and add restore logic to roll back to it when needed.
- **Reboot tracking with alive signals** — confirm the full update process completes by tracking liveness: an alive signal from the MCU on the CAN bus, and an alive Ethernet message sent by the breakout board on wake-up, watched by the Python script.
