/*

  Library for using a shift registers on the Raspberry Pi Pico (originally developed on a SN74HC595N). This library supports
  SIPO (Serial-In-Parallel-Out; SHIFTREGISTER_OUTPUT-type), PISO (Parallel-In-Serial-Out; SHIFTREGISTER_INPUT-type) and hybrid
  (both SIPO and PISO). In a hybrid configuration both SIPO and PISO registers share the clock and latch lines. 

  As the RP2040 processor is 32 bits the size of the buffers is limited to 4 octets (32 bits). This limits the amount of cascaded
  shiftregisters to 4. To change to 64 bits:
  - Change MAX_SIZEINOCTETS to 8
  - Change "uin32_t" in the code below to "uint64_t".

  A buffersize of 64 bits will work on the RP2040, but will require an additional CPU cycle per request. 

  Delays for clock and latches can be adjusted by modifying ClockDelayUS and LatchDelayUS to meet the speed of devices;
  for instance gamecontrollers use fast shiftregisters (values can be set to '1') while display controllers like the 
  HD44780 require a value of 50 usec.

  Output can be inverted by setting InvertOutput to 'true'. This can be useful for controlling relaisboards like the HW-316
  that require the output to be inverted.

  Copyright (c) 2024 Maarten Klarenbeek (https://github.com/mjklaren)
  Distributed under the GPLv3 license

*/


#ifndef MyHardwareShiftRegister
#define MyHardwareShiftRegister

#include <stdlib.h>
#include "pico/stdlib.h"


#define SHIFTREGISTER_CLOCKDELAY_US        5    // Default value; can be overwritten for slower devices.
#define SHIFTREGISTER_LATCHDELAY_US        5    // Default value; can be overwritten for slower devices.
#define SHIFTREGISTER_INPUT                0
#define SHIFTREGISTER_OUTPUT               1
#define SHIFTREGISTER_HYBRID               2
#define MAX_SIZEINOCTETS                   4    // 4 octets equals 32 bits; the Raspberry Pico RP2040 is 32 bits.


typedef struct 
{
  // Port numbers used to access the shift register, delays used when talking to the register and the length of the buffer (multiple of 8, max. 64 bits).
  uint8_t Type, ClockGPIO, DataInGPIO, DataOutGPIO, LatchGPIO, SizeInOctets;  // SizeInOctets max 4 (=32 bits).
  uint16_t ClockDelayUS, LatchDelayUS;

  // The buffer of the register - max 32 bits (4 cascaded shift registers). 
  uint32_t InputBuffer, OutputBuffer;

  // Option to invert output
  bool InvertOutput;
} ShiftRegister;


void ShiftRegisterPulseLatch(ShiftRegister *Register)
{
  gpio_put(Register->LatchGPIO, 1);
  sleep_us(Register->LatchDelayUS);
  gpio_put(Register->LatchGPIO, 0);
}


void ShiftRegisterPulseClock(ShiftRegister *Register)
{
  gpio_put(Register->ClockGPIO, 1);
  sleep_us(Register->ClockDelayUS);
  gpio_put(Register->ClockGPIO, 0);
  sleep_us(Register->ClockDelayUS);
}


void ShiftRegisterWrite(ShiftRegister *Register)
{
  // Write the individual bits from the buffer (integer) to the shift register; starting with MSB

  uint32_t WriteMask=(1 << ((Register->SizeInOctets*8)-1));
  for(uint8_t counter=0; counter<(Register->SizeInOctets*8); counter++)
  {
    if(Register->InvertOutput==false)  // Do we need to invert the output?
      gpio_put(Register->DataOutGPIO, ((WriteMask & Register->OutputBuffer)>0?1:0));
    else
      gpio_put(Register->DataOutGPIO, ((WriteMask & Register->OutputBuffer)>0?0:1));
    ShiftRegisterPulseClock(Register);
    WriteMask=(WriteMask>>1);
  }
  ShiftRegisterPulseLatch(Register);
}


void ShiftRegisterRead(ShiftRegister *Register)
{
  // Read the bits into the buffer from the shift register; starting with MSB
  Register->InputBuffer=0;

  // Set the latch port to high
  gpio_put(Register->LatchGPIO, 1);

  for(uint8_t counter=0; counter<(Register->SizeInOctets*8); counter++)
  {
    Register->InputBuffer<<=1;
    Register->InputBuffer+=(gpio_get(Register->DataInGPIO)?1:0);
    ShiftRegisterPulseClock(Register);
  }

  // All read; set the latch to low
  gpio_put(Register->LatchGPIO, 0);
}


void ShiftRegisterReadWrite(ShiftRegister *Register)
{
  // Hybrid configuration; first write to the outgoing shift register.
  uint32_t WriteMask=(1 << ((Register->SizeInOctets*8)-1));
  for(uint8_t counter=0;counter<(Register->SizeInOctets*8);counter++)
  {
    // Write the next bit
    if(Register->InvertOutput==false)  // Do we need to invert the output?
      gpio_put(Register->DataOutGPIO, ((WriteMask & Register->OutputBuffer)>0?1:0));
    else
      gpio_put(Register->DataOutGPIO, ((WriteMask & Register->OutputBuffer)>0?0:1));
    WriteMask=(WriteMask>>1);

    // Move to the next bit - pulse the clock
    ShiftRegisterPulseClock(Register);
  }

  // Ready with writing. Set the latch port to high; this also enables reading from the incoming shift register.
  // Read bits into the buffer starting with MSB
  gpio_put(Register->LatchGPIO, 1);
  Register->InputBuffer=0;

  WriteMask=(1 << ((Register->SizeInOctets*8)-1));
  for(uint8_t counter=0;counter<(Register->SizeInOctets*8);counter++)
  {
    // Read the next bit
    Register->InputBuffer<<=1;
    Register->InputBuffer+=(gpio_get(Register->DataInGPIO)?1:0);

    // Move to the next bit - pulse the clock
    ShiftRegisterPulseClock(Register);
  }

  // All read and written; set the latch to low
  gpio_put(Register->LatchGPIO, 0);
}


// "Fill" the register with either zeroes or ones.
void ShiftRegisterFill(ShiftRegister *Register, uint8_t FillValue)
{
  // Write all zeroes or all ones to the register.
  for (uint8_t counter=0; counter<(Register->SizeInOctets*8); counter++)
  {
    gpio_put(Register->DataOutGPIO,(FillValue==0?0:1));
    ShiftRegisterPulseClock(Register);
  }
  ShiftRegisterPulseLatch(Register);
}


// Update the shift register, depending on the type of circuit.
void ShiftRegisterUpdate(ShiftRegister *Register)
{
  switch(Register->Type)
  {
    case SHIFTREGISTER_INPUT:  ShiftRegisterRead(Register);
                               break;
    case SHIFTREGISTER_OUTPUT: ShiftRegisterWrite(Register);
                               break;
    case SHIFTREGISTER_HYBRID: ShiftRegisterReadWrite(Register);
                               break;
  }
}


// Create a Shiftregister struct, initialize the specified ports and set the initial value in the register.
ShiftRegister *ShiftRegisterCreate(uint8_t Type, uint8_t ClockGPIO, uint8_t DataInGPIO, uint8_t DataOutGPIO, uint8_t LatchGPIO, uint32_t InitialValue, uint8_t SizeInOctets)
{
  ShiftRegister *Register;

  // Check if a valid size of the register is requested.
  if(SizeInOctets>MAX_SIZEINOCTETS)
    return(NULL);

  // Initialize ports and set the pins as output. No error checking for now.
  gpio_init(ClockGPIO);
  gpio_set_dir(ClockGPIO, GPIO_OUT);
  if(DataInGPIO!=0)
  {
    gpio_init(DataInGPIO);
    gpio_set_dir(DataInGPIO, GPIO_IN);
  }  
  if(DataOutGPIO!=0)
  {
    gpio_init(DataOutGPIO);
    gpio_set_dir(DataOutGPIO, GPIO_OUT);
    gpio_put(DataOutGPIO, 0);
  }
  gpio_init(LatchGPIO);
  gpio_set_dir(LatchGPIO, GPIO_OUT);
  gpio_put(LatchGPIO, 0);

  // Create a struct for this shift register, set it's values and return the pointer
  Register=(ShiftRegister *)malloc(sizeof(ShiftRegister));
  Register->Type=Type;
  Register->ClockGPIO=ClockGPIO;
  Register->DataInGPIO=DataInGPIO;
  Register->DataOutGPIO=DataOutGPIO;
  Register->LatchGPIO=LatchGPIO;
  Register->InputBuffer=0;
  Register->OutputBuffer=InitialValue;
  Register->SizeInOctets=SizeInOctets;
  Register->ClockDelayUS=SHIFTREGISTER_CLOCKDELAY_US;    // Default value; can be adjusted for slower devices.
  Register->LatchDelayUS=SHIFTREGISTER_LATCHDELAY_US;    // Default value; can be adjusted for slower devices.
  Register->InvertOutput=false;                          // Default value; can be adjusted (e.g. for using relais boards).
  ShiftRegisterUpdate(Register);
  return(Register);
}

#endif
