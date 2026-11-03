// Native filter-bank UI/automation, stereo FX, CPU meter and state checks.
#define ORBIT_ACID_CHECK_FOLDER "OPTIONS_CHECK_FOLDER"
#define main acidFxRegressionMain
#include "test-acid-fx-host.cpp"
#undef main
int main(int argc,char**)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        struct Cleanup{~Cleanup(){checkedOptions.reset();checkedApply={};}}cleanup;
        ProjectView host;host.stopTimer();host.setSize(1440,900);
        const juce::File file(ORBIT_ACID_CHECK_FOLDER "/acid-spread.orbit");
        using Plugin=orbit::synth::AcidBassPlugin;
        if(argc>1)
        {
            acidFxCheck(host.loadProjectFile(file),"fresh spread project load failed");
            auto* p=dynamic_cast<Plugin*>(host.generatorControlPlugin(host.project.getTrackId(0)));
            auto* b=dynamic_cast<Plugin*>(host.generatorControlPlugin(host.project.getTrackId(1)));
            acidFxCheck(p && b && p->parameters[17]->getCurrentValue()==1 && p->parameters[18]->getCurrentValue()==2 && p->parameters[19]->getCurrentValue()==70 && p->parameters[20]->getCurrentValue()==100 && b->parameters[17]->getCurrentValue()==0,"fresh-process spread states not independent");
            acidFxCheck(!host.optionBool("showAudioCpu",true),"CPU setting did not persist");host.updateCpuReadout();acidFxCheck(!host.cpuReadout.isVisible(),"hidden CPU meter returned on relaunch");
            host.songModeButton.onClick();auto audio=acidFxRender(host,"spread-fresh");double stereo=0;for(int n=0;n<audio.getNumSamples();++n)stereo+=std::abs(audio.getSample(0,n)-audio.getSample(1,n));acidFxCheck(stereo>.1,"fresh spread stereo audio lost");
            std::cout<<"PASS: fresh-process per-instance spread settings/audio and CPU visibility preference.\n";return 0;
        }
        host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();host.createAcidDemo();host.closePianoRollView();host.openGeneratorControls(id,false);
        auto* p=dynamic_cast<Plugin*>(host.generatorControlPlugin(id));acidFxCheck(p && p->parameters.size()==Plugin::parameterCount && p->parameters[17]->getCurrentValue()==0,"new Acid bank not Off by default");
        auto& window=*host.generatorControlWindows.at(id).window;auto* panel=dynamic_cast<orbit::synth::AcidBassEditor*>(window.instrument.get());acidFxCheck(panel,"missing native Acid editor");
        const auto original=window.getBounds();panel->setAdvancedExpanded(true);
        acidFxCheck(window.getHeight()>=original.getHeight()+140 && panel->getLocalBounds().contains(panel->spreadKnobs[1].getBounds()) && panel->spreadKnobs[1].isVisible(),"Advanced expansion clipped controls");
        panel->setEffectsExpanded(true);acidFxCheck(window.getHeight()>=original.getHeight()+270 && panel->getLocalBounds().contains(panel->spreadKnobs[1].getBounds()) && panel->knobs[12].isVisible(),"FX plus Advanced expansion clipped");
        panel->setEffectsExpanded(false);acidFxCheck(window.getHeight()==original.getHeight()+140,"FX collapse hid Advanced");panel->setAdvancedExpanded(false);acidFxCheck(window.getBounds()==original,"Advanced collapse failed to restore bounds");panel->setAdvancedExpanded(true);
        const auto undo=host.undoHistory.size();panel->spreadEnabled.setToggleState(true,juce::dontSendNotification);panel->spreadEnabled.onClick();panel->voices.setSelectedId(3,juce::sendNotificationSync);
        panel->spreadKnobs[0].onDragStart();panel->spreadKnobs[0].setValue(40,juce::sendNotificationSync);panel->spreadKnobs[0].setValue(70,juce::sendNotificationSync);panel->spreadKnobs[0].onDragEnd();panel->spreadKnobs[1].setValue(100,juce::sendNotificationSync);
        acidFxCheck(p->parameters[17]->getCurrentValue()==1 && p->parameters[18]->getCurrentValue()==2 && p->parameters[19]->getCurrentValue()==70 && host.undoHistory.size()==undo+4 && host.isDirty,"spread UI gestures or Undo grouping failed");
        host.undoProjectAction();p=dynamic_cast<Plugin*>(host.generatorControlPlugin(id));acidFxCheck(p->parameters[20]->getCurrentValue()==0,"spread width Undo failed");host.redoProjectAction();p=dynamic_cast<Plugin*>(host.generatorControlPlugin(id));acidFxCheck(p->parameters[20]->getCurrentValue()==100,"spread width Redo failed");
        panel=dynamic_cast<orbit::synth::AcidBassEditor*>(host.generatorControlWindows.at(id).window->instrument.get());
        int targetIndex=-1;panel->onEditEvents=[&](int index,juce::Component*){targetIndex=index;};panel->spreadKnobs[0].onEvents(&panel->spreadKnobs[0]);acidFxCheck(targetIndex==19,"spread automation menu points to wrong parameter");
        host.saveOption("showAudioCpu",true);host.updateCpuReadout();acidFxCheck(host.cpuReadout.isVisible() && host.cpuReadout.getText().startsWith("CPU ") && host.getLocalBounds().contains(host.cpuReadout.getBounds()) && !host.cpuReadout.getBounds().intersects(host.buildLabel.getBounds()),"CPU readout missing or overlaps build label");
        host.handleMenuResult(4,521);acidFxCheck(!host.cpuReadout.isVisible() && !host.optionBool("showAudioCpu",true),"CPU menu toggle failed");host.handleMenuResult(4,521);acidFxCheck(host.cpuReadout.isVisible(),"CPU show failed");host.handleMenuResult(2,306);acidFxCheck(!host.cpuReadout.isVisible(),"CPU survives hidden status area");host.handleMenuResult(2,306);acidFxCheck(host.cpuReadout.isVisible(),"CPU missing after status area restored");host.handleMenuResult(4,521);
        host.addInstrumentTrack(true);auto second=host.selectedProjectTrackId();acidFxCheck(dynamic_cast<Plugin*>(host.generatorControlPlugin(second))->parameters[17]->getCurrentValue()==0,"spread leaked to second generator");host.selectGeneratorControlTrack(id);
        host.project.setPlaylistClip(id,host.project.getPatternId(id,0),0,0,true);host.refreshEnginePlaylist();host.songModeButton.onClick();host.setTrackEqEnabled(0,false);host.setTrackEqEnabled(host.getMixerTracks().size(),false);host.setMasterGainDb(0);
        auto stereo=acidFxRender(host,"spread-stereo");double difference=0;for(int n=0;n<stereo.getNumSamples();++n){acidFxCheck(std::isfinite(stereo.getSample(0,n)) && std::isfinite(stereo.getSample(1,n)),"spread render invalid");difference+=std::abs(stereo.getSample(0,n)-stereo.getSample(1,n));}acidFxCheck(difference>.1,"native host collapsed spread to mono");
        p->parameters[10]->setParameter(60,juce::sendNotificationSync);p->parameters[16]->setParameter(1,juce::sendNotificationSync);auto fx=acidFxRender(host,"spread-stereo-fx");double fxDifference=0;for(int n=0;n<fx.getNumSamples();++n)fxDifference+=std::abs(fx.getSample(0,n)-fx.getSample(1,n));acidFxCheck(fxDifference>.1,"Acid FX collapsed spread to mono");
        p->parameters[10]->setParameter(0,juce::sendNotificationSync);p->parameters[16]->setParameter(0,juce::sendNotificationSync);
        auto target=host.targetForParameter(*p->parameters[19]);orbit::automation::Points points;points.add({0,0});points.add({16,1});host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,points,target.pluginId);host.updateEngineAutomation();acidFxCheck(p->parameters[19]->hasAutomationPoints(),"spread automation not projected");p->parameters[19]->updateStream();
        auto automated=acidFxRender(host,"spread-automation");double automatedDifference=0;for(int n=0;n<automated.getNumSamples();++n)automatedDifference+=std::abs(automated.getSample(0,n)-stereo.getSample(0,n));acidFxCheck(automatedDifference>.1,"spread automation has no audible output effect");
        host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,{},target.pluginId);host.updateEngineAutomation();p->parameters[19]->setParameter(70,juce::sendNotificationSync);
        host.openGeneratorControls(id,false);panel=dynamic_cast<orbit::synth::AcidBassEditor*>(host.generatorControlWindows.at(id).window->instrument.get());panel->setAdvancedExpanded(true);panel->setEffectsExpanded(true);acidFxCheck(host.generatorControlWindows.at(id).window->getHeight()<=juce::jmax(original.getHeight(),570),"reopening then expanding compounded saved panel height");
        {auto image=host.generatorControlWindows.at(id).window->createComponentSnapshot(host.generatorControlWindows.at(id).window->getLocalBounds());juce::FileOutputStream out(juce::File(ORBIT_ACID_CHECK_FOLDER "/acid-spread-ui.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
        host.saveToFile(file);acidFxCheck(host.loadProjectFile(file),"spread same-process reload failed");p=dynamic_cast<Plugin*>(host.generatorControlPlugin(id));acidFxCheck(p->parameters[19]->getCurrentValue()==70,"saved spread state lost");auto legacy=p->state.createCopy();for(size_t n=17;n<21;++n)legacy.removeProperty(Plugin::ids[n],nullptr);p->restorePluginStateFromValueTree(legacy);acidFxCheck(p->parameters[17]->getCurrentValue()==0 && p->parameters[19]->getCurrentValue()==0,"legacy state enabled spread");
        std::cout<<"PASS: Advanced/FX expansion, new parameter UI/grouped Undo/Redo/event target, independent instances, native stereo through FX, actual automation render, legacy Off, save/reopen and CPU meter show/hide/menu.\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
