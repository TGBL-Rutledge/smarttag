// Copyright 2016 Silicon Laboratories, Inc.

// -----------------------------------------------------------------------------
// Includes

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include EMBER_AF_API_COMMAND_INTERPRETER2
#include EMBER_AF_API_NETWORK_MANAGEMENT
#include EMBER_AF_API_CONNECTION_MANAGER
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include EMBER_AF_API_ZCL_CORE

#if defined(CORTEXM3_EFM32_MICRO)
#include "em_adc.h"
#include "em_cmu.h"
#if defined(EMBER_AF_PLUGIN_GLIB)
#include "spidrv.h"
#include EMBER_AF_API_GLIB
#endif //EMBER_AF_PLUGIN_GLIB
#endif //CORTEXM3_EFM32_MICRO

#define ALIAS(x) x##Alias
#include "app/thread/plugin/udp-debug/udp-debug.c"
#include "plugin/serial/serial.h"
#include <stdio.h>
#include "rutledge-epaper-module/Display_EPD_W21.h"
#include "rutledge-common.h"

// -----------------------------------------------------------------------------
// Definitions

// LCD UI macros
#define EUI_ROW 5
#define JOIN_KEY_ROW 15
#define NODE_TYPE_ROW 35
#define NETWORK_ID_ROW 45
#define PAN_ID_ROW 55
#define CHANNEL_ROW 65
#define IP_ADDRESS_INITIAL_ROW 75

#define ROW_SIZE 10
#define START_COLUMN 5
#define LCD_ASCII_SCREEN_WIDTH 20

#define PADDED_ASCII_BUFFER_SIZE 32
#define IP_ADDRESS_CHAR_TRIM 9

#define LCD_OPAQUE_VALUE 0
#define LCD_REFRESH_MS 200

// Every IDLE_PERIOD_TIMEOUT_MS the key is printed to the console
#define IDLE_PERIOD_TIMEOUT_MS (60 * MILLISECOND_TICKS_PER_SECOND)

// some helpers for parsing JSON payloads
#define IP_JSON_KEY "\"ip\":\""
#define PORT_JSON_KEY "\"port\":"

// -----------------------------------------------------------------------------
// Private functions

static void announceToAddress(const EmberIpv6Address *newBorderRouter);
static void finishAnnounceToAddress(void);
static void sensorActuatorIdleState(void);

static uint32_t readTemperature(void);
//vlad
static void startScanAir(void);

// These functions will be used when the WSTK is used as the platform to
// populate the LCD with useful information
#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
static char* nodeTypeToString(EmberNodeType nodeType);
static void lcdPrintNetworkStatus(EmberNetworkStatus status, uint8_t printRow);
static void lcdPrintJoinCode(uint8_t *joinKey, uint8_t printRow);
static void lcdPrintIpv6Address(const EmberIpv6Address *address,
                                uint8_t printRow,
                                uint8_t screenWidth);
static void lcdPrintCurrentDeviceState(void);
#endif //EMBER_AF_PLUGIN_GLIB

static void startReportingTemperature(const EmberIpv6Address *newSubscriberAddress,
                                      const uint16_t newSubscriberPort);
static void stopReportingTemperature(void);
static void reportTemperatureStatusHandler(EmberCoapStatus status,
                                           EmberCoapCode code,
                                           EmberCoapReadOptions *options,
                                           uint8_t *payload,
                                           uint16_t payloadLength,
                                           EmberCoapResponseInfo *info);

// -----------------------------------------------------------------------------
// Externals
extern void OTAGetGlobalAddressCallback(const EmberIpv6Address *address,
                                 uint32_t preferredLifetime,
                                 uint32_t validLifetime,
                                 uint8_t addressFlags);
extern EmberEventControl emZclOtaBootloadClientEventControl;

// -----------------------------------------------------------------------------
// Globals

uint8_t scanCounter=0;
RutledgeScanData top3Data[3] = { 0 };
RutledgeScanData scanData[MAX_SCANNING_DEVICES] = { 0 };
static EmberIpv6Address globalAddress;
static uint8_t globalAddressString[EMBER_IPV6_ADDRESS_STRING_SIZE] = { 0 };
static EmberIpv6Address discoverRequestAddress = { { 0 } };
static EmberIpv6Address subscriberAddress = { { 0 } };
static uint16_t subscriberPort = EMBER_COAP_PORT;
static uint8_t joinKey[EMBER_JOIN_KEY_MAX_SIZE + 1] = { 0 };
static uint8_t joinKeyLength = 0;

static const EmberIpv6Address allMeshRouters = {
  { 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, }
};

static uint8_t failedReports;
#define REPORT_FAILURE_LIMIT 3
#define WAIT_PERIOD_MS   (60 * MILLISECOND_TICKS_PER_SECOND)
#define REPORT_PERIOD_S (5)
#define REPORT_PERIOD_MS (REPORT_PERIOD_S * MILLISECOND_TICKS_PER_SECOND)

static void findOnOffServer(void);
bool haveOnOffServer = false;
static EmberZclDestination_t zclLight;

static const uint8_t deviceAnnounceUri[] = "device/announce";
static const uint8_t temperatureUri[] = "device/temperature";

enum {
  INITIAL                              = 0,
  RESUME_NETWORK                       = 1,
  JOIN_NETWORK                         = 2,
  JOIN_NETWORK_COMPLETION              = 3,
  WAIT_FOR_SUBSCRIPTIONS               = 4,
  REPORT_TEMP_TO_SUBSCRIBER            = 5,
  WAIT_FOR_DATA_CONFIRMATION           = 6,
  JOINED_TO_NETWORK                    = 7,
  RESET_NETWORK_STATE                  = 8,
  IDLE                                 = 9,
};

static uint8_t state = INITIAL;

EmberEventControl stateEventControl;
EmberEventControl updateDisplayEventControl;
EmberEventControl ezModeEventControl;
EmberEventControl temperatureReadControl;

static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs);
#define setNextState(nextState)       setNextStateWithDelay((nextState), 0)
#define repeatState()                 setNextStateWithDelay(state, 0)
#define repeatStateWithDelay(delayMs) setNextStateWithDelay(state, (delayMs))

// Keep our EUI string for JSON payloads, take size in bytes x2 to account for
// ASCII and add 1 for a NULL terminator
static uint8_t euiString[EUI64_SIZE * 2 + 1] = { 0 };
static uint8_t exiString[EUI64_SIZE * 2 + 1] = { 0 };

// String helper function prototypes
static void formatEuiString(uint8_t* euiString, size_t strSz, const uint8_t* euiBytes);
static const uint8_t *printableNetworkId(void);

#if defined(CORTEXM3_EFM32_MICRO)
static void adcSetup(void);
static int32_t convertToCelsius(int32_t adcSample);
static uint32_t adcRead(void);

// @brief Initialize ADC for temperature sensor readings in single join
static void adcSetup(void)
{
  // Enable ADC clock
  CMU_ClockEnable(cmuClock_ADC0, true);

  // Base the ADC configuration on the default setup.
  ADC_Init_TypeDef       init  = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef sInit = ADC_INITSINGLE_DEFAULT;

  // Initialize timebases
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(400000, 0);
  ADC_Init(ADC0, &init);

  // Set input to temperature sensor. Reference must be 1.25V
  sInit.reference   = adcRef1V25;
  sInit.acqTime     = adcAcqTime8; // Minimum time for temperature sensor
  sInit.posSel      = adcPosSelTEMP;
  ADC_InitSingle(ADC0, &sInit);
}

// @brief  Do one ADC conversion
// @return ADC conversion result
static uint32_t adcRead(void)
{
  ADC_Start(ADC0, adcStartSingle);
  while ((ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0) {
    // Busy wait
  }
  return ADC_DataSingleGet(ADC0);
}

// @brief Convert ADC sample values to celsius.
// @detail See section 25.3.4.1 in the reference manual for detail
//   on temperature measurement and conversion.
// @param adcSample Raw value from ADC to be converted to celsius
// @return The temperature in milli degrees celsius.
static int32_t convertToCelsius(int32_t adcSample)
{
  uint32_t calTemp0;
  uint32_t calValue0;
  int32_t readDiff;
  float temp;

  // Factory calibration temperature from device information page.
  calTemp0 = ((DEVINFO->CAL & _DEVINFO_CAL_TEMP_MASK)
              >> _DEVINFO_CAL_TEMP_SHIFT);

  calValue0 = ((DEVINFO->ADC0CAL3
                // _DEVINFO_ADC0CAL3_TEMPREAD1V25_MASK is not correct in
                //    current CMSIS. This is a 12-bit value, not 16-bit.
                & 0xFFF0)
               >> _DEVINFO_ADC0CAL3_TEMPREAD1V25_SHIFT);

  if ((calTemp0 == 0xFF) || (calValue0) == 0xFFF) {
    // The temperature sensor is not calibrated
    return -100.0;
  }

  // Vref = 1250mV
  // TGRAD_ADCTH = 1.835 mV/degC (from datasheet)

  readDiff = calValue0 - adcSample;
  temp     = ((float)readDiff * 1250);
  temp    /= (4096 * -1.835);

  // Calculate offset from calibration temperature
  temp     = (float)calTemp0 - temp;
  return (uint32_t)(temp * 1000);
}
#else
static void adcSetup(void)
{
  // No setup needed for non EFR devices
}
#endif //CORTEXM3_EFM32_MICRO

#if defined(EMBER_AF_API_CONNECTION_MANAGER_JIB)
uint8_t emberConnectionManagerJibGetJoinKeyCallback(uint8_t **returnedJoinKey)
{
  joinKey[EMBER_JOIN_KEY_MAX_SIZE] = 0;
  static uint8_t joinKeyLength = 0;

  // Using a join key generated from the EUI64 of the node is a security risk,
  // but is useful for demonstration purposes. See the warning above. This
  // hash generates alphanumeric characters in the ranges 0-9 and A-U. The
  // Thread specification disallows the characters I, O, Q, and Z, for
  // readability. I, O, and Q are therefore remapped to V, W, and X. Z is not
  // generated, so it is not remapped.
  const EmberEui64 *eui64;
  eui64 = emberEui64();
  for (joinKeyLength = 0; joinKeyLength < EUI64_SIZE; (joinKeyLength)++) {
    joinKey[joinKeyLength] = ((eui64->bytes[joinKeyLength] & 0x0F)
                              + (eui64->bytes[joinKeyLength] >> 4));
    joinKey[joinKeyLength] += (joinKey[joinKeyLength] < 10 ? '0' : 'A' - 10);
    if (joinKey[joinKeyLength] == 'I') {
      joinKey[joinKeyLength] = 'V';
    } else if (joinKey[joinKeyLength] == 'O') {
      joinKey[joinKeyLength] = 'W';
    } else if (joinKey[joinKeyLength] == 'Q') {
      joinKey[joinKeyLength] = 'X';
    }
  }

  emberAfCorePrintln("Join device to network using EUI \"%x%x%x%x%x%x%x%x\" and join passphrase \"%s\"",
                     emberEui64()->bytes[7],
                     emberEui64()->bytes[6],
                     emberEui64()->bytes[5],
                     emberEui64()->bytes[4],
                     emberEui64()->bytes[3],
                     emberEui64()->bytes[2],
                     emberEui64()->bytes[1],
                     emberEui64()->bytes[0],
                     joinKey);

  *returnedJoinKey = joinKey;
  return joinKeyLength;
}

void emberConnectionManagerConnectCompleteCallback(EmberConnectionManagerConnectionStatus status)
{
  if (status == EMBER_CONNECTION_MANAGER_STATUS_CONNECTED) {
    setNextState(IDLE);
  }
}
#endif //defined(EMBER_AF_API_CONNECTION_MANAGER_JIB)

void emberAfInitCallback(void)
{
  // This callback is called after the stack has been initialized.  It is our
  // opportunity to do initialization, like resuming the network or joining a
  // new network.

  // Get and store EUI since we need it around for JSON strings
  formatEuiString(euiString, sizeof(euiString), emberEui64()->bytes);
  emberAfCorePrintln("Eui64: >%s", euiString);

  // Set up the ADC for temperature conversions
  adcSetup();

  // Set Led to on by default
  //halSetLed(BOARD_ACTIVITY_LED);

  // Taken from 7021 specification sheet
  uint16_t maxTempDeciC = (125 * 100);
  uint16_t minTempDeciC = (uint16_t)(-1 * (40 * 100));

  // Write the min and max temperature values for sensor-actuator
  emberZclWriteAttribute(1, // endpoint
                         &emberZclClusterTempMeasurementServerSpec,
                         EMBER_ZCL_CLUSTER_TEMP_MEASUREMENT_SERVER_ATTRIBUTE_TEMP_MAX_MEASURED_VALUE,
                         (uint8_t *)&maxTempDeciC,
                         sizeof(maxTempDeciC));

  emberZclWriteAttribute(1, // endpoint
                         &emberZclClusterTempMeasurementServerSpec,
                         EMBER_ZCL_CLUSTER_TEMP_MEASUREMENT_SERVER_ATTRIBUTE_TEMP_MIN_MEASURED_VALUE,
                         (uint8_t *)&minTempDeciC,
                         sizeof(minTempDeciC));

  // Start the ReadEvent, which will re-activate itself perpetually
  emberEventControlSetActive(temperatureReadControl);
  emberEventControlSetActive(updateDisplayEventControl);

#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
  // If it is present, initialize the graphics display
  GRAPHICS_Init();
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_clear(&glibContext);
#endif //EMBER_AF_PLUGIN_GLIB
}

void temperatureMeasurementServerReadEventHandler(void)
{
  emberEventControlSetInactive(temperatureReadControl);
  uint32_t temperatureMilliC = readTemperature();
  uint16_t temperatureCentiC = temperatureMilliC / 10;
  emberAfCorePrintln("Temperature report: %d [milli C]", temperatureMilliC); //tgbl
  emberZclWriteAttribute(1, // endpoint
                         &emberZclClusterTempMeasurementServerSpec,
                         EMBER_ZCL_CLUSTER_TEMP_MEASUREMENT_SERVER_ATTRIBUTE_TEMP_MEASURED_VALUE,
                         (uint8_t *)&temperatureCentiC,
                         sizeof(temperatureCentiC));

  emberEventControlSetDelayMS(temperatureReadControl, REPORT_PERIOD_MS);
}

static void findOnOffServer(void)
{
  // Find a binding to an On/Off server.
  for (EmberZclBindingId_t i = 0; i < EMBER_ZCL_BINDING_TABLE_SIZE; i++) {
    EmberZclBindingEntry_t entry = { 0 };
    if (emberZclGetBinding(i, &entry)) {
      if (!haveOnOffServer
          && emberZclAreClusterSpecsEqual(&emberZclClusterOnOffClientSpec,
                                          &entry.clusterSpec)) {
        haveOnOffServer = true;
        emZclReadDestinationFromBinding(&entry, &zclLight);
      }
    }
  }
  if (!haveOnOffServer) {
    emberAfCorePrintln("ERR: Not paired");
  }
}

void ezModeEventHandler(void)
{
  emberEventControlSetInactive(ezModeEventControl);
  emberAfCorePrintln("Starting EZ Mode...");
  emberZclStartEzMode();
}

void emberZclGetDefaultReportableChangeCallback(EmberZclEndpointId_t endpointId,
                                                const EmberZclClusterSpec_t * clusterSpec,
                                                EmberZclAttributeId_t attributeId,
                                                void * buffer,
                                                size_t bufferLength)
{
  // Defines reportable change values for each attribute that is being reported
}

void emberZclGetDefaultReportingConfigurationCallback(EmberZclEndpointId_t endpointId,
                                                      const EmberZclClusterSpec_t * clusterSpec,
                                                      EmberZclReportingConfiguration_t * configuration)
{
  //  Defines the default reporting configuration for each cluster on each endpoint.

  // We have reportable attributes in the OnOff server and teperature server
  if (emberZclAreClusterSpecsEqual(&emberZclClusterOnOffServerSpec,
                                   clusterSpec)) {
    configuration->minimumIntervalS = 1;
    configuration->maximumIntervalS = 60;
  } else if (emberZclAreClusterSpecsEqual(&emberZclClusterTempMeasurementServerSpec,
                                          clusterSpec)) {
    configuration->minimumIntervalS = 1;
    configuration->maximumIntervalS = 60;
  }
}

void emberZclNotificationCallback(const EmberZclNotificationContext_t * context,
                                  const EmberZclClusterSpec_t * clusterSpec,
                                  EmberZclAttributeId_t attributeId,
                                  const void * buffer,
                                  size_t bufferLength)
{
  // A notification has been recieved
  if (emberZclAreClusterSpecsEqual(&emberZclClusterOnOffServerSpec, clusterSpec)
      && attributeId == EMBER_ZCL_CLUSTER_ON_OFF_SERVER_ATTRIBUTE_ON_OFF) {
    // If the notification was from an OnOff server, report the change in state
    bool onOff = *((bool*)buffer);
    emberAfCorePrintln("Bound device is %s", (onOff ? "on" : "off"));
  }
}

void emberZclPostAttributeChangeCallback(EmberZclEndpointId_t endpointId,
                                         const EmberZclClusterSpec_t * clusterSpec,
                                         EmberZclAttributeId_t attributeId,
                                         const void * buffer,
                                         size_t bufferLength)
{
  // An attribute has changed value

  // When the OnOff attribute changes, toggle the LED to match

  if (emberZclAreClusterSpecsEqual(&emberZclClusterOnOffServerSpec, clusterSpec)
      && attributeId == EMBER_ZCL_CLUSTER_ON_OFF_SERVER_ATTRIBUTE_ON_OFF) {
    bool on = *((bool*)buffer);
    emberAfCorePrintln("Switching LED %s", (on ? "on." : "off."));
   // (on ? halSetLed : halClearLed)(BOARD_ACTIVITY_LED);
  }
}

void borderRouterDiscoveryHandler(EmberCoapCode code,
                                  uint8_t *uri,
                                  EmberCoapReadOptions *options,
                                  const uint8_t *payload,
                                  uint16_t payloadLength,
                                  const EmberCoapRequestInfo *info)
{
  // Announce to the remote address trying to discover
  announceToAddress(&info->remoteAddress);
}

// announce
void announceCommand(void)
{
  // We can force an announce to all mesh routers with an announce command
  announceToAddress(&allMeshRouters);
}

static void announceToAddress(const EmberIpv6Address *address)
{
  // We announce to the requested address in response to a discovery (or a CLI
  // command).
  MEMCOPY(&discoverRequestAddress, address, sizeof(EmberIpv6Address));

  emberAfCorePrint("Announcing to address to: ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&discoverRequestAddress));
  emberAfCorePrintln("");

  // Get the global address to return to the border router when attaching
  emberGetGlobalAddresses(NULL, 0); // all prefixes

  // The announcement will finish once the global address is returned
}

//callback is also used by sensor-actuator-implementation.c,so determine the current event and share code
void emberGetGlobalAddressReturn(const EmberIpv6Address *address,
                                 uint32_t preferredLifetime,
                                 uint32_t validLifetime,
                                 uint8_t addressFlags)
{
  if(emberEventControlGetActive(emZclOtaBootloadClientEventControl) == TRUE)
  {
      OTAGetGlobalAddressCallback(address, preferredLifetime, validLifetime, addressFlags);
  }
  else
  {
      if (emberIsIpv6UnspecifiedAddress(address)) {
        return;
      }

      MEMCOPY(&globalAddress, address, sizeof(EmberIpv6Address));
      emberIpv6AddressToString(&globalAddress,
                               globalAddressString,
                               sizeof(globalAddressString));
      emberAfCorePrintln("Global address assigned %s", globalAddressString);
      finishAnnounceToAddress();

      emberEventControlSetInactive(updateDisplayEventControl);
  }
}

static void finishAnnounceToAddress(void)
{
  EmberStatus status;

  // If we returned and got an IP address, return it otherwise just return
  // the EUI and device type
  // Send this CoAP response to the border router when it tries to discover:
  //     {
  //        "ip":"aaaa:0000:0000:0000:1234:1234:1234:1234",
  //        "eui64":"1234123412341234",
  //        "type":"sensor-actuator-node"
  //     }

  // NULL JSON that is a sensor-actuator-node

  char jsonString[140] =
    "{\"ip\":\"0000:0000:0000:0000:0000:0000:0000:0000\",\"eui64\":\"0000000000000000\",\"macExtID\":\"0000000000000000\",\"type\":\"rutledge\"}\0";
  uint8_t strSz = strlen("{\"ip\":\"");
  char euiKey[] = "\",\"eui64\":\"";
  char maxExtIDKey[] = "\",\"macExtID\":\"";
  char jsonClose[] = "\",\"type\":\"rutledge\"}";

  // If we got a global address put it in the JSON
  strncpy(jsonString + strSz, (char const*)globalAddressString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)globalAddressString);

  // Put the EUI64 in the JSON
  strncpy(jsonString + strSz, (char const*)euiKey, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)euiKey);
  strncpy(jsonString + strSz, (char const*)euiString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)euiString);

  // Put the macExtendedID in the JSON
    strncpy(jsonString + strSz, (char const*)maxExtIDKey, sizeof(jsonString) - strSz);
    strSz += strlen((char const*)maxExtIDKey);
    strncpy(jsonString + strSz, (char const*)exiString, sizeof(jsonString) - strSz);
    strSz += strlen((char const*)exiString);

  strncpy(jsonString + strSz, (char const*)jsonClose, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)jsonClose);

  // Use defaults
  EmberCoapSendInfo info = { 0 };
  status = emberCoapPost(&discoverRequestAddress,
                         deviceAnnounceUri,
                         (uint8_t const*)jsonString,
                         strlen(jsonString),
                         NULL, // handler
                         &info);
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Discovery announcement failed: 0x%x", status);
  }
}

void subscribeHandler(EmberCoapCode code,
                      uint8_t *uri,
                      EmberCoapReadOptions *options,
                      const uint8_t *payload,
                      uint16_t payloadLength,
                      const EmberCoapRequestInfo *info)
{
  EmberCoapCode responseCode;
  EmberIpv6Address addressOfSubscriber = { { 0 } };
  const EmberIpv6Address *newSubscriberAddress = &info->remoteAddress;
  int rc = 0;
  char* ipAddressKey;
  char ipAddressValue[64] = { 0 };
  int ipAddressValueSize = 0;
  char* portKey;
  uint16_t portValue = EMBER_COAP_PORT;

  // Check the payload to see if it wants us to subscribe to a remote address
  if (payload) {
    // Split JSON payload into key + value pairs
    //"{\"ip\":\"0000:0000:0000:0000:0000:0000:0000:0000\",\"port\":\"00000\"}\0";

    // Locate the address of the keys in the payload
    ipAddressKey = strstr((char const*)payload, IP_JSON_KEY);
    if (ipAddressKey != NULL) {
      rc = sscanf(ipAddressKey,
                  "%*[^:]:\"%[^\"]\"%n,",
                  ipAddressValue,
                  &ipAddressValueSize);
      if (rc == 1) {
        emberAfCorePrintln("Parsed JSON IP Address: \"%s\"", ipAddressValue);

        if (!emberIpv6StringToAddress((const uint8_t *)ipAddressValue,
                                      &addressOfSubscriber) ) {
          emberAfCorePrintln("Failed to read malformed subscriber address: %s",
                             ipAddressValue);
          // newSubscriberAddress is initialized to info->remoteAddress. If
          // payload was malformatted we automatically failover to the source
          // address of the request packet
        } else {
          newSubscriberAddress = &addressOfSubscriber;
        }
      } else {
        // If the address is malformatted, clear our local variable to denote
        // the error
        emberAfCorePrintln("Failed to read malformed IP Address (rc=%d", rc);
        ipAddressValue[0] = '\0';
      }
    }
    portKey = strstr((const char*)payload, PORT_JSON_KEY);
    if (portKey != NULL) {
      rc = sscanf(portKey, "%*[^:]:%hu[^\",}]", &portValue);
      if (rc == 1) {
        emberAfCorePrintln("Parsed JSON Port: %d", portValue);
      } else {
        emberAfCorePrintln("Failed to read malformed subscriber port (rc=%d)",
                           rc);
        portValue = EMBER_COAP_PORT;
      }
    }
  }

  if (state == WAIT_FOR_SUBSCRIPTIONS) {
    startReportingTemperature(newSubscriberAddress, portValue);
    responseCode = EMBER_COAP_CODE_204_CHANGED;
  } else {
    responseCode = EMBER_COAP_CODE_503_SERVICE_UNAVAILABLE;
  }

  emberCoapRespondWithCode(info, responseCode);
}

static void startReportingTemperature(const EmberIpv6Address *newSubscriberAddress,
                                      uint16_t newSubscriberPort)
{
  assert(state == WAIT_FOR_SUBSCRIPTIONS);

  MEMCOPY(&subscriberAddress, newSubscriberAddress, sizeof(EmberIpv6Address));
  subscriberPort = newSubscriberPort;
  failedReports = 0;

  emberAfCorePrint("Starting reporting to subscriber at [");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&subscriberAddress));
  emberAfCorePrintln("]:(%d)", subscriberPort);

  setNextState(REPORT_TEMP_TO_SUBSCRIBER);
}
static void stopReportingTemperature(void)
{
  // We stop reporting temperature in response to failed reports (or a CLI
  // command). Once we detach, we wait for a new subscriber.

  assert(state == REPORT_TEMP_TO_SUBSCRIBER
         || state == WAIT_FOR_DATA_CONFIRMATION);

  emberAfCorePrintln("Stopped reporting to subscriber");

  setNextState(WAIT_FOR_SUBSCRIPTIONS);
}


void buzzerHandler(EmberCoapCode code,
                   uint8_t *uri,
                   EmberCoapReadOptions *options,
                   const uint8_t *payload,
                   uint16_t payloadLength,
                   const EmberCoapRequestInfo *info)
{
#if defined(CORTEXM3_EFM32_MICRO)
  emberAfCorePrintln("WARN: Received buzzer command, no buzzer HW supported");
  emberCoapRespondWithCode(info, EMBER_COAP_CODE_404_NOT_FOUND);
#else
  emberAfCorePrintln("Sound Buzzer");
  halStackIndicatePresence();
  emberCoapRespondWithCode(info, EMBER_COAP_CODE_204_CHANGED);
#endif
}

static void sensorActuatorIdleState(void)
{
  assert(state == IDLE);
  // The ZCL subsystem and button0 double press ISR
  // handle EZ mode binding and all commands, so the
  // sensor/actuator remains in an Idle state after initialization.
  repeatStateWithDelay(WAIT_PERIOD_MS);
}

static uint32_t readTemperature(void)
{
  int32_t temperatureMC = 0;

#if defined(CORTEXM3_EFM32_MICRO)
  uint32_t tempRead = 0;
#else
  uint16_t value = 0;
  int16_t volts = 0;
#endif

#if defined(CORTEXM3_EFM32_MICRO)
  tempRead = adcRead();
  temperatureMC = convertToCelsius(tempRead);
#else
  halStartAdcConversion(ADC_USER_APP,
                        ADC_REF_INT,
                        TEMP_SENSOR_ADC_CHANNEL,
                        ADC_CONVERSION_TIME_US_256);
  halReadAdcBlocking(ADC_USER_APP, &value);
  volts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
  volts = halConvertValueToVolts(value);
  temperatureMC = (1591887L - (171 * (int32_t)volts)) / 10;
#endif
  return temperatureMC;
}

static void reportTemperatureStatusHandler(EmberCoapStatus status,
                                           EmberCoapCode code,
                                           EmberCoapReadOptions *options,
                                           uint8_t *payload,
                                           uint16_t payloadLength,
                                           EmberCoapResponseInfo *info)
{
  // We track the success or failure of reports so that we can determine when
  // we have lost the subscriber.  A series of consecutive failures is the
  // trigger to detach from the current subscriber and find a new one.  Any
  // successfully-transmitted report clears past failures.

  if (state == WAIT_FOR_DATA_CONFIRMATION) {
    if (status == EMBER_COAP_MESSAGE_ACKED) {
      failedReports = 0;
    } else {
      failedReports++;
      emberAfCorePrintln("ERR: Report timed out - failure %u of %u",
                         failedReports,
                         REPORT_FAILURE_LIMIT);
    }
    if (failedReports < REPORT_FAILURE_LIMIT) {
      setNextStateWithDelay(REPORT_TEMP_TO_SUBSCRIBER, REPORT_PERIOD_MS);
    } else {
      stopReportingTemperature();
    }
  }
}

void netResetHandler(EmberCoapCode code,
                     uint8_t *uri,
                     EmberCoapReadOptions *options,
                     const uint8_t *payload,
                     uint16_t payloadLength,
                     const EmberCoapRequestInfo *info)
{
  emberAfCorePrintln("Performing net reset from CoAP");
  // No CoAP response required, these will be non-confirmable requests
  emberConnectionManagerLeaveNetwork();
}

#if FALSE
void emberButtonPressIsr(uint8_t button, EmberButtonPress press)
{
  // Workaround for em3588 to filter spurious button interrupts seen on cold
  // boot, or loss of power. This workaround has a side-effect of ignoring the
  // first button press on 'warm' resets.  HAL does not currently have a method
  // to mask or ignore interrupts on startup.
  #if defined(CORTEXM3_EM3588_MICRO)
  static uint8_t button0Ignore = 1;
  static uint8_t button1Ignore = 1;
  if ((button0Ignore != 0) && button == BUTTON0) {
    button0Ignore--;
    return;
  }
  if ((button1Ignore != 0) && button == BUTTON1) {
    button1Ignore--;
    return;
  }
  #endif

  // Button 1 - triggers a net reset, then tries to join a network
  // Button 0 - if bound to an On/Off server, button 0 will send
  // an On/Off client 'toggle' command.

  switch (button) {
    case BUTTON0:
      switch (press) {
        case EMBER_SINGLE_PRESS:
          emberAfCorePrintln("Button 0 Pressed");
          // Only report a button press if we have a bind
          findOnOffServer();

          if (haveOnOffServer) {
            emberAfCorePrintln("Sending a toggle.");
            emberZclSendClusterOnOffServerCommandToggleRequest(&zclLight, NULL, NULL);
          }
          break;

        case EMBER_DOUBLE_PRESS:
          emberAfCorePrintln("Activating EZ Mode...");
          emberEventControlSetActive(ezModeEventControl);
          break;

        default:
          assert(false);
          break;
      }
      break;

    case BUTTON1:
      switch (press) {
        case EMBER_SINGLE_PRESS:
          emberAfCorePrintln("Button 1 Pressed");
          emberConnectionManagerStartConnect();
          break;

        case EMBER_DOUBLE_PRESS:
          break;

        default:
          assert(false);
          break;
      }
      break;

    default:
      assert(false);
      break;
  }
  ;

}
#endif

void emberUdpHandler(const uint8_t *destination,
                     const uint8_t *source,
                     uint16_t localPort,
                     uint16_t remotePort,
                     const uint8_t *payload,
                     uint16_t payloadLength)
{
  ALIAS(emberUdpHandler)(destination,
                         source,
                         localPort,
                         remotePort,
                         payload,
                         payloadLength);
}

bool emberAfPluginIdleSleepOkToSleepCallback(uint32_t durationMs)
{
  // Once we join to a network, we will automatically stay awake, because we
  // are a router.  Before we join, we would could sleep, but we prevent that
  // by returning false here.

#ifdef BADGE_DEVICE  //smart tag device
	  return true;
#else 					//smart relay device
	  return false;
#endif

}

//TODO: need to update for subscriber mechanism from rftagid project
void stateEventHandler(void)
{
  emberEventControlSetInactive(stateEventControl);
  emberEventControlSetActive(updateDisplayEventControl);

  switch (state) {
    case IDLE:
      sensorActuatorIdleState();
      break;
    default:
      assert(false);
      break;
  }
}

static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs)
{
  state = nextState;
  emberEventControlSetDelayMS(stateEventControl, delayMs);
}

static void formatEuiString(uint8_t* euiString, size_t strSz, const uint8_t* euiBytes)
{
  uint8_t i;
  uint8_t strOffset = 0;
  uint8_t *euiStringEnd = euiString + strSz;

  assert(euiString != NULL);
  assert(strSz > 0);

  // Prints two nybles in hex format into euiString per byte in euiBytes
  for (i = 0; i < EUI64_SIZE && (euiString + strOffset < euiStringEnd); i++) {
    snprintf((char*)(euiString + strOffset),
             strSz,
             "%X%X",
             (euiBytes[7 - i] >> 4 & 0x0F),
             (euiBytes[7 - i] & 0x0F));
    strOffset += 2;
  }
}

// Helper function used to specify source/destination port
static EmberStatus coapSendUri(EmberCoapCode code,
                               const EmberIpv6Address *destination,
                               const uint16_t destinationPort,
                               const uint16_t sourcePort,
                               const uint8_t *uri,
                               const uint8_t *body,
                               uint16_t bodyLength,
                               EmberCoapResponseHandler responseHandler)
{
  // Use defaults for everything except the ports.
  EmberCoapSendInfo info = {
    .localPort = sourcePort,
    .remotePort = destinationPort,
  };
  return emberCoapSend(destination,
                       code,
                       uri,
                       body,
                       bodyLength,
                       responseHandler,
                       &info);
}

//vlad
static void startScanAir() {
	emberAfCorePrintln("startScanAir");
	initDataBeforeScan();

}
//vlad TOP3 it just a terminology introduced by GLENN WEST
int top3Compare(const void *s1, const void *s2) {
	RutledgeScanData *e1 = (RutledgeScanData *) s1;
	RutledgeScanData *e2 = (RutledgeScanData *) s2;
	return e2->rssi - e1->rssi;
}

void sendAir(uint8_t status) {
	/*for (int i = 0 ; i <scanCounter; i++)
	 {
	 RutledgeScanData * tmp;
	 tmp = scanData+i;
	 emberAfCorePrintln("%d: 0x%4x ", i, tmp);
	 emberAfCorePrint("long id: ");
	 emberAfCoreDebugExec(emberAfPrintExtendedPanId(tmp->longId));
	 emberAfCorePrintln("");
	 emberAfCorePrintln("rssi: %d dBm", tmp->rssi);

	 }*/
	bool isNeededToSend = false;
	scanCounter = MIN(MAX_TOP_DEVICES, scanCounter);
	emberAfCorePrintln("\tscanCounter: %d ", scanCounter);
	qsort(scanData, scanCounter, sizeof(RutledgeScanData), top3Compare);
	uint8_t  i, j, totalCompared = 0;
	float maxRssi, minRssi;
	char tempS[32] = { 0 }, tempT[32] = { 0 };
	RutledgeScanData * tmpS;
	RutledgeScanData * tmpT;
	for (i = 0; i < scanCounter && !isNeededToSend; i++) {
		tmpS = scanData+i;
		formatEuiString(tempS, sizeof(exiString), tmpS->longId);
		emberAfCorePrint("\t\t[-]::tmpS->longId: %s compare with ", tempS);
		//for (j = 0; j < scanCounter && !isNeededToSend; j++) {
			tmpT = top3Data+i;
			formatEuiString(tempT, sizeof(exiString), tmpT->longId);
			emberAfCorePrintln(" [-]::tmpT->longId: %s ", tempT);
			if (strcmp(tempS, tempT) == 0) {

				maxRssi = (float)MAX(tmpS->rssi, tmpT->rssi);
				minRssi = (float)MIN(tmpS->rssi, tmpT->rssi);
				/*emberAfCorePrintln("maxRssi: %d ", (int)maxRssi);
				emberAfCorePrintln("minRssi: %d ", (int)minRssi);
				emberAfCorePrintln("maxRssi: %d ", (int)(maxRssi*(100+COMPARE_DEVIATION_PERCENT) / 100));
				emberAfCorePrintln("minRssi: %d ", (int)(minRssi*(100+COMPARE_DEVIATION_PERCENT) / 100));*/
				if (abs(minRssi-maxRssi)
						> abs(maxRssi*COMPARE_DEVIATION_PERCENT / 100)) {

				}
			}else{
				isNeededToSend = true;
									break;
			}
			totalCompared++;
		//}
	}
	emberAfCorePrintln("totalCompared: %d , isNeededToSend %d", totalCompared, isNeededToSend);
	if (isNeededToSend || totalCompared!=scanCounter) {
		MEMCOPY(top3Data, scanData,
		MAX_TOP_DEVICES * sizeof(RutledgeScanData));
		char tempString[32] = { 0 };
		char tempString2[37] = { 0 };
		// NULL JSON that returns the temp and ID of the device
		char jsonString[512] =
				"{\"ID\":\"0000000000000000\",\"rssi\":\"0\"}";
		char deviceKey[] = "\",\"d%d\":\"";
		char rssiKey[] = "\",\"r%d\":\"";
		char jsonClose[] = "\"}";
		uint8_t strSz = strlen("{\"ID\":\"");
		strncpy(jsonString + strSz, (char const*) exiString,
				sizeof(jsonString) - strSz);
		strSz += strlen((char const*) exiString);
		EPD_W21_POWERPIN_HIGH();
		for (i = 0; i < scanCounter; i++) {
			tmpS = scanData + i;
			///emberAfCorePrintln("%d: 0x%4x ", i, tmp);
			/*emberAfCorePrint("long id: ");
			 emberAfCoreDebugExec(emberAfPrintExtendedPanId(tmp->longId));
			 emberAfCorePrintln("");
			 emberAfCorePrintln("rssi: %d dBm", tmp->rssi);*/
			// Put the temp in JSON, for the end of the string
			sprintf(tempString, deviceKey, i);

			//sprintf(tempString, "%s", tmp->longId);
			strncpy(jsonString + strSz, (char const*) tempString,
					sizeof(jsonString) - strSz);
			strSz += strlen((char const*) tempString);
			formatEuiString(tempString, sizeof(exiString), tmpS->longId);

			if (!i) {
				sprintf(tempString2, "EXM:%s|EUI:%s",
						(char const*) exiString, (char const*) euiString);
				EPD_DisplayScanInfo(i, tempString2);

			}
				sprintf(tempString2, "TOP%d:[%s][%d]",
										 i+1,(char const*) tempString, tmpS->rssi);



			strncpy(jsonString + strSz, (char const*) tempString,
					sizeof(jsonString) - strSz);
			strSz += strlen((char const*) tempString);
			sprintf(tempString, rssiKey, i);
			strncpy(jsonString + strSz, (char const*) tempString,
					sizeof(jsonString) - strSz);
			strSz += strlen((char const*) tempString);

			sprintf(tempString, "%d", tmpS->rssi);
			strncpy(jsonString + strSz, (char const*) tempString,
					sizeof(jsonString) - strSz);
			strSz += strlen((char const*) tempString);
			EPD_DisplayScanInfo(i+1, tempString2);
			MEMSET(tempString,0, sizeof(tempString));
			MEMSET(tempString2,0, sizeof(tempString2));
		}
		EPD_UpdateDisplay();
		EPD_W21_POWERPIN_LOW();
		strncpy(jsonString + strSz, (char const*) jsonClose,
				sizeof(jsonString) - strSz);
		strSz += strlen((char const*) jsonClose);

		status = coapSendUri(EMBER_COAP_CODE_POST, &subscriberAddress,
				subscriberPort, //dport
				EMBER_COAP_PORT, //sport
				temperatureUri, (uint8_t const*) jsonString, strlen(jsonString),
				&reportTemperatureStatusHandler);

		if (status == EMBER_SUCCESS) {
			setNextState(WAIT_FOR_DATA_CONFIRMATION);
		} else {
			emberAfCorePrintln("ERR: Reporting failed: 0x%x", status);
			repeatStateWithDelay(REPORT_PERIOD_MS);
		}
	} else {
		setNextStateWithDelay(REPORT_TEMP_TO_SUBSCRIBER, REPORT_PERIOD_MS);
	}
}


#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
static char* nodeTypeToString(EmberNodeType nodeType)
{
  static char* returnString[] = { "UNKNOWN_DEVICE ",   // 0
                                  "ERROR          ",   // 1
                                  "ROUTER         ",   // 2
                                  "END_DEVICE     ",   // 3
                                  "SLEEPY_END_DEV ",   // 4
                                  "ERROR          ",   // 5
                                  "ERROR          ",   // 6
                                  "COMMISSIONER   " };  // 7
  switch (nodeType) {
    case EMBER_UNKNOWN_DEVICE:
      return returnString[EMBER_UNKNOWN_DEVICE];
      break;
    case EMBER_ROUTER:
      return returnString[EMBER_ROUTER];
      break;
    case EMBER_END_DEVICE:
      return returnString[EMBER_END_DEVICE];
      break;
    case EMBER_SLEEPY_END_DEVICE:
      return returnString[EMBER_SLEEPY_END_DEVICE];
      break;
    case EMBER_COMMISSIONER:
      return returnString[EMBER_COMMISSIONER];
      break;
    default:
      return returnString[1];
      break;
  }
}
#endif

void updateDisplayEventHandler(void)
{
  emberEventControlSetInactive(updateDisplayEventControl);
#if true

  emberAfCorePrintln("Testing ABCDEFGHIJKLMNOPQRSTX");

  emberEventControlSetDelayMS(updateDisplayEventControl, REPORT_PERIOD_MS);


#endif


#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
  EmberNetworkStatus networkStatus;

  lcdPrintCurrentDeviceState();
  networkStatus = emberNetworkStatus();
  lcdPrintNetworkStatus(networkStatus, 15);

  emberEventControlSetDelayMS(updateDisplayEventControl, LCD_REFRESH_MS);
#endif
}

// Utility functions provided to make writing to the LCD on the WSTK easier

#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
void lcdPrintCurrentDeviceState(void)
{
  EmberNetworkStatus networkStatus;
  EmberNetworkParameters parameters = { { 0 } };
  char tempString[LCD_ASCII_SCREEN_WIDTH + 1] = { 0 };

  // Clear the screen
  GLIB_clear(&glibContext);

  // Print EUI
  sprintf(tempString, "EUI:%s", euiString);
  GLIB_drawString(&glibContext,
                  tempString,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  EUI_ROW,
                  LCD_OPAQUE_VALUE);

  // Print the join key
  lcdPrintJoinCode(joinKey, JOIN_KEY_ROW);

  // Print network status
  networkStatus = emberNetworkStatus();
  emberEventControlSetActive(updateDisplayEventControl);

  // If we are on a network, we can print other network parameters
  if (networkStatus != EMBER_NO_NETWORK) {
    uint8_t i;

    // Get network parameters
    emberGetNetworkParameters(&parameters);

    // Print Node Type
    sprintf(tempString, "Type: %s", nodeTypeToString(parameters.nodeType));
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    NODE_TYPE_ROW,
                    LCD_OPAQUE_VALUE);

    // Print network ID
    MEMCOPY(tempString, parameters.networkId, EMBER_NETWORK_ID_SIZE);
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    NETWORK_ID_ROW,
                    LCD_OPAQUE_VALUE);

    // Print the PAN ID
    sprintf(tempString,
            "Pan:%x%x%x%x%x%x%x%x",
            parameters.extendedPanId[7],
            parameters.extendedPanId[6],
            parameters.extendedPanId[5],
            parameters.extendedPanId[4],
            parameters.extendedPanId[3],
            parameters.extendedPanId[2],
            parameters.extendedPanId[1],
            parameters.extendedPanId[0]);

    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    PAN_ID_ROW,
                    LCD_OPAQUE_VALUE);

    // Print channel
    sprintf(tempString, "Channel: %d", parameters.channel);
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    CHANNEL_ROW,
                    LCD_OPAQUE_VALUE);

    // Print IP addresses
    for (i = 0; i < EMBER_MAX_IPV6_ADDRESS_COUNT; i++) {
      EmberIpv6Address address;
      if (emberGetLocalIpAddress(i, &address)) {
        lcdPrintIpv6Address(&address,
                            IP_ADDRESS_INITIAL_ROW + ROW_SIZE * i,
                            LCD_ASCII_SCREEN_WIDTH);
      }
    }
  }
  DMD_updateDisplay();
}

void lcdPrintJoinCode(uint8_t *joinKey, uint8_t printRow)
{
  char strJoinKey[LCD_ASCII_SCREEN_WIDTH + 1] = { 0 };
  sprintf(strJoinKey, "Join Phrase:%s", joinKey);
  GLIB_drawString(&glibContext,
                  strJoinKey,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  printRow,
                  LCD_OPAQUE_VALUE);
}

void lcdPrintNetworkStatus(EmberNetworkStatus status, uint8_t printRow)
{
  switch (status) {
    case EMBER_NO_NETWORK:
      GLIB_drawString(&glibContext,
                      "NS: NO_NETWORK",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    case EMBER_SAVED_NETWORK:
      GLIB_drawString(&glibContext,
                      "NS: SAVED_NETWORK",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    case EMBER_JOINING_NETWORK:
      GLIB_drawString(&glibContext,
                      "NS: JOINING_NETWORK",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    case EMBER_JOINED_NETWORK_ATTACHING:
      GLIB_drawString(&glibContext,
                      "NS: JOINED_ATTACHING",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    case EMBER_JOINED_NETWORK_ATTACHED:
      GLIB_drawString(&glibContext,
                      "NS: JOINED_ATTACHED",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    case EMBER_JOINED_NETWORK_NO_PARENT:
      GLIB_drawString(&glibContext,
                      "NS: JOINED_NO_PARENT",
                      PADDED_ASCII_BUFFER_SIZE,
                      START_COLUMN,
                      printRow,
                      LCD_OPAQUE_VALUE);
      break;
    default:
      //PRINT ERROR
      break;
  }
}

void lcdPrintIpv6Address(const EmberIpv6Address *address,
                         uint8_t printRow,
                         uint8_t screenWidth)
{
  uint8_t addrString[EMBER_IPV6_ADDRESS_STRING_SIZE] = { 0 };
  char shortAddrString[LCD_ASCII_SCREEN_WIDTH + 1] = "....................";
  uint8_t i;
  uint8_t addrLen;

  emberIpv6AddressToString(address, addrString, sizeof(addrString));
  addrLen = strlen((char const*)addrString);

  // The LCD is too short to print the entire address so print the first set of
  // characters
  for (i = 0; i < IP_ADDRESS_CHAR_TRIM; i++) {
    shortAddrString[i] = addrString[i];
  }
  // Then print the last set of characters
  for (i = screenWidth - IP_ADDRESS_CHAR_TRIM; i < screenWidth; i++) {
    shortAddrString[i] = addrString[addrLen - screenWidth + i];
  }
  GLIB_drawString(&glibContext,
                  shortAddrString,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  printRow,
                  LCD_OPAQUE_VALUE);
}

#endif //EMBER_AF_PLUGIN_GLIB

/* revision history
DATE(dd/mm/yy) AUTHOR description
06/02/18  tgbl

 TODO: porting the subscribe/broadcast mechanism from rftagid project
 waitForSubscriptions
 subscribeHandler
 startReportTemperature
 staeeventhandler fomr rftagid- leverage fomr connection manager?
  susbscriber boracast
  stopreportcommand
  stopreportingtemperature
  reportTemperatureStatusHandler using coap messaging for subscriber mechanism
  reportTemperature
*/

