# ShiftRegister
C-library to interface with shiftregisters on the Raspberry Pi Pico (or compatible boards). This library was originally developed on a Raspberry Pi Pico using 74HC595 and 74HC165 shift registers. It supports SIPO (Serial-In-Parallel-Out; SHIFTREGISTER_OUTPUT-type), PISO (Parallel-In-Serial-Out; SHIFTREGISTER_INPUT-type) and hybrid configurations (both SIPO and PISO). In a hybrid configuration both SIPO and PISO registers share the clock and latch lines. 

As the RP2040 processor is 32 bits the size of the buffers is limited to 4 octets (32 bits). This limits the amount of cascaded shiftregisters to 4. To change to 64 bits:
  - Change MAX_SIZEINOCTETS to 8
  - Change "uin32_t" in the code to "uint64_t".

A buffersize of 64 bits will work on the RP2040, but will require an additional CPU cycle per request. 

An example application is provided to control generic 8 bit controllers/"joysticks", like the legacy 8-bit Gameboy controller. Check the wiring diagram below:

<img width="322" alt="Wiring diagram" src="https://github.com/mjklaren/ShiftRegister/assets/127024801/2a9b6e51-51ac-4120-90fc-d81baf549a61">

The source code is distributed under the GPLv3 license.
