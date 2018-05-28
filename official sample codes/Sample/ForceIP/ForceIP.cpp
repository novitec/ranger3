// Copyright 2017-2018 SICK AG. All rights reserved.

#include "GenIRanger.h"
#include "SampleUtils.h"
#include "SingleDeviceConsumer.h"

#include <fstream>
#include <string>

void usage(int, char* argv[])
{
  std::cout << "Usage:" << std::endl
            << argv[0] << std::endl;
}

int main(int argc, char* argv[])
{
  InputWaiter waiter;
  if (argc != 1)
  {
    usage(argc, argv);
    return 1;
  }

  std::string ctiFile = Sample::getPathToProducer();
  if (ctiFile.empty())
  {
    return 1;
  }

  // Initialize GenTL and open transport layer
  Sample::Consumer consumer(ctiFile);
  GenTL::TL_HANDLE tlHandle = consumer.open();
  if (tlHandle == GENTL_INVALID_HANDLE)
  {
    return 1;
  }

  // Will print out a list of all available interfaces
  Sample::InterfaceList interfaces = consumer.getInterfaces(tlHandle);
  int i = 0;
  for (auto& theInterface : interfaces)
  {
    auto interfaceId = theInterface.first;
    auto displayName = theInterface.second;
    std::cout << "  " << i << ". " << interfaceId << std::endl;
    std::cout << "    " << displayName << std::endl;
    i++;
  }
  std::cout << "Select interface by its index: " << std::endl;
  int32_t input = Sample::getNumericInput();
  Sample::InterfaceId interfaceId
    = consumer.findInterfaceByIndex(interfaces, input);
  if (interfaceId.empty())
  {
    return 1;
  }

  GenTL::IF_HANDLE interfaceHandle = consumer.openInterfaceById(interfaceId);
  if (interfaceHandle == GENTL_INVALID_HANDLE)
  {
    return 1;
  }

  GenTLApi* tl = consumer.tl();

  // Create a NodeMap of the GenTL Producer interface module
  // This allows access to GigEVision specific functionality, such as ForceIP,
  // that is not part of the GenTL C api.
  Sample::GenTLPort interfacePort = Sample::GenTLPort(interfaceHandle, tl);
  GenApi::CNodeMapRef interfaceNodeMap = consumer.getNodeMap(&interfacePort,
                                                             "InterfacePort");
  std::string interfaceGevXml = consumer.getNodeMapXML(interfaceHandle);

  // Perform a scan, to find devices on the interface
  bool8_t changed = false;
  CC(tl, tl->IFUpdateDeviceList(interfaceHandle, &changed, 100));

  Sample::showDeviceList(interfaceNodeMap);

  std::cout << "Select device by its index, to change its IP address: "
    << std::endl;
  input = Sample::getNumericInput();

  GenApi::CIntegerPtr deviceSelector
    = interfaceNodeMap._GetNode("DeviceSelector");
  deviceSelector->SetValue(input);

  std::cout << "New IP address: " << std::endl;
  std::string newIpAddress;
  std::getline(std::cin, newIpAddress);
  std::cout << "New Subnet mask: " << std::endl;
  std::string newSubnetMask;
  std::getline(std::cin, newSubnetMask);

  std::cout << "Changing IP address with ForceIP...";

  GenApi::CIntegerPtr gevForceIpAddress
    = interfaceNodeMap._GetNode("GevDeviceForceIPAddress");
  gevForceIpAddress->SetValue(Sample::ip4FromString(newIpAddress));

  GenApi::CIntegerPtr gevForceSubnetMask
    = interfaceNodeMap._GetNode("GevDeviceForceSubnetMask");
  gevForceSubnetMask->SetValue(Sample::ip4FromString(newSubnetMask));

  GenApi::CCommandPtr gevForceIp = interfaceNodeMap._GetNode("GevDeviceForceIP");
  gevForceIp->Execute();
  while (!gevForceIp->IsDone())
  {
    std::cout << ".";
    Sleep(10); // ms
  }
  std::cout << " done!\n" << std::endl;

  std::cout << "Re-scanning for devices" << std::endl;

  CC(tl, tl->IFUpdateDeviceList(interfaceHandle, &changed, 100));
  Sample::showDeviceList(interfaceNodeMap);

  consumer.closeInterface(interfaceHandle);
  consumer.close();

  return 0;
}
