"""Every input of Strelka's level-4 calculations, each with its source or marked as assumed.

Every number has one home: this file. The scripts beside it derive from it; nothing derived is
typed into a page by hand.
"""

# --- The record -----------------------------------------------------------------------------
STEP_BYTES = 12        # the turn and the mean force of one step, six i16 (the design)
BLOCK_BYTES = 512      # a block, a sector of the card, laid out after CWA's AX block (cwa.h)
DATA_HEAD = (          # a data block's head and check, its fields and bytes; 40 steps between
    ("the kind", 1),
    ("the steps in it", 1),
    ("the session", 2),
    ("the block's number", 4),
    ("the period of its first step", 4),
    ("the block's place in the period, from 4 kHz", 1),
    ("the steps missing", 1),
    ("the steps with the accelerometer out of range", 1),
    ("the steps with the gyro out of range", 1),
    ("the first step's offset from its place, ns", 4),
    ("free", 8),
    ("a CRC-32", 4),
)
INFO_EVERY_STEPS = 120     # an information block after every 120 steps, three data blocks, at any
                           # rate (RECORD.md)
INFO_A_STEP_BYTES = 4      # in it, each step's offset i16 ns, the sync's signal i8 dBm, its
                           # Povodok and the syncs heard, a nibble each
RATES_HZ = (50, 100, 200, 400, 500, 1000, 2000, 4000, 8000, 16000, 32000)  # into memory, raw
                       # from 4 kHz, 32 kHz a short trial mode; nothing under 50 Hz
DEFAULT_HZ = 100       # about 100 Hz into memory (the README)

# --- The set --------------------------------------------------------------------------------
BELKAS = (32, 63)          # in use up to 32, the firmware holding 63 (the README)
POVODOKS = 4               # one to four, each its own sync in a slot of its own (the README)

# --- The inside link, the Povodoks' syncs and the Belkas' slots ------------------------------
PHY_BIT_S = 2e6            # the proprietary mode at 2 Mbit/s, Bluetooth's 2M on air; 4 Mbit/s set
                           # aside, outside the module's grant (WHY.md)
PERIOD_S = 0.020           # one period, its syncs and its slots (PRINCIPLE.md, the inside link)
PACKET_HEAD_BYTES = 15     # every packet of the set's own frame before its body: the preamble 2 B
                           # (PCNF0.PLEN at 16 bits), the address 4 B, who 1 B, the length 1 B,
                           # S0 1 B, the period's number 4 B, the state 2 B (PRINCIPLE.md, the
                           # inside link)
CRC_BYTES = 3              # the radio's CRC (PRINCIPLE.md, the inside link)
SYNC_BODY_BYTES = 4        # a SYNC's body: a signature by AES-CCM from the set's key and the
                           # period, so that no one forges it (PRINCIPLE.md, the inside link)
SYNC_GUARD_S = 30e-6       # the guard before the first SYNC (PRINCIPLE.md)
TURN_S = 50e-6             # from receiving to sending: the receiver off in 1 µs, the fast ramp-up
                           # 40 µs, nRF54LM20A datasheet v1.0, 11.14.8; the rest a margin
SLOT_GAP_S = 36e-6         # between two Belkas' packets, the reserve against their drift
                           # (PRINCIPLE.md)
LIVE_OFF_EVERY_S = 1.0     # the live view off, a LIVE with the Belka's state alone once a second
QUATERNION_BYTES = 4       # a turn, "smallest three", 2 bits and three of 10 bits (PRINCIPLE.md)
LIVE_HZ = (50, 100)        # the live view a Belka, chosen per session; up to 100 Hz, so that the
                           # period leaves room for the PASSes (the README)
MISS_BYTES = 8             # a MISS's body: a bit for each Belka, 1-63, the first Povodok missed
PASS_EACH_BYTES = 4        # in a PASS's body, before each missed Belka's steps: its number 1 B, its
                           # LIVE's S0 1 B and its state 2 B
BODY_MAX_BYTES = 249       # a packet's body at most: the radio's length field of 8 bits counts the
                           # period 4 B, the state 2 B and the body, 255 B at most
MEASURE_S = 0.2e-3         # assumed: at the period's end, a Povodok's turn to another channel, its
                           # noise read, and back
CHANNELS = 40              # 2402-2480 MHz by 2 MHz, a channel as wide as 2 Mbit/s takes: Bluetooth
                           # LE's 40, the Core Specification, vol 6, part A, 2
SET_CHANNELS = 39          # the set works on the first 39; the 40th, 2480 MHz, is the search's alone
SEARCH_ID_BYTES = 8        # a HERE's body, a new Belka's answer to a CALL: its chip's ID whole
SEARCH_NUMBERS = 65536     # the number a new Belka draws from its ID, 16 bits
LIVE_MSG_HEAD_BYTES = 21   # the live view's message to the phone, its head (PHONE.md)
LIVE_MSG_EACH_BYTES = 5    # in it, before each Belka's steps: its number, its S0, who gave it, its
                           # state
UDP_MAX_BYTES = 1400       # a message at most: under the 1 472 B a 1 500 B MTU leaves after the IP
                           # and UDP heads, with a margin
USB_BIT_S = 480e6          # high-speed USB: nRF54LM20A datasheet v1.0, 8.27.1
WIFI_BIT_S = 86e6          # the nRF7002's MCS7 on air, 20 MHz: Fanstel's WM02C20C
                           # specification, draft 0.82

# --- The download, by Nordic's ESB with SMP on it ----------------------------------------------
EXAMS_S = (900, 3600)      # a quarter of an hour, the least a charge must last; and an hour
ESB_PAYLOAD_BYTES = 252    # an ESB packet's payload at most: nRF Connect SDK, Enhanced ShockBurst,
                           # CONFIG_ESB_MAX_PAYLOAD_LENGTH, 1-252
ESB_HEAD_BYTES = 10        # around the payload: the preamble 2 B, the address 5 B, the length 1 B,
                           # S1's 3 bits and the CRC 2 B (nRF Connect SDK, esb.c, at 2 Mbit/s)
ESB_ACK_BYTES = 10         # the acknowledgement, the same with no payload
ESB_FRAG_HEAD_BYTES = 5    # Strelka's fragment of an SMP frame in an ESB payload: the fragment and its
                           # index 1 B, the count against a replay 4 B (COMMANDS.md)
ESB_MIC_BYTES = 4          # the fragment's tag by AES-CCM with the set's key (COMMANDS.md)
SMP_FRAME_BYTES = 2048     # assumed: an SMP frame of a file's download, some 2 KB of the record
SMP_HEAD_BYTES = 64        # assumed: the SMP header 8 B, its CBOR fields and the request asking
STATE_EVERY = 50           # one period in 50, once a second, every Belka's state in its slot: the
                           # map of which Povodok hears which Belka, renewed
DOCK_BYTE_S = 35e6         # assumed: what a high-speed USB hub carries in practice, the Belkas in the
                           # dock copied at once, each at most its card's bus

# --- The memory, the user's microSD card ------------------------------------------------------
CARD_BYTES = 8e9           # assumed: the user's card, 8 GB; SPI mode reads up to 2 TB (the SD
                           # Physical Layer Simplified Specification 9.10, 3)
CARD_BIT_S = 8e6           # the card on SPIM20 at 8 Mbit/s, the most a SPIM of the peripheral domain
                           # gives (nRF54LM20A, 11.19.1); the card itself bears 25 MHz in its default
                           # speed (ATP's industrial microSD sheet)
CARD_BUSY_S = (("SDHC", 0.250), ("SDXC", 0.500))  # the card busy at most at a write's end (the SD
                           # specification 9.10, 4.6.2.2)
RAM_BYTES = 512e3          # the processor's RAM, nRF54LM20A datasheet v1.0, table 6
CARD_BUFFER_BYTES = 128 * 1024  # the record's buffer in RAM before the card, a quarter of it
FAT32_FILE_BYTES = 2**32 - 1  # a file on FAT32 at most, 4 GB
SYNC_FILE_S = 3            # the file's length saved this often, a power cut losing so much at most

# --- The current, rough: a low and a high case; the measurement decides ---------------------
RX_MA = 3.3                # nRF54LM20A product page, receiving, 3 V
TX_MA = 10.9               # nRF54LM20A datasheet v1.0, sending at +8 dBm, 3 V; kept for the +5 dBm
                           # sent by default too, whose figure lies between it and 5,0 mA at 0 dBm
                           # (the product page)
RAMP_S = 40e-6             # the radio's fast ramp-up, counted at the receiving current (11.14.8)
IMU_MA = 0.70              # ICM-42686-P, six axes in low-noise mode, DS-000639
CPU_MA = 2.6               # the processor running, nRF54LM20A datasheet v1.0
CPU_DUTY = (0.5, 1.0)      # assumed: the share of time it runs, low and high
CARD_MA = (0.2, 3.0)       # assumed: the card idle in SPI mode between its runs, and its writes
                           # averaged; up to 100 mA while it writes (ATP, 4.1-4.2)
BOARD_MA = 0.02            # the rest of the board: the LDOs, the gauge, the Hall switches, the
                           # charger's chain (PARTS.md, the unit asleep)
LED_MA = 1.1               # a colour sunk by a processor pin through 1 kΩ (PARTS.md, the light)
LED_DUTY = (0.02, 0.10)    # assumed: green, fast blinks while measuring
WIFI_MA = (56, 260)        # the nRF7002 receiving and sending at its full power, 5 GHz:
                           # its product specification, key features
CELL_MAH = 720             # LTOPOW LFP103042, LiFePO4, its product page (the owner, 12 Oct 2026)

# --- The reach, rough: free space against the module's budget --------------------------------
FREQ_HZ = 2.44e9           # the middle of the 2,4 GHz band
TX_DBM = 5                 # the radio's power out, conducted, by default (the reach, PRINCIPLE.md)
TX_FULL_DBM = 8            # its full power, a setting: nRF54LM20A datasheet v1.0, 11.14.4
RX_SENS_DBM = -93          # its sensitivity at 2 Mbit/s Bluetooth LE, nRF54LM20A datasheet v1.0,
                           # 11.14.5 — none is given for the proprietary 2 Mbit/s mode; taken for it
ANTENNA_DB = 4.24          # the BM20F's antenna, +12,61 dBm radiated from +8,37 dBm out (the BM20C
                           # series specification v1.00, the models); not counted in the budget
EIRP_FREE_DBM = 10         # under it EN 300 328 asks neither adaptivity nor a limit to the medium's
                           # use (EN 300 328 V2.2.2, adaptivity and medium utilisation for the
                           # wideband modulations other than FHSS)
BUDGET_DB = TX_DBM - RX_SENS_DBM  # the budget at the default power
MU_MAX = 0.10              # above 10 dBm, a non-adaptive equipment's medium utilisation, (P / 100 mW)
                           # × the duty cycle, at most (EN 300 328 V2.2.2, 4.3.2.5.3)
TX_SEQ_MAX_S = 10e-3       # its Tx-sequence at most, a Tx-gap after it as long and 3,5 ms at least
TX_GAP_MIN_S = 3.5e-3      # (EN 300 328 V2.2.2, 4.3.2.4.3)
BODY_DB = 40               # a person across the path at 2,45 GHz, up to 40 dB: Cotton et al.,
                           # UCC; a cow's trunk unknown, the measurement decides
REACH_M = (                # what is reckoned: the distance, metres
    ("a Belka to a Povodok round a dog", 1),
    ("a Belka to a Povodok round a cow", 3),
    ("a Belka to a Povodok on a stable's ceiling", 5),
    ("a Belka to a Povodok in the hand, a horse trotted away", 30),
)

# --- The clock loop, rough: a simulation; the measurement on two units decides ----------------
CRYSTAL_PPM = 20           # the carrier within ±20 ppm from -40 to 105 °C with the load set to
                           # 14,5 pF: Fanstel BM20C series specification v1.00, Crystal
WANDER_PPM_MIN = 1.0       # assumed: the crystal's drift a minute as the unit warms on the animal
SYNC_ERR_S = 0.5e-6        # assumed: the sync's arrival as stamped, a bit at 2 Mbit/s; a Povodok
                           # other than the first locked on the first's sync within it
SYNC_LOST = 0.10           # assumed: the share of periods a Belka hears none of the syncs
TICK_S = 62.5e-9           # one tick of the 16 MHz peripheral clock that times CLKIN and the
                           # data-ready stamp: nRF54LM20A datasheet v1.0, 3.3, the peripheral domain
LOOP_HZ = 0.1              # Claude's choice: the loop's natural frequency, damping 0,707
ACQUIRE_S = 1.0            # Claude's choice: the first second of syncs measures the frequency
GAPS_S = (1, 5, 10)        # no sync this long, for the hold-over; 10 s stops the record,
                           # a setting (the README)
SEED = 1                   # the simulation's random numbers, fixed so that the table repeats

# --- The sensor's noise against the motion, rough -------------------------------------------
ACC_NOISE_G = 70e-6        # the accelerometer's noise a √Hz: ICM-42688-P, DS-000347 table 2
                           # (z axis); ICM-42686-P, DS-000639, the same
GYRO_NOISE_DPS = 5.3e-3    # the gyro's noise a √Hz, the worse of the two: ICM-42686-P, DS-000639
                           # table 1; ICM-42688-P 2,8e-3, DS-000347 table 1
STRIDES_HZ = (             # assumed: the stride's frequency of the gaits the vet looks at
    ("a cow at the walk", 0.8),
    ("a horse at the trot", 1.5),
    ("a dog at the trot", 2.5),
)
SIDEWAYS_G = 1.0           # assumed, generous: the trunk's sideways acceleration
TILT_S = 10                # assumed: the gyro alone this long, without the accelerometer's help
