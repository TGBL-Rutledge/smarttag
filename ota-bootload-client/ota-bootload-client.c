// Copyright 2017 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_HAL
#include EMBER_AF_API_STACK
#include EMBER_AF_API_ZCL_CORE
#include EMBER_AF_API_ZCL_CORE_WELL_KNOWN
#include EMBER_AF_API_ZCL_OTA_BOOTLOAD_CORE
#include EMBER_AF_API_ZCL_OTA_BOOTLOAD_STORAGE_CORE
#include "ota-bootload-client.h"
#include <stdio.h>

#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#else
  #define emberAfPluginOtaBootloadClientPrint(...)
  #define emberAfPluginOtaBootloadClientPrintln(...)
  #define emberAfPluginOtaBootloadClientFlush()
  #define emberAfPluginOtaBootloadClientDebugExec(x)
  #define emberAfPluginOtaBootloadClientPrintBuffer(buffer, len, withSpace)
  #define emberAfPluginOtaBootloadClientPrintString(buffer)
#endif

#ifdef EMBER_SCRIPTED_TEST
  #include "ota-bootload-client-test.h"
#else
  #include "thread-callbacks.h"
#endif

// TODO: if the network goes down, set the state to NONE and erase global data.
// TODO: handle ImageNotify command.
// TODO: should we use the event-queue system to pass global data around?
// TODO: run the ota-bootload-client-test.c with eeprom storage implementation.
// TODO: add callback for each block rx? So app can resize/backoff next block?

// -----------------------------------------------------------------------------
// Constants

#define MAX_JITTER_MS (MILLISECOND_TICKS_PER_SECOND * 10)

// Larger blocksizes produce shorter OTA times
#define DEFAULT_BLOCK_SIZE_LOG 8 /* 128 */

#define DOWNLOAD_BLOCK_ERROR_BACKOFF_MS 100
#define UPGRADE_END_RESPONSE_BACKOFF_MS 100

// -----------------------------------------------------------------------------
// Types

enum {
  DELAY_TYPE_MILLISECONDS = 0x00,
  DELAY_TYPE_SECONDS      = 0x01,
  DELAY_TYPE_MINUTES      = 0x02,
};
typedef uint8_t DelayType;

enum {
  STATE_NONE,
  STATE_DISCOVER_SERVER,
  STATE_QUERY_NEXT_IMAGE,
  STATE_DOWNLOAD,
  STATE_VERIFY_DOWNLOAD,
  STATE_UPGRADE_END,
  STATE_BOOTLOAD,
};
typedef uint8_t State_t;
static State_t state;

typedef struct {
  uint16_t nextBlockNumber;
  uint8_t logBlockSize;
  uint8_t path[32]; // TODO: how big should this be?
} BlockResponseData;

// -----------------------------------------------------------------------------
// Globals

EmberEventControl emZclOtaBootloadClientEventControl;

static EmberZclOtaBootloadClientServerInfo_t serverInfo;
#define HAVE_SERVER_INFO()  (serverInfo.endpointId != EMBER_ZCL_ENDPOINT_NULL)
#define ERASE_SERVER_INFO() (serverInfo.endpointId = EMBER_ZCL_ENDPOINT_NULL)

static EmberZclOtaBootloadFileSpec_t fileSpecToDownload;
#define HAVE_FILE_SPEC_TO_DOWNLOAD()  (fileSpecToDownload.version != EMBER_ZCL_OTA_BOOTLOAD_FILE_VERSION_NULL)
#define ERASE_FILE_SPEC_TO_DOWNLOAD() (fileSpecToDownload.version = EMBER_ZCL_OTA_BOOTLOAD_FILE_VERSION_NULL)

static EmberZclStatus_t downloadStatus;
#define HAVE_DOWNLOAD_STATUS() (downloadStatus != EMBER_ZCL_STATUS_NULL)
#define ERASE_DOWNLOAD_STATUS() (downloadStatus = EMBER_ZCL_STATUS_NULL)

static bool ipAddressResolved;
// -----------------------------------------------------------------------------
// Private API

static void scheduleQueryNextImage(bool addJitter);
static void queryNextImageResponseHandler(EmberZclMessageStatus_t status,
                                          const EmberZclCommandContext_t *context,
                                          const EmberZclClusterOtaBootloadServerCommandQueryNextImageResponse_t *response);
static void processQueryNextImageResponse(const EmberZclClusterOtaBootloadServerCommandQueryNextImageResponse_t *response);
static void blockResponseHandler(EmberCoapStatus status,
                                 EmberCoapCode code,
                                 EmberCoapReadOptions *options,
                                 uint8_t *payload,
                                 uint16_t payloadLength,
                                 EmberCoapResponseInfo *info);
static void downloadComplete(void);
static void scheduleUpgradeEnd(void);
static void upgradeEndResponseHandler(EmberZclMessageStatus_t status,
                                      const EmberZclCommandContext_t *context,
                                      const EmberZclClusterOtaBootloadServerCommandUpgradeEndResponse_t *response);
static void processUpgradeEndResponse(const EmberZclClusterOtaBootloadServerCommandUpgradeEndResponse_t *response);

static uint32_t convertToMs(uint32_t delay,
                            DelayType delayType,
                            bool addJitter)
{
  const uint8_t delayShifts[] = {
    0,  // DELAY_TYPE_MILLISECONDS
    10, // DELAY_TYPE_SECONDS
    16, // DELAY_TYPE_MINUTES
  };
  assert(delayType < COUNTOF(delayShifts));
  delay <<= delayShifts[delayType];

  if (addJitter) {
    delay += (halCommonGetRandom() % MAX_JITTER_MS);
  }

  return delay;
}

static void setState(State_t newState, uint32_t delayMs)
{
  state = newState;
  emberEventControlSetDelayMS(emZclOtaBootloadClientEventControl, delayMs);
}

static void serverInfoToZclDestination(EmberZclDestination_t *destination)
{
  // TODO: send this command to an IPV6 address until UID discovery works.
  assert(HAVE_SERVER_INFO());
  destination->network.address = serverInfo.address;
  destination->network.flags =
    (serverInfo.scheme == EMBER_ZCL_SCHEME_COAPS
     ? (EMBER_ZCL_USE_COAPS_FLAG | EMBER_ZCL_HAVE_IPV6_ADDRESS_FLAG)
     : EMBER_ZCL_HAVE_IPV6_ADDRESS_FLAG);
  destination->network.port = serverInfo.port;
  destination->application.data.endpointId = serverInfo.endpointId;
  destination->application.type = EMBER_ZCL_APPLICATION_DESTINATION_TYPE_ENDPOINT;
}

static void noneStateHandler(void)
{
}

// -------------------------------------
// OTA server discovery

static void scheduleDiscoverServer(bool eraseServerInfo, bool addJitter)
{
  if (eraseServerInfo) {
    ERASE_SERVER_INFO();
  }
  setState(STATE_DISCOVER_SERVER,
           convertToMs(EMBER_AF_PLUGIN_OTA_BOOTLOAD_CLIENT_DISCOVER_SERVER_PERIOD_MINUTES,
                       DELAY_TYPE_MINUTES,
                       addJitter));
}

static void dnsLookupResponseHandler(EmberDnsLookupStatus status,
                                     const uint8_t *domainName,
                                     uint8_t domainNameLength,
                                     const EmberDnsResponse *response,
                                     void *applicationData,
                                     uint16_t applicationDataLength)
{
  emberAfPluginOtaBootloadClientPrintln("dnsLookupResponseHandler: %d", status);
  switch (status) {
    case EMBER_DNS_LOOKUP_SUCCESS:
      serverInfo.address = response->ipAddress;
      emberAfPluginOtaBootloadClientPrint("Found via DNS: ");
      emberAfPluginOtaBootloadClientDebugExec(emberAfPrintIpv6Address(&serverInfo.address));
      emberAfPluginOtaBootloadClientPrintln("->%d", serverInfo.port);
      scheduleQueryNextImage(false);
      break;
    case EMBER_DNS_LOOKUP_NO_BORDER_ROUTER:
    case EMBER_DNS_LOOKUP_NO_BORDER_ROUTER_RESPONSE:
    case EMBER_DNS_LOOKUP_NO_DNS_RESPONSE:
      emberAfPluginOtaBootloadClientPrintln("No DNS response: %d", status);
      scheduleDiscoverServer(true, false); // Let's try again
      break;
    case EMBER_DNS_LOOKUP_BORDER_ROUTER_RESPONSE_ERROR:
    case EMBER_DNS_LOOKUP_NO_DNS_RESPONSE_ERROR:
      emberAfPluginOtaBootloadClientPrintln("DNS Error: %d", status);
      scheduleDiscoverServer(true, false); // Let's try again
      break;
    case EMBER_DNS_LOOKUP_NO_DNS_SERVER:
    case EMBER_DNS_LOOKUP_NO_ENTRY_FOR_NAME:
    case EMBER_DNS_LOOKUP_NO_BUFFERS:
      emberAfPluginOtaBootloadClientPrintln("Can't lookup host: %d", status);
      scheduleDiscoverServer(true, false); // Let's try again
      break;
    default:
      assert(false);
  }
}

static void discoverServerResponseHandler(EmberCoapStatus status,
                                          EmberCoapCode code,
                                          EmberCoapReadOptions *options,
                                          uint8_t *payload,
                                          uint16_t payloadLength,
                                          EmberCoapResponseInfo *info)
{
  if (HAVE_SERVER_INFO()) { // We have already found a server to download from
    return;
  }
  switch (status) {
    case EMBER_COAP_MESSAGE_TIMED_OUT:
    case EMBER_COAP_MESSAGE_RESET:
      // Since our discovery is multicast, we get the TIMED_OUT status when we
      // are done receiving responses (note that we could have received no
      // responses). If we found a server, we should have already moved on to the
      // next state. Otherwise, then we keep trying to discover a server.
      scheduleDiscoverServer(false, false); // eraseServerInfo? addJitter?
      break;
    case EMBER_COAP_MESSAGE_ACKED:
      // This means that our request was ACK'd, but the actual response is coming.
      break;
    case EMBER_COAP_MESSAGE_RESPONSE:
    {
      if (payloadLength != 0) {
        // TODO: when am I COAPS?
        // TODO: shouldn't the UID be contained somewhere in this response?
        // TODO: how do we get the endpoint from the response?
        serverInfo.scheme = EMBER_ZCL_SCHEME_COAP;
        serverInfo.address = info->remoteAddress;
        serverInfo.port = info->remotePort;
        emberAfPluginOtaBootloadClientPrint("Discovered server: ");
        emberAfPluginOtaBootloadClientDebugExec(emberAfPrintIpv6Address(&serverInfo.address));
        emberAfPluginOtaBootloadClientPrintln("->%d", serverInfo.port);
        MEMSET(serverInfo.uid.bytes, 0xA0, EMBER_ZCL_UID_SIZE);
        serverInfo.endpointId = 1;
        if (!emberZclOtaBootloadClientServerDiscoveredCallback(&serverInfo)) {
          ERASE_SERVER_INFO();
        } else {
          scheduleQueryNextImage(false);
        }
      } else {
        scheduleDiscoverServer(false, false); // eraseServerInfo? addJitter?
      }
      break;
    }
    default:
      assert(false);
  }
}

void OTAGetGlobalAddressCallback(const EmberIpv6Address *address,
                                 uint32_t preferredLifetime,
                                 uint32_t validLifetime,
                                 uint8_t addressFlags)
{
	  if (ipAddressResolved) {
	    return;
	  }
	  if (emberIsIpv6UnspecifiedAddress(address)) {
	    emberAfPluginOtaBootloadClientPrintln("DNS Lookup: Failed to get Global Address");
	    scheduleDiscoverServer(true, false); // addJitter?
	    return;
	  }
	  if (emberDnsLookup(serverInfo.name,
	                     serverInfo.nameLength,
	                     address->bytes,
	                     dnsLookupResponseHandler,
	                     NULL,
	                     0) != EMBER_SUCCESS) {
	    emberAfPluginOtaBootloadClientPrintln("DNS Lookup: Failed to lookup hostname");
	    scheduleDiscoverServer(true, false); // addJitter?
	  } else {
	    ipAddressResolved = true;
	  }
}

static void discoverServerStateHandler(void)
{
  if (HAVE_SERVER_INFO()) { // we have the server, go to next state.
    scheduleQueryNextImage(false); // addJitter?
  } else if ( emberZclOtaBootloadClientServerHasStaticAddressCallback(&serverInfo)) {
    emberAfPluginOtaBootloadClientPrint("Using static OTA server IP: ");
    emberAfPluginOtaBootloadClientDebugExec(emberAfPrintIpv6Address(&serverInfo.address));
    emberAfPluginOtaBootloadClientPrintln("->%d", serverInfo.port);
    scheduleQueryNextImage(false);
  } else if ( emberZclOtaBootloadClientServerHasDnsNameCallback(&serverInfo) ) {
    emberAfPluginOtaBootloadClientPrintln("Using DNS lookup");
    ipAddressResolved = false;
    emberGetGlobalAddresses(NULL, 0); // force an update of the global address
  } else if (emberZclOtaBootloadClientServerHasDiscByClusterId(&emberZclClusterOtaBootloadServerSpec, discoverServerResponseHandler)) {
    emberAfPluginOtaBootloadClientPrintln("Using upgrade server discovery");
  } else {
    emberAfPluginOtaBootloadClientPrintln("Failed to discoverServer");
    scheduleDiscoverServer(true, false); // addJitter?
  }
}

// -------------------------------------
// Query Next Image

static void scheduleQueryNextImage(bool addJitter)
{
  setState(STATE_QUERY_NEXT_IMAGE,
           convertToMs(EMBER_AF_PLUGIN_OTA_BOOTLOAD_CLIENT_QUERY_NEXT_IMAGE_PERIOD_MINUTES,
                       DELAY_TYPE_MINUTES,
                       addJitter));
}

static void queryNextImageStateHandler(void)
{
  EmberZclOtaBootloadFileSpec_t fileSpec;
  EmberZclOtaBootloadHardwareVersion_t hardwareVersion
    = EMBER_ZCL_OTA_BOOTLOAD_HARDWARE_VERSION_NULL;
  if (!emberZclOtaBootloadClientGetQueryNextImageParametersCallback(&fileSpec,
                                                                    &hardwareVersion)) {
    scheduleQueryNextImage(false); // addJitter?
    return;
  }

  // TODO: should we create our own constants for this fieldControl value?
  EmberZclDestination_t destination;
  serverInfoToZclDestination(&destination);
  EmberZclClusterOtaBootloadServerCommandQueryNextImageRequest_t request = {
    .fieldControl = 0,
    .manufacturerId = fileSpec.manufacturerCode,
    .imageType = fileSpec.type,
    .currentFileVersion = fileSpec.version,
    .hardwareVersion = hardwareVersion,
  };
  EmberStatus status
    = emberZclSendClusterOtaBootloadServerCommandQueryNextImageRequest(&destination,
                                                                       &request,
                                                                       queryNextImageResponseHandler);
  if (status != EMBER_SUCCESS) {
    // If we failed to submit the message for transmission, it could be
    // something on our side, so let's just try again later.
    scheduleQueryNextImage(true); // addJitter?
  }
}

static void queryNextImageResponseHandler(EmberZclMessageStatus_t status,
                                          const EmberZclCommandContext_t *context,
                                          const EmberZclClusterOtaBootloadServerCommandQueryNextImageResponse_t *response)
{
  switch (status) {
    case EMBER_ZCL_MESSAGE_STATUS_DISCOVERY_TIMEOUT:
    case EMBER_ZCL_MESSAGE_STATUS_COAP_TIMEOUT:
    case EMBER_ZCL_MESSAGE_STATUS_COAP_RESET:
      // If we couldn't reach the server for some reason, we need to go back and
      // try to find it again. Maybe it dropped off the network?
      emberAfPluginOtaBootloadClientPrintln("Query: Got COAP Timeout/Reset");
      scheduleDiscoverServer(true, false); // eraseServerInfo? addJitter?
      break;
    case EMBER_ZCL_MESSAGE_STATUS_COAP_ACK:
      // This means that our request was ACK'd, but the actual response is coming.
      emberAfPluginOtaBootloadClientPrintln("Query: Got COAP ACK");
      break;
    case EMBER_ZCL_MESSAGE_STATUS_COAP_RESPONSE:
      emberAfPluginOtaBootloadClientPrintln("Query: Got COAP Response");
      processQueryNextImageResponse(response);
      break;
    default:
      emberAfPluginOtaBootloadClientPrintln("Query: Bad query: %d", status);
      scheduleDiscoverServer(true, false); // eraseServerInfo? addJitter?
  }
}

static void processQueryNextImageResponse(const EmberZclClusterOtaBootloadServerCommandQueryNextImageResponse_t *response)
{
  fileSpecToDownload.manufacturerCode = response->manufacturerId;
  fileSpecToDownload.type = response->imageType;
  fileSpecToDownload.version = response->fileVersion;

  // TODO: handle this case more gracefully.
  EmberZclOtaBootloadStorageInfo_t storageInfo;
  emberZclOtaBootloadStorageGetInfo(&storageInfo, NULL, 0);
  assert(response->imageSize <= storageInfo.maximumFileSize);

  EmberZclOtaBootloadStorageFileInfo_t storageFileInfo;
  bool fileExists
    = (emberZclOtaBootloadStorageFind(&fileSpecToDownload, &storageFileInfo)
       == EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_SUCCESS);

  switch (response->status) {
    case EMBER_ZCL_STATUS_SUCCESS:
      if (emberZclOtaBootloadClientStartDownloadCallback(&fileSpecToDownload,
                                                         fileExists)) {
        setState(STATE_DOWNLOAD, 0); // run now
      } else {
        scheduleQueryNextImage(false); // addJitter?
      }
      break;
    case EMBER_ZCL_STATUS_NOT_AUTHORIZED:
      emberAfPluginOtaBootloadClientPrintln("Query: Not authorized");
      scheduleDiscoverServer(true, false); // eraseServerInfo? addJitter?
      break;
    case EMBER_ZCL_STATUS_NO_IMAGE_AVAILABLE:
      emberAfPluginOtaBootloadClientPrintln("Query: No Image Available");
      scheduleQueryNextImage(false); // addJitter?
      break;
    default:
      emberAfPluginOtaBootloadClientPrintln("Bad query: %d", response->status);
      scheduleDiscoverServer(true, false); // eraseServerInfo? addJitter?
  }
}

// -------------------------------------
// Download

static uint32_t getCurrentDownloadFileOffset(void)
{
  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  EmberZclOtaBootloadStorageFileInfo_t storageFileInfo;
  EmberZclOtaBootloadStorageStatus_t storageStatus
    = emberZclOtaBootloadStorageFind(&fileSpecToDownload, &storageFileInfo);
  // TODO: handle this case more gracefully.
  assert(storageStatus != EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_FAILED);

  if (storageStatus == EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_INVALID_FILE) {
    storageStatus = emberZclOtaBootloadStorageCreate(&fileSpecToDownload);
    // TODO: handle this case more gracefully.
    assert(storageStatus == EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_SUCCESS);

    storageStatus = emberZclOtaBootloadStorageFind(&fileSpecToDownload, &storageFileInfo);
    // TODO: handle this case more gracefully.
    assert(storageStatus == EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_SUCCESS);
  }

  return storageFileInfo.size;
}

static void setServerUriPath(uint8_t *path, size_t pathLength)
{
  // make sure the path is large enough to fit the format "<URI>/MMMM-HHHH-VVVVVVVV"
  assert(pathLength >= strlen(EM_ZCL_OTA_BOOTLOAD_UPGRADE_URI) + 1 + 4 + 1 + 4 + 1 + 8 + 1);
  sprintf(path, "%s/%04x-%04x-%08x", EM_ZCL_OTA_BOOTLOAD_UPGRADE_URI, fileSpecToDownload.manufacturerCode, fileSpecToDownload.type, fileSpecToDownload.version);
}

static void handleBlockError(uint8_t *errors)
{
  (*errors)++;
  if (*errors >= EMBER_AF_PLUGIN_OTA_BOOTLOAD_CLIENT_BLOCK_ERROR_THRESHOLD) {
    *errors = 0;
    downloadStatus = EMBER_ZCL_STATUS_ABORT;
    downloadComplete();
  } else {
    setState(STATE_DOWNLOAD,
             convertToMs(DOWNLOAD_BLOCK_ERROR_BACKOFF_MS,
                         DELAY_TYPE_MILLISECONDS,
                         true)); // addJitter?
  }
}

static void downloadStateHandler(void)
{
  // TODO: what if the offset of the file is not at a multiple of block size?
  size_t offset = getCurrentDownloadFileOffset();
  assert(offset % (1 << DEFAULT_BLOCK_SIZE_LOG) == 0);
  uint32_t blockNumber = offset >> DEFAULT_BLOCK_SIZE_LOG;
  EmberCoapOption blockOption;
  emberInitCoapOption(&blockOption,
                      EMBER_COAP_OPTION_BLOCK2,
                      emberBlockOptionValue(false, // more?
                                            DEFAULT_BLOCK_SIZE_LOG,
                                            blockNumber));

  BlockResponseData responseData = { 0 };
  responseData.logBlockSize = DEFAULT_BLOCK_SIZE_LOG;
  setServerUriPath(responseData.path, sizeof(responseData.path) / sizeof(uint8_t));

  assert(HAVE_SERVER_INFO());
  EmberCoapSendInfo info = {
    .nonConfirmed = false,

    .localAddress = { { 0 } }, // use default
    .localPort = 0,        // use default
    .remotePort = 0,       // use default

    .options = &blockOption,
    .numberOfOptions = 1,

    .responseTimeoutMs = 0, // use default

    .responseAppData = (void *)&responseData,
    .responseAppDataLength = sizeof(responseData),

    .transmitHandler = NULL // unused
  };
  EmberStatus status = emberCoapGet(&serverInfo.address,
                                    responseData.path,
                                    blockResponseHandler,
                                    &info);
  if (status != EMBER_SUCCESS) {
    // If we fail to submit the message to the stack, just try again. Maybe
    // second time is the charm?
    setState(STATE_DOWNLOAD, 0); // run now
  }
}

static void blockResponseHandler(EmberCoapStatus status,
                                 EmberCoapCode code,
                                 EmberCoapReadOptions *options,
                                 uint8_t *payload,
                                 uint16_t payloadLength,
                                 EmberCoapResponseInfo *info)
{
  static uint8_t errors = 0;
  BlockResponseData *blockResponseData
    = (BlockResponseData *)info->applicationData;
  EmberCoapBlockOption block2Option;
  if (status == EMBER_COAP_MESSAGE_ACKED) {
    // If we got an ACK, that means the response is coming later, so just wait
    // for the next response.
    return;
  } else if (status != EMBER_COAP_MESSAGE_RESPONSE
             || code != EMBER_COAP_CODE_205_CONTENT
             || !emberReadBlockOption(options,
                                      EMBER_COAP_OPTION_BLOCK2,
                                      &block2Option)
             || !emberVerifyBlockOption(&block2Option,
                                        payloadLength,
                                        blockResponseData->logBlockSize)) {
    emberAfPluginOtaBootloadClientPrintln("block error: %d, %d", status, code);
    handleBlockError(&errors);
    return;
  }

  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  bool moreBlocks = block2Option.more;
  size_t offset = block2Option.number << block2Option.logSize;
  EmberZclOtaBootloadStorageStatus_t storageStatus
    = emberZclOtaBootloadStorageWrite(&fileSpecToDownload,
                                      offset,
                                      payload,
                                      payloadLength);
  if (storageStatus != EMBER_ZCL_OTA_BOOTLOAD_STORAGE_STATUS_SUCCESS) {
    // If the storage is in a weird state, then we might keep hitting this
    // error. If that is the case, we can just let our errors go over the
    // threshold so we will give up on the download.
    emberAfPluginOtaBootloadClientPrintln("block error: Storage status fail");
    handleBlockError(&errors);
  } else if (!moreBlocks) {
    setState(STATE_VERIFY_DOWNLOAD, 0); // run now
  } else {
    blockResponseData->nextBlockNumber++;
    // TODO: what should we do if we can't submit the next request to the stack?
    // We could reschedule a request event, maybe we should just be doing this
    // always? We may need to back off to give the server time to breathe.
    assert(emberCoapRequestNextBlock(EMBER_COAP_CODE_GET,
                                     blockResponseData->path,
                                     &block2Option,
                                     blockResponseHandler,
                                     info)
           == EMBER_SUCCESS);
  }
}

static void downloadComplete(void)
{
  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  assert(HAVE_DOWNLOAD_STATUS());
  downloadStatus
    = emberZclOtaBootloadClientDownloadCompleteCallback(&fileSpecToDownload,
                                                        downloadStatus);
  scheduleUpgradeEnd();
}

// -------------------------------------
// Verify download

static void verifyDownloadStateHandler(void)
{
  // TODO: implement image verification.
  downloadStatus = EMBER_ZCL_STATUS_SUCCESS;
  downloadComplete();
}

// -------------------------------------
// Upgrade end request

static void scheduleUpgradeEnd(void)
{
  // Add some backoff into this guy, since there may be many nodes that have
  // downloaded an image (in the multicast scenario).
  setState(STATE_UPGRADE_END,
           convertToMs(UPGRADE_END_RESPONSE_BACKOFF_MS,
                       DELAY_TYPE_MILLISECONDS,
                       true)); // addJitter?
}

static void upgradeEndStateHandler(void)
{
  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  assert(HAVE_DOWNLOAD_STATUS());
  EmberZclDestination_t destination;
  serverInfoToZclDestination(&destination);
  EmberZclClusterOtaBootloadServerCommandUpgradeEndRequest_t request = {
    .status = downloadStatus,
    .manufacturerId = fileSpecToDownload.manufacturerCode,
    .imageType = fileSpecToDownload.type,
    .fileVersion = fileSpecToDownload.version,
  };
  EmberStatus status
    = emberZclSendClusterOtaBootloadServerCommandUpgradeEndRequest(&destination,
                                                                   &request,
                                                                   upgradeEndResponseHandler);
  if (status != EMBER_SUCCESS) {
    // If we failed to submit the message for transmission, it could be
    // something on our side, so let's just try again later. We jitter the
    // response since giving the stack some time to breathe could solve our
    // problem.
    emberAfPluginOtaBootloadClientPrintln("Download command fail: %d", status);
    scheduleUpgradeEnd();
  } else if (downloadStatus != EMBER_ZCL_STATUS_SUCCESS) {
    // If our downloadStatus is not successful, then we are going to get a
    // default response from the server, so we should kick off the next image
    // query here.
    emberAfPluginOtaBootloadClientPrintln("Download fail: %d", downloadStatus);
    scheduleQueryNextImage(false); // addJitter?
  }
}

static void upgradeEndResponseHandler(EmberZclMessageStatus_t status,
                                      const EmberZclCommandContext_t *context,
                                      const EmberZclClusterOtaBootloadServerCommandUpgradeEndResponse_t *response)
{
  switch (status) {
    case EMBER_ZCL_MESSAGE_STATUS_DISCOVERY_TIMEOUT:
    case EMBER_ZCL_MESSAGE_STATUS_COAP_TIMEOUT:
    case EMBER_ZCL_MESSAGE_STATUS_COAP_RESET:
      // If we couldn't reach the server for some reason, then try again to send
      // the command.
      // TODO: we should probably add this to our error threshold!
      emberAfPluginOtaBootloadClientPrintln("Timeout/reset");
      scheduleUpgradeEnd();
      break;
    case EMBER_ZCL_MESSAGE_STATUS_COAP_ACK:
      emberAfPluginOtaBootloadClientPrintln("COAP ack");
      // This means that our request was ACK'd, but the actual response is coming.
      break;
    case EMBER_ZCL_MESSAGE_STATUS_COAP_RESPONSE:
      emberAfPluginOtaBootloadClientPrintln("COAP response");
      processUpgradeEndResponse(response);
      break;
    default:
      assert(false);
  }
}

static void processUpgradeEndResponse(const EmberZclClusterOtaBootloadServerCommandUpgradeEndResponse_t *response)
{
  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  if (fileSpecToDownload.manufacturerCode != response->manufacturerId
      || fileSpecToDownload.type != response->imageType
      || fileSpecToDownload.version != response->fileVersion) {
    // TODO: handle default response that may be sent back (see 11.13.6.4).
    // As a workaround for this issue, let's just reschedule the download.
    emberAfPluginOtaBootloadClientPrintln("Mismatched firmware image properties %d:%d, %d:%d, %d:%d", fileSpecToDownload.manufacturerCode,
                                          response->manufacturerId, fileSpecToDownload.type, response->imageType, fileSpecToDownload.version, response->fileVersion);
    scheduleQueryNextImage(false);
    return;
  }

  // This logic is brought you by 11.11.4 in the OTA spec. If currentTime is 0,
  // then upgradeTime is the offset, in seconds, of when we should perform the
  // upgrade. Else if upgradeTime is not 0 and not 0xFFFFFFFF, then we should
  // treat the two values like UTC times. Else if the upgrade time is 0xFFFFFFFF,
  // then that means to check back in later to see if we can upgrade.
  if (response->currentTime < 0xFFFFFFFF) {
    emberAfPluginOtaBootloadClientPrintln("Will bootload in %d seconds", response->upgradeTime - response->currentTime);
    setState(STATE_BOOTLOAD,
             convertToMs(response->upgradeTime - response->currentTime,
                         DELAY_TYPE_SECONDS,
                         false)); // addJitter?
  } else {
    // TODO: reschedule upgrade end request.
  }
}

// -------------------------------------
// Bootload

static void bootloadStateHandler(void)
{
  assert(HAVE_FILE_SPEC_TO_DOWNLOAD());
  emberZclOtaBootloadClientPreBootloadCallback(&fileSpecToDownload);

  emberAfPluginOtaBootloadClientPrintln("Loading new image");
  EmberStatus status = halAppBootloaderInstallNewImage();
  // This call should not return!
  (void)status;
}

// -----------------------------------------------------------------------------
// Public API downward

void emZclOtaBootloadClientInitCallback(void)
{
  ERASE_SERVER_INFO();
  ERASE_FILE_SPEC_TO_DOWNLOAD();
  ERASE_DOWNLOAD_STATUS();
  if (emberZclOtaBootloadClientSetVersionInfoCallback()) {
    emberAfPluginOtaBootloadClientPrintln("Failure initializing OTA attributes");
  }
  setState(STATE_NONE, 0); // run now
}

void emZclOtaBootloadClientNetworkStatusCallback(EmberNetworkStatus newNetworkStatus,
                                                 EmberNetworkStatus oldNetworkStatus,
                                                 EmberJoinFailureReason reason)
{
#ifdef EMBER_AF_PLUGIN_OTA_BOOTLOAD_CLIENT_AUTO_START
  if (newNetworkStatus == EMBER_JOINED_NETWORK_ATTACHED) {
    scheduleDiscoverServer(false, true); // eraseServerInfo? addJitter?
  }
#endif
}

void emZclOtaBootloadClientEventHandler(void)
{
  emberEventControlSetInactive(emZclOtaBootloadClientEventControl);

  typedef void (*StateHandler)(void);
  const StateHandler stateHandlers[] = {
    noneStateHandler,           // STATE_NONE
    discoverServerStateHandler, // STATE_DISCOVER_SERVER
    queryNextImageStateHandler, // STATE_QUERY_NEXT_IMAGE
    downloadStateHandler,       // STATE_DOWNLOAD
    verifyDownloadStateHandler, // STATE_VERIFY_DOWNLOAD
    upgradeEndStateHandler,     // STATE_UPGRADE_END
    bootloadStateHandler,       // STATE_BOOTLOAD
  };
  assert(state < COUNTOF(stateHandlers));
  (*(stateHandlers[state]))();
}
