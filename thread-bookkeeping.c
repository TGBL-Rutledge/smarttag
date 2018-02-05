// This file is generated by Simplicity Studio.  Please do not edit manually.
//
//

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_ZCL_CORE

// Main function declarations.
void emberAfMainCallback(MAIN_FUNCTION_PARAMETERS);

void emAfMain(MAIN_FUNCTION_PARAMETERS)
{
  emberAfMainCallback(MAIN_FUNCTION_ARGUMENTS);
}

// Init function declarations.
void emberAfInitCallback(void);
void emberCommandReaderInit(void);
void emberAfPluginEepromInitCallback(void);
void emberAfPluginHeartbeatNodeTypeInit(void);
void emZclOtaBootloadClientInitCallback(void);
void emZclOtaBootloadStorageEepromInitCallback(void);
void emZclInitHandler(void);

void emAfInit(void)
{
  emberAfInitCallback();
  emberCommandReaderInit();
  emberAfPluginEepromInitCallback();
  emberAfPluginHeartbeatNodeTypeInit();
  emZclOtaBootloadClientInitCallback();
  emZclOtaBootloadStorageEepromInitCallback();
  emZclInitHandler();
}

// Tick function declarations.
void emberTick(void);
void emberAfTickCallback(void);
void emberAfPluginCliTickCallback(void);
void emberSerialBufferTick(void);

void emAfTick(void)
{
  emberTick();
  emberAfTickCallback();
  emberAfPluginCliTickCallback();
  emberSerialBufferTick();
}

// MarkApplicationBuffers function declarations.
void emberAfMarkApplicationBuffersCallback(void);
void emZclCacheMarkApplicationBuffersHandler(void);
void emZclReportingMarkApplicationBuffersHandler(void);

void emAfMarkApplicationBuffers(void)
{
  emberAfMarkApplicationBuffersCallback();
  emZclCacheMarkApplicationBuffersHandler();
  emZclReportingMarkApplicationBuffersHandler();
}

// NetworkStatus function declarations.
void emberAfNetworkStatusCallback(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emConnectionManagerJibNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emZclOtaBootloadClientNetworkStatusCallback(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emZclBindingNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emZclCacheNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emZclGroupNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
void emZclReportingNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);

void emberNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason)
{
  emberAfNetworkStatusCallback(newNetworkStatus, oldNetworkStatus, reason);
  emConnectionManagerJibNetworkStatusHandler(newNetworkStatus, oldNetworkStatus, reason);
  emZclOtaBootloadClientNetworkStatusCallback(newNetworkStatus, oldNetworkStatus, reason);
  emZclBindingNetworkStatusHandler(newNetworkStatus, oldNetworkStatus, reason);
  emZclCacheNetworkStatusHandler(newNetworkStatus, oldNetworkStatus, reason);
  emZclGroupNetworkStatusHandler(newNetworkStatus, oldNetworkStatus, reason);
  emZclReportingNetworkStatusHandler(newNetworkStatus, oldNetworkStatus, reason);
}

// PreAttributeChange function declarations.
bool emberZclPreAttributeChangeCallback(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength);

bool emZclPreAttributeChange(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength)
{
  return emberZclPreAttributeChangeCallback(endpointId, clusterSpec, attributeId, buffer, bufferLength);
}

// PostAttributeChange function declarations.
void emberZclPostAttributeChangeCallback(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength);
void emZclIdentifyServerPostAttributeChangeHandler(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength);
void emZclReportingPostAttributeChangeHandler(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength);

void emZclPostAttributeChange(EmberZclEndpointId_t endpointId, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength)
{
  emberZclPostAttributeChangeCallback(endpointId, clusterSpec, attributeId, buffer, bufferLength);
  emZclIdentifyServerPostAttributeChangeHandler(endpointId, clusterSpec, attributeId, buffer, bufferLength);
  emZclReportingPostAttributeChangeHandler(endpointId, clusterSpec, attributeId, buffer, bufferLength);
}

// Notification function declarations.
void emberZclNotificationCallback(const EmberZclNotificationContext_t *context, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength);

void emZclNotification(const EmberZclNotificationContext_t *context, const EmberZclClusterSpec_t *clusterSpec, EmberZclAttributeId_t attributeId, const void *buffer, size_t bufferLength)
{
  emberZclNotificationCallback(context, clusterSpec, attributeId, buffer, bufferLength);
}
