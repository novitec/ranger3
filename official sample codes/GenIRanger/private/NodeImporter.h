// Copyright 2016-2018 SICK AG. All rights reserved.

#ifndef GENIRANGER_NODEIMPORTER_H
#define GENIRANGER_NODEIMPORTER_H

#include "ConfigReader.h"
#include "ConfigWriter.h"
#include "NodeTraverser.h"

#include <set>

namespace GenIRanger
{

/** Opposite from NodeExporter, sets the value of each writable node. */
class NodeImporter : public NodeTraverser
{
public:
  NodeImporter(ConfigReader reader);
  ~NodeImporter();

  const std::vector<std::string>& getErrors() const;

protected:
  /** Saves the visited selector so that the key for the upcoming node can be
      reconstructed.
  */
  virtual void enterSelector(const GenApi::CNodePtr& selector) override;
  /** Pops the latest visited selected */
  virtual void leaveSelector(const GenApi::CNodePtr& selector) override;

  virtual void leaveCategory(const GenApi::CCategoryPtr& category) override;

  /** Reads the imported value and sets it */
  virtual void onLeaf(const GenApi::CNodePtr& node);

  /** Checks if there were any parameters in the configuration file that has not
      been imported. This could happen if the parameter doesn't exist in the
      device node map.
  */
  void checkUnimportedParameters();

private:
  ConfigReader mReader;
  std::vector<GenApi::CNodePtr> mSelectors;

  std::set<std::string> mRemainingToImport;
  std::vector<std::string> mErrors;
};

}
#endif