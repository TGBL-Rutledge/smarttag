// Copyright 2015 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_EVENT_QUEUE
#include EMBER_AF_API_HAL
#ifdef EMBER_AF_API_SERIAL
  #include EMBER_AF_API_SERIAL
#endif
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include "thread-bookkeeping.h"
#include "thread-callbacks.h"
#ifdef HAL_CONFIG
  #include "hal-config.h"
  #include "ember-hal-config.h"
#endif

extern const EmberEventData emAppEvents[];
extern EventQueue emAppEventQueue;

// In testing, we cannot sleep or idle if the simulator is transmitting on any
// of the serial ports.
#ifdef EMBER_TEST
extern bool doingSerialTx[];
  #define simulatorDoingSerialTx(port) doingSerialTx[port]
#else
  #define simulatorDoingSerialTx(port) false
#endif

static bool okToIdleOrSleep(void)
{
#ifdef EMBER_AF_API_SERIAL
#ifdef EM_NUM_SERIAL_PORTS
  // With numbered serial ports, if any port has pending incoming or outgoing
  // data or if the simulator is transmitting, we do not sleep.
  uint8_t i;
  for (i = 0; i < EM_NUM_SERIAL_PORTS; i++) {
    if (!emberSerialUnused(i)
        && (emberSerialReadAvailable(i) != 0
            || emberSerialWriteUsed(i) != 0
            || simulatorDoingSerialTx(i))) {
      return false;
    }
  }
#else // EM_NUM_SERIAL_PORTS
  // Without numbered ports, if there is pending incoming or outgoing data, we
  // do not sleep.
  if (!emberSerialUnused(APP_SERIAL)
      && (emberSerialReadAvailable(APP_SERIAL) != 0
          || emberSerialWriteUsed(APP_SERIAL) != 0
          || simulatorDoingSerialTx(APP_SERIAL))) {
    return false;
  }
#endif // EM_NUM_SERIAL_PORTS
#endif // EMBER_AF_API_SERIAL
  return true;
}

static uint32_t min(uint32_t a, uint32_t b)
{
  return (a < b ? a : b);
}

void emberAfPluginIdleSleepTickCallback(void)
{
  // Turn interrupts off so that we can safely determine whether we can sleep
  // or idle.  Both halSleepForMilliseconds and halCommonIdleForMilliseconds
  // forcibly enable interrupts.  We can call neither function or exactly one
  // of them without risking a race condition.  Note that if we don't call
  // either, we have to enable interrupts ourselves.
  INTERRUPTS_OFF();

  bool flag_ = false;           //okToIdleOrSleep(); //true;

  // We can idle or sleep if some basic conditions are satisfied.  If not, we
  // stay awake and active.  Otherwise, we try to sleep or idle, in that order.
  if (flag_) {
    // The maximum duration for both sleeping and idling is determined by the
    // time to the next stack or framework event.  The stack also tells us
    // whether the radio needs to be on.  If so, we cannot sleep, but we can
    // idle.
    EmberIdleRadioState radioStateResult;
    uint32_t durationMs = min(emberStackIdleTimeMs(&radioStateResult),
                              emberMsToNextQueueEvent(&emAppEventQueue));
    durationMs = emberMsToNextEvent(emAppEvents, durationMs);
    uint32_t attemptedDurationMs = durationMs;

    // If the stack says the radio does not need to be on, we may sleep.
    // However, if the sleep duration is below our minimum threshold, we don't
    // bother.  It takes time to shut everything down and bring everything back
    // up and, at some point, it becomes useless to sleep.  If the radio can be
    // off and the duration is long enough, we give the application and plugins
    // one last chance to stay awake if they want.
    if (radioStateResult != IDLE_WITH_RADIO_ON
        && EMBER_AF_PLUGIN_IDLE_SLEEP_MINIMUM_SLEEP_DURATION_MS <= durationMs
        && emberAfOkToSleep(durationMs)) {
      emberStackPowerDown();
      halPowerDown();
      // WARNING: The following function enables interrupts, so we will cease
      // to be atomic by the time it returns.
      halSleepForMilliseconds(&durationMs);
      halPowerUp();
      emberStackPowerUp();
      emberAfPluginIdleSleepWakeUp(attemptedDurationMs - durationMs);
      emberAfDebugPrintln("slept for %lu ms",
                          (attemptedDurationMs - durationMs));
      return;
    }

#ifndef EMBER_TEST
#ifndef EMBER_NO_IDLE_SUPPORT
    // If we are here, it means we could not sleep, so we will try to idle
    // instead.  We give the application and plugins one last chance to stay
    // active if they want.  Note that interrupts are still disabled at this
    // point.
    if (durationMs != 0 && emberAfOkToIdle(durationMs)) {
      // WARNING: The following function enables interrupts, so we will cease
      // to be atomic by the time it returns.
      halCommonIdleForMilliseconds(&durationMs);
      emberAfPluginIdleSleepActive(attemptedDurationMs - durationMs);
      emberAfDebugPrintln("idled for %lu ms",
                          (attemptedDurationMs - durationMs));
      return;
    }
#endif // !EMBER_NO_IDLE_SUPPORT
#endif // !EMBER_TEST
  } // okToIdleOrSleep

  // If we are here, it means we did not sleep or idle.  Interrupts are still
  // disabled at this point, so we have to enable them again.
  INTERRUPTS_ON();
}
