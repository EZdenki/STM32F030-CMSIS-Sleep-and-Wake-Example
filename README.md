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

## Summary of Sleep Modes
+ ### **STANDBY MODE**
  + Consumes less that 10 uA while asleep.<br>
  + Standby Mode halts all functionality and provides the lowest sleep power requirement.<br>
  + Note that upon waking up, the chip is basically in a reset state.<br>
+ **Set up Standby Mode:**
  ```
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;          // Enable PWR control clock
  SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;      // Set SLEEPDEEP bit
  PWR->CR      |= PWR_CR_PDDS | PWR_CR_LPDS;  // Set Standby mode
  ```
+ **Wake from Standby Mode:** Any of the following can be used to wake from standby-sleep:<br>
  + Ground the NRST pin (press the RESET button)
  + A rising edge of WKUP1 (PA0) pin
    WKUP1 is enabled by adding the following code to the above set up code:
    ```
     PWR->CSR |= PWR_CSR_EWUP1;  // Enable wake-up on WKUP1 (PA0)
    ```
    Note that if the WKUP pin is enabled as above, then the pin is forced into input mode with a
    built-in pulldown. so the pin must be brought up to VCC to wake the chip.
  + RTC Interrupt (Not available on this chip variant).
+ ### **STOP MODE**
  + Consumes approx. 230 uA (at 3.3 V) down to 15 uA (at 2.0 V) while asleep.
  + Standby Mode halts 1.8V domain clocks and HSI/HSE oscillators.
+ **Set Up Stop Mode:** <br>
  ```
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;      // Enable PWR control clock
  SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;  // Set SLEEPDEEP bit
  PWR->CR      |= PWR_CR_LPDS ;           // Put voltage regulater in low power mode
  ```
+ **Wake from Stop Mode:** <br>
  Any EXTI line interrupt event will wake the chip from the stop mode. Used this mode to wake from button presses.
+ ### **SLEEP MODE**
  + Consumes approx. 1.1 mA while asleep.
  + This mode saves the least amount of power (approx. 40%) but can be woken up by **any** interrupt event
+ **Set up Sleep Mode:** <br>
  ```
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;      // Enable PWR control clock
  ```
+ **Wake from Sleep Mode:** <br>
  Pretty much any interrupt event, will wake the chip up from sleep mode.

## To Put the Chip to Sleep after the Desired Sleep Mode is Set Up:
The following describes how to set up and wake from the various sleep modes. After the
sleep mode is set up, enter the sleep mode by executing the following:
```
PWR->CR |= PWR_CR_CWUF;   // Clear wake-up flag
__WFI();                  // Go to sleep
```
If all of the processing is handled via interrupt handlers, then the above would be in
an endless loop in ```main()``` as:
```
while( 1 )
{
  PWR->CR |= PWR_CR_CWUF;   // Clear wake-up flag
  __WFI();                  // Go to sleep
}
```
By doing so, the chip would be put into sleep, woken up by some event, then the event would be processed
in the event handler, and then immediately return back to sleep.

### See ```main.c``` for additional details
