// Copyright 2016-2018 SICK AG. All rights reserved.

#include "GenIRanger.h"
#include "SampleUtils.h"
#include "SingleDeviceConsumer.h"

#include <fstream>
#include <string>

void usage(int, char* argv[])
{
  std::cout << "Usage:" << std::endl
            << argv[0] <<  " <file path for parameter file>" << std::endl;
  Sample::printDeviceOptionsHelp();
  std::cout << std::endl;
  std::cout << "File path should include the name of the file to import "
            << "including extension." << std::endl
            << "E.g. C:\\ParameterFiles\\parameters.csv" << std::endl;
}

int main(int argc, char* argv[])
{
  InputWaiter waiter;
  Sample::DeviceSelector selector;

  if (!Sample::parseDeviceOptionsHelper(argc, argv, selector))
  {
    return 1;
  }

  std::string filePath = "saved_parameters.csv";
  if (!Sample::parseArgumentHelper(argc, argv,
    "file path for parameter file including extension", filePath))
  {
    usage(argc, argv);
    return 1;
  }

  std::string ctiFile = Sample::getPathToProducer();
  if (ctiFile.empty())
  {
    return 1;
  }

  Sample::Consumer consumer(ctiFile);
  if (consumer.open() == GENTL_INVALID_HANDLE)
  {
    return 1;
  }

  GenTL::IF_HANDLE interfaceHandle = nullptr;
  GenTL::DEV_HANDLE deviceHandle;
  if (selector.isSelected())
  {
    deviceHandle = selector.openDevice(consumer, interfaceHandle);
  }
  else
  {
    deviceHandle = Sample::openDeviceInteractively(consumer, interfaceHandle);
  }
  if (deviceHandle == GENTL_INVALID_HANDLE)
  {
    return 1;
  }

  // Function pointer to GenTL methods
  GenTLApi* tl = consumer.tl();

  // Connect device node map to GenTL
  GenTL::PORT_HANDLE devicePort;
  CC(tl, tl->DevGetPort(deviceHandle, &devicePort));
  Sample::GenTLPort port = Sample::GenTLPort(devicePort, tl);
  GenApi::CNodeMapRef device = consumer.getNodeMap(&port);

  std::ifstream inputStream(filePath);
  if (inputStream.good())
  {
    GenIRanger::importDeviceParameters(device._Ptr, inputStream);
    inputStream.close();

    std::cout << "Imported device parameters from " << filePath << std::endl;
  }
  else
  {
    std::cerr << "ERROR: Parameter file " << filePath
      << " could not be opened" << std::endl;
    return 1;
  }

  consumer.closeDevice(deviceHandle);
  consumer.closeInterface(interfaceHandle);
  consumer.close();

  return 0;
}
