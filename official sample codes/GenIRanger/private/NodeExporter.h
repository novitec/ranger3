// Copyright 2016-2018 SICK AG. All rights reserved.

#ifndef GENIRANGER_NODEEXPORTER_H
#define GENIRANGER_NODEEXPORTER_H

#include "ConfigWriter.h"
#include "NodeTraverser.h"
#include <string>

namespace GenIRanger
{

/** Traverses a GenICam node map and exports all node with RW or WO access */
class NodeExporter : public NodeTraverser
{
public:
  NodeExporter(ConfigWriter& formatter);
  ~NodeExporter();

  const std::vector<std::string>& getErrors() const;

protected:
  /** Outputs the visited category */
  virtual void enterCategory(const GenApi::CCategoryPtr& category);
  virtual void leaveCategory(const GenApi::CCategoryPtr& category);

  /** Forwards the visited selector to the ConfigWriter */
  virtual void enterSelector(const GenApi::CNodePtr& selector);
  virtual void leaveSelector(const GenApi::CNodePtr& selector);

  /** Outputs the visited node if it's considered to be a configuration node */
  virtual void onLeaf(const GenApi::CNodePtr& node);

private:
  ConfigWriter& mFormat;

  std::vector<std::string> mErrors;
};

}
#endif