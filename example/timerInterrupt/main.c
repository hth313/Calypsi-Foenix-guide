// ----------------------------------------------------
// Demo Code for a Timer Interrupt
//
// By Ernesto Contreras
// Aug 2022
// ----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <intrinsics65816.h>
#include "foenix/timer.h"
#include "foenix/interrupt.h"

static void initInterrupt(void);
static bool running;

void *origIRQVector;      // keep old interrupt vector here

static int counter = 0;
static int counter2 = 0;

void main () {
  char buf[80];
  setvbuf(stdout, buf, _IOLBF, 80);

  printf("TIMER DEMO:\nInstall IRQ routine that counts 10 seconds using Timer1 IRQ\n");
  initInterrupt();
  running = true;
  int old = counter2;

  while (running) {
    __wait_for_interrupt();   // sleep until we get an interrupt

    __disable_interrupts();   // protect access to counter2
    if (old != counter2) {
      old = counter2;
      __enable_interrupts();
      printf(" %d ",old);        // old does not need protection and has same value
      fflush(stdout);
    } else {
      __enable_interrupts();
    }
  }

  __disable_interrupts();      // protect restoring original vector
  VectorIRQ = origIRQVector;
  __enable_interrupts();
  fputc('\n', stdout);
}

__attribute__((interrupt))
void interruptHandler() {  // New routine that handles the interrupt vector
  // Reset interrupt by Reading it and writing Pending register
  InterruptPending.reg0 = InterruptPending.reg0;

  counter++;
  // take action every 60 times the interrupt is executed
  // (every second since Timer was set for 1/60th)
  if (counter >= 60) {
    counter = 0;  // reset counter
    counter2++;
  }

  if (counter2 >= 10) {
    running = false;     // tell program to terminate
  }
}


//---------------------------------------------------
// Install Timer1 Interrupt every 1/60th of a second
//---------------------------------------------------
static void initInterrupt(void) {

 // Timers count system clock transitions
 // The largest value can only be 2^24 / system clock
 // Foenix U & FMX system clock = 14318180Hz (14Mhz)
 // 16777215/14318180 = 1.17 seconds
 // Thx to @beethead at Discord for the explanation! -  // https://discord.com/channels/6919152917#21990194/824003519706562593/1016581280437112892

// ---------------------------
// Install Timer1 Interrupt
// ---------------------------
  __disable_interrupts();               // disable interrupts while modifying interrupt
  origIRQVector = VectorIRQ ;           // Copy the original Interrupt Vector
  VectorIRQ = interruptHandler;         // install the new routine
  InterruptMask.reg0 &= ~InterruptTimer1;

  printf("* Modified IRQ Mask to allow processing Timer1 IRQ\n");
  // ------------------
  // Inititalize timer1
  // ------------------
  SetTimeValue(Timer[1].value, MicrosToClockValue(1000000 / 60));
  SetTimeValue(Timer[1].compare_value, MillisToClockValue(0));
  Timer[1].reload = true;
  Timer[1].control = TimerEnable | TimerLoad | TimerCountDown;
  printf("* Configured Timer1 to generate an IRQ every 1/60th of a second\n");
  __enable_interrupts();                  // enable interrupts
}
