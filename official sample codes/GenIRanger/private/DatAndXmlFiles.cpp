// Copyright 2018 SICK AG. All rights reserved.

#include "DatAndXmlFiles.h"

namespace GenIRanger {

DatAndXmlFiles::DatAndXmlFiles(std::string filePathWithNoEnding)
  : mDataStream(filePathWithNoEnding + ".dat", openMode)
  , mXmlStream(filePathWithNoEnding + ".xml", openMode)
{
  mDataStream.exceptions(std::ios::failbit | std::ios::badbit);
  mXmlStream.exceptions(std::ios::failbit | std::ios::badbit);
}

DatAndXmlFiles::~DatAndXmlFiles()
{
  mDataStream.close();
  mXmlStream.close();
}

void DatAndXmlFiles::writeData(ComponentPtr& component)
{
  writeData(component->data().data(), component->data().size());
}

void DatAndXmlFiles::writeMarkData(LineMetadataPtr encoderValues)
{
  const Metadata notUsed = 0;
  for (auto encoderValue : *encoderValues)
  {
    writeSingleMarkValue(encoderValue);
    writeSingleMarkValue(notUsed);
    writeSingleMarkValue(notUsed);
    writeSingleMarkValue(notUsed);
    writeSingleMarkValue(notUsed);
  }
}

void DatAndXmlFiles::writeSingleMarkValue(const Metadata value)
{
  writeData(reinterpret_cast<const uint8_t*>(&value), sizeof(Metadata));
}

void DatAndXmlFiles::writeXml(std::string& xml)
{
  mXmlStream << xml;
}

}
