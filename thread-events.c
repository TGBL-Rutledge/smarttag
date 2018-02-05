// This file is generated by Simplicity Studio.  Please do not edit manually.
//
//

// Generated events.
#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "hal/hal.h"


void stateEventHandler(void);
extern EmberEventControl stateEventControl;

void updateDisplayEventHandler(void);
extern EmberEventControl updateDisplayEventControl;

void ezModeEventHandler(void);
extern EmberEventControl ezModeEventControl;

void temperatureMeasurementServerReadEventHandler(void);
extern EmberEventControl temperatureReadControl;

void emConnectionManagerNetworkStateEventHandler(void);
extern EmberEventControl emConnectionManagerNetworkStateEventControl;

void emConnectionManagerOrphanEventHandler(void);
extern EmberEventControl emConnectionManagerOrphanEventControl;

void emConnectionManagerOkToLongPollHandler(void);
extern EmberEventControl emConnectionManagerOkToLongPollEventControl;

void emberAfPluginHeartbeatNodeTypeEventHandler(void);
extern EmberEventControl emberAfPluginHeartbeatNodeTypeEventControl;

void emZclIdentifyServerEventHandler(void);
extern EmberEventControl emZclIdentifyServerEventControl;

void emZclOtaBootloadClientEventHandler(void);
extern EmberEventControl emZclOtaBootloadClientEventControl;

void emZclOtaBootloadStorageEepromEraseEventHandler(void);
extern EmberEventControl emZclOtaBootloadStorageEepromEraseEventControl;

void emZclEzModeEventHandler(void);
extern EmberEventControl emZclEzModeEventControl;

void emberAfPluginButton0EventHandler(void);
extern EmberEventControl emberAfPluginButton0EventControl;

void emberAfPluginButton1EventHandler(void);
extern EmberEventControl emberAfPluginButton1EventControl;



const EmberEventData emAppEvents[] = {
  {&stateEventControl, stateEventHandler},
  {&updateDisplayEventControl, updateDisplayEventHandler},
  {&ezModeEventControl, ezModeEventHandler},
  {&temperatureReadControl, temperatureMeasurementServerReadEventHandler},
  {&emConnectionManagerNetworkStateEventControl, emConnectionManagerNetworkStateEventHandler},
  {&emConnectionManagerOrphanEventControl, emConnectionManagerOrphanEventHandler},
  {&emConnectionManagerOkToLongPollEventControl, emConnectionManagerOkToLongPollHandler},
  {&emberAfPluginHeartbeatNodeTypeEventControl, emberAfPluginHeartbeatNodeTypeEventHandler},
  {&emZclIdentifyServerEventControl, emZclIdentifyServerEventHandler},
  {&emZclOtaBootloadClientEventControl, emZclOtaBootloadClientEventHandler},
  {&emZclOtaBootloadStorageEepromEraseEventControl, emZclOtaBootloadStorageEepromEraseEventHandler},
  {&emZclEzModeEventControl, emZclEzModeEventHandler},
  {&emberAfPluginButton0EventControl, emberAfPluginButton0EventHandler},
  {&emberAfPluginButton1EventControl, emberAfPluginButton1EventHandler},
  {NULL, NULL}
};
