**********
Interrupts
**********

In this chapter we will look at interrupts, what they are, how they
work, why we may want to use them and best practices.

Interrupts or polling
=====================

Imagine that you want to check if a sprite has collided or data is
received at the serial port. Maybe we want to change some graphics
assets half way into drawing the full screen, how would we do it?
We can of course go around and as the graphics chip if there has been
a collision, we can check the serial port register and we can keep
track of scan lines to see when it is time to change the graphics
assets. This is what is normally called *polling*, we poll each and
everything we are interested in at regular intervals.

There are some potential drawback with polling, we waste processing
resources to check if something has happened that maybe does not
happen so often and it can be tricky to intervene such checking code
with lengthy calculations in the application.

Instead of polling for events, would it not be better to be told that
something we are interested in has occurred? We can just run our
lengthy calculation, and be *interrupted* when something we are
interested in occurred. We process the interrupt and resume the normal
application code with the lengthy calculation after we are done the
interrupt.

.. note::

   Polling is much simpler to program than interrupts and there is
   nothing wrong making things simple if it works in your
   application. If your main program has nothing special to do, it can
   just  poll for events and then go off and handle it quickly, then come
   back polling for the next event. However, we may want to use
   interrupts if we have long calculations or need quick response
   time.

How it works
============

Most processors have the ability to service interrupts. Typically
external hardware triggers an interrupt by communicating with the CPU.
This can be a simple mechanism with a single input that changes level,
e.g. 6502 or it may be more elaborate bus transaction where the
external hardware provides a vector number, e.g. 68000.

When the CPU is requested to service an interrupt, it typically
follows a sequence such as:

1. Check if the interrupts is allowed to execute. There may be an
   enable bit for it as on the 6502 and 65816. On the 68000 there is a
   current interrupt level that can only be interrupted by an
   interrupt with a higher priority. If the interrupt is currently
   blocked, it will wait until it is permitted to run.
2. When the interrupt is accepted, the CPU pushed some few registers
   on the stack and disabled further interrupts by raising the
   interrupt level or disabling further interrupts. The registers
   pushed are typically the PC (program counter) and the status
   register which holds the interrupt level or disable bit.
3. The active interrupt vector is fetched and put in PC. This is the
   address of the interrupt service routine.
4. As PC is loaded with the address of the interrupt service routine,
   execution continues there. Such routine may not alter any
   registers, as doing so could disrupt the code we executed prior to
   the interrupt, we want to continue there later as if nothing have
   happened. To do this the interrupt routine will typically push some
   additional registers it is going to use to ensure it can restore
   the full CPU state when done.
5. When the interrupt routine is done, there is typically some
   instructions to restore the registers it pushed at the beginning
   and then the routine exits using a special return instruction that
   restores the registers that were pushed by the CPU before entering
   the interrupt service routine.
6. Execution continues outside in the main code as if nothing has
   happened.

Using C for interrupts
======================

The compiler allows for writing interrupt service routines in C. They
are very much like a normal function, but they do not return any
result and do not take any arguments. You attach an interrupt
attribute to an interrupt function and the compiler will generate code
to preserve registers properly and use the correct way of returning
from the interrupt.

Best practices
==============

As with anything, there are best practices that it is good to be aware
of. These are not rules you must follow, but they are good to
understand and you can then decide if you want to do it differently.

Make them fast
--------------

Normally you want interrupt routines to be quick as spending excessive
time in interrupts prevent other interrupts from happening. A quick
interrupt provides low *latency*, allowing multiple interrupts to be
serviced fast.

To make an interrupt fast, do as little processing as possible. If you
need to reload some hardware registers it can typically be done
quickly in the interrupt routine. Implementing a complete animation
in the interrupt may not be the best idea. If a collision happened,
you may want to update some state, e.g. remove a star ship that was
blown up, rather than doing that, it may be better to set some flag or
status that it happened and let the main code handle the state
transformations.

Another thing to think about is that making a function calls from an
interrupt can be costly. The 68000 can handle it well, it is more
expensive on the 65816 and especially the 6502. This is because the
interrupt must save additional registers due to calling conventions
that typically specify that certain registers are ``scratch'' in a
function call, which means that they may get clobbered. Execution in
the main code does not suffer from this as such functions take
advantage of these registers to provide better code. In interrupts
they need to be preserved at the start of the interrupt routine if a
function call is made.

You can avoid this overhead by not making function calls from the
interrupt, or if you do, ensure that such functions are small and can
be (and are) inlined as inline expansion is just that, expanding code
in place rather than making a call.

Do not call non-reentrant functions
-----------------------------------

If you do call functions from an interrupt, make sure that the
function you call is reentrant. Many C library functions are not
reentrant and calling them from an interrupt as well as from the main
code may corrupt its internal state, causing hard to debug problems.

One function that is not a good idea to call from an interrupt is
``printf()``. There are several reasons for this:

1. ``printf()`` will typically output data and will often make use of
   internal buffers. Writing to such buffers involves updating indexes
   or pointers related to the buffer and such code is typically not
   reentrant, it may have several machine instruction related to such
   updates and if it is interrupted by a routine which also calls
   ``printf()`` you can imagine that using those buffer related
   variables when they are already half updated, can cause corruption
   and strange problem in its state.
2. A large routine such as ``printf()`` can consume a significant
   amount of stack and calling that in an interrupt means that
   depending on when the interrupt occurs, it may or may not grow the
   stack excessively. It may be hard to track down problems related to
   interrupts that consume significant amount of stack.
3. As mentioned earlier, you want your interrupt routine to be fast
   and ``printf()`` is not a very fast routine. It may fill buffers
   with data but it may also flush out such buffers which means the
   text it produced will go somewhere and this may cause communication
   with some display device or sending data elsewhere which may take
   time.

Protect common resources
------------------------

We saw that ``printf()`` has internal state that may be corrupted if
called from both the main code as well as from an interrupt
routine. Even if we make out interrupt routine simple and fast, we may
want to update some small shared variables for later inspection by the
main code. We have to be careful with such variables as they may
involve multiple instructions as well.

For interrupts this is normally not a problem as we seldom have
interrupts that interrupt and that are both sharing the same
resource. It is a different story in the main code. Consider:

.. code::

   volatile int shared_state;

   void inspect() {
     if (shared_state) {
        shared -= 1;
     }
   }

   __attribute__((interrupt)) void my_handler() {
     shared_state = 2;
   }

In the case the variable ``shared_state`` is used by both the main
code and the interrupt routine. What if this variable requires
multiple machines instructions, or as in the example where it is first
tested and based on the outcome of the test it is altered. In an
interrupt happens in between, it may have undesirable consequences.
To solve this, we must protect such code by preventing the interrupt
from happening:

.. code::

   volatile int shared_state;

   void inspect() {
     __interrupt_state_t state = __get_interrupt_state();
     __disable_interrupts();
     if (shared_state) {
        shared -= 1;
     }
     __restore_interrupt_state(state);
   }

This will prevent interrupts from happening in a short sequence where
we want to keep it steady. The code is also written so that it
restores the previous interrupt state, which be good practice as if we
just blindly disabled and enabled interrupts, it may alter the
interrupt status if the ``inspect()`` function was called with
interrupt disabled.

.. note::

   It is good practice to identify variables that are shared in this
   way by interrupts and the main code. You may want such variables as
   they allow for communication between the interrupt and the main
   code, but you probably want to keep them as few as practically
   possible and keep such code sequences short and have as few of them
   as possible.

Interrupt processing
====================

An application typically spend most time in normal execution. , running
the application code. A program that performs calculations



dives into interrupts which are routines that service
external events with short notice. The normal execution flow is
suspended and the CPU enters interrupt execution.

This can be related to graphics,
such as we reach a particular scan line, there is a collision
involving a sprite, some communication hardware needs service and many
other things.

Normal program flow is interrupted by the interrupt routine which
performs its actions. When the interrupt routine is done, the
execution state is restored to what it was before the interrupt and
execution resumes in the routine that was interrupted as if nothing
happened.



Interrupts on 6502
==================

Interrupts on 65816
===================

Interrupts on 68000
===================

Interrupt overhead
==================

Interrupt controller
====================

SOF interrupt example
=====================
