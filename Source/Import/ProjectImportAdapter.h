#pragma once

#include "ProjectImportPlan.h"
#include "../Project/ProjectDocument.h"

namespace orbit::import
{
// Prepares a detached replacement document. No partial writes to base/result
// on validation failure. Existing tracks and file association are preserved.
// Global tempo/meter may only change when importing into an empty project;
// otherwise they must match, as tempo-map merging is not supported yet.
bool prepareProjectImport(const ProjectDocument& base, const ProjectImportPlan&,
                          ProjectDocument& result, std::string& error);

// One UndoableAction for the complete document change. The host must keep the
// document alive until undo history is cleared, and refresh its engine/view
// after perform/undo/redo. No engine or UI objects are touched by the adapter.
bool applyProjectImport(ProjectDocument&, const ProjectImportPlan&,
                        juce::UndoManager&, std::string& error);
}
