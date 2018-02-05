// Copyright 2017 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include EMBER_AF_API_CONNECTION_MANAGER
#include EMBER_AF_API_CONNECTION_MANAGER_JIB
#include INCLUDE_RTCDRIVER

#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif

#define ROUTER            EMBER_ROUTER
#define END_DEVICE        EMBER_END_DEVICE
#define SLEEPY_END_DEVICE EMBER_SLEEPY_END_DEVICE

#define NETWORK_JOIN_RETRY_DELAY_MS (1024 * 5)

#define TIME_TO_WAIT_FOR_LONG_POLLING_MS 60000

//------------------------------------------------------------------------------
// Event function forward declaration
EmberEventControl emConnectionManagerNetworkStateEventControl;
EmberEventControl emConnectionManagerOrphanEventControl;
EmberEventControl emConnectionManagerOkToLongPollEventControl;

static void resetOkToLongPoll(void);
static void setNextState(uint8_t nextState);
static void joinNetwork(void);

enum {
  INITIAL                              = 0,
  RESUME_NETWORK                       = 1,
  JOIN_NETWORK                         = 2,
  WAIT_FOR_JOIN_NETWORK                = 3,
  JOINED_TO_NETWORK                    = 4,
  RESET_NETWORK_STATE                  = 5,
};

static uint8_t state = INITIAL;
static uint8_t joinAttemptsRemaining = 0;
static bool isOrphaned = false;
static bool isSearching = false;

static bool okToLongPoll = false;

static uint8_t *joinKey;
static uint8_t joinKeyLength = 0;

//tgbl
uint8_t  disconnectedCnt = 0;	//number of times disconnected
uint32_t timeConnectedMax = 0;	//longest duration network is connected
uint32_t timeConnected = 0;     // current connected duration
uint32_t timeDisconnected = 0; //duration network is down
uint32_t timeDisconnected_start = 0; //capture the first time when disconected event happen
uint32_t timeConnected_start = 0;     // current connected duration

void emberConnectionManagerStartConnect(void)
{
  // This function will attempt to rejoin a network in all cases, if it is
  // already attached to a network it will perform a reset which automatically
  // tries to rejoin
  EmberNetworkStatus networkStatus = emberNetworkStatus();
  isSearching = true;

  switch (networkStatus) {
    case EMBER_SAVED_NETWORK:
      joinAttemptsRemaining = EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_NUM_REJOIN_ATTEMPTS;
      setNextState(RESUME_NETWORK);
      break;
    case EMBER_NO_NETWORK:
    case EMBER_JOINED_NETWORK_ATTACHED:
    default:
      // If there is no saved network, then get the prefix joining key, set the
      // join attempts to the maximum, and reset the network state to trigger a
      // join/rejoin
      joinKeyLength = emberConnectionManagerJibGetJoinKeyCallback(&joinKey);

      if (joinKeyLength == 0) {
        emberAfCorePrintln("No join key set!");
        emberConnectionManagerConnectCompleteCallback(EMBER_CONNECTION_MANAGER_STATUS_NO_KEY);
        return;
      }

      joinAttemptsRemaining = EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_NUM_REJOIN_ATTEMPTS;
      setNextState(RESET_NETWORK_STATE);
      break;
  }
}

void emberConnectionManagerStopConnect(void)
{
  EmberNetworkStatus networkStatus = emberNetworkStatus();

  // if we're already on the network, then a start connect should do nothing
  if (networkStatus == EMBER_JOINED_NETWORK_ATTACHED) {
    return;
  }

  joinAttemptsRemaining = 0;
  isSearching = false;
}

void emConnectionManagerJibNetworkStatusHandler(EmberNetworkStatus newNetworkStatus,
                                                EmberNetworkStatus oldNetworkStatus,
                                                EmberJoinFailureReason reason)
{
  static bool wasOrphaned = false;
  // This callback is called whenever the network status changes, like when
  // we finish joining to a network or when we lose connectivity.  If we have
  // no network, we try joining to one as long as join attempts remain.  Join
  // attempts are reset whenever emberConnectionManagerStartConnect is called.
  // If we have a saved network, we try to resume operations on that network.

  emberEventControlSetInactive(emConnectionManagerNetworkStateEventControl);

  switch (newNetworkStatus) {
    case EMBER_NO_NETWORK:
      if (oldNetworkStatus == EMBER_JOINING_NETWORK) {
        emberAfCorePrintln("ERR: Joining failed: 0x%x", reason);

        disconnectedCnt++;
	  
	  	//Capture first instance 
		if (timeDisconnected_start == 0)
		{
		 	timeDisconnected_start = RTCDRV_GetWallClock();
		}

        if (timeConnected_start != 0) {
			timeConnected = RTCDRV_GetWallClock() - timeConnected_start;
			if (timeConnected > timeConnectedMax) {
				timeConnectedMax = timeConnected;
			}
			if (timeConnected < 60)
			{
				emberAfCorePrintln("Was connected for %d seconds. Longest time recorded: %d seconds.",timeConnected, timeConnectedMax);
				timeConnected_start = 0;
	      	} 
			else
			{
				emberAfCorePrintln("Was connected for %d mins and %d secs. Longest time recorded: %d seconds.",(timeConnected/60), (timeConnected%60), timeConnectedMax);
				timeConnected_start = 0;
	      	}
        }
      }
      setNextState(JOIN_NETWORK);
      break;
    case EMBER_SAVED_NETWORK:
      setNextState(RESUME_NETWORK);
      break;
    case EMBER_JOINING_NETWORK:
      // Wait for either the "attaching" or "no network" state.
      break;
    case EMBER_JOINED_NETWORK_ATTACHING:
      // Wait for the "attached" state.
      if (oldNetworkStatus == EMBER_JOINED_NETWORK_ATTACHED) {
        emberAfCorePrintln("Trying to re-connect...");
        isOrphaned = true;
        wasOrphaned = true;
      }
      break;
    case EMBER_JOINED_NETWORK_ATTACHED:
      emberAfCorePrintln("Connected to network: %s",
                         (state == RESUME_NETWORK
                          ? "Resumed"
                          : (state == JOIN_NETWORK
                             ? "Joined"
                             : "Rejoined")));
      joinAttemptsRemaining = 0;
      isSearching = false;
      if (EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ROLE == SLEEPY_END_DEVICE) {
        emberEventControlSetDelayMS(emConnectionManagerOkToLongPollEventControl, TIME_TO_WAIT_FOR_LONG_POLLING_MS);
      }
      emberConnectionManagerConnectCompleteCallback(EMBER_CONNECTION_MANAGER_STATUS_CONNECTED);

      if (timeDisconnected_start != 0)
      {
      	timeDisconnected = RTCDRV_GetWallClock() - timeDisconnected_start;
		if (timeDisconnected < 60)
		{
			emberAfCorePrintln("Disconnected for %d seconds.\nOccurrences: %d time(s)",timeDisconnected, disconnectedCnt);
			timeDisconnected_start = 0;
      	} 
		else
		{
			emberAfCorePrintln("Disconnected for %d mins and %d secs.\nOccurrences: %d time(s)",(timeDisconnected/60), (timeDisconnected%60), disconnectedCnt);
			timeDisconnected_start = 0;
      	}

      }

	  timeConnected_start = RTCDRV_GetWallClock();

      
      break;
    case EMBER_JOINED_NETWORK_NO_PARENT:
      if (wasOrphaned) {
        emberEventControlSetDelayMinutes(emConnectionManagerOrphanEventControl,
                                         EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ORPHAN_REJOIN_DELAY_MINUTES);
        emberAfCorePrintln("Retrying join in %d minutes\n",
                           EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ORPHAN_REJOIN_DELAY_MINUTES);

		disconnectedCnt++;
      } else {
        wasOrphaned = true;
        emberEventControlSetActive(emConnectionManagerOrphanEventControl);
		disconnectedCnt++;
      }
      break;
    default:
      assert(false);
      break;
  }
}

void emConnectionManagerOkToLongPollHandler(void)
{
  emberAfCorePrintln("%dms has passed, now OK to long poll",
                     TIME_TO_WAIT_FOR_LONG_POLLING_MS);
  emberEventControlSetInactive(emConnectionManagerOkToLongPollEventControl);
  okToLongPoll = true;
}

bool emberConnectionManagerIsOkToLongPoll(void)
{
  return okToLongPoll;
}

void emberConnectionManagerSearchForParent(void)
{
  if (isOrphaned) {
    emberEventControlSetActive(emConnectionManagerOrphanEventControl);
  }
}

void emberResetNetworkStateReturn(EmberStatus status)
{
  // If we ever leave the network, we go right back to joining again.  This
  // could be triggered by an external CLI command.

  if (status != EMBER_SUCCESS) {
    emberResetNetworkState();
  } else {
    isOrphaned = false;
  }
}

void emConnectionManagerOrphanEventHandler(void)
{
  emberEventControlSetInactive(emConnectionManagerOrphanEventControl);
  isOrphaned = true;
  emberAfCorePrintln("Device still orphaned, attempt to reattach");
  emberAttachToNetwork();
}

bool emberConnectionmanagerIsOrphaned(void)
{
  return isOrphaned;
}

bool emberConnectionManagerIsSearching(void)
{
  return isSearching;
}

void emberConnectionManagerLeaveNetwork(void)
{
  isOrphaned = false;
  emberResetNetworkState();
}

static void resumeNetwork(void)
{
  emberAfCorePrintln("Resuming...");
  emberResumeNetwork();
}

void emberResumeNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to resume.  If
  // so, the result is reported later as a network status change.  If we cannot
  // even attempt to resume, we are likely in an orphan state

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to resume: 0x%x", status);
    emberAfCorePrintln("Trying again in %d minutes",
                       EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ORPHAN_REJOIN_DELAY_MINUTES);
    emberEventControlSetDelayMinutes(emConnectionManagerOrphanEventControl,
                                     EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ORPHAN_REJOIN_DELAY_MINUTES);
  }
}

static void joinNetwork(void)
{
  // When joining a network, we look for one specifically with our network id.
  // The commissioner must have our join key for this to succeed.

  EmberNetworkParameters parameters = { { 0 } };

  assert(state == JOIN_NETWORK);

  if (joinKeyLength == 0) {
    // It shouldn't be possible to get here without having first gone through
    // the startConnect() call.  In case that somehow happens, provide a path
    // to get the key anyway.  If that still doesn't provide a valid key, then
    // there's nothing we can do to join the network
    joinKeyLength = emberConnectionManagerJibGetJoinKeyCallback(&joinKey);

    if (joinKeyLength == 0) {
      emberAfCorePrintln("No join key set!");
      return;
    }
  }

  if (joinAttemptsRemaining > 0) {
    // Decrement the join counter
    joinAttemptsRemaining--;

    emberAfCorePrintln("Joining network using join passphrase \"%s\", join attempts left = %d",
                       joinKey,
                       joinAttemptsRemaining);

    parameters.nodeType = EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_ROLE;
    parameters.radioTxPower = EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_RADIO_TX_POWER;
    parameters.joinKeyLength = joinKeyLength;
    MEMCOPY(parameters.joinKey, joinKey, parameters.joinKeyLength);

    emberJoinNetwork(&parameters,
                     (EMBER_NODE_TYPE_OPTION
                      | EMBER_TX_POWER_OPTION
                      | EMBER_JOIN_KEY_OPTION),
                     EMBER_ALL_802_15_4_CHANNELS_MASK);
  } else {
    // No join attempts remain
    isSearching = false;
    emberAfCorePrintln("Unable to join within %d attempts",
                       EMBER_AF_PLUGIN_CONNECTION_MANAGER_JIB_NUM_REJOIN_ATTEMPTS);
    emberConnectionManagerConnectCompleteCallback(EMBER_CONNECTION_MANAGER_STATUS_TIMED_OUT);
  }
}

void emberJoinNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to join.  If
  // so, the result is reported later as a network status change.  Otherwise,
  // we just try to join again as long as join attempts remain.

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to join: 0x%x", status);
    setNextState(JOIN_NETWORK);
  } else {
    setNextState(WAIT_FOR_JOIN_NETWORK);
  }
}

void emberAttachToNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to attach.  If
  // so, the result is reported later as a network status change.  If we cannot
  // even attempt to attach, we just give up and reset our network state, which
  // will trigger a fresh join attempt.

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to reattach: 0x%x", status);
    setNextState(RESET_NETWORK_STATE);
  }
}

static void resetNetworkState(void)
{
  emberAfCorePrintln("Resetting...");
  emberResetNetworkState();
}

void emConnectionManagerNetworkStateEventHandler(void)
{
  emberEventControlSetInactive(emConnectionManagerNetworkStateEventControl);

  switch (state) {
    case RESUME_NETWORK:
      resetOkToLongPoll();
      resumeNetwork();
      break;
    case JOIN_NETWORK:
      resetOkToLongPoll();
      joinNetwork();
      break;
    case WAIT_FOR_JOIN_NETWORK:
      // Wait for joinNetwork to complete
      break;
    case RESET_NETWORK_STATE:
      resetOkToLongPoll();
      resetNetworkState();
      break;
    default:
      assert(false);
  }
}

static void resetOkToLongPoll(void)
{
  emberAfCorePrintln("Resetting OK to Long Poll");
  emberEventControlSetInactive(emConnectionManagerOkToLongPollEventControl);
  okToLongPoll = false;
}

static void setNextState(uint8_t nextState)
{
  state = nextState;
  emberEventControlSetActive(emConnectionManagerNetworkStateEventControl);
}
