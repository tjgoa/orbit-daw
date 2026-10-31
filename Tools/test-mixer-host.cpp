static void mixCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
#include "test-mixer-routing-host.cpp"
#include "test-mixer-saturation-host.cpp"
int main(int argc,char**)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/mixer-settings.orbit");
        if(argc>1)
        {
            mixCheck(host.loadProjectFile(file),"fresh mixer project load failed");
            auto* track=host.getGeneratorTracks()[1];auto* send=track->getAuxSendPlugin(0);
            mixCheck(send!=nullptr && !host.getTrackSendLinked(1,0) && track->pluginList.indexOf(send)<track->pluginList.indexOf(track->getVolumePlugin()),"pre-fader send state/order lost in fresh process");
            mixCheck(std::abs(host.getTrackEqParameter(1,1)-6.0f)<0.01f && std::abs(host.getTrackGainDb(1)+8)<0.01,"copied channel values lost in fresh process");
            mixCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/mixer-routes.orbit")),"fresh multi-route project failed");
            mixCheck(host.getTrackOutputRoutes(0)==7 && host.isMasterVolumeBeforeInserts()
                && host.songEventPoints(host.targetForParameter(*host.masterVolumePlugin()->volParam)).size()==2,"multi-route/Master automation did not survive fresh reopen");
            mixCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/mixer-saturation.orbit")) && host.getTrackEqSaturation(0) && host.getTrackEqSaturation(1),"EQ saturation lost in fresh process");
            std::cout<<"PASS: Mixer settings and native pre-fader routing restored in a fresh process.\n";return 0;
        }
        host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
        {MixerWindow::ChannelStrip strip;strip.setSize(142,750);strip.setMixControlsVisible(true);strip.setInsertRow(0,"Peak Compressor",true);strip.resized();
         int bypass=0,opened=0,menus=0;strip.onToggleInsert=[&](int){++bypass;};strip.onEditInsert=[&](int){++opened;};
         strip.onInsertMenu=[&](int,juce::Component*){++menus;};auto& button=strip.insertButtons[0];
         auto event=[&](float x,int flags){const juce::Point<float> pt{x,10};return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),pt,juce::ModifierKeys(flags),1,0,0,0,0,&button,&button,juce::Time::getCurrentTime(),pt,juce::Time::getCurrentTime(),1,false);};
         auto light=event(8,juce::ModifierKeys::leftButtonModifier);button.mouseDown(light);button.mouseUp(light);
         mixCheck(bypass==1 && opened==0,"left insert light did not exclusively bypass");
         button.onClick(); // dispatch the name action synchronously in the headless harness
         mixCheck(opened==1 && bypass==1,"effect name did not exclusively open editor");
         auto right=event(30,juce::ModifierKeys::rightButtonModifier);button.mouseDown(right);button.mouseUp(right);
         mixCheck(menus==1 && opened==1,"right-click opened editor instead of context menu");
         mixCheck(!strip.bypassInsertButtons[0].isVisible() && !strip.removeInsertButtons[0].isVisible()
            && button.getWidth()>strip.getWidth()-18,"duplicate insert buttons remain visible or consume width");}

        // The pointer and blue arc share JUCE's clockwise-from-top convention.
        // At maximum both must reach the lower-right end, leaving only the
        // intended bottom gap rather than an extra unused upper-right segment.
        {MixerWindow::ChannelStrip::EqKnob eq;MixerWindow::ChannelStrip::SendKnob send;
         eq.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);send.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);eq.setSize(90,90);send.setSize(90,90);eq.setRange(-20,20);send.setRange(-60,6);
         auto blueNear=[](const juce::Image& image,int x,int y){for(int dx=-2;dx<=2;++dx)for(int dy=-2;dy<=2;++dy){auto c=image.getPixelAt(x+dx,y+dy);if(c.getRed()<130 && c.getGreen()>125 && c.getBlue()>160)return true;}return false;};
         for(int maximum:{0,1})
         {eq.setValue(maximum?20:-20,juce::dontSendNotification);send.setValue(maximum?6:-60,juce::dontSendNotification);
          auto eqImage=eq.createComponentSnapshot(eq.getLocalBounds()),sendImage=send.createComponentSnapshot(send.getLocalBounds());

          juce::FileOutputStream e(juce::File(juce::String("OPTIONS_CHECK_FOLDER/eq-knob-")+juce::String(maximum)+".png"));juce::PNGImageFormat().writeImageToStream(eqImage,e);
          juce::FileOutputStream f(juce::File(juce::String("OPTIONS_CHECK_FOLDER/send-knob-")+juce::String(maximum)+".png"));juce::PNGImageFormat().writeImageToStream(sendImage,f);
          std::cout<<"Knob maximum="<<maximum<<" EQ blue="<<blueNear(eqImage,68,68)<<" Send blue="<<blueNear(sendImage,74,66)<<" values="<<eq.getValue()<<","<<send.getValue()<<"\n";
          mixCheck(blueNear(eqImage,68,68)==bool(maximum) && blueNear(sendImage,74,66)==bool(maximum),"knob fill fails to reach its maximum endpoint or remains filled at minimum");}}

        const auto first=host.project.getTrackId(0),second=host.project.getTrackId(1);
        host.setTrackGainDb(0,-8);host.setTrackPan(0,-0.3);host.setTrackEqParameter(0,1,6);host.setTrackEqEnabled(0,true);
        host.setTrackSendGainDb(0,0,-10);host.setTrackSendLinked(0,0,false);
        const auto linkedUndo=host.undoHistory.size();host.setTrackSendLinked(0,0,true);
        auto* source=host.getGeneratorTracks()[0];auto* send=source->getAuxSendPlugin(0);
        mixCheck(source->pluginList.indexOf(send)>source->pluginList.indexOf(source->getVolumePlugin()),"linked send did not move after the native volume/pan stage");
        mixCheck(host.undoHistory.size()==linkedUndo+1,"send routing did not create one Undo");host.undoProjectAction();
        source=host.getGeneratorTracks()[0];send=source->getAuxSendPlugin(0);
        mixCheck(!host.getTrackSendLinked(0,0) && source->pluginList.indexOf(send)<source->pluginList.indexOf(source->getVolumePlugin()),"routing Undo did not restore pre-fader stage");
        // An inactive send remembers its routing before creation.
        host.setTrackSendLinked(0,2,false);host.setTrackSendGainDb(0,2,-18);
        source=host.getGeneratorTracks()[0];send=source->getAuxSendPlugin(2);
        mixCheck(source->pluginList.indexOf(send)<source->pluginList.indexOf(source->getVolumePlugin()),"new send ignored remembered pre-fader preference");
        host.project.setPatternNoteTicks(second,0,60,0,64,100);
        orbit::automation::Points curve;curve.add({0.25,0.4f});host.project.replacePatternAutomationPoints(second,0,"MIDI","Modulation","Modulation",curve);
        auto* destination=host.getGeneratorTracks()[1];auto* instrument=destination->pluginList.findFirstPluginOfType<te::FourOscPlugin>();
        mixCheck(instrument!=nullptr,"destination synth absent");const auto instrumentId=instrument->itemID.toString();
        host.setTrackOutputBus(1,1);const auto routeOwner=host.getGeneratorTracks()[1]->getOutput().getDestinationTrack()->itemID;
        host.copyMixerSettings(0,false);const auto undo=host.undoHistory.size();host.pasteMixerSettings(1,false);
        mixCheck(host.undoHistory.size()==undo+1,"channel paste was not one transaction");
        mixCheck(std::abs(host.getTrackGainDb(1)+8)<0.01 && std::abs(host.getTrackPan(1)+0.3)<0.01 && std::abs(host.getTrackEqParameter(1,1)-6)<0.01,"channel settings paste missed level/pan/EQ");
        mixCheck(std::abs(host.getTrackSendGainDb(1,0)+10)<0.01 && !host.getTrackSendLinked(1,0),"channel paste missed send levels/link mode");
        mixCheck(host.getGeneratorTracks()[1]->getOutput().getDestinationTrack()->itemID==routeOwner && host.getGeneratorTracks()[1]->pluginList.findFirstPluginOfType<te::FourOscPlugin>()->itemID.toString()==instrumentId,"settings paste replaced instrument or routing");
        mixCheck(host.project.getPatternNotes(second,0).size()==1 && host.project.getPatternAutomationLaneCount(second,0)==1,"settings paste modified notes or event lanes");
        host.undoProjectAction();mixCheck(std::abs(host.getTrackGainDb(1))<0.01,"settings paste Undo failed");host.redoProjectAction();
        host.copyMixerSettings(0,true);host.setTrackGainDb(1,-3);const auto eqUndo=host.undoHistory.size();host.pasteMixerSettings(1,true);
        mixCheck(std::abs(host.getTrackGainDb(1)+3)<0.01 && host.undoHistory.size()==eqUndo+1,"EQ-only paste affected level or split Undo");
        host.pasteMixerSettings(1,false);mixCheck(std::abs(host.getTrackGainDb(1)+3)<0.01,"EQ-only clipboard incorrectly allowed a full channel paste");
        host.setTrackGainDb(1,-8);host.copyMixerSettings(0,false);
        auto* eqParameter=host.mixerEqParameter(1,1,false);mixCheck(eqParameter!=nullptr,"EQ parameter binding absent");
        auto target=host.targetForParameter(*eqParameter);host.openParameterEvents(target,false);
        mixCheck(host.songEventWindow->panel.trackId==second && host.songEventWindow->panel.targets[size_t(host.songEventWindow->panel.selectedTarget)].parameterId==eqParameter->paramID,"EQ events opened the wrong parameter");
        const auto master=host.getMixerTracks().size();host.copyMixerSettings(0,true);host.pasteMixerSettings(master,true);
        mixCheck(std::abs(host.getTrackEqParameter(master,1)-6)<0.01,"Master EQ paste failed");
        const auto owner=host.mixerOwnerId(1);host.ensureFxReturnTrack(7);mixCheck(host.mixerIndexForOwner(owner)==1 && host.mixerIndexForOwner("mixer:master")==host.getMixerTracks().size(),"context identity changed when mixer modules were inserted");
        const auto returnIndex=host.getMixerTracks().indexOf(host.getFxReturnTrack(7));
        const auto renameUndo=host.undoHistory.size();host.renameMixerChannel(returnIndex,"Long Reverb");
        mixCheck(host.getFxReturnTrack(7)->getName()=="Long Reverb" && host.undoHistory.size()==renameUndo+1,"return rename did not work as one Undo");
        host.undoProjectAction();mixCheck(host.getFxReturnTrack(7)->getName()!="Long Reverb","return rename Undo failed");host.redoProjectAction();
        host.showMixer(false);host.mixerWindow->refresh(1);
        mixCheck(bool(host.mixerWindow->channelStrips[1]->onSettingsMenu) && bool(host.mixerWindow->channelStrips[1]->eqControls[0][0].onEvents)
            && bool(host.mixerWindow->channelStrips[1]->sends[0].onEvents),"strip context bindings missing");
        auto image=host.mixerWindow->createComponentSnapshot(host.mixerWindow->getLocalBounds());
        {juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/mixer-settings.png"));juce::PNGImageFormat().writeImageToStream(image,output);}
        host.saveToFile(file);mixCheck(host.loadProjectFile(file),"mixer settings reopen failed");
        mixCheck(host.project.getTrackId(0)==first && host.project.getTrackId(1)==second && !host.getTrackSendLinked(1,0),"mixer settings lost stable identities or send mode");
        checkMixerRouting(host);checkMixerEqSaturation(host);
        std::cout<<"PASS: channel/EQ value clipboard, one-paste Undo/Redo, instrument/note/event/routing preservation, pre/post-fader native send order and remembered lazy creation, EQ event binding, module identity, native save/reopen. Hardware disabled; listening remains manual.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
