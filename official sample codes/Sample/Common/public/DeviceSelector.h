// Copyright 2017-2018 SICK AG. All rights reserved.

#ifndef DEVICE_SELECTOR_H
#define DEVICE_SELECTOR_H

#include "Consumer.h"

namespace Sample
{

/**
   Helper class to handle device discovery via MAC and IP address
*/
class DeviceSelector
{
public:
  DeviceSelector();

  /** When openDevice is called the device with the selected mac will
      be opened
  */
  void selectDeviceMac(uint64_t mac);

  /** When openDevice is called the device with the selected ip will
      be opened
   */
  void selectDeviceIp(uint32_t ip);

  /** Returns true if a device has been selected */
  bool isSelected();

  /** Open the device that was previously selected by one the of the
      select methods. If no device is selected or if the device open
      method otherwise fails GENTL_INVALID_HANDLE is returned.
   */
  GenTL::DEV_HANDLE openDevice(Sample::Consumer& consumer,
                               GenTL::IF_HANDLE& interfaceHandle);

private:
  bool handleArgument(int& argc, char *argv[],
                      int idx,
                      const std::string& argName,
                      std::string &result,
                      bool& failure);
  bool mUseMac;
  uint64_t mSelectedMac;
  bool mUseIp;
  uint32_t mSelectedIp;

};

}
#endif
