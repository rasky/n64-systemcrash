## n64-systemcrash

This is a Nintendo 64 test ROM that crashes the console in several ways.
It can be used as a testsuite by emulators that want to make sure that
emulate correctly the different crashing behaviors.

With "crash", we intend a real hardware "bug" that causes a real console
to become unresponsive or otherwise frozen; sometimes, it is the CPU that
freezes, other times it might a different hardware component to become
unresponsive (eg: the RDP). Depending on the kind of crash, even the RESET
button is not sufficient to recover and a full power-cycle is required.

### Motivation

The goal is to help more emulators implementing hardware crashes. While
hardware crashes will hopefully never happen in a final, working game,
they can be extremely common during development of game hacks and
homebrew games, especially those using the old Nintendo libraries
(libultra) which basically do not shield against them.

Notice that homebrew and hack development is often affected by other kind of
"crashes" which are not real hardware crashes but rather CPU exceptions.
For instance, a common type which is seldom faithfully reproduced by emulators
is FPU exceptions. These are outside of the scope of this ROM, but they
are likely covered by [n64-systemtest](https://github.com/lemmy-64/n64-systemtest)
instead.

### How to run the ROM

The ROM runs several tests in sequence; each test is supposed to crash/hang
the console. After each test, the user is required to do a full power cycle
of the console. Do not use the RESET button: it is often not sufficient to
recover from a hardware crash; the ROM will detect attempts to press the
RESET button and warns against it.

The ROM will save the current state in SRAM, so that it can continue execution
after a power cycle, and eventually display a result screen. The need of a SRAM
is declared in the ROM header using the [Advanced Homebrew ROM Header](https://n64brew.dev/wiki/ROM_Header#Homebrew_ROM_Header_special_flags). Make sure to use a flashcart
loader or an emulator that supports this; otherwise, find a way to manually
force your tool to activate SRAM emulation while running this ROM.

If you want to abort the testsuite at any point during execution and start
from scratch, keep the Z button pressed on the first controller during
a power-cycle; this will force the ROM to discard the SRAM contents and
start from scratch again.
