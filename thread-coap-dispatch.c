// This file is generated by Simplicity Studio.  Please do not edit manually.
//
//

#ifdef CONFIGURATION_HEADER
  #include CONFIGURATION_HEADER
#endif // CONFIGURATION_HEADER
#include EMBER_AF_API_COAP_DISPATCH

EmberAfCoapDispatchHandler borderRouterDiscoveryHandler;
EmberAfCoapDispatchHandler buzzerHandler;
EmberAfCoapDispatchHandler emZclHandler;
EmberAfCoapDispatchHandler netResetHandler;
EmberAfCoapDispatchHandler subscribeHandler;

const EmberAfCoapDispatchEntry emberAfCoapDispatchTable[] = {
  {EMBER_AF_COAP_DISPATCH_METHOD_POST, "borderrouter/discover", 21, borderRouterDiscoveryHandler},
  {EMBER_AF_COAP_DISPATCH_METHOD_POST, "subscriber/subscribe", 20, subscribeHandler},
  {EMBER_AF_COAP_DISPATCH_METHOD_ANY, "zcl/", 4, emZclHandler},
  {EMBER_AF_COAP_DISPATCH_METHOD_ANY, ".well-known/", 12, emZclHandler},
  {EMBER_AF_COAP_DISPATCH_METHOD_POST, "device/buzzer", 13, buzzerHandler},
  {EMBER_AF_COAP_DISPATCH_METHOD_POST, "device/reset", 12, netResetHandler},
  {0, NULL, 0, NULL}, // terminator
};
