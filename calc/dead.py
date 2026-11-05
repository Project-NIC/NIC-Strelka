"""What has been struck from the design, and must not come back into a page.

Each entry is a pattern and what it was. `check.py` fails on any of them in a page, but for the
pages still waiting to be rewritten, listed in STALE: those it names, with what they still carry,
and it fails when one of them carries nothing struck any longer, so that the list only shrinks.
WHY.md, the page of what was set aside, is exempt.
"""

DEAD = (
    (r"Vo[žz]ak", "Povodok's old name"),
    (r"\bbeacons?\b", "the beacon, now the sync"),
    (r"\bflood", "the flood"),
    (r"Glossy", "the flood's clock"),
    (r"\bparents?\b|\bchild(ren)?\b|\bgenerations?\b|\brepeaters?\b",
     "a Belka relaying for others"),
    (r"\bhops?\b", "the beacon's repeat"),
    (r"\bNAND\b|W25N", "NAND flash on the board, the Belka's now the card and Kryska's NOR"),
    (r"half a block|half-block|\bhalves\b", "the download in halves, now in thirds"),
    (r"AN7002Q|Raytac", "Raytac's Wi-Fi module, now Fanstel's WM02C20C"),
    (r"TLV755|TPS22918|BQ25306|BQ25628|BQ25190|MAX17262", "a part replaced"),
    (r"CRC-16", "the record's CRC-16, now CRC-32"),
    (r"at every power-on", "the gyro's zero at power-on, now after the warm-up"),
    (r"in three parts|in thirds|a third of the block", "the download in thirds, now ESB and SMP"),
    (r"start (at|from) (a )?period|stop at (a )?period|multiple of 16",
     "the start and the stop by a period's number, now the sync's state"),
    (r"\b8[01] channels|band of 8[01]\b|channels? 0–80\b",
     "the band in 1 MHz steps, now 40 channels of 2 MHz"),
    (r"288 µs", "the slot without the period's number, now 300 µs"),
    (r"1–64|holds? 64\b|holding 64\b", "Belkas numbered 1–64, now 1–63"),
    (r"\bbackground", "the background's item, now the Belka's state in every packet's head"),
    (r"live packet|search sync", "the packets' old names, now SYNC, LIVE, CALL and HERE"),
    (r"\bTLS\b", "TLS to the phone, struck: the data not sensitive, the set guarded by signatures"),
    (r"the next channel|moves a period early", "the channel announced without the count, now the "
     "spare channel and the move counted down"),
    (r"SPIM00|\bP2\.0[0-9]\b", "the card on SPIM00's pins on P2, now SPIM20 on P3"),
    (r"\bperiod 3\b|\bu24\b", "the period of 3 B on the air, now 4 B"),
    (r"who to whom", "the field's old name, now who"),
    (r"Povodok (fitted )?with the sensor", "Povodok measuring too"),
    (r"(?<!-)\btimeslots?\b|ESB_MPSL_TIMESLOT",
     "ESB in MPSL's timeslots, now esb_init() and esb_disable() every period"),
    (r"no one documents", "ESB's turns undocumented, now Nordic's way"),
    (r"NCP361|BQ25170|TPS62933|LM66200|MAX17055|BAT54",
     "the supply set by resistors alone, now the nPM1300"),
    (r"\bVBUCK\b|ideal diodes?|(?-i:(?<!/)\bHOLD\b)|P1\.23|CHG_STAT|CHG_PG",
     "the buck at 3,93 V, the ideal diodes as the switch, the hold and the charger's pins, now the "
     "PMIC's power path, ship mode and interrupt"),
    (r"\bBudk", "the box's old name, now Korabl"),
    (r"\bDezik\b|\bCygan\b|first boards, Zvjozd", "the kits' and the prototypes' names: the test "
     "boards are not described"),
    (r"two fittings|fitted two ways|the board it shares with Povodok|one board for both units",
     "one board fitted two ways, now three base boards of one core"),
    (r"Semjorka[^.|\n]{0,40}\bboards?\b|\bboards?\b[^.|\n]{0,20}\[?Semjork",
     "Semjorka as the board, now Materinka; Semjorka the set's case"),
)

ALLOWED = (
    "the access point's beacon",   # Wi-Fi's own beacon, IEEE 802.11d
    "Lea and Flood",               # SplitMix64's authors, PRINCIPLE.md's search
)

EXEMPT = ("WHY.md",)

STALE = (                  # pages still to be rewritten; each leaves the list when it is
)
