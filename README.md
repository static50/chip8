# CHIP-8

A CHIP-8 emulator in C using SDL2, built as a learning project.

The emulator is a work in progress. The current snapshot does not compile yet: the SDL helper declarations and the `drawDisplay` parameter type still need to be fixed, and rendering is unfinished.

The intended toolchain is GCC (C17) with SDL2. The emulator takes a path to a CHIP-8 ROM as its single argument.
