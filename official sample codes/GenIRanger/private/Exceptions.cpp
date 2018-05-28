// Copyright 2016-2018 SICK AG. All rights reserved.

#include "Exceptions.h"

namespace GenIRanger
{

GenIRangerException::GenIRangerException(const std::string& message)
  : std::exception(message.c_str())
{
  // Empty
}

ImportException::ImportException(const std::string& message)
  : GenIRangerException(message)
{
  // Empty
}

ExportException::ExportException(const std::string& message)
  : GenIRangerException(message)
{
  // Empty
}

ConfigurationFileException::ConfigurationFileException(const std::string& message)
  : ImportException(message)
{
  // Empty
}

DeviceLogException::DeviceLogException(const std::string& message)
  : GenIRangerException(message)
{
  // Empty
}

SaveException::SaveException(const std::string& message)
  : GenIRangerException(message)
{
  // Empty
}

}