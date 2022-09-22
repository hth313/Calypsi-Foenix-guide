#include <stdbool.h>
#include <intrinsics65816.h>
#include "foenix/interrupt.h"
#include "foenix/vicky.h"

void *origIRQVector;

color24_t teal = { 204, 204, 0 };
color24_t pink = { 127, 0, 255 };

bool flag;

__attribute__((interrupt)) void interruptHandler () {
  if ((InterruptPending.reg0 & InterruptStartOfLine)) {
    InterruptPending.reg0 = InterruptStartOfLine;
    if (flag) {
      Vicky.borderColor = teal;
    } else {
      Vicky.borderColor = pink;
    }
    flag = !flag;
  }
}

void main () {
  __disable_interrupts();

  // Configure Vicky
  Vicky.videoResolution = Resolution_320x240;
  Vicky.lineCompareValue0 = 100;
  Vicky.lineCompareValue1 = 150;
  Vicky.lineInterruptControl = 3;

  Vicky.borderColor = teal;

  // Configure interrupt controller
  InterruptMask.reg0 &= ~InterruptStartOfLine;

  // Set vector, keeping the old one for good measure
  origIRQVector = VectorIRQ;
  VectorIRQ = interruptHandler;

  __enable_interrupts();
  while (true)
    ;
}
