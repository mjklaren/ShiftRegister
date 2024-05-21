/*
   Library to read input from a generic 8 bit game controller with a DE9 serial connector, using the ShiftRegister library.

   Wiring diagram:

   Rasperry             DE9 9-pins           
   Pi Pico              connector  
   ========             ==========    
   DATA_GPIO  ------------- (2)
   LATCH_GPIO ------------- (3)
   CLOCK_GPIO ------------- (4)
                            (6) ------ +5V
                            (8) ------ Ground


  Remarks:
  - The controller only detects one key press at a time. When multiple keys are pressed simultanously only the first is registered.
  - When a key is held the controller returns the GC8BIT_KEY_HELD value. This should be interpreted as the last registered key
    still being pressed down.
  - When a key is released a different value is returned by the game controller (e.g. GC8BIT_UP_RELEASED). Only the value 
    GC8BIT_RIGHT_RELEASED is not known as it is most likely identical to GC8BIT_KEY_HELD (0).
  - When either the A or B key is pressed the GC8BIT_AB value is returned. Only when the A or B key is released the return value 
    identifies the key that was pressed (e.g. GC8BIT_A_RELEASED).

  In applications it should be sufficient to process the return values GC8BIT_UP, GC8BIT_DOWN, GC8BIT_LEFT, GC8BIT_RIGHT,
  GC8BIT_SELECT, GC8BIT_START, GC8BIT_A_RELEASED and GC8BIT_B_RELEASED. For practical, low speed purposes a delay between polls
  of upto 100msec should be acceptable.
*/

#include "ShiftRegister.c"


// Return values for the 8 bit game controller.
#define GC8BIT_DELAY             1  // Shift register delay in usec
#define GC8BIT_NOKEY             255
#define GC8BIT_KEY_HELD          0
#define GC8BIT_UP                240
#define GC8BIT_UP_RELEASED       7
#define GC8BIT_DOWN              248
#define GC8BIT_DOWN_RELEASED     3
#define GC8BIT_LEFT              252 
#define GC8BIT_LEFT_RELEASED     1
#define GC8BIT_RIGHT             254 
#define GC8BIT_RIGHT_RELEASED       // Unknown?
#define GC8BIT_SELECT            192
#define GC8BIT_SELECT_RELEASED   31
#define GC8BIT_START             224
#define GC8BIT_START_RELEASED    15
#define GC8BIT_AB                128
#define GC8BIT_A_RELEASED        63    
#define GC8BIT_B_RELEASED        127


uint8_t GC8BitPoll(ShiftRegister *Controller)
{
  ShiftRegisterUpdate(Controller);
  return((uint8_t)Controller->InputBuffer);  
}


ShiftRegister *GC8BitInit(uint8_t ClockGPIO, uint8_t DataInGPIO, uint8_t LatchGPIO)
{
  // Create the struct needed for read the NES game controller.
  ShiftRegister *Controller=ShiftRegisterCreate(SHIFTREGISTER_INPUT,ClockGPIO,DataInGPIO,0,LatchGPIO,0,1);

  // Alter the clock speed for reading of the shift register - when needed. The shift register of the game controller is reasonably fast.
  Controller->ClockDelayUS=GC8BIT_DELAY;
  Controller->LatchDelayUS=GC8BIT_DELAY;

  // Return the pointer to the shift register that controls communication to the display.
  return(Controller);
}


