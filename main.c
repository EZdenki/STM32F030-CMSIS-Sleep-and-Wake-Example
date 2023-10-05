//  ==========================================================================================
//  main.c for STM32F030-CMSIS-Sleep-and-Wake-Example
//  ------------------------------------------------------------------------------------------
//  Examples of how to put the microcontroller in power-saving sleep mode and also how to wake
//  up using an external interrupt such as a button press. Cyclic-sleep operation is also
//  covered.
//
//  For example:
//    1. After the microcontroller is powered on or reset, automatically sleep after some
//       time. Wake upon a button press. Then after a delay, sleep again ... repeat.
//    2. After doing initial processing, go to sleep, and then wake up intermittently to do
//       periodic processing, then go back to sleep again, and repeat.
//
//  ------------------------------------------------------------------------------------------
//  https://github.com/EZdenki/STM32F030-CMSIS-Sleep-and-Wake-Example
//  Released under the MIT License
//  Copyright (c) 2023
//  Mike Shegedin, EZdenki.com
//  Version 1.0    5 Oct 2023    Updated PCB diagram, code and comments
//  Version 0.9   26 Aug 2023    Started
//  ------------------------------------------------------------------------------------------
//  Target Microcontroller and Devices:
//    * STM32F030Fxxx
//    * Buttons on PA0 (pin 6), PA1 (pin 7) and PA2 (pin 8) are tied to GND
//    * Another button also n PA0 tied to VCC to be used as the WKUP1 wake-up pin 1
//    * Button tied to NRST and then 22k resistor to 
//    * LEDs  and current limiting resistors on PA3 (pin 9),PA4 (pin 10), and PA5 (pin 11)
//    
//  ------------------------------------------------------------------------------------------
//  Hardware Setup:
//
//                                       STM32F030F4xx     
//                                        ╭────╮╭────╮
//                                  BOOT0 │1       20│ SWCLK -- [SWCLK│ST-Link V2]
//      GND -- [Reset Button] ---,    PF0 │2       19│ SWCLK -- [SWDIO│          ]
//                               |    PF1 │3       18│ PA10
//     VCC -- [WKUP Button] ---, '-- NRST │4       17│ PA9
//                             |     VDDA │5 ----- 16│ VCC -- VCC
//        GND -- [Button 1] ---'----- PA0 │6       15│ GND -- GND
//        GND -- [Button 2] --------- PA1 │7       14│ PB1
//        GND -- [Button 3] --------- PA2 │8       13│ PA7
//        GND -- [LED 3] --- [1K] --- PA3 │9       12│ PA6
//        GND -- [LED 2] --- [1K] --- PA4 │10      11│ PA5 -- [1K] -- [LED 3] -- GND
//                                        ╰──────────╯
//
//  ==========================================================================================
//  IMPORTANT:
//  While experimenting, the chip may be forced into a sleep state such that it cannot be
//  detected by the ST-LINK programmer. In such cases, press and hold down the reset button
//  (i.e. ground the reset pin), initiate the program upload. When the upload pauses because
//  the reset button is pressed, release the reset button to allow the program to upload.
//  While in Standby mode, it may be difficult to upload even by pressing the reset button.
//  In such cases, hold the Boot pin (pin 1) high (VCC), then press the Reset button, and
//  then proceed with the upload (while keeping the Boot pin high.) Release the Boot pin after
//  the upload is complete.
//  ==========================================================================================

#include "stm32f030x6.h"

//  ==========================================================================================
//  Summary of Sleep Modes
//  ------------------------------------------------------------------------------------------
//  The following describes how to set up and wake from the various sleep modes. After the
//  sleep mode is set up, enter the sleep mode by executing the following:
//
//    PWR->CR |= PWR_CR_CWUF;   // Clear wake-up flag
//    __WFI();                  // Go to sleep
//
//  If all of the processing is handled via interrupt handlers, then the above would be in
//  and endless loop as:
//      while( 1 )
//      {
//        PWR->CR |= PWR_CR_CWUF;   // Clear wake-up flag
//        __WFI();                  // Go to sleep
//      }
//  By doing so, the chip would be put into sleep, woken up by some event, process the
//  event in the event handler, and then immediately return back to sleep.
//
//
//  __STANDBY_MODE
//    Consumes less that 10 uA while asleep.
//    Standby Mode halts all functionality and provides the lowest sleep power requirement.
//    Note that upon waking up, the chip is basically in a reset state.
//
//    Set Up Standby Mode:
//      RCC->APB1ENR |= RCC_APB1ENR_PWREN;          // Enable PWR control clock
//      SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;      // Set SLEEPDEEP bit
//      PWR->CR      |= PWR_CR_PDDS | PWR_CR_LPDS;  // Set Standby mode
//
//    Exit Standby Mode -- Any of the following can be used to wake from standby-sleep:
//      1. Grounding NRST pin.
//      2. Rising edge of WKUP1 (PA0) pin:
//           PWR->CSR |= PWR_CSR_EWUP1;  // Enable wake-up on WKUP1 (PA0)
//         Note that while in Standby mode, the WKUP pin is forced into the input mode with
//         a built-in pulldown, so the pin must be brought up to VCC to wake the chip.
//      3. Normally the RTC could also be used to wake the chip, but the chip variant being 
//        used here does not support the RTC.
//
//  __STOP_MODE
//    Consumes approx. 230 uA (at 3.3 V) down to 15 uA (at 2.0 V) while asleep.
//    Standby Mode halts 1.8V domain clocks and HSI/HSE oscillators.
//
//    Set Up Stop Mode:
//      RCC->APB1ENR |= RCC_APB1ENR_PWREN;      // Enable PWR control clock
//      SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;  // Set SLEEPDEEP bit
//      PWR->CR      |= PWR_CR_LPDS ;           // Put voltage regulater in low power mode
//  
//    Exit Stop Mode:
//      Will wake from the stop mode via any active EXTI line interrupt event.
//
//  __SLEEP_MODE
//    Consumes approx. 1.1 mA while asleep.
//    This mode saves the least amount of power (approx. 40%) but can be woken up by *any*
//    interrupt event
//
//    Enter Sleep Mode:
//      RCC->APB1ENR |= RCC_APB1ENR_PWREN;      // Enable PWR control clock
//
//    Exit Sleep Mode:
//      Pretty much any interrupt event, will wake the chip up from sleep mode.
//
//  ------------------------------------------------------------------------------------------

//  Choose the sleep mode by uncommenting the desired mode.

// #define __STANDBY_MODE
// #define __STOP_MODE
 #define __SLEEP_MODE


//  ==========================================================================================
//  Interrupt Defines
//  Comment out the define to set up and run the desired type of interrupt. Multiple
//  interrupts may be set up simultaneously.
//
//  __BUTTON_INTERRUPT:
//    Interrupt generated on the rising edge of PA0, PA1, and PA2. Pressing and releasing a
//    button will result in a rising edge and trigger one of the following:
//      PA0: Will call EXTI0_1_IRQHandler while setting the PR0; Turn ON PA3 LED.
//      PA1: Will call EXTI0_1_IRQHandler while setting the PR1; Turn OFF PA3 LED.
//      PA2: Will call EXTI2_3_IRQHandler while setting the PR2: Toggle PA3 LED.
//    Note that EXTI0 and EXTI1 both result in a call to EXTI0_1_IRQHandler. The code inside
//    the handler must determine which line actually caused the interrupt by checking the
//    EXTI->PR register for the PR0 bit (for PA0) or PR1 bit (for PA1). Likewise for EXTI2
//    and EXTI3 calling EXTI2_3_IRQHandler.
//
//  __TIMER_INTERRUPT
//    TIM14 is set up to overflow at a certain perdiod. Each time the timer overflows, an
//    interrupt is generated which calls TIM14_IRQHandler. This handler toggles the PA4 LED.
//
//  __SYSTICK_INTERRUPT
//    Once the SysTick interrupt is initialized, an interrupt is generated each time that x
//    clock cycles have occurred, where x is a 24-bit number initialized in the
//    SysTick_Config( x ) procedure. On an 8 MHz clock, the slowest time between calls to the
//    interrupt handler is approx. 2 seconds when using a value of (uint32_t)16E6 for x.
//  ==========================================================================================

// #define __BUTTON_INTERRUPT
 #define __TIMER_INTERRUPT
// #define __SYSTICK_INTERRUPT


#ifdef __BUTTON_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  EXTI0_1_IRQHandler
//  ------------------------------------------------------------------------------------------
// void EXTI0_1_IRQHandler( void )
// This is the interrupt handler for external interrupt lines 0 (PA0) and 1 (PA1), so it is
// called when either of these lines generate an interrupt. You likely want to use the Pending
// Register to confirm which line generated the interrupt and act accordingly. Be sure to
// clear the interrupt by setting the bit in the Pending Register to effectively clear it.
//
// Note that the handler pauses for appox. 3 seconds. During this time, other button presses
// will not generate interrupts. This is to ilu
void
EXTI0_1_IRQHandler( void )
{
  if( EXTI->PR & EXTI_PR_PR0 )      // If detected rising edge on PA0:
  {
    GPIOA->ODR |= GPIO_ODR_3 | GPIO_ODR_4 | GPIO_ODR_5;       // Turn ON PA3 LED
    for( uint32_t x = 0; x<65000; x++ );
    while( GPIOA->ODR & GPIO_IDR_0 ) ;
    for(uint32_t x=0; x<(uint32_t)2e6; x++) ; // Pause approx 3 s
    EXTI->PR |= EXTI_PR_PR0;        // Clear the interrupt by *setting* the Pending Reg. bit
  }
  else
    if( EXTI->PR & EXTI_PR_PR1 )    // If rising edge detected on PA1:
    {
      GPIOA->ODR &= ~(GPIO_ODR_3 | GPIO_ODR_4 | GPIO_ODR_5);    // Turn OFF PA3 LED
      for( uint32_t x = 0; x<65000; x++ );
      while( GPIOA->ODR & GPIO_IDR_1 ) ;
      for(uint32_t x=0; x<(uint32_t)2e6; x++) ; // Pause approx 3 s
      EXTI->PR |= EXTI_PR_PR1;      // Clear the interrupt by *setting* the Pending Reg. bit
    }

}


//  ------------------------------------------------------------------------------------------
//  EXTI2_3_IRQHandler
//  ------------------------------------------------------------------------------------------
// void EXTI2_3_IRQHandler( void )
// This is the interrupt handler for external interrupt lines 2 (PA2) and 3 (PA3), so it is
// called when the rising edge on PA2 generates an interrupt. External interrupt line 3 on
// PA3 is not set up. Since only one of the two interrupt lines are used, it is not strictly
// required to check betwen PA2 and PA3, however, PA2 must be cleared for the next event.
void
EXTI2_3_IRQHandler( void )
{
//  if( EXTI->PR & EXTI_PR_PR2 )    // If detected rising edge on PA2: (Not needed since
  {                                 // line 3 / PA3 is not used here.)  
    GPIOA->ODR ^= (GPIO_ODR_3 | GPIO_ODR_4 | GPIO_ODR_5);       // Toggle PA3 LED
    for( uint32_t x = 0; x<100000; x++ );
    while( !(GPIOA->IDR & GPIO_IDR_2) ) ;
    for(uint32_t x=0; x<(uint32_t)2e6; x++) ; // Pause approx 3 s
    EXTI->PR |= EXTI_PR_PR2;        // Clear the interrupt by *setting* the Pending Reg. bit
  }
}
#endif // __BUTTON_INTERRUPT


#ifdef __TIMER_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  TIM14_IRQHandler
//  ------------------------------------------------------------------------------------------
// void TIM14_IRQHandler( void )
// This function will be called when the TIM14 interrupt is generated. Note that the function
// must clear the TIM14 interrupt flag (TIM_SR_UIF) everytime so that the next interrupt will
// be recognized.
// The name of the TIM14_IRQHandler and other interrupt handler function names to use are
// defined in STM32CubeF0\Core_Startup\Startup_stm32f030f4px.s 

void
TIM14_IRQHandler( void )
{
  GPIOA->ODR |= GPIO_ODR_4;                 // Flash LED 2
  for(uint32_t x=0; x<15000; x++) ;
  GPIOA->ODR &= ~GPIO_ODR_4;

  for(uint32_t x=0; x<(uint32_t)2e6; x++) ; // Pause approx 3 s
  
  GPIOA->ODR |= GPIO_ODR_4;                 // Flash LED 2
  for(uint32_t x=0; x<15000; x++) ;
  GPIOA->ODR &= ~GPIO_ODR_4;

  TIM14->SR &= ~TIM_SR_UIF;                 // Clear timer interrupt flag
}
#endif // __TIMER_INTERRUPT


#ifdef __SYSTICK_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  SysTick_Handler
//  ------------------------------------------------------------------------------------------
// void SysTick_Handler( void )
// This function will be called when the SysTick timer overflows. The SysTick timer must
// be initialized by calling the SysTick_Config( x ), where x is the number of clock ticks.
// Note that x is a 24-bit number, meaning that with the 8 MHz internal clock, the longest
// time that can be set is approx 16E6, or a time of 2 seconds.

void
SysTick_Handler( void )
{
  GPIOA->ODR ^= GPIO_ODR_5;   // Toggle LED 3
}
#endif // __SYSTICK_INTERRUPT



//  ==========================================================================================
//  main
//  ==========================================================================================

int
main( void )
{

//  ------------------------------------------------------------------------------------------
//  Set up GPIO pins as inputs and outputs as required
//  ------------------------------------------------------------------------------------------

  // Set up GPIO PA0, PA1, and PA2 as inputs with pullups
  RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;                 // Enable GPIO Port A
  GPIOA->PUPDR |= (0b01 << GPIO_PUPDR_PUPDR0_Pos) |   // Set pullups on PA0, PA1, PA2
                  (0b01 << GPIO_PUPDR_PUPDR1_Pos) |
                  (0b01 << GPIO_PUPDR_PUPDR2_Pos);
  
  // Set up GPIO PA3, PA4, and PA5 as outputs for LED
  GPIOA->MODER |= ( 0b01 << GPIO_MODER_MODER3_Pos |   // Set PA3, PA4, PA5 as outputs
                    0b01 << GPIO_MODER_MODER4_Pos |
                    0b01 << GPIO_MODER_MODER5_Pos );


#ifdef __BUTTON_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  Configure GPIO pins as interrupt triggers
//  ------------------------------------------------------------------------------------------

//  To configure a line as interrupt source, use the following procedure:
//  1. Enable the System Configuration Controller to allow GPIO pins to trigger interrupts
//  2. Unmask the desired interrupt by setting the bit in the EXTI_IMR register.
//  3. Configure the Trigger Selection bits of the Interrupt line (EXTI_RTSR and EXTI_FTSR)
//     so that the interrupt is triggered on the rising and/or falling edge of this line.
//  4. Configure the enable and mask bits that control the NVIC IRQ channel mapped to the
//     EXTI so that an interrupt coming from one of the EXTI line can be correctly
//     acknowledged.
//  5. Set the priority of this interrupt. In this example, the interrupts generated on
//     lines 0 and 1 will have a lower priority than the pin on line 2. Therefore, pressing
//     button 3 will interrupt the delay caused by pressing buttons 1 or 2. However,
//     pressing buttons 1 or 2 will not interrupt the delay caused by pressing button 3.
//
//  Note that the EXTI_IMR_MRx lines default to pins on GPIOA. If pins on another GPIO port
//  are desired, then the appropriate port must be set up in the SYSCFG_ESXTICRx register.
//  For example, if you want to use PB1 instead of PA1 for EXTI_IMR_MR1, then you must set
//  the EXTI1[3:0] bits in SYSCFG_EXTICR1 to 0x001 as follows:
//    SYSCFG->EXTICR1 |= (0b0001 << SYSCFG_EXTICR1_EXTI1_Pos);
//  See the "System configuration controller (SYSCFG)" section in the reference manual for
//  details.

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Turn on System Configuration Controller to allow
                                        // the GPIO pins to trigger interrupts.
  EXTI->IMR  |= EXTI_IMR_MR0 |    // Unmask EXTI interrupts on lines 0, 1, and 2
                EXTI_IMR_MR1 |
                EXTI_IMR_MR2;

  EXTI->RTSR |= EXTI_RTSR_TR0 |   // Trigger on rising edge of lines 0, 1, and 2
                EXTI_RTSR_TR1 |
                EXTI_RTSR_TR2;

  NVIC_EnableIRQ( EXTI0_1_IRQn );       // Enable this interrupt for lines 0 and 1
  NVIC_SetPriority( EXTI0_1_IRQn, 1 );  // Set the priority to 1. (0 = highest)

  NVIC_EnableIRQ( EXTI2_3_IRQn );       // Enable this interrupt for lines 2 and 3
  NVIC_SetPriority( EXTI2_3_IRQn, 0 );  // Set the desired priority (0 = highest)
#endif // __BUTTON_INTERRUPT


#ifdef __TIMER_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  Configure TIM14 as interrupt trigger
//  ------------------------------------------------------------------------------------------

//  To set a timer as an interrupt trigger, use the following procedure:
//  1. Set up the timer normally to overflow at the desired rate.
//  2. Enable the UIE bit in the DIER register of the timer to have an interrupt triggered
//     when the timer overflows.
//  3. Enable the NVIC TIMx_IRQn
//  4. Set the NVIC TIMx_IRQn priority as needed

  // Set up the TIM14 prescaler for a 1 ms clock pulse, and have it turn over and toggle the
  // interrupt every 10 seconds.
  RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;  // Enable TIM14
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Turn on System Configuration Controller to allow
                                        // the GPIO pins to trigger interrupts.
  TIM14->PSC    = 8000-1;               // Set prescaler to x8000 for 1 ms clock
  TIM14->ARR    = 10000-1;              // Set Auto Reload Register to 10,000 ms (10 s)
  TIM14->CR1   |= TIM_CR1_CEN;          // Start the timer
  TIM14->DIER  |= TIM_DIER_UIE;         // Have TIM14 generate interrupt when it overflows

  NVIC_EnableIRQ( TIM14_IRQn );         // Enable TIM14_IRQn
  NVIC_SetPriority( TIM14_IRQn, 1);     // Set priority for TIM14_IRQn
#endif // __TIMER_INTERRUPT


#ifdef __SYSTICK_INTERRUPT
//  ------------------------------------------------------------------------------------------
//  Configure SysTick as interrupt trigger
//  ------------------------------------------------------------------------------------------

//  To set the SysTick as an interrupt, use the following procedure:
//    Call SysTick_Config with a 24-bit value which is the number of clock ticks between
//    calls to the interrupt handler.
//    Note that a call to NVIC_EnableIRQ( SysTick_IRQn ) is not required.
  SysTick_Config( (uint32_t)16E6 );       // Configure the number of clock ticks between calls
                                          // to the SysTick interrupt handler.
  NVIC_SetPriority( SysTick_IRQn, 0 );    // Set the desired priority of the SysTick interrupt
                                          // though it defaults to 3, which is pretty high
#endif // __SYSTICK_INTERRUPT


#ifdef __STANDBY_MODE
//  Standby Mode -- Less than 10 uA while asleep
//  Standby Mode halts all functionality and provides the lowest sleep power requirement.
//  The following can be used to wake from Standby mode. Waking up is basically a reset of
//  the chip.
//  1. Grounding NRST pin.
//  2. Rising edge of WKUP1 (PA0) pin. Note that while in Standby mode, the pin is forced into
//     the input mode with a built-in pulldown. So can only be woken by bringing the pin up
//     to VCC.
//
RCC->APB1ENR |= RCC_APB1ENR_PWREN;          // Enable PWR control clock
SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;      // Set SLEEPDEEP bit
PWR->CR      |= PWR_CR_PDDS | PWR_CR_LPDS;  // Set Standby mode
PWR->CSR     |= PWR_CSR_EWUP1;              // Enable wake-up on WKUP1 (PA0) pin.
#endif


#ifdef __STOP_MODE
//  Stop Mode -- 230 uA (at 3.3 V) to 15 uA (at 2.0 V) while asleep
//  Standby Mode halts 1.8V domain clocks and HSI/HSE oscillators. Can be woken up via any
//  active EXTI line.
RCC->APB1ENR |= RCC_APB1ENR_PWREN;          // Enable PWR control clock
SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;      // Set SLEEPDEEP bit
PWR->CR      |= PWR_CR_LPDS ;               // Set to put voltage regulater in low power mode
#endif



#ifdef __SLEEP_MODE
//  Sleep Mode -- 1.1 mA while asleep
//  Sleep mode cuts the power consumption by approx 40%. It allows wake-up events from
//  any type of interrupt, including interrupts from general purpose clocks.
//  Basically uses the default power settings. Just need to enable the power interface clock
//  (PWREN) in RCC->APB1ENR. 
RCC->APB1ENR |= RCC_APB1ENR_PWREN;          // Enable PWR control clock
#endif


  // Main Cyclic Sleep Loop
  // This is where we go to sleep, and where well will reapper when woken up.
  while( 1 )
  {
    PWR->CR |= PWR_CR_CWUF;   // Clear wake-up flag
  TIM14->SR &= ~TIM_SR_UIF;                 // Clear timer interrupt flag
    __WFI();                  // Go to sleep
  }

} // End of main()
