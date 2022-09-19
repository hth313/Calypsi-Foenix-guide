#include <stdbool.h>
#include <intrinsics65816.h>
#include "foenix/interrupt.h"
#include "foenix/vicky.h"

void *origIRQVector;

__attribute__((interrupt)) void interruptHandler () {
  if ((InterruptPending.reg0 & InterruptStartOfLine)) {
    InterruptPending.reg0 = InterruptStartOfLine;
    Vicky.borderColor.red += 5;
  }
}

void main () {
  __disable_interrupts();

  // Configure Vicky
  Vicky.videoResolution = Resolution_320x240;
  Vicky.lineCompareValue0 = 50;

  Vicky.borderColor.green = 50;
  Vicky.borderColor.blue = 50;

  // Configure interrupt controller
  InterruptMask.reg0 |= InterruptStartOfLine;

  // Set vector, keeping the old one for good measure
  origIRQVector = VectorIRQ;
  VectorIRQ = interruptHandler;

  __enable_interrupts();

  while (true)
    ;
}
