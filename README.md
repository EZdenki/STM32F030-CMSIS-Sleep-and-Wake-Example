# STM32F030-CMSIS-Basic-Interrupts
Examples of how to set up basic interrupt routines including triggers based on button presses,
timer overflow, or the SysTick (system clock).

## Interrupt Types
The following types of interrupts are defined. They can be enabled or disabled by commenting
out the relevant #define statements near the top of ```main.c```. Multiple interrupts may be set up simultaneously.

* __BUTTON_INTERRUPT:<br>
  Interrupt generated on the rising edge of PA0 and PA1. Pressing and releasing the button
  on PA0 will cause EXTI0_1_IRQHandler to be called and will turn ON the LED attached to
  PF0. Likewise, pressing and releasing the button on PA1 will cause EXTI0_1_IRQHandler
  to be called and will turn OFF the LED.
  Note that EXTI0 and EXTI1 both result in a call to the same interrupt handler. The code
  inside the handler must determine which line actually caused the interrupt by checking
  the EXTI->PR register for the PR0 bit (for PA0) or PR1 bit (for PA1).

* __TIMER_INTERRUPT<br>
  Interrupt generated each time that TIM14 overflows.

* __SYSTICK_INTERRUPT
  Interrupt generated each time that x clock cycles have occurred, where x is a 24-bit
  number initialized in the SysTick_Config( x ) procedure. On an 8 MHz clock, the
  slowest time between calls to the interrupt handler is approx. 2 seconds when using a
  value of (uint32_t)16E6 for x.

## Hardware Setup
```
                                                          ST-Link V2 
                                  STM32F030F4xx           ╭───────╮     
                                   ╭────╮╭────╮           │    GND├───────╮
                             BOOT0 │1       20│ SWCLK ────┤SWCLK  │       │
       GND ── [1K] ── [LED] ── PF0 │2       19│ SWCLK ────┤SWDIO  │       │
                               PF1 │3       18│ PA10      │   3.3V├───╮   │
                              NRST │4       17│ PA9       ╰───────╯   │   │
                              VDDA │5       16│ VCC ──────── VCC ─────╯   │
          GND ── [Button 1] ── PA0 │6       15│ GND ──────── GND ─────────╯
          GND ── [Button 2] ── PA1 │7       14│ PB1
                               PA2 │8       13│ PA7
                               PA3 │9       12│ PA6
                               PA4 │10      11│ PA5
                                   ╰──────────╯
```

### See ```main.c``` for additional details
