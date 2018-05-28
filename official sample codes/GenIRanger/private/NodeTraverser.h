// Copyright 2016-2018 SICK AG. All rights reserved.

#ifndef GENIRANGER_NODETRAVERSER_H
#define GENIRANGER_NODETRAVERSER_H

#include "GenICam.h"

namespace GenIRanger
{
/** Base class for traversing GenICam node map structure */
class NodeTraverser
{
public:
  NodeTraverser();

  virtual ~NodeTraverser() = 0;

  /** Traverse the node tree starting from the specified node */
  virtual void traverse(const GenApi::CNodePtr& node);

  GenApi::FeatureList_t::iterator
  removeNotAvailableFeatures(GenApi::FeatureList_t::iterator begin,
                             GenApi::FeatureList_t::iterator end);

protected:
  /** Performs an operation when entering the scope a category */
  virtual void enterCategory(const GenApi::CCategoryPtr& category);

  /** Performs an operation when leaving the scope of a category */
  virtual void leaveCategory(const GenApi::CCategoryPtr& category);

  /** Performs an operation when encountering a selector. These aren't treated
      as scopes like a category.
  */
  virtual void enterSelector(const GenApi::CNodePtr& selector);

  /** Action to perform when the SelectorState returned from enterSelector goes
      out of scope.
  */
  virtual void leaveSelector(const GenApi::CNodePtr& selector);


  /** Performs an operation when encountering a node which isn't a category or
      selector.
  */
  virtual void onLeaf(const GenApi::CNodePtr& node);

private:
  /** Recursively find all values for the given node and its selectors */
  void IterateAllValues(
    const GenApi::CNodePtr& node,
    GenApi::FeatureList_t::iterator begin,
    GenApi::FeatureList_t::iterator end);

};

}
#endif