// Orbit Geonkick audition adapter. GPL-3.0-or-later; see ThirdParty/Geonkick/UPSTREAM.md.
std::unique_ptr<orbit::synth::GeonkickEditor> createGeonkickEditor(const juce::String& id,orbit::synth::GeonkickPlugin& plugin)
{
    auto editor=std::make_unique<orbit::synth::GeonkickEditor>(&plugin,[this]{rememberUndo("Adjust Geonkick");},[this]{isDirty=true;},
        [this,&plugin]
        {
            if(edit!=nullptr && edit->getTransport().getCurrentPlaybackContext()==nullptr)edit->getTransport().ensureContextAllocated();
            plugin.auditionVelocity.store(110,std::memory_order_relaxed);
        },
        [this,id]{selectGeneratorControlTrack(id);createGeonkickDemo();});
    editor->onEditEvents=[this,&plugin](int p,juce::Component* c){showParameterEventMenu(*plugin.parameters[size_t(p)],c);};return editor;
}
void createGeonkickDemo()
{
    const auto id=selectedProjectTrackId();if(dynamic_cast<orbit::synth::GeonkickPlugin*>(generatorControlPlugin(id))==nullptr)return;
    int slot=selectedPatternIndex();
    if(project.isPatternCreated(id,slot) || !project.getPatternNotes(id,slot).empty())
    {slot=-1;for(int i=0;i<64;++i)if(!project.isPatternCreated(id,i) && project.getPatternNotes(id,i).empty()){slot=i;break;}}
    if(slot<0){status.setText("No empty pattern slot for the demo. Existing notes were kept.",juce::dontSendNotification);return;}
    rememberUndo("Create Geonkick audition pattern");
    project.setPatternLengthSteps(id,slot,16);project.setPatternName(id,slot,"Drum Audition");
    constexpr std::array<int,8> ticks{0,128,256,384,512,640,768,896};
    constexpr std::array<int,8> velocities{127,40,100,65,127,80,100,50};
    for(size_t i=0;i<ticks.size();++i)project.appendPatternNoteTicks(id,slot,36,ticks[i],32,velocities[i]);
    project.setPatternCreated(id,slot,true);project.setTrackSelectedPatternIndex(id,slot);selectedBankIndex=slot/8;selectedSlotIndex=slot%8;clearNoteSelection();
    readPatternSteps();refreshControlPattern(id);if(auto window=generatorControlWindows.find(id);window!=generatorControlWindows.end())window->second.window->refreshPattern();
    status.setText("Drum audition pattern ready. Select Pattern and Play; compare Sound recipes and velocity response. Save/reopen keeps the sound.",juce::dontSendNotification);
}
