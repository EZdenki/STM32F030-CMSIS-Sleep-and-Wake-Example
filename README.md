# STM32F030 CMSIS Sleep and Wake Example
Demonstrates how to enter and wake from various sleep modes of the STM32F030 microprocessor using CMSIS directives (no HAL).<br>
<br>
Waking up uses interrupts, which were described in detail in the following repository:<br>
[STM32F030 CMSIS Basic Interrupts](https://github.com/EZdenki/STM32F030-CMSIS-Basic-Interrupts)<br>

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
