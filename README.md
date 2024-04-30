Classes for reading various disk image formats
===============

Work in progress...

Does not use GNU autotools for configure, instead goofy shell script...sorry!

Standalone, does not require any libraries other than standard C++

disklib_test.cpp is trivial tester.

Read only for now, no write support.

Currently reads entire image into memory.

Sub-directory support is minimal, if any.

Some support for partition tables.

Most are unfinished, many are just placeholders.

Readers for:
+ Acorn
+ Apple2 DOS 3
+ ProDOS
+ Atari 8 bit
+ Commodore 8 bit
+ Various CP/M
+ Some emulator formats
+ FAT (some LFN, different # of bits)
+ Local filesystem (not terribly useful standalone!)
+ Classic Mac HFS (not plus)
+ Classic MacOS partition table.
+ MBR partition table.
+ TRS80 z80 based machies.
