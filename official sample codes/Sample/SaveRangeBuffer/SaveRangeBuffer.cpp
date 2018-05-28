// Copyright 2016-2018 SICK AG. All rights reserved.

#include "DeviceSelector.h"
#include "GenIRanger.h"
#include "SampleUtils.h"
#include "SingleDeviceConsumer.h"

#include <fstream>
#include <string>

void usage(int, char* argv[])
{
  std::cout << "Usage:" << std::endl
            << argv[0] <<  " <file path for buffer>" << std::endl;
  Sample::printDeviceOptionsHelp();
  std::cout << std::endl;
  std::cout << "File path should include the name of the file to save without "
            << "extension." << std::endl
            << "E.g. C:\\MyBuffers\\buffer" << std::endl;
}

int main(int argc, char* argv[])
{
  InputWaiter waiter;
  Sample::DeviceSelector selector;

  if (!Sample::parseDeviceOptionsHelper(argc, argv, selector))
  {
    return 1;
  }

  std::string filePath = "saved_range_buffer";
  if (!Sample::parseArgumentHelper(argc, argv, "file path for buffer", filePath))
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
  // Initialize GenTL and open transport layer
  GenTL::TL_HANDLE tlHandle = consumer.open();
  if (tlHandle == GENTL_INVALID_HANDLE)
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

  char idBuffer[1024];
  size_t idSize = sizeof(idBuffer);

  // Open stream
  GenTL::DS_HANDLE dataStreamHandle;
  CC(tl, tl->DevGetDataStreamID(deviceHandle, 0, idBuffer, &idSize));
  CC(tl, tl->DevOpenDataStream(deviceHandle, idBuffer, &dataStreamHandle));

  // Switch to 3D
  GenApi::CEnumerationPtr deviceType = device._GetNode("DeviceScanType");
  *deviceType = "Linescan3D";

  // Fetch buffer width and number of lines in buffer
  GenApi::CEnumerationPtr regionSelector = device._GetNode("RegionSelector");
  *regionSelector = "Scan3dExtraction1";
  GenApi::CIntegerPtr height = device._GetNode("Height");
  int64_t bufferHeight = height->GetValue();

  // Fetch info where on the sensor the 3D data is extracted
  *regionSelector = "Region1";
  // Width in Scan3dExtraction1 and Region1 are mirrored
  GenApi::CIntegerPtr width = device._GetNode("Width");
  GenApi::CIntegerPtr offsetX = device._GetNode("OffsetX");
  GenApi::CIntegerPtr offsetY = device._GetNode("OffsetY");
  int64_t aoiHeight = height->GetValue();
  int64_t aoiOffsetX = offsetX->GetValue();
  int64_t aoiOffsetY = offsetY->GetValue();

  int64_t bufferWidth = width->GetValue();
  int64_t buffer16Size = bufferWidth * bufferHeight * 2;
  uint8_t* pBuffer16 = new uint8_t[buffer16Size];

  // Setup buffer
  GenApi::CIntegerPtr payload = device._GetNode("PayloadSize");
  int64_t payloadSize = payload->GetValue();
  uint8_t* pBuffer = new uint8_t[payloadSize];
  GenTL::BUFFER_HANDLE hBuffer;
  CC(tl, tl->DSAnnounceBuffer(dataStreamHandle,
                              pBuffer,
                              static_cast<size_t>(payloadSize),
                              nullptr,
                              &hBuffer));
  CC(tl, tl->DSQueueBuffer(dataStreamHandle, hBuffer));

  // Register event
  GenTL::EVENT_HANDLE hEvent;
  CC(tl, tl->GCRegisterEvent(dataStreamHandle,
                             GenTL::EVENT_NEW_BUFFER, &hEvent));

  // Start acquisition
  GenApi::CIntegerPtr paramLock = device._GetNode("TLParamsLocked");
  paramLock->SetValue(1);

  // Grab one buffer
  CC(tl, tl->DSStartAcquisition(dataStreamHandle,
                                GenTL::ACQ_START_FLAGS_DEFAULT, 1));
  GenApi::CCommandPtr command = device._GetNode("AcquisitionStart");
  command->Execute();

  std::cout << "Grabbing buffer..." << std::endl;

  // Wait for buffer to be received
  GenTL::EVENT_NEW_BUFFER_DATA bufferData;
  size_t eventSize = sizeof(bufferData);
  CC(tl, tl->EventGetData(hEvent, &bufferData, &eventSize, -1));

  // Stop acquisition and clean up
  command = device._GetNode("AcquisitionStop");
  command->Execute();
  paramLock->SetValue(0);
  CC(tl, tl->DSStopAcquisition(dataStreamHandle, GenTL::ACQ_STOP_FLAGS_KILL));
  CC(tl, tl->DSFlushQueue(dataStreamHandle, GenTL::ACQ_QUEUE_ALL_DISCARD));
  CC(tl, tl->GCUnregisterEvent(dataStreamHandle, GenTL::EVENT_NEW_BUFFER));
  CC(tl, tl->DSRevokeBuffer(dataStreamHandle, hBuffer,
                            reinterpret_cast<void**>(&pBuffer), nullptr));

  std::cout << "Converting buffer..." << std::endl;

  GenIRanger::convert12pTo16(
    static_cast<uint8_t*>(pBuffer),
    bufferWidth * bufferHeight * 12 / 8,
    pBuffer16,
    &buffer16Size);

  std::cout << "Saving buffer to " << filePath.c_str() << "..." << std::endl;
  GenIRanger::saveBuffer16(
    pBuffer16,
    bufferWidth,
    bufferHeight,
    aoiHeight,
    aoiOffsetX,
    aoiOffsetY,
    filePath);

  delete[] pBuffer16;
  delete[] pBuffer;
  CC(tl, tl->DSClose(dataStreamHandle));
  consumer.closeDevice(deviceHandle);
  consumer.closeInterface(interfaceHandle);
  consumer.close();

  return 0;
}
