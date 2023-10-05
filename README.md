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
                               PF0 │2       19│ SWCLK ────┤SWDIO  │       │
                               PF1 │3       18│ PA10      │   3.3V├───╮   │
                              NRST │4       17│ PA9       ╰───────╯   │   │
    VCC ── [WKUP Button] ──╮  VDDA │5───────16│ VCC ──────── VCC ─────╯   │
    GND ── [Button 1] ─────┴── PA0 │6       15│ GND ──────── GND ─────────╯
    GND ── [Button 2] ──────── PA1 │7       14│ PB1
    GND ── [Button 2] ──────── PA2 │8       13│ PA7
    GND ── [LED 1] ── [1K] ─── PA3 │9       12│ PA6
    GND ── [LED 2] ── [1K] ─── PA4 │10      11│ PA5 ── [1K] ── [LED 3] ── GND
                                   ╰──────────╯
```
### IMPORTANT:
While experimenting, the chip may be forced into a sleep state such that it cannot be
detected by the ST-LINK programmer. In such cases, press and hold down the reset button
(i.e. ground the reset pin), initiate the program upload. When the upload pauses because
the reset button is pressed, release the reset button to allow the program to upload.<br>
While in Standby mode, it may be difficult to upload even by pressing the reset button. 
In such cases, hold the Boot pin (pin 1) high (VCC), then press the Reset button, and
then proceed with the upload (while keeping the Boot pin high.) Release the Boot pin after
the upload is complete.

### See ```main.c``` for additional details
