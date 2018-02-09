// Copyright 2015 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_EVENT_QUEUE
#include EMBER_AF_API_HAL
#include "thread-bookkeeping.h"
#include "rutledge-epaper-module/Display_EPD_W21.h"
#include "rutledge-common/common.h"

#ifdef HAL_CONFIG
  #include "hal-config.h"
  #include "ember-hal-config.h"
#endif
// If serial functionality is enabled, we will initialize the serial ports during
// startup.  This has to happen after the HAL is initialized.
#ifdef EMBER_AF_API_SERIAL
  #include EMBER_AF_API_SERIAL
  #define SERIAL_INIT EMBER_AF_SERIAL_PORT_INIT
#else
  #define SERIAL_INIT()
#endif

// If we are running in simulation, we need to setup our "fake" EEPROM, which
// is just simulated EEPROM. This has to happen right after the HAL is
// initialized, but before the stack or app is initialized so that the EEPROM
// will be usable immediately. Calling this setupFakeEeprom() API will properly
// initialize the HalEepromInformationType data for the "fake" EEPROM. The
// partSize and addressBase are multiple of 2 that match some of the EEPROM
// implementations in base. We set pageEraseRequired to true in order to plan
// for a worst case scenario. We set the wordSizeBytes to 2 because this is the
// case for most EEPROM implementations in base.
#ifdef EMBER_TEST
  #include "micro/unix/simulation/fake-eeprom.h"
  #define EEPROM_INIT()                              \
  setupFakeEeprom(204800,   /* partSize */           \
                  0,        /* addressBase */        \
                  FAKE_EEPROM_DEFAULT_PAGE_SIZE,     \
                  FAKE_EEPROM_DEFAULT_PAGE_ERASE_MS, \
                  true,     /* pageEraseRequired? */ \
                  2);       /* wordSizeBytes */
#else
  #define EEPROM_INIT()
#endif

// If printing is enabled, we will print some diagnostic information about the
// most recent reset and also during runtime.  On some platforms, extended
// diagnostic information is available.
#if defined(EMBER_AF_API_SERIAL) && defined(EMBER_AF_PRINT_ENABLE)
  #ifdef EMBER_AF_API_DIAGNOSTIC_CORTEXM3
    #include EMBER_AF_API_DIAGNOSTIC_CORTEXM3
  #endif
static void printResetInformation(void);
  #define PRINT_RESET_INFORMATION printResetInformation
  #define emberAfGuaranteedPrint(...) \
  emberSerialGuaranteedPrintf(APP_SERIAL, __VA_ARGS__)
  #define emberAfGuaranteedPrintln(format, ...) \
  emberSerialGuaranteedPrintf(APP_SERIAL, format "\r\n", __VA_ARGS__);
#else
  #define PRINT_RESET_INFORMATION()
  #define emberAfGuaranteedPrint(...)
  #define emberAfGuaranteedPrintln(...)
#endif

#ifdef EMBER_AF_API_MANAGEMENT_CLIENT
  #include EMBER_AF_API_MANAGEMENT_CLIENT
  #define PROCESS_MANAGEMENT_COMMAND managementCommandTick
#else
  #define PROCESS_MANAGEMENT_COMMAND()
#endif

// Our entry point is typically main(), except during testing.
#ifdef EMBER_TEST
  #define MAIN nodeMain
#else
  #define MAIN main
#endif

static void loop(void);

extern const EmberEventData emAppEvents[];
EmberTaskId emAppTask;
EventQueue emAppEventQueue;

static bool stackIsInitialized;
static bool epdInitialized;
static bool applicationIsInitialized;
static uint32_t initTimeMs;
#define INIT_TIMEOUT_MS (10 * MILLISECOND_TICKS_PER_SECOND)

int MAIN(MAIN_FUNCTION_PARAMETERS)
{
  // Let the application and plugins do early initialization.  This function is
  // generated.
  emAfMain(MAIN_FUNCTION_ARGUMENTS);

  // Initialize the HAL and enable interrupts.
  halInit();
  INTERRUPTS_ON();

  // Initialize the EEPROM.
  EEPROM_INIT();

  // Initialize the serial ports.
  SERIAL_INIT();
#ifdef MODULE_EPD
	  /* initialize e-paper */
	    EPD_Spi_Init();
#endif
  // Display diagnostic information about the most recent reset.
  PRINT_RESET_INFORMATION();

  // Initialize a task for the application and plugin events and enable idling.
  emAppTask = emberTaskInit(emAppEvents);
  emberTaskEnableIdling(true);
  emInitializeEventQueue(&emAppEventQueue);

  // Initialize the stack.
  stackIsInitialized = false;
  applicationIsInitialized = false;
  epdInitialized = false;
  initTimeMs = halCommonGetInt32uMillisecondTick();
  emberInit();

  // Run the application loop, which usually never terminates.
  loop();

  return 0;
}

static void loop(void)
{
  while (true) {
    // Reset the watchdog timer to prevent a timeout.
    halResetWatchdog();

    // Process management commands from the NCP, if applicable.  This is done
    // before checking whether stack initialization has finished, because the
    // initialization status is itself a management command.
    PROCESS_MANAGEMENT_COMMAND();

    // Wait until the stack is initialized before allowing the application and
    // plugins to run, so that so that the application doesn't get ahead of the
    // stack.  If initialization takes too long, we try again.
    if (!stackIsInitialized) {
      uint32_t nowMs = halCommonGetInt32uMillisecondTick();
      if (INIT_TIMEOUT_MS <= elapsedTimeInt32u(initTimeMs, nowMs)) {
        emberAfGuaranteedPrintln("Waiting for NCP...", 0);
        initTimeMs = nowMs;
        emberInit();
      }
      continue;
    }
	if(!epdInitialized)
	{
		// reset struct hold info of Tag
		memset(&personInformation,0, sizeof(personInformation));

		halResetWatchdog();
		emberAfCorePrint("+++ EPD_init \n");

		#ifdef EPD_W21_FONT_SIZE_20_ENABLED
		SetFont(&Font20);
		#endif
		#ifdef EPD_W21_FONT_SIZE_16_ENABLED
		SetFont(&Font16);
		#endif
		#ifdef EPD_W21_FONT_SIZE_12_ENABLED
		SetFont(&Font12);
		#endif
		#ifdef EPD_W21_FONT_SIZE_8_ENABLED
		SetFont(&Font8);
		#endif
		EPD_init(1); //initialize e-paper
		emberAfGuaranteedPrint("+++ EPD_W21_DispInit()\n");
		halCommonDelayMilliseconds(50);
		EPD_clean();
		halCommonDelayMilliseconds(10);
		EPD_clean();
		halCommonDelayMilliseconds(200);


		EPD_W21_WriteLUT((unsigned char *) LUTDefault_part);
		EPD_W21_POWERON();
		halCommonDelayMilliseconds(100);

		memcpy(personInformation.name,EPD_STR_NAME,strlen(EPD_STR_NAME));
		memcpy(personInformation.company,EPD_STR_COMPANY,strlen(EPD_STR_COMPANY));
		memcpy(personInformation.scope,EPD_STR_SCOPE,strlen(EPD_STR_SCOPE));

		//EPD_PrintDefaultInfo();
		DisplayStringAt(1,13,"EPAPER TEST EPAPER TEST EPAPER TEST", TEXT_LEFT_MODE);
		EPD_UpdateDisplay();


		#ifndef DISABLE_WATCHDOG
		 halInternalEnableWatchDog();
		#endif
		 epdInitialized = true;
	}
    // Initialize the application and plugins.  Whenever the stack initializes,
    // these must be reinitialized.  This function is generated.
    if (stackIsInitialized && !applicationIsInitialized) {
      emAfInit();
      applicationIsInitialized = true;
    }

    // Let the stack, application, and plugins run periodic tasks.  This
    // function is generated.
    emAfTick();

    // Run the application and plugin events.
    emberRunTask(emAppTask);
    emberRunEventQueue(&emAppEventQueue);

    simulatedTimePassesMs(emberMsToNextEvent(emAppEvents,
                                             emberMsToNextQueueEvent(&emAppEventQueue)));
  }
}

#ifdef EMBER_AF_PRINT_ENABLE

static void printResetInformation(void)
{
  // Information about the most recent reset is printed during startup to aid
  // in debugging.
  emberAfGuaranteedPrintln("Reset info: 0x%x (%p)",
                           halGetResetInfo(),
                           halGetResetString());
#ifdef EMBER_AF_API_DIAGNOSTIC_CORTEXM3
  emberAfGuaranteedPrintln("Extended reset info: 0x%2x (%p)",
                           halGetExtendedResetInfo(),
                           halGetExtendedResetString());
  if (halResetWasCrash()) {
    halPrintCrashSummary(APP_SERIAL);
    halPrintCrashDetails(APP_SERIAL);
    halPrintCrashData(APP_SERIAL);
  }
#endif // EMBER_AF_API_DIAGNOSTIC_CORTEXM3
}

#endif // EMBER_AF_PRINT_ENABLE

void emberInitReturn(EmberStatus status)
{
  // If initialization fails, we have to assert because something is wrong.
  // Whenever the stack initializes, the application and plugins must be
  // reinitialized.
  emberAfGuaranteedPrintln("Init: 0x%x", status);
  assert(status == EMBER_SUCCESS);
  stackIsInitialized = true;
  applicationIsInitialized = false;
}

// TODO: This should not be necessary in the application.  See EMIPSTACK-324.
bool ipModemLinkPreparingForPowerDown(void)
{
  return false;
}

void emberResetMicroHandler(EmberResetCause resetCause)
{
  // We only print the reset cause for host applications because SoC
  // applications will automatically print the reset cause at startup.
#ifdef EMBER_HOST
  static const char * const resetCauses[] = {
    "UNKNOWN",
    "FIB",
    "BOOTLOADER",
    "EXTERNAL",
    "POWER ON",
    "WATCHDOG",
    "SOFTWARE",
    "CRASH",
    "FLASH",
    "FATAL",
    "FAULT",
    "BROWNOUT",
  };
  emberAfGuaranteedPrintln("Reset info: 0x%x (%p)",
                           resetCause,
                           (resetCause < COUNTOF(resetCauses)
                            ? resetCauses[resetCause]
                            : resetCauses[EMBER_RESET_UNKNOWN]));
#endif // EMBER_HOST
}

void emberMarkApplicationBuffersHandler(void)
{
  // Mark scheduled events in the queue and run the marking functions of those
  // events.
  emberMarkEventQueue(&emAppEventQueue);

  // Let the application and plugins mark their buffers.  This function is
  // generated.
  emAfMarkApplicationBuffers();
}

#ifdef SWO_ENABLED //only for EFR32 family
void setupSWOForPrint(void)
{
  /* Enable GPIO clock. */
	CMU_ClockEnable(cmuClock_GPIO, true);

  /* Enable Serial wire output pin */
  GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;

    /* Set location 0 */
  GPIO->ROUTELOC0 = GPIO_ROUTELOC0_SWVLOC_LOC0;

  /* Enable output on pin - GPIO Port F, Pin 2 */
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;

  /* Enable debug clock AUXHFRCO */
  CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
  CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

  /* Wait until clock is ready */
  while (!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

  /* Enable trace in core debug */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  ITM->LAR  = 0xC5ACCE55;
  ITM->TER  = 0x0;
  ITM->TCR  = 0x0;
  TPI->SPPR = 2;
  TPI->ACPR = 0x15;	// changed from 0x0F on Giant, etc. to account for 19 MHz default AUXHFRCO frequency
  ITM->TPR  = 0x0;
  DWT->CTRL = 0x400003FE;
  ITM->TCR  = 0x0001000D;
  TPI->FFCR = 0x00000100;
  ITM->TER  = 0x1;
}
#endif

#if false //old code for other chip families
void setupSWOForPrint(void)
{
  /* Enable GPIO clock. */
  CMU_ClockEnable(cmuClock_GPIO, true);

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_LEOPARD_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /* Enable Serial wire output pin */
  GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;

  /* Set location 0 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  /* Enable output on pin - GPIO Port F, Pin 2 */
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#elif defined(CORTEXM3_EFR32_MICRO)
  /* Enable Serial wire output pin */
  GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;

  /* Set location 0 */
  GPIO->ROUTELOC0 = GPIO_ROUTELOC0_SWVLOC_LOC0;

  /* Enable output on pin - GPIO Port F, Pin 2. P[x] whereby x=0 (portA), x=1(portB), x=5(portF) etc.*/
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
  /* Enable Serial wire output pin */
  GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;

  /* Set location 1 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) |GPIO_ROUTE_SWLOCATION_LOC1;
  /* Enable output on pin */
  GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
  GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif

  /* Enable debug clock AUXHFRCO */
  CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

  /* Wait until clock is ready */
  while (!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

  /* Enable trace in core debug */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  ITM->LAR  = 0xC5ACCE55;
  ITM->TER  = 0x0;
  ITM->TCR  = 0x0;
  TPI->SPPR = 2;
  TPI->ACPR = 0xf;
  ITM->TPR  = 0x0;
  DWT->CTRL = 0x400003FF;
  ITM->TCR  = 0x0001000D;
  TPI->FFCR = 0x00000100;
  ITM->TER  = 0x1;
}
#endif
