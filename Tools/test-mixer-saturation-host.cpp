static void checkMixerEqSaturation(ProjectView& host)
{
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
    auto* track=host.getGeneratorTracks()[0];for(auto* plugin:track->pluginList)if(plugin->isSynth())plugin->setEnabled(false);
    track->insertWaveClip("EQ fixture",juce::File("OPTIONS_CHECK_FOLDER/routing-input.wav"),{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
    host.setTrackGainDb(0,0);host.setTrackEqParameter(0,0,1000);host.setTrackEqParameter(0,1,20);
    const auto cold=mixerRenderPeak(host,"eq-clean");const auto undo=host.undoHistory.size();host.setTrackEqSaturation(0,true);
    const auto warm=mixerRenderPeak(host,"eq-warm");
    mixCheck(host.undoHistory.size()==undo+1 && warm<cold*0.99f && warm>cold*0.7f,"EQ saturation did not produce a bounded nonlinear response with one Undo");
    host.undoProjectAction();mixCheck(!host.getTrackEqSaturation(0),"EQ saturation Undo failed");host.redoProjectAction();
    mixCheck(host.getTrackEqSaturation(0),"EQ saturation Redo failed");
    host.copyMixerSettings(0,true);const auto pasteUndo=host.undoHistory.size();host.pasteMixerSettings(1,true);
    mixCheck(host.getTrackEqSaturation(1) && host.undoHistory.size()==pasteUndo+1,"EQ clipboard lost saturation or split its transaction");
    host.setTrackEqEnabled(0,false);mixCheck(!host.getTrackEqSaturationPlugin(0)->isEnabled() && host.getTrackEqSaturation(0),"EQ bypass did not suspend its saturation stage while retaining the preference");
    host.setTrackEqEnabled(0,true);mixCheck(host.getTrackEqSaturationPlugin(0)->isEnabled(),"EQ enable did not restore saturation");
    juce::AudioBuffer<float> audio(2,8);audio.clear();audio.setSample(0,0,std::numeric_limits<float>::quiet_NaN());audio.setSample(1,1,1000);
    te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),0,8,nullptr,0,{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(8.0/48000)},false,false,true,false);
    host.getTrackEqSaturationPlugin(0)->applyToBuffer(context);
    mixCheck(audio.getSample(0,0)==0 && std::isfinite(audio.getSample(1,1)) && audio.getSample(1,1)<=1,"EQ saturation emitted invalid/over-range output");
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/mixer-saturation.orbit");host.saveToFile(file);mixCheck(host.loadProjectFile(file),"EQ saturation reopen failed");
    mixCheck(host.getTrackEqSaturation(0) && host.getTrackEqSaturation(1) && std::abs(mixerRenderPeak(host,"eq-warm-reopened")/warm-1)<0.01,"EQ saturation state or rendered response lost on reopen");
    std::cout<<"PASS: native EQ saturation renders, bounded/finite stereo output, EQ bypass coupling, settings clipboard, single Undo/Redo and native reopen.\n";
}
