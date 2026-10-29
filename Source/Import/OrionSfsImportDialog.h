#pragma once

#include "ProjectImportPlan.h"
#include <JuceHeader.h>
#include <functional>

namespace orbit::import
{
// Public host boundary contains no Orion records or engine types. The callback
// must atomically stage/commit the neutral plan and report any failure.
std::unique_ptr<juce::Component> createOrionSfsPreview(const juce::File&,
                          std::function<bool(const ProjectImportPlan&, std::string&)> commit);
void launchOrionSfsPreview(const juce::File&, juce::Component* parent,
                          std::function<bool(const ProjectImportPlan&, std::string&)> commit);
}
