// Included inside ProjectView. All transformations preserve exact native ticks
// and coincident records; selection keys address a pitch/onset stack.
void saveSelectedPianoPreferences()
{
    if(instrumentTrack==nullptr || pianoRollWindow==nullptr)return;
    const auto prefs=pianoRollWindow->getPreferences();
    auto old=instrumentTrack->state.getChildWithName("PIANOROLL");
    if(old.isValid() && old.isEquivalentTo(prefs))return;
    if(old.isValid())instrumentTrack->state.removeChild(old,nullptr);
    instrumentTrack->state.addChild(prefs.createCopy(),-1,nullptr);isDirty=true;
}
void loadSelectedPianoPreferences()
{
    if(instrumentTrack==nullptr || pianoRollWindow==nullptr)return;
    const auto prefs=instrumentTrack->state.getChildWithName("PIANOROLL");
    pianoRollWindow->setPreferences(prefs.isValid()?prefs:defaultPianoRollPreferences);
}
void auditionPianoEdit(int pitch,bool down)
{
    if(down)
    {
        pianoEditAuditionTrack=selectedProjectTrackId();
        pianoEditAuditionChannel=juce::jlimit(1,16,int(instrumentTrack->state.getProperty("orbitMidiChannel",1)));
    }
    if(auto* track=engineTrackForProjectId(pianoEditAuditionTrack))
    {
        if(down && edit->getTransport().getCurrentPlaybackContext()==nullptr)edit->getTransport().ensureContextAllocated();
        track->injectLiveMidiMessage(down?juce::MidiMessage::noteOn(pianoEditAuditionChannel,pitch,juce::uint8(100))
                                         :juce::MidiMessage::noteOff(pianoEditAuditionChannel,pitch),{});
        liveMidiRecipients.addIfNotAlreadyThere(pianoEditAuditionTrack);
    }
    if(!down)pianoEditAuditionTrack.clear();
}
void renamePianoPattern()
{
    if(!canEditPattern())return;
    const auto id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    juce::AlertWindow dialog("Rename Pattern","Name this generator pattern.",juce::AlertWindow::NoIcon);
    dialog.addTextEditor("name",project.getPatternName(id,slot));dialog.addButton("Rename",1);dialog.addButton("Cancel",0);
    if(dialog.runModalLoop()!=1)return;
    const auto name=dialog.getTextEditorContents("name").trim();if(name.isEmpty() || name==project.getPatternName(id,slot))return;
    rememberUndo("Rename pattern");project.setPatternName(id,slot,name);isDirty=true;
    pianoRollWindow->refresh(getSelectedPatternTitle());if(playlistWindow!=nullptr)playlistWindow->refresh();
}
bool handlePatternEditorShortcut(const juce::KeyPress& key)
{
    if(!canEditPattern())return false;
    const auto c=juce::CharacterFunctions::toLowerCase(key.getTextCharacter());const int code=key.getKeyCode();
    if(code==juce::KeyPress::tabKey){closePianoRollView();return true;}
    if(code==juce::KeyPress::numberPad8 || code==juce::KeyPress::numberPad2){pianoRollWindow->scrollPitches(code==juce::KeyPress::numberPad8?12:-12);return true;}
    if(code==juce::KeyPress::numberPad4 || code==juce::KeyPress::numberPad6)
    {
        notePasteTick=juce::jlimit(0,project.getPatternLengthSteps(selectedProjectTrackId(),selectedPatternIndex())*64-1,notePasteTick+(code==juce::KeyPress::numberPad6?64:-64));
        edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(notePasteTick/256.0*60/project.getInitialTempoBpm()));return true;
    }
    if(c>='1' && c<='8'){selectPatternSlot(int(c-'1'));return true;}
    if(code==juce::KeyPress::upKey || code==juce::KeyPress::downKey || code==juce::KeyPress::pageUpKey || code==juce::KeyPress::pageDownKey)
    {transformPianoPhrase(0,code==juce::KeyPress::upKey?1:code==juce::KeyPress::downKey?-1:code==juce::KeyPress::pageUpKey?12:-12);return true;}
    if(code==juce::KeyPress::leftKey || code==juce::KeyPress::rightKey){transformPianoPhrase(1,code==juce::KeyPress::leftKey?-64:64);return true;}
    if(c=='n'){renamePianoPattern();return true;}
    if(c=='s' || c=='x' || c=='y'){transformPianoPhrase(c=='s'?2:c=='x'?3:4);return true;}
    return pianoRollWindow->handleUiShortcut(key);
}
bool moveSelectedPianoNotes(int pitches,int ticks,bool duplicate)
{
    if(!canEditPattern() || selectedNoteKeys.empty())return false;
    auto moved=editableNotes();if(moved.empty())return false;
    const auto id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    auto data=capturePatternData(id,slot);
    if(!orbit::editing::translatePhrase(moved,pitches,ticks,data.lengthSteps*64))return false;
    if(!duplicate)std::erase_if(data.notes,[this](const auto& n){return selectedNoteKeys.contains({n.pitch,n.startTick});});
    data.notes.insert(data.notes.end(),moved.begin(),moved.end());
    if(!pianoRollGestureActive)rememberUndo(duplicate?"Duplicate selected notes":"Move selected notes");
    writePatternData(id,slot,data);selectedNoteKeys.clear();allNotesSelected=false;
    for(const auto& n:moved)selectedNoteKeys.emplace(n.pitch,n.startTick);
    isDirty=true;pianoRollGestureChanged=true;readPatternSteps();invalidatePlaylistPatternPreview(id,slot);
    return true;
}
void setPianoNoteVelocity(int pitch,int tick,int velocity)
{
    const auto id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    auto data=capturePatternData(id,slot);bool changed=false;
    for(auto& n:data.notes)if(n.pitch==pitch && n.startTick==tick && n.velocity!=velocity)
    {n.velocity=juce::jlimit(1,127,velocity);changed=true;}
    if(!changed)return;
    writePatternData(id,slot,data);isDirty=true;readPatternSteps();invalidatePlaylistPatternPreview(id,slot);
}
void transformPianoPhrase(int operation,int amount=0)
{
    if(!canEditPattern())return;
    auto notes=editableNotes();if(notes.empty())return;
    const auto original=notes;const auto id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    auto data=capturePatternData(id,slot);const bool whole=selectedNoteKeys.empty();
    juce::String label;
    if(operation==0){if(!orbit::editing::transpose(notes,amount))return;label="Transpose notes";}
    else if(operation==1){notes=orbit::editing::rotatePhrase(notes,amount,data.lengthSteps*64);label="Rotate notes";}
    else if(operation==2){notes=orbit::editing::scissorPhrase(notes);label="Scissor notes";}
    else{orbit::editing::flipPhrase(notes,operation==3,data.lengthSteps*64,whole);label=operation==3?"Flip notes horizontally":"Flip notes vertically";}
    if(notes==original)return;
    std::erase_if(data.notes,[&](const auto& n){return whole || selectedNoteKeys.contains({n.pitch,n.startTick});});
    data.notes.insert(data.notes.end(),notes.begin(),notes.end());rememberUndo(label);
    writePatternData(id,slot,data);selectedNoteKeys.clear();allNotesSelected=false;
    if(!whole)for(const auto& n:notes)selectedNoteKeys.emplace(n.pitch,n.startTick);
    finishPatternEdit(label+".");
}
