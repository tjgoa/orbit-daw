static void fxCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool fxMenuContains(const juce::PopupMenu& menu,const juce::String& text)
{
    for(juce::PopupMenu::MenuItemIterator it(menu);it.next();)
    {const auto& item=it.getItem();if(item.text.contains(text) || (item.subMenu!=nullptr && fxMenuContains(*item.subMenu,text)))return true;}return false;
}
static juce::PluginDescription fxDescription(const char* name,int id,bool instrument,int inputs=2)
{
    juce::PluginDescription d;d.name=name;d.descriptiveName=name;d.pluginFormatName="VST3";d.manufacturerName="Fixture";d.fileOrIdentifier=juce::String("fixture-")+name+".vst3";
    d.uniqueId=id;d.deprecatedUid=id;d.isInstrument=instrument;d.numInputChannels=inputs;d.numOutputChannels=inputs;return d;
}
#include "test-effect-workspaces-host.cpp"
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/effects.orbit");
    if(argc>1)
    {
        fxCheck(host.loadProjectFile(file),"fresh effect-chain project load failed");
        fxCheck(host.getInsertPlugins(host.getMixerTracks().size()).size()==4 && host.getInsertPlugins(1).size()==3,"effect chain counts lost in fresh process");
        const auto externalFile=juce::File("OPTIONS_CHECK_FOLDER/vst-effect.orbit");
        if(externalFile.existsAsFile())
        {fxCheck(host.loadProjectFile(externalFile),"fresh VST3 effect project failed");auto* plugin=dynamic_cast<te::ExternalPlugin*>(host.getInsertPlugins(0).getLast());fxCheck(plugin!=nullptr && !plugin->isMissing() && !plugin->isSynth(),"VST3 effect did not restore in fresh process");}
        std::cout<<"PASS: serial insert chains, four Master slots and optional VST3 effect restored in a fresh process.\n";return 0;
    }
    fxWorkspaceCheck(host);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
    auto& manager=host.engine.getPluginManager();
    manager.knownPluginList.addType(fxDescription("Zulu FX",100,false));manager.knownPluginList.addType(fxDescription("Alpha FX",101,false,0));
    manager.knownPluginList.addType(fxDescription("Synth Fixture",102,true));
    const auto descriptions=host.knownVst3Effects();fxCheck(descriptions.size()==2 && descriptions[0].name=="Alpha FX","effect catalogue excluded fast metadata, included synths or lost sorting");
    const auto menu=host.makeInsertMenu();fxCheck(fxMenuContains(menu,"VST Effects") && fxMenuContains(menu,"Alpha FX") && !fxMenuContains(menu,"VST3 effects are not available"),"Insert VST Effects remains unavailable");
    {Vst3InstrumentBrowser browser(manager.pluginFormatManager,manager.knownPluginList,[]{return juce::StringArray{};},[]{},[](const auto&){},false);
     fxCheck(browser.instruments.size()==2 && browser.instruments[0].name=="Alpha FX","effect browser filter did not match the menu");}
    {auto menu=host.makeInsertContextMenu(0,0,descriptions);
     fxCheck(fxMenuContains(menu,"Built-in Effects") && fxMenuContains(menu,"Tube Distortion") && fxMenuContains(menu,"VST3 Effects")
        && fxMenuContains(menu,"Alpha FX") && !fxMenuContains(menu,"Add Effect"),"context menu still hides choices behind Add Effect");}
    host.addInsert(0,1);host.addInsert(0,3);host.addInsert(0,2);
    {const auto owner=host.mixerOwnerId(0),projectId=host.project.getProjectId(),replaceId=host.getInsertPlugins(0)[1]->itemID.toString();
     const auto first=host.getInsertPlugins(0)[0]->itemID.toString(),third=host.getInsertPlugins(0)[2]->itemID.toString();const auto before=host.undoHistory.size();
     host.handleInsertContextChoice(owner,projectId,replaceId,descriptions,108);
     auto plugins=host.getInsertPlugins(0);fxCheck(plugins.size()==3 && plugins[0]->itemID.toString()==first && plugins[2]->itemID.toString()==third
       && dynamic_cast<orbit::mixing::PeakCompressorPlugin*>(plugins[1])!=nullptr,"occupied full-chain slot replacement changed neighbors or failed");
     fxCheck(host.undoHistory.size()==before+1,"slot replacement was not one Undo");host.undoProjectAction();
     fxCheck(host.getInsertPlugins(0)[1]->itemID.toString()==replaceId,"replacement Undo lost original slot");
     host.handleInsertContextChoice(owner,projectId,"deleted-plugin",descriptions,108);
     fxCheck(host.getInsertPlugins(0)[1]->itemID.toString()==replaceId,"stale context replaced a different effect");
     host.removeInsert(0,2);}
    host.addInsert(1,2);
    auto* compressor=dynamic_cast<te::CompressorPlugin*>(host.getInsertPlugins(0)[0]);fxCheck(compressor!=nullptr,"source compressor absent");compressor->setThreshold(.14f);compressor->setRatio(.3f);
    const auto sourcePlugin=compressor->itemID.toString(),second=host.project.getTrackId(1);
    host.setEffectSidechain(sourcePlugin,host.getGeneratorTracks()[1]->itemID.toString());
    const auto sourceParam=host.targetForParameter(*compressor->getAutomatableParameter(0));orbit::automation::Points points;points.add({0,.2f});points.add({8,.8f});
    host.project.replaceSongAutomationPoints(sourceParam.trackId,sourceParam.pluginName,sourceParam.parameterId,sourceParam.parameterName,points,sourceParam.pluginId);
    const auto destinationVolume=host.getGeneratorTracks()[1]->getVolumePlugin();const auto destinationTarget=host.targetForParameter(*destinationVolume->volParam);
    host.project.replaceSongAutomationPoints(second,destinationTarget.pluginName,destinationTarget.parameterId,destinationTarget.parameterName,points,destinationTarget.pluginId);
    host.copyInsertEffects(0);const auto pasteUndo=host.undoHistory.size();fxCheck(host.pasteInsertEffects(1,true),"effect chain paste refused");
    fxCheck(host.undoHistory.size()==pasteUndo+1 && host.getInsertPlugins(1).size()==2,"chain paste was not one transaction");
    auto* copied=dynamic_cast<te::CompressorPlugin*>(host.getInsertPlugins(1)[0]);fxCheck(copied!=nullptr && std::abs(copied->getThreshold()-.14f)<.001 && copied->itemID.toString()!=sourcePlugin,"effect settings/type/identity did not clone");
    fxCheck(!copied->getSidechainSourceID().isValid() && !copied->useSidechainTrigger.get(),"unsafe copied self-sidechain was retained");
    fxCheck(host.project.getSongAutomationLaneCount(second)==1 && host.songEventPoints(destinationTarget).size()==2 && copied->getAutomatableParameter(0)->getCurve().getNumPoints()==0,"effect paste copied automation or changed destination event lanes");
    host.undoProjectAction();fxCheck(host.getInsertPlugins(1).size()==1 && dynamic_cast<te::ReverbPlugin*>(host.getInsertPlugins(1)[0])!=nullptr,"chain paste Undo failed");host.redoProjectAction();
    const auto refusedUndo=host.undoHistory.size();fxCheck(!host.pasteInsertEffects(1,false) && host.undoHistory.size()==refusedUndo,"capacity refusal partially edited the chain");
    host.copyInsertEffects(0,1);fxCheck(host.pasteInsertEffects(1,false) && host.getInsertPlugins(1).size()==3,"single effect append failed");
    const auto movedId=host.getInsertPlugins(1)[2]->itemID.toString();host.moveInsertEffect(1,2,-1);fxCheck(host.getInsertPlugins(1)[1]->itemID.toString()==movedId,"serial insert reorder failed");
    const auto enabled=host.getInsertPlugins(1)[1]->isEnabled();const auto toggleUndo=host.undoHistory.size();host.toggleInsert(1,1);
    fxCheck(host.getInsertPlugins(1)[1]->isEnabled()!=enabled && host.undoHistory.size()==toggleUndo+1,"insert bypass was not undoable");host.undoProjectAction();
    host.copyInsertEffects(0);const auto master=host.getMixerTracks().size();fxCheck(host.pasteInsertEffects(master,true),"Master chain paste failed");host.addInsert(master,6);host.addInsert(master,4);
    fxCheck(host.getInsertPlugins(master).size()==4,"fourth Master insert still refused");const auto fullUndo=host.undoHistory.size();host.addInsert(master,5);fxCheck(host.getInsertPlugins(master).size()==4 && host.undoHistory.size()==fullUndo,"fifth Master insert mutated the chain");
    host.showMixer(true);fxCheck(host.mixerWindow->masterStrip.insertButtons[3].isVisible() && !host.mixerWindow->masterStrip.insertButtons[3].getBounds().isEmpty(),"fourth Master insert is hidden/unlaid out");
    const auto ret=host.getMixerTracks().indexOf(host.getFxReturnTrack(0));host.addInsert(ret,1);host.addInsert(ret,3);host.addInsert(ret,5);host.addInsert(ret,2);fxCheck(host.getInsertPlugins(ret).size()==4,"return channel cannot use four inserts");
    host.setTrackEqParameter(master,1,3);host.setTrackEqSaturation(master,true);host.setMasterMuted(true);
    fxCheck(host.getInsertPlugins(master).size()==4 && host.getTrackEqSaturationPlugin(master)!=nullptr && host.masterInsertEnd()==host.edit->getMasterPluginList().size()-1,"internal EQ/saturation/mute consumed Master insert slots");
    host.setMasterMuted(false);
    host.saveToFile(file);fxCheck(host.loadProjectFile(file),"effect chain reopen failed");
    // Optional installed-plugin integration. No third-party binary enters an archive.
    const juce::String path=VST_EFFECT_PATH_LITERAL;
    if(path.isNotEmpty())
    {
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,path);const juce::PluginDescription* effect=nullptr;
        for(auto* d:found)if(!d->isInstrument){effect=d;break;}
        fxCheck(effect!=nullptr,"installed fixture has no VST3 audio effect class");manager.knownPluginList.addType(*effect);
        fxCheck(host.addVst3Effect(0,*effect),"real VST3 effect insertion failed");
        auto* plugin=dynamic_cast<te::ExternalPlugin*>(host.getInsertPlugins(0).getLast());fxCheck(plugin!=nullptr && plugin->getAudioPluginInstance()!=nullptr && !plugin->isSynth(),"real VST3 slot did not contain an effect instance");
        auto fxWindow=host.mixerWindow->openEffectWindows().at(plugin->itemID.toString());
        fxCheck(fxWindow->isVisible() && fxWindow->nativeEditor!=nullptr && fxWindow->getParentComponent()==&host,"external FX did not auto-open its independent native-wrapper panel");
        fxWindow->minimiseButtonPressed();host.openInsertEditor(0,host.getInsertPlugins(0).size()-1);
        fxCheck(fxWindow->isVisible() && !fxWindow->isMinimised(),"native FX did not restore its existing instance");

        int parameter=-1;for(int i=2;i<plugin->getNumAutomatableParameters();++i)if(plugin->getAutomatableParameter(i)->getParameterName().containsIgnoreCase("feedback")){parameter=i;break;}
        if(parameter<0 && plugin->getNumAutomatableParameters()>2)parameter=2;
        fxCheck(parameter>=0,"VST3 fixture has no editable parameter");plugin->getAutomatableParameter(parameter)->setNormalisedParameter(.37f,juce::sendNotification);
        const auto value=plugin->getAutomatableParameter(parameter)->getCurrentNormalisedValue();const auto id=plugin->getAutomatableParameter(parameter)->paramID;
        // Exercise the real vendor DSP in the native offline graph before saving.
        const auto audioFile=juce::File("OPTIONS_CHECK_FOLDER/vst-effect-input.wav");juce::WavAudioFormat wav;
        {auto stream=std::make_unique<juce::FileOutputStream>(audioFile);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));fxCheck(writer!=nullptr,"VST fixture writer failed");juce::AudioBuffer<float> audio(2,48000);
         for(int n=0;n<48000;++n){const float sample=.02f*std::sin(float(n)*.04f);audio.setSample(0,n,sample);audio.setSample(1,n,sample);}writer->writeFromAudioSampleBuffer(audio,0,48000);}
        auto* sourceTrack=host.getGeneratorTracks()[0];for(auto* p:sourceTrack->pluginList)if(p->isSynth())p->setEnabled(false);
        for(auto* p:host.getInsertPlugins(0))if(p!=plugin)p->setEnabled(false);for(auto* p:host.getInsertPlugins(host.getMixerTracks().size()))p->setEnabled(false);
        host.setTrackEqEnabled(host.getMixerTracks().size(),false);host.setMasterGainDb(0);
        sourceTrack->insertWaveClip("VST fixture",audioFile,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
        te::Renderer::Parameters render(*host.edit);render.destFile=juce::File("OPTIONS_CHECK_FOLDER/vst-effect-render.wav");render.audioFormat=&wav;render.sampleRateForAudio=48000;render.bitDepth=32;render.usePlugins=true;render.useMasterPlugins=true;
        render.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(2)};
        {te::Renderer::RenderTask task("VST3 effect validation",render,nullptr,nullptr);int attempts=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++attempts<100000){}fxCheck(task.errorMessage.isEmpty() && attempts<100000,"VST3 native DSP render failed");}
        std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(render.destFile),true));fxCheck(reader!=nullptr,"VST3 render unreadable");juce::AudioBuffer<float> rendered(2,int(reader->lengthInSamples));reader->read(&rendered,0,rendered.getNumSamples(),0,true,true);
        for(int c=0;c<2;++c)for(int n=0;n<rendered.getNumSamples();++n)fxCheck(std::isfinite(rendered.getSample(c,n)),"VST3 rendered nonfinite samples");fxCheck(rendered.getMagnitude(0,rendered.getNumSamples())>.000001,"VST3 fixture render was silent");
        host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/vst-effect.orbit"));fxCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/vst-effect.orbit")),"VST3 effect reopen failed");
        plugin=dynamic_cast<te::ExternalPlugin*>(host.getInsertPlugins(0).getLast());bool restored=false;
        for(auto* p:plugin->getAutomatableParameters())if(p->paramID==id && std::abs(p->getCurrentNormalisedValue()-value)<.002)restored=true;
        fxCheck(restored,"VST3 effect parameter state lost on reopen");
        fxCheck(host.edit->state.getChildWithName("ORBITPLUGINCATALOG").getNumChildren()>0,"project omitted its VST3 description catalogue");
        juce::String legacyError;auto legacy=ProjectDocument::loadFromFile(juce::File("OPTIONS_CHECK_FOLDER/vst-effect.orbit"),legacyError);fxCheck(bool(legacy),"legacy fixture read failed");
        auto legacyState=legacy->getTracktionEditState();auto catalogue=legacyState.getChildWithName("ORBITPLUGINCATALOG");legacyState.removeChild(catalogue,nullptr);legacy->setTracktionEditState(legacyState);
        const auto legacyFile=juce::File("OPTIONS_CHECK_FOLDER/vst-effect-no-catalog.orbit");fxCheck(legacy->saveToFile(legacyFile,legacyError),"legacy fixture write failed");manager.knownPluginList.clear();
        fxCheck(host.loadProjectFile(legacyFile),"legacy VST project failed without a scan cache");plugin=dynamic_cast<te::ExternalPlugin*>(host.getInsertPlugins(0).getLast());fxCheck(plugin!=nullptr && !plugin->isMissing(),"legacy VST3 class identity did not resolve without cache");
        host.copyInsertEffects(0,host.getInsertPlugins(0).size()-1);host.createProjectDiscardingCurrent();
        fxCheck(host.pasteInsertEffects(host.getMixerTracks().size(),false),"VST3 effect clipboard failed across projects");
        fxCheck(dynamic_cast<te::ExternalPlugin*>(host.getInsertPlugins(host.getMixerTracks().size()).getLast())!=nullptr,"cross-project VST3 paste lost plugin type");
        std::cout<<"PASS: installed VST3 effect discovery, insertion, native DSP render, vendor parameter state/reopen, cache-independent loading and cross-project clipboard: "<<effect->name<<". Vendor GUI/listening remain manual.\n";
    }
    std::cout<<"PASS: audio-effect catalogue/browser separation, enabled Insert menu, serial settings clipboard with unique IDs and one Undo, retained destination events/no copied curves, unsafe sidechain clearing, append/replace/capacity refusal, reorder/bypass and four visible Master/return inserts, native save/reopen.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
