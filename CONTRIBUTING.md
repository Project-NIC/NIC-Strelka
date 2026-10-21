<div align="center">

# Contributing

**How a change comes into NIC-Strelka.**

↑ [NIC-Strelka](README.md)

</div>

---

This repository holds the latest version, one commit; the work between versions goes on in a
private repository, and each version replaces the commit here. A change is welcome all the same:
open an issue or a discussion with what you found, or a pull request against `main` — it is read,
taken into the work, and comes out with the next version, credited in that version's commit.

## The pages

- **A page states the design alone**: what is, not what was or what else was weighed. What was
  set aside and why goes into [`WHY.md`](WHY.md); what is not yet decided into
  [`STATUS.md`](STATUS.md), *What is open*.
- **Every fact cites its source** — the data sheet with its revision, the standard, the paper —
  and gives the figure as the source gives it. A figure computed is computed by a script in
  [`calc/`](calc/), whose output the page carries between its markers.
- **The form**: English; lines of 100 characters at most outside tables and links; the decimal
  comma and a space before the unit (3,2 V, 720 mAh); names as [`NAMES.md`](NAMES.md) has them.
  No prices and no stock: they change faster than the pages.
- **The three READMEs go together**: a change to `README.md` is made in `README.cs.md` and
  `README.ru.md` too, in the same place.
- `python3 calc/check.py` holds the pages to this — the computed tables against the scripts, the
  widths, the links, the words the pages do not use — and runs in CI on every push.

## The code

- **The firmware's common code**, `firmware/common/`, is plain C and runs on the host:
  `python3 tools/test_firmware.py` builds it and holds it to the Python tools, byte for byte.
- **The Zephyr code** builds against nRF Connect SDK v3.4.0 and has run on no hardware;
  [`firmware/README.md`](firmware/README.md) says how it is built. A file that has not run says
  so in its head, and a change to it is built before it is sent. A change that has run on a board
  is the best news there is: say which board, and what you saw.
- **The tools**, `tools/`, have their tests beside them; **the application**, `app/`, its own,
  listed in [`app/README.md`](app/README.md).
- A change to the design and a change to the code go together: the page first, the code to it.

## What helps most

What is open is listed in [`STATUS.md`](STATUS.md); what the firmware still lacks in
[`firmware/README.md`](firmware/README.md); what the application lacks in
[`app/README.md`](app/README.md). A board on a desk — Nordic's nRF54LM20 DK — is what the design
waits for most.
