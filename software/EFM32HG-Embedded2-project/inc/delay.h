/***************************************************************************//**
 * @file delay.h
 * @brief Delay functionality.
 * @version 3.0
 * @author Brecht Van Eeckhoudt
 ******************************************************************************/


/* Include guards prevent multiple inclusions of the same header */
#ifndef _DELAY_H_
#define _DELAY_H_


/* Includes necessary for this header file */
#include <stdint.h> /* (u)intXX_t */


/** Public definition to select which delay to use
 *    @li `1` - Use SysTick delays.
 *    @li `0` - Use EM2/3 RTC compare delays. */
#define SYSTICKDELAY 0


/** Public definition to select the use of the crystal or the oscillator
 *    @li `0` - Use the low-frequency crystal oscillator (LFXO), EM2 sleep is used.
 *    @li `1` - Use the ultra low-frequency RC oscillator (ULFRCO), EM3 sleep is used but delays are less precise timing-wise.
 *              ** EM3: All unwanted oscillators are disabled, they don't need to manually disabled before `EMU_EnterEM3`.**    */
#define ULFRCO 1


/* Public prototypes */
void delay (uint32_t msDelay);
void sleep (uint32_t sSleep);
bool RTC_checkWakeup (void);
void RTC_clearWakeup (void);
uint32_t RTC_getPassedSleeptime (void);


#endif /* _DELAY_H_ */
