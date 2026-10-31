// Modified Orbit port. GPL-3.0 + NOTICE.
// 9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9
bool isNineKitTrack(const juce::String& id)
{return dynamic_cast<orbit::synth::NineKitPlugin*>(generatorControlPlugin(id))!=nullptr;}
std::unique_ptr<orbit::synth::NineKitEditor> createNineKitEditor(const juce::String& id,orbit::synth::NineKitPlugin& plugin)
{
    using Editor=orbit::synth::NineKitEditor;Editor::Actions actions;
    const auto projectId=project.getProjectId();const auto valid=[this,id,projectId]{return project.getProjectId()==projectId && isNineKitTrack(id);};
    actions.begin=[this]{rememberUndo("Adjust 9W9");};actions.changed=[this]{isDirty=true;};
    actions.preview=[this,&plugin](int drum){if(edit && !edit->getTransport().getCurrentPlaybackContext())edit->getTransport().ensureContextAllocated();if(juce::isPositiveAndBelow(drum,11))plugin.auditionMask.fetch_or(1u<<drum,std::memory_order_relaxed);};
    actions.demo=[this,id,valid]{if(valid()){selectGeneratorControlTrack(id);createNineKitDemo();}};
    actions.readLength=[this,id,valid]{return valid()?project.getPatternLengthSteps(id,project.getTrackSelectedPatternIndex(id)):16;};
    actions.length=[this,id,valid](int n){if(!valid() || n<1)return;rememberUndo("Change 9W9 pattern length");const int slot=project.getTrackSelectedPatternIndex(id);project.setPatternLengthSteps(id,slot,n);refreshControlPattern(id);};
    actions.velocity=[this,id,&plugin,valid](int drum,int step){if(!valid())return 0;const int slot=project.getTrackSelectedPatternIndex(id),pitch=plugin.noteForVoice(drum);return project.getPatternNote(id,slot,pitch,step)?project.getPatternNoteVelocity(id,slot,pitch,step):0;};
    actions.step=[this,id,&plugin,valid](int drum,int step,int velocity)
    {
        if(!valid())return;const int slot=project.getTrackSelectedPatternIndex(id),pitch=plugin.noteForVoice(drum);
        rememberUndo("Edit 9W9 step");project.setPatternCreated(id,slot,true);project.setPatternNote(id,slot,pitch,step,velocity>0);
        if(velocity>0){project.setPatternNoteVelocity(id,slot,pitch,step,velocity);project.setPatternNoteLength(id,slot,pitch,step,1);}refreshControlPattern(id);
    };
    actions.readAccent=[this,id,valid](int step){if(valid())for(const auto& a:project.getPatternSwitches(id,project.getTrackSelectedPatternIndex(id)))if(!a.slide && a.startTick<=step*64 && a.startTick+a.lengthTicks>step*64)return true;return false;};
    actions.accent=[this,id,valid](int step,bool on){if(!valid())return;rememberUndo("Edit 9W9 Accent");int slot=project.getTrackSelectedPatternIndex(id);project.setPatternCreated(id,slot,true);project.setPatternSwitch(id,slot,false,step*64,64,on);refreshControlPattern(id);};
    auto editor=std::make_unique<Editor>(&plugin,std::move(actions));editor->onEditEvents=[this,&plugin](int p,juce::Component* c){showParameterEventMenu(*plugin.parameters[size_t(p)],c);};return editor;
}
void createNineKitDemo()
{
    const auto id=selectedProjectTrackId();auto* plugin=dynamic_cast<orbit::synth::NineKitPlugin*>(generatorControlPlugin(id));if(!plugin)return;
    int slot=selectedPatternIndex();if(project.isPatternCreated(id,slot) || !project.getPatternNotes(id,slot).empty()){slot=-1;for(int i=0;i<64;++i)if(!project.isPatternCreated(id,i) && project.getPatternNotes(id,i).empty()){slot=i;break;}}
    if(slot<0){status.setText("All pattern slots are occupied. The existing notes were kept.",juce::dontSendNotification);return;}
    rememberUndo("Create 9W9 demo beat");project.setPatternLengthSteps(id,slot,64);project.setPatternName(id,slot,"909 Demo");
    auto note=[&](int drum,int step,int velocity=100){project.appendPatternNoteTicks(id,slot,plugin->noteForVoice(drum),step*64,32,velocity);};
    for(int s=0;s<64;++s){if(s%8==0)note(0,s,110);if(s%8==4)note(1,s,110);if(s%16==12)note(6,s,90);if(s%2==0)note(8,s,s%4==0?85:50);if(s%16==6)note(7,s,95);if(s>=32 && s%8==2)note(9,s,80);if(s%16==0)project.setPatternSwitch(id,slot,false,s*64,64,true);}
    note(10,0,100);note(10,32,90);note(5,21,75);note(5,43,90);note(2,50,110);note(3,52,110);note(4,54,110);
    project.setPatternCreated(id,slot,true);project.setTrackSelectedPatternIndex(id,slot);selectedBankIndex=slot/8;selectedSlotIndex=slot%8;clearNoteSelection();readPatternSteps();refreshControlPattern(id);
    if(auto w=generatorControlWindows.find(id);w!=generatorControlWindows.end())w->second.window->refreshPattern();status.setText("9W9 demo ready: four bars, all eleven sounds. Select Pattern and Play; change drums to edit their steps.",juce::dontSendNotification);
}
