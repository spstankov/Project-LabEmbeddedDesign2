/***************************************************************************//**
 * @file delay.c
 * @brief Delay functionality.
 * @version 1.8
 * @author Brecht Van Eeckhoudt
 *
 * ******************************************************************************
 *
 * @section Versions
 *
 *   @li v1.0: Moved delay functionality from `util.c` to this file.
 *   @li v1.1: Changed global variables to be static (~hidden) and added dbprint `\ n \ r ` fixes.
 *   @li v1.2: Removed EM1 delay method (see note in delayRTCC_EM2).
 *   @li v1.3: Cleaned up includes, added option to select SysTick/EM2 delay functionality
 *             and moved all of the logic in one `delay` method. Renamed sleep method.
 *   @li v1.4: Changed names of static variables, made initRTCcomp static.
 *   @li v1.5: Updated documentation.
 *   @li v1.6: Changed RTCcomp names to RTC.
 *   @li v1.7: Moved IRQ handler of RTC to this file.
 *   @li v1.8: Added ULFRCO logic.
 *
 *   @todo @li Enable disable/enable clock functionality?
 *         @li Check EMU_EnterEM2/3 true/false effect.
 *
 ******************************************************************************/


#include <stdint.h>    /* (u)intXX_t */
#include <stdbool.h>   /* "bool", "true", "false" */
#include "em_device.h" /* Include necessary MCU-specific header file */
#include "em_cmu.h"    /* Clock management unit */
#include "em_emu.h"    /* Energy Management Unit */
#include "em_gpio.h"   /* General Purpose IO */
#include "em_rtc.h"    /* Real Time Counter (RTC) */

#include "../inc/delay.h"       /* Corresponding header file */
#include "../inc/pin_mapping.h" /* PORT and PIN definitions */
#include "../inc/debugging.h" 	/* Enable or disable printing to UART */
#include "../inc/util.h"    	/* Utility functionality */


/* Local variables */
static volatile uint32_t msTicks; /* Volatile because it's modified by an interrupt service routine */
static bool RTC_initialized = false;

#ifdef SYSTICKDELAY /* SysTick delay selected */
static bool SysTick_initialized = false;
#endif /* SysTick/RTC selection */


/* Local prototype */
static void initRTC (void);


/**************************************************************************//**
 * @brief
 *   Wait for a certain amount of milliseconds.
 *
 * @details
 *   This method also initializes SysTick/RTC if necessary.
 *
 * @param[in] msDelay
 *   The delay time in milliseconds.
 *****************************************************************************/
void delay (uint32_t msDelay)
{

#ifdef SYSTICKDELAY /* SysTick delay selected */

	/* Initialize SysTick if not already the case */
	if (!SysTick_initialized)
	{
		/* Initialize and start SysTick
		 * Number of ticks between interrupt = cmuClock_CORE/1000 */
		if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1);

#ifdef DEBUGGING /* DEBUGGING */
	dbinfo("SysTick initialized");
#endif /* DEBUGGING */

		SysTick_initialized = true;
	}
	else
	{
		/* Enable SysTick interrupt and counter by setting their bits. */
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	}

	/* Wait a certain amount of ticks */
	uint32_t curTicks = msTicks;
	while ((msTicks - curTicks) < msDelay);

	/* Disable SysTick interrupt and counter (needs to be done before entering EM2/3) by clearing their bits. */
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk & ~SysTick_CTRL_ENABLE_Msk;

#else /* EM2/3 RTC delay selected */


	/* Initialize RTC if not already the case */
	if (!RTC_initialized) initRTC();
	else
	{
		/* TODO: Enable necessary oscillator and clocks */
	}

	/* Set RTC compare value for RTC compare register 0 depending on ULFRCO/LFXO selection */

#ifdef ULFRCO /* ULFRCO selected */

	if ((ULFRCOFREQ_MS * msDelay) <= 0x00ffffff) RTC_CompareSet(0, (ULFRCOFREQ_MS * msDelay));
	else
	{

#ifdef DEBUGGING /* DEBUGGING */
		dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUGGING */

		error(14);
	}

#else /* LFXO selected */

	if ((LFXOFREQ_MS * msDelay) <= 0x00ffffff) RTC_CompareSet(0, (LFXOFREQ_MS * msDelay));
	else
	{

#ifdef DEBUGGING /* DEBUGGING */
		dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUGGING */

		error(15);
	}

#endif /* ULFRCO/LFXO selection */


	/* Start the RTC */
	RTC_Enable(true);

	/* Enter EM2/3 depending on ULFRCO/LFXO selection */

#ifdef ULFRCO /* ULFRCO selected */
	/* In EM3, high and low frequency clocks are disabled. No oscillator (except the ULFRCO) is running.
	 * Furthermore, all unwanted oscillators are disabled in EM3. This means that nothing needs to be
	 * manually disabled before the statement EMU_EnterEM3(true); */
	EMU_EnterEM3(true); /* TODO: "true" doesn't seem to have any effect (save and restore oscillators, clocks and voltage scaling) */
#else /* LFXO selected */
	EMU_EnterEM2(true); /* TODO: "true" doesn't seem to have any effect (save and restore oscillators, clocks and voltage scaling) */
#endif /* ULFRCO/LFXO selection */

	/* TODO: Disable used oscillator and clocks after wake-up */

#endif /* SysTick/RTC selection */

}


/**************************************************************************//**
 * @brief
 *   Sleep for a certain amount of seconds in EM2/3.
 *
 * @details
 *   This method also initializes the RTC if necessary.
 *
 * @param[in] sSleep
 *   The sleep time in seconds.
 *****************************************************************************/
void sleep (uint32_t sSleep)
{
	/* Initialize RTC if not already the case */
	if (!RTC_initialized) initRTC();
	else
	{
		/* TODO: Enable necessary oscillator and clocks */
	}

#ifdef DEBUGGING /* DEBUGGING */
#ifdef ULFRCO /* ULFRCO selected */
	dbwarnInt("Sleeping in EM3 for ", sSleep, " s");
#else /* LFXO selected */
	dbwarnInt("Sleeping in EM2 for ", sSleep, " s");
#endif /* ULFRCO/LFXO selection */
#endif /* DEBUGGING */

	/* Set RTC compare value for RTC compare register 0 depending on ULFRCO/LFXO selection */

#ifdef ULFRCO /* ULFRCO selected */

	if ((ULFRCOFREQ * sSleep) <= 0x00ffffff) RTC_CompareSet(0, (ULFRCOFREQ * sSleep));
	else
	{

#ifdef DEBUGGING /* DEBUGGING */
		dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUGGING */

		error(16);
	}

#else /* LFXO selected */

	if ((LFXOFREQ * sSleep) <= 0x00ffffff) RTC_CompareSet(0, (LFXOFREQ * sSleep));
	else
	{

#ifdef DEBUGGING /* DEBUGGING */
		dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUGGING */

		error(17);
	}

#endif /* ULFRCO/LFXO selection */


	/* Start the RTC */
	RTC_Enable(true);

	/* Enter EM2/3 depending on ULFRCO/LFXO selection */

#ifdef ULFRCO /* ULFRCO selected */
	/* In EM3, high and low frequency clocks are disabled. No oscillator (except the ULFRCO) is running.
	 * Furthermore, all unwanted oscillators are disabled in EM3. This means that nothing needs to be
	 * manually disabled before the statement EMU_EnterEM3(true); */
	EMU_EnterEM3(true); /* TODO: "true" doesn't seem to have any effect (save and restore oscillators, clocks and voltage scaling) */
#else /* LFXO selected */
	EMU_EnterEM2(true); /* TODO: "true" doesn't seem to have any effect (save and restore oscillators, clocks and voltage scaling) */
#endif /* ULFRCO/LFXO selection */

	/* TODO: Disable used oscillator and clocks after wake-up */
}


/**************************************************************************//**
 * @brief
 *   RTC initialization.
 *
 * @details
 *   The RTC (compare) functionality uses the low-frequency crystal oscillator
 *   (LFXO) as the source.
 *
 * @note
 *   This is a static method because it's only internally used in this file
 *   and called by other methods if necessary.
 *****************************************************************************/
static void initRTC (void)
{

#ifdef ULFRCO /* ULFRCO selected */

	/* Enable the ultra low-frequency RC oscillator for the RTC */
	//CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true); /* The ULFRCO is always on */

	/* Enable the clock to the interface of the low energy modules
	 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
	CMU_ClockEnable(cmuClock_HFLE, true);

	/* Route the ULFRCO clock to the RTC */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);

#else /* LFXO selected */

	/* Enable the low-frequency crystal oscillator for the RTC */
	CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

	/* Enable the clock to the interface of the low energy modules
	 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
	CMU_ClockEnable(cmuClock_HFLE, true);

	/* Route the LFXO clock to the RTC */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

#endif /* ULFRCO/LFXO selection */


	/* Turn on the RTC clock */
	CMU_ClockEnable(cmuClock_RTC, true);

	/* Allow channel 0 to cause an interrupt */
	RTC_IntEnable(RTC_IEN_COMP0);
	RTC_IntClear(RTC_IFC_COMP0); /* TODO: this was in ULFRCO but not in LFXO example? */
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Configure the RTC settings */
	RTC_Init_TypeDef rtc = RTC_INIT_DEFAULT;
	rtc.enable = false; /* Don't start counting when initialization is done */

	/* Initialize RTC with pre-defined settings */
	RTC_Init(&rtc);

#ifdef DEBUGGING /* DEBUGGING */
#ifdef ULFRCO /* ULFRCO selected */
	dbinfo("RTC initialized with ULFRCO\n\r");
#else /* LFXO selected */
	dbinfo("RTC initialized with LFXO\n\r");
#endif /* ULFRCO/LFXO selection */
#endif /* DEBUGGING */

	RTC_initialized = true;
}


/**************************************************************************//**
 * @brief
 *   Interrupt Service Routine for system tick counter.
 *****************************************************************************/
void SysTick_Handler (void)
{
	msTicks++; /* Increment counter necessary by SysTick delay functionality */
}


/**************************************************************************//**
 * @brief
 *   Interrupt Service Routine for the RTC.
 *
 * @note
 *   The *weak* definition for this method is located in `system_efm32hg.h`.
 *****************************************************************************/
void RTC_IRQHandler (void)
{
	/* Disable the counter */
	RTC_Enable(false);

	/* Clear the interrupt source */
	RTC_IntClear(RTC_IFC_COMP0);
}