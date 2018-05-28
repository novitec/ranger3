// Copyright 2016-2018 SICK AG. All rights reserved.

#include "NodeTraverser.h"

#include "Exceptions.h"
#include "GenIUtil.h"
#include "NodeUtil.h"

#include <algorithm>

using namespace GenApi;
using namespace GenICam;

namespace GenIRanger
{

NodeTraverser::NodeTraverser()
{}

NodeTraverser::~NodeTraverser()
{}

void NodeTraverser::IterateAllValues(
  const CNodePtr& node,
  FeatureList_t::iterator begin,
  FeatureList_t::iterator end)
{
  // The recursion done in this method can be seen as a multi-level for-loop
  // where it will start incrementing the value of the deepest selector before
  // moving up to the next. This way we can go through all possible values for
  // the selected node.
  CNodePtr selector = *begin;

  enterSelector(selector);

  // Automatically leave the selector when out of scope is reached, either by
  // returning normally or when an exception is thrown.
  auto deleter = [&](void* ptr){ leaveSelector(selector); };
  std::unique_ptr<void, decltype(deleter)> ptr(new int, deleter);

  // Assume continuous value range
  if(NodeUtil::isInteger(selector))
  {
    CIntegerPtr intSelector = static_cast<CIntegerPtr>(selector);
    int64_t min = intSelector->GetMin();
    int64_t max = intSelector->GetMax();

    for (int64_t i = min; i <= max; i++)
    {
      try
      {
        intSelector->SetValue(i);
      }
      catch (GenericException e)
      {
        std::stringstream ss;
        ss << "Cannot set " << selector->GetName() << " to " << i <<
          ". Library exception: " << e.GetDescription();
        GenIUtil::throwAndLog(ss.str());
      }

      // Loop over all possible values for next selector if any
      if (begin + 1 != end)
      {
        IterateAllValues(node, begin + 1, end);
      }
      else
      {
        // All selectors for the node have been set. Perform operation on node.
        onLeaf(node);
      }
    }
  }
  else if(NodeUtil::isEnumeration(selector))
  {
    CEnumerationPtr enumSelector = static_cast<CEnumerationPtr>(selector);
    NodeList_t entries;
    enumSelector->GetEntries(entries);

    for (NodeList_t::iterator it = entries.begin(); it != entries.end(); it++)
    {
      CEnumEntryPtr entry = static_cast<CEnumEntryPtr>(*it);

      // Skip all entries that are not available or not implemented, such as
      // unavailable regions for RegionSelector.
      if(!GenApi::IsAvailable(entry->GetNode()))
      {
        continue;
      }

      try
      {
        enumSelector->SetIntValue(entry->GetValue());
      }
      catch (GenericException& e)
      {
        std::stringstream ss;
        ss << "Cannot set " << selector->GetName() << " to " <<
          entry->GetSymbolic() << ". Library exception: " << e.GetDescription();
        // Leave selectors so that they aren't kept to next node
        GenIUtil::throwAndLog(ss.str());
      }
      // Loop over all possible values for next selector if any
      if (begin + 1 != end)
      {
        IterateAllValues(node, begin + 1, end);
      }
      else
      {
        // All selectors for the node have been set. Perform operation on node.
        onLeaf(node);
      }
    }
  }
  else
  {
    std::stringstream ss;
    ss << "Unsupported selector: " << node->GetName() << std::endl;
    GenIUtil::throwAndLog(ss.str());
  }
}

void NodeTraverser::traverse(const CNodePtr& node)
{
  CCategoryPtr category = static_cast<CCategoryPtr>(node);
  auto name = node->GetName();
  try
  {
    if (category.IsValid())
    {
      enterCategory(category);
      FeatureList_t features;
      category->GetFeatures(features);
      for (FeatureList_t::iterator it = features.begin(); it != features.end(); it++)
      {
        traverse(*it);
      }
      leaveCategory(category);
    }
    else
    {
      // Apparently all writable nodes can be casted to a selector
      CSelectorPtr selector = static_cast<CSelectorPtr>(node);
      if (selector.IsValid())
      {
        FeatureList_t nodesSelected;
        FeatureList_t selectors;
        // If a node is a real Selector, it will point out number of features by
        // the <pSelected> tag. These are the 'selected' features.
        selector->GetSelectedFeatures(nodesSelected);
        // A node which is contained within one or more <pSelected> tags will
        // have all its selectors listed as 'selecting' features.
        selector->GetSelectingFeatures(selectors);
        // There are features selecting this node, iterate over all possible
        // values it can have.
        if(!selectors.empty())
        {
          FeatureList_t::iterator begin = selectors.begin();
          FeatureList_t::iterator end = selectors.end();
          end = removeNotAvailableFeatures(begin, end);
          if (begin != end)
          {
            // Recursively go over all selector combinations for the node
            IterateAllValues(node, begin, end);
          }
        }
        else if(!nodesSelected.empty())
        {
          // Selector, don't do anything. Its value shouldn't be stored.
        }
        else
        {
          // Feature node without dependencies to selectors
          onLeaf(node);
        }
      }
      else
      {
        // Skip nodes that aren't writable
      }
    }
  }
  // TODO For now we only catch and ignore, basically skip that node
  catch(GenICam::GenericException& e)
  {
    std::stringstream ss;
    ss << "GenericException: " << name << " " << e.what();
    std::cout << ss.str() << std::endl;
  }
  catch (GenIRangerException& e)
  {
    std::stringstream ss;
    ss << "GenIRangerException: " << name << " " << e.what();
    std::cout << ss.str() << std::endl;
  }
  catch(std::exception& e)
  {
    std::stringstream ss;
    ss << "std::exception: " << name << " " << e.what();
    std::cout << ss.str() << std::endl;
  }
}

void NodeTraverser::enterCategory(const CCategoryPtr& node)
{
  // Do nothing by default
}

void NodeTraverser::leaveCategory(const CCategoryPtr& node)
{
  // Do nothing by default
}

void NodeTraverser::enterSelector(const CNodePtr& selector)
{
  // Do nothing by default
}

void NodeTraverser::leaveSelector(const CNodePtr& node)
{
  // Do nothing by default
}

void NodeTraverser::onLeaf(const CNodePtr& node)
{
  // Do nothing by default
}

GenApi::FeatureList_t::iterator
NodeTraverser::removeNotAvailableFeatures(GenApi::FeatureList_t::iterator begin,
                                          GenApi::FeatureList_t::iterator end)
{
  return std::remove_if(begin, end, [](GenApi::IValue*& value) {
    return !GenApi::IsAvailable(value->GetNode());
  });
}

}