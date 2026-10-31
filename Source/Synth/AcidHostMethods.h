bool isAcidTrack(const juce::String& id)
{
    if(auto* track=engineTrackForProjectId(id))
        for(auto* plugin:track->pluginList)if(dynamic_cast<orbit::synth::AcidBassPlugin*>(plugin)!=nullptr)return true;
    return false;
}
void addAcidMarkers(te::MidiClip& clip,const juce::String& id,int pattern)
{
    if(!isAcidTrack(id) && !isNineKitTrack(id))return;
    const auto values=project.getPatternSwitches(id,pattern);std::set<int> ticks{0};
    const int end=project.getPatternLengthSteps(id,pattern)*64;
    for(const auto& value:values){ticks.insert(value.startTick);if(value.startTick+value.lengthTicks<end)ticks.insert(value.startTick+value.lengthTicks);}
    for(const auto& note:project.getPatternNotes(id,pattern))ticks.insert(note.startTick);
    for(const auto tick:ticks)
    {
        int flags=0;for(const auto& value:values)
            if(value.startTick<=tick && value.startTick+value.lengthTicks>tick)flags|=value.slide?2:1;
        // Tracktion stores ordinary controller values in its 14-bit domain.
        clip.getSequence().addControllerEvent(tracktion::BeatPosition::fromBeats(tick/256.0),119,flags<<7,nullptr);
    }
}
void createAcidDemo()
{
    const auto id=selectedProjectTrackId();if(!isAcidTrack(id))return;
    int slot=selectedPatternIndex();
    if(project.isPatternCreated(id,slot) || !project.getPatternNotes(id,slot).empty())
    {
        slot=-1;for(int i=0;i<64;++i)if(!project.isPatternCreated(id,i) && project.getPatternNotes(id,i).empty()){slot=i;break;}
    }
    if(slot<0){status.setText("No empty pattern slot for the demo. Clear a slot first.",juce::dontSendNotification);return;}
    rememberUndo("Create Acid Bass demo");
    constexpr std::array<int,16> pitches{36,36,43,36,39,36,46,43,36,48,39,36,43,36,34,36};
    project.setPatternLengthSteps(id,slot,16);project.setPatternName(id,slot,"Acid Demo");
    for(int i=0;i<16;++i)if(i!=5 && i!=13)project.appendPatternNoteTicks(id,slot,pitches[i],i*64,48,100);
    for(int i:{0,3,7,10,14})project.setPatternSwitch(id,slot,false,i*64,64,true);
    for(int i:{1,6,8,14,15})project.setPatternSwitch(id,slot,true,i*64,64,true);
    project.setPatternCreated(id,slot,true);project.setTrackSelectedPatternIndex(id,slot);
    selectedBankIndex=slot/8;selectedSlotIndex=slot%8;clearNoteSelection();isDirty=true;
    for(int i=0;i<8;++i){patternBankButtons[i]->setToggleState(i==selectedBankIndex,juce::dontSendNotification);patternSlotButtons[i]->setToggleState(i==selectedSlotIndex,juce::dontSendNotification);}
    readPatternSteps();rebuildPatternClip();
    if(pianoRollWindow!=nullptr && !pianoRollWindow->isVisible())showPianoRoll();
    status.setText("Acid demo ready. Use Pattern Play; toggle Accent and Slide below the note grid, or return to Instrument to adjust the sound.",juce::dontSendNotification);
}
