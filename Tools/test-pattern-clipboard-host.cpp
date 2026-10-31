static void checkMacPatternClipboard(ProjectView& host)
{
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.setSize(1440,900);
    host.showPianoRoll();auto* roll=host.pianoRollWindow.get();auto* grid=roll->grid;
    const auto id=host.selectedProjectTrackId();
    host.project.setPatternCreated(id,0,true);host.project.setPatternLengthSteps(id,0,128);
    for(const auto& n:std::vector<ProjectDocument::PatternNote>{{60,0,64,1},{60,0,32,127},{72,1280,90,90}})
        host.project.appendPatternNoteTicks(id,0,n.pitch,n.startTick,n.lengthTicks,n.velocity);
    host.project.setPatternSwitch(id,0,false,0,64,true);host.project.setPatternSwitch(id,0,true,128,64,true);
    host.readPatternSteps();host.clearNoteSelection();
    auto command=[](char letter,int extra=0){return juce::KeyPress(int(letter),juce::ModifierKeys(juce::ModifierKeys::commandModifier|extra),0);};
    const auto source=host.capturePatternData(id,0).notes;
    patternCheck(grid->keyPressed(command('C')) && host.patternClipboard.notes.size()==3,"Mac Cmd-C with no text character did not copy the pattern");
    // Select a blank destination and position the editor manually. A delayed
    // new-pattern focus request must not re-centre it when notes are pasted.
    host.selectPatternSlot(1);host.notePasteTick=0;
    grid->setPitchScrollPosition(67.25);roll->viewport.setViewPosition(120,0);
    const auto view=roll->viewPosition();const auto before=host.undoHistory.size();roll->focusPatternPitches=true;
    auto sameView=[&]
    {const auto now=roll->viewPosition();return std::abs(now.pitchRow-view.pitchRow)<1e-8 && std::abs(now.timeStep-view.timeStep)<.06;};
    patternCheck(grid->keyPressed(command('V')),"Mac Cmd-V with no text character was not handled");
    const auto pasted=host.project.getPatternNotes(id,1);
    patternCheck(pasted.size()==source.size() && host.capturePatternData(id,1).notes==source && host.project.getPatternLengthSteps(id,1)==128
        && host.project.getPatternSwitches(id,1).size()==2,"whole-pattern paste lost pitch/timing, coincidence, velocities, length or switches");
    patternCheck(sameView(),"paste moved the manually positioned Piano Roll");
    patternCheck(host.undoHistory.size()==before+1,"paste did not create one Undo");
    patternCheck(roll->keyPressed(command('Z')) && host.project.getPatternNotes(id,1).empty(),"Mac Cmd-Z did not undo paste");
    patternCheck(sameView(),"Undo moved the Piano Roll");
    patternCheck(host.keyPressed(command('Z',juce::ModifierKeys::shiftModifier)) && host.capturePatternData(id,1).notes==source,"Mac Cmd-Shift-Z did not redo paste");
    patternCheck(sameView(),"Redo moved the Piano Roll");
    for(int repeat=0;repeat<3;++repeat)
    {
        host.notePasteTick=8192*(repeat+1);host.clearNoteSelection();
        patternCheck(grid->keyPressed(command('V')) && sameView(),"repeated paste drifted the Piano Roll");
        patternCheck(grid->keyPressed(command('Z')) && sameView(),"repeated Undo drifted the Piano Roll");
    }
    const auto blocked=host.undoHistory.size();
    patternCheck(!grid->keyPressed(command('V',juce::ModifierKeys::altModifier)) && host.undoHistory.size()==blocked,"Cmd-Option-V accidentally pasted notes");
    patternCheck(!host.keyPressed(command('Q')) && !host.keyPressed(command('H')),"host stole macOS Quit/Hide shortcuts");
    patternCheck(grid->keyPressed(command('A')) && host.selectedNoteKeys.size()==2,"Mac Cmd-A did not select coincident note stacks");
    // Ctrl events can contain ASCII control characters rather than letters.
    patternCheck(grid->keyPressed(juce::KeyPress('C',juce::ModifierKeys(juce::ModifierKeys::ctrlModifier),3)),"Ctrl-C control-character event was not handled");
    host.notePasteTick=4096;
    patternCheck(grid->keyPressed(command('X')) && host.project.getPatternNotes(id,1).empty() && sameView(),"Cmd-X cut failed or moved view");
    patternCheck(grid->keyPressed(command('V')) && host.project.getPatternNotes(id,1).size()==3 && sameView(),"selection clipboard paste failed or moved view");
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/mac-clipboard.orbit");host.saveToFile(file);
    patternCheck(host.loadProjectFile(file) && host.project.getPatternNotes(id,1).size()==3,"Mac clipboard edit did not survive native reopen");
    std::cout<<"PASS: native-shaped Mac Cmd-C/V/X/A/Z/Shift-Z with zero text, Ctrl control-character input, coincidence/timing/velocity/switch preservation, blank-slot and repeated paste/Undo/Redo view stability, protected Option/Q/H modifiers and native reopen.\n";
}
