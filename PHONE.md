<div align="center">

# Povodok and the phone

**What the first Povodok and the application say to each other, to the byte.**

↑ [NIC-Strelka](README.md)

</div>

---

The phone or the computer joins the set's first Povodok alone, which gathers the whole set
([`PRINCIPLE.md`](PRINCIPLE.md), *The first Povodok gathers the set*); or, the set gathered by the
network, every Povodok, each sending what it heard itself, the commands still to the first
(*The three ways of gathering*, there). Every number of the live view's message is little-endian,
as on the radio and the card.

## The link

- **USB or Wi-Fi at 5 GHz, TCP/IP over either.** On the lead Povodok is a network adapter, USB's
  CDC-NCM; on Wi-Fi a client of the access point — a phone's hotspot, a laptop, a building's
  network — at 5 GHz alone, 2,4 GHz refused for the set's own radio. A phone's hotspot at 5 GHz
  takes the four Povodoks of the network's gathering at once, as Android's do; an iPhone's hotspot
  is not relied on for it.
- **Found by name, as a printer is.** Every Povodok announces itself on the network by mDNS and
  DNS-SD, Zephyr's own: the service `_strelka._tcp` on port 1337, the instance `Strelka 12 P1
  A1B2C3D4` — the set's own number, given with the set (64/19), the Povodok's 1–4 as written on its
  box, and the set's address; a set with no number of its own `Strelka P1 A1B2C3D4`.
  Joined to a network, the application lists every Povodok of its set without
  searching the addresses, the network's way of gathering among them; on USB the lead's one address
  is Povodok's.
- **The adapter is CDC-NCM**, the one class that Windows 11, macOS, Linux, iPhone and Android take
  without a driver — Windows 10 with its own driver chosen by hand ([Microsoft's
  list](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/supported-usb-classes);
  [Apple's developer forums](https://developer.apple.com/forums/thread/802640); [Android's
  EthernetTracker](https://android.googlesource.com/platform/packages/modules/Connectivity/+/refs/heads/main/service-t/src/com/android/server/ethernet/EthernetTracker.java)).
  Povodok gives the phone its address by DHCP, reports the link up only once its network is ready —
  iOS asks once — and carries a MAC address with the locally-administered bit clear, so that Android
  names the interface `eth`; a phone offers no serial port to an application, so on the lead it is
  this or nothing.
- **On the lead Povodok is 192.168.7.1**, the mask 255.255.255.0, and gives the phone an address
  from 192.168.7.2 by DHCP; on Wi-Fi both take theirs from the network's own DHCP.
- **The network's name and password** come over Bluetooth LE in Povodok's service mode, at every
  start, a network kept or none, so that a stable's new network is given there; Bluetooth LE goes
  off once a phone has joined Povodok, on the network or the lead, or at 64/20, and the radio is the
  set's ([`FIRMWARE.md`](FIRMWARE.md), *The service mode*). The network lost is joined again by
  itself, after 5 s and then later each time, up to a minute. Povodok advertises as `Strelka
  Povodok` with one service; its characteristics are written under encryption, the link paired
  first, so the password never goes in the clear:

| UUID | the characteristic | what |
|---|---|---|
| `5354524c-4b41-0001-8000-00805f9b34fb` | the service | |
| `…-0002-…` | the name | write, the network's name as UTF-8, 32 B at most |
| `…-0003-…` | the password | write, UTF-8, 64 B at most; none for an open network |
| `…-0004-…` | join | write u8, 1: the name and the password kept in Povodok's settings, at 5 GHz alone, and the network joined — at every later start too |
| `…-0005-…` | the outcome | read u8: 0 nothing yet, 1 joining, 2 joined, 3 refused |

- **SMP over TCP** for the commands, the settings, the downloads and the firmware: Povodok on port
  1337, the Belka numbered n on 1337 + n, the commands and the frames in
  [`COMMANDS.md`](COMMANDS.md).
- **The live view over UDP**, a message a period, below; a period lost is left lost. Povodok
  sends it to the address the application's connection on port 1337 came from, to UDP port 1337
  there, while that connection stands and a frame has come on it in the last 3 s — the
  application reads 64/14 once a second — the latest such connection when there are more; silent
  longer, the live view stops, and goes on with the next frame.
- **The lock** as [`COMMANDS.md`](COMMANDS.md) has it: the password checked by a challenge and its
  answer.

## The live view, a message a period

The head, 21 B:

| bytes | the field | what |
|---|---|---|
| 0 | the kind u8 | 1, the live view; the other kinds to come |
| 1 | the version u8 | the format's |
| 2–3 | the session u16 | the session's number, as on the card |
| 4–7 | the period u32 | the period's number, as on the card |
| 8–9 | the set's state | from the first Povodok's SYNC ([`PRINCIPLE.md`](PRINCIPLE.md), *The inside link*) |
| 10 | the SYNC's S0 | the live view set and the Belkas' item asked |
| 11 | the Povodoks heard u8 | a bit for each Povodok, 0–3, whose SYNC the first heard; bit 0 itself |
| 12–19 | the Povodoks' states, 4 × 2 B | from their SYNCs: the margin, or the measurement; zeros where not heard |
| 20 | the Belkas u8 | the entries that follow |

Then for each Belka, 5 B and its steps:

| bytes | the field | what |
|---|---|---|
| 0 | the Belka u8 | 1–63 |
| 1 | its LIVE's S0 | the steps it carries and the item its state carries |
| 2 | who gave it u8 | 0 the first Povodok heard it itself; 1–3 the Povodok that passed it or, gathered by the network, heard it and sent it itself |
| 3–4 | its state | the item its S0 names |
| 5– | its steps | 1 or 2, a quaternion of 4 B or a raw step of 12 B each |

<!-- calc:phone-live -->
With 32 Belkas raw at 100 Hz, 29 B each, a message is 949 B, 47 kB/s, 0,38 Mbit/s; up to 47 Belkas
go in one message of 1 400 B at most, more in two of the same head, the phone joining them by the
period.
<!-- /calc -->

The map of the band the application draws itself: from the Povodoks' measurements in their states
and from the Belkas' item 8, the channel each measured and its noise.

An example, period 1000, raw at 100 Hz, two Belkas:

```
the head:  1 | 1 | session 7 | 1000 | the record runs, spare 12 | raw 100 Hz, item 8
           | Povodoks 0, 1, 2 | their states | 2 Belkas
Belka 5:   5 | raw 100 Hz, item 8 | 0, heard itself | channel 17, −92 dBm | 2 steps, 24 B
Belka 9:   9 | raw 100 Hz, item 6 | 2, passed by Povodok 2 | state 0, fault 8 | 2 steps, 24 B
```
