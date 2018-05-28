// Copyright 2018 SICK AG. All rights reserved.

#include "ChunkAdapter.h"
#include "DeviceSelector.h"
#include "SampleUtils.h"

#include <GenApi/ChunkAdapterGEV.h>

#include <string>

/**
   This sample demonstrates how to attach to chunk metadata carried in
   each buffer. It also shows how to access the metadata through the
   device node map after attaching to the buffer.
*/
int main(int argc, char* argv[])
{
  InputWaiter waiter;
  Sample::DeviceSelector selector;

  if (!Sample::parseDeviceOptionsHelper(argc, argv, selector))
  {
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

  // Enable chunk metadata
  GenApi::CBooleanPtr chunkModeActive = device._GetNode("ChunkModeActive");
  *chunkModeActive = true;

  // Switch to 3D
  GenApi::CEnumerationPtr deviceType = device._GetNode("DeviceScanType");
  *deviceType = "Linescan3D";

  // Fetch buffer width and number of lines in buffer
  GenApi::CEnumerationPtr regionSelector = device._GetNode("RegionSelector");
  *regionSelector = "Scan3dExtraction1";
  GenApi::CIntegerPtr height = device._GetNode("Height");
  // Grab 10 lines per 3D profile
  height->SetValue(10);
  int64_t bufferHeight = height->GetValue();

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

  // See inside the Sample::ChunkAdapter class to understand how GenTL
  // is used to attach the chunk adapter.
  Sample::ChunkAdapter chunkAdapter(tl, dataStreamHandle);
  chunkAdapter.attachNodeMap(device._Ptr);
  chunkAdapter.attachBuffer(hBuffer, pBuffer);

  GenApi::CIntegerPtr chunkWidth = device._GetNode("ChunkWidth");
  GenApi::CIntegerPtr chunkHeight = device._GetNode("ChunkHeight");
  std::cout << "Chunk width: " << chunkWidth->GetValue()
    << ", height: " << chunkHeight->GetValue() << std::endl;
  GenApi::CIntegerPtr chunkScanLineSelector =
    device._GetNode("ChunkScanLineSelector");
  GenApi::CIntegerPtr chunkTimestamp = device._GetNode("ChunkTimestamp");
  GenApi::CIntegerPtr chunkEncoderValue = device._GetNode("ChunkEncoderValue");
  for (int64_t i = chunkScanLineSelector->GetMin();
       i <= chunkScanLineSelector->GetMax(); ++i)
  {
    chunkScanLineSelector->SetValue(i);
    std::cout << "Line: " << i
      << ", timestamp: " << chunkTimestamp->GetValue()
      // Encoder is not used to trigger lines in this sample but a
      // connected encoder's value will be in the metadata anyway.
      << ", encoder: " << chunkEncoderValue->GetValue() << std::endl;
  }

  chunkAdapter.detachBuffer();
  chunkAdapter.detachNodeMap();

  // Stop acquisition and clean up
  command = device._GetNode("AcquisitionStop");
  command->Execute();
  paramLock->SetValue(0);
  CC(tl, tl->DSStopAcquisition(dataStreamHandle, GenTL::ACQ_STOP_FLAGS_KILL));
  CC(tl, tl->DSFlushQueue(dataStreamHandle, GenTL::ACQ_QUEUE_ALL_DISCARD));
  CC(tl, tl->GCUnregisterEvent(dataStreamHandle, GenTL::EVENT_NEW_BUFFER));
  CC(tl, tl->DSRevokeBuffer(dataStreamHandle, hBuffer,
                            reinterpret_cast<void**>(&pBuffer), nullptr));

  delete[] pBuffer;
  CC(tl, tl->DSClose(dataStreamHandle));
  consumer.closeDevice(deviceHandle);
  consumer.closeInterface(interfaceHandle);
  consumer.close();

  return 0;
}
