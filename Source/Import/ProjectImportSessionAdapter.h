#pragma once

#include "ProjectImportAdapter.h"
#include <tracktion_engine/tracktion_engine.h>

namespace orbit::import
{
struct PreparedProjectImportSession
{
    ProjectDocument document;
    std::unique_ptr<tracktion::engine::Edit> edit;
    int selectedTrackIndex = -1;
};

// Stages a native document and matching engine session. Only neutral plans
// cross this boundary. The source edit's graph/transport and live document
// remain untouched; the host owns the eventual snapshot/undo commit.
bool prepareProjectImportSession(tracktion::engine::Engine&, const ProjectDocument&,
                                  tracktion::engine::Edit&, const ProjectImportPlan&,
                                  PreparedProjectImportSession&, std::string& error);
}
