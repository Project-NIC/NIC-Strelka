/// NIC-Strelka's protocol as the application speaks it, in Dart alone: SMP over TCP with the
/// lock (COMMANDS.md), and the live view's messages over UDP (PHONE.md). The Python tools of
/// `tools/` are the reference; what this package does, they did first, and the Povodok
/// simulator there holds both to the same bytes.
library;

export 'src/live_view.dart';
export 'src/smp.dart';
