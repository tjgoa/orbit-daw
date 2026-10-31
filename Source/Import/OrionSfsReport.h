#pragma once

#include "OrionSfsReader.h"

namespace orbit::import::orion
{
// Deterministic, read-only preview. Does not require JUCE or mutate a project.
std::string createInspectionReport(const ImportDocument&, const SfsInspection&);
}
