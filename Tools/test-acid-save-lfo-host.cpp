#define ORBIT_RACK_MAIN acidRackRegressionMain
#include "test-acid-rack-host.cpp"
#undef ORBIT_RACK_MAIN
int main(int argc,char** argv)
{
 try
 {
    if(argc>1 && juce::String(argv[1])=="--rack")return acidRackRegressionMain(1,nullptr);
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1680,1200);
    const juce::File folder("OPTIONS_CHECK_FOLDER"),file=folder.getChildFile("Evening Acid.orbit");
    auto* settings=h.applicationProperties.getUserSettings();acidFxCheck(settings!=nullptr,"isolated preferences missing");
    if(argc>1)
    {
        acidFxCheck(h.getSuggestedProjectFile("orbit")==file,"last saved name not remembered after restart");
        acidFxCheck(h.loadProjectFile(file),"fractional shape project reopen failed");
        auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(h.project.getTrackId(0)));
        auto* tube=dynamic_cast<orbit::mixing::DistortionPluginBase*>(h.getInsertPlugins(0)[0]);
        acidFxCheck(p && tube && std::abs(p->parameters[25]->getCurrentValue()-.37f)<1.e-6 && std::abs(tube->parameters[3]->getCurrentValue()-1.62f)<1.e-6,"fractional shape lost after fresh reopen");
        std::cout<<"PASS: fresh last-save filename and fractional synth/insert Shape persistence.\n";return 0;
    }
    for(const auto* key:{"lastProjectSaveFile","recentProjectPaths","defaultProjectPath"})settings->removeValue(key);
    h.createProjectDiscardingCurrent();
    acidFxCheck(h.getSuggestedProjectFile("orbit").getFileName()=="Untitled Project.orbit","fresh default changed");
    settings->setValue("defaultProjectPath",folder.getFullPathName());
    acidFxCheck(h.getSuggestedProjectFile("orbit")==folder.getChildFile("Untitled Project.orbit"),"configured folder ignored");
    settings->setValue("defaultProjectPath",folder.getChildFile("Configured.orbit").getFullPathName());
    acidFxCheck(h.getSuggestedProjectFile("orbit").getFileName()=="Configured.orbit","configured filename ignored");
    settings->setValue("defaultProjectPath",folder.getFullPathName());
    settings->setValue("recentProjectPaths",folder.getChildFile("Previous Song.orbit").getFullPathName());
    acidFxCheck(h.getSuggestedProjectFile("orbit").getFileName()=="Previous Song.orbit","legacy last-name fallback missing");
    h.addInstrumentTrack(true);const auto id=h.selectedProjectTrackId();h.openGeneratorControls(id,false);
    auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(id));
    auto& window=*h.generatorControlWindows.at(id).window;auto* editor=dynamic_cast<orbit::synth::AcidBassEditor*>(window.instrument.get());acidFxCheck(editor && p,"Acid editor missing");window.setBounds(30,90,900,300);
    for(int mask=0;mask<8;++mask)
    {
        editor->setEffectsExpanded((mask&1)!=0);editor->setAdvancedExpanded((mask&2)!=0);editor->setLfoExpanded((mask&4)!=0);
        acidFxCheck(editor->preferredHeight()==190+((mask&1)?350:0)+((mask&2)?130:0)+((mask&4)?200:0),"foldouts use incorrect independent heights");
        acidFxCheck(window.getHeight()==300+editor->preferredHeight()-190,"owning generator did not resize with foldouts");
        for(auto& k:editor->lfoKnobs)acidFxCheck(k.isVisible()==bool(mask&4) && (!(mask&4) || editor->getLocalBounds().contains(k.getBounds())),"LFO section hidden/clipped/coupled to Advanced");
        for(auto& k:editor->spreadKnobs)acidFxCheck(k.isVisible()==bool(mask&2),"Advanced coupled to LFO");
        for(auto& k:editor->extraKnobs)acidFxCheck(k.isVisible()==bool(mask&1),"FX coupled to other sections");
    }
    editor->setAdvancedExpanded(false);editor->setEffectsExpanded(false);
    {auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out(folder.getChildFile("acid-lfo-section.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
    editor->setLfoExpanded(false);acidFxCheck(window.getHeight()==300,"foldout collapse lost original height");editor->setEffectsExpanded(true);
    acidFxCheck(!p->parameters[25]->isDiscrete() && editor->extraKnobs[3].getInterval()==0,"synth Shape still discrete");
    editor->extraKnobs[3].setValue(.37,juce::sendNotificationSync);acidFxCheck(std::abs(p->parameters[25]->getCurrentValue()-.37f)<1.e-6 && editor->extraKnobs[3].getTextFromValue(.37)=="0.37","Shape fractional value/display rounded");
    p->parameters[21]->setParameter(1,juce::sendNotification);p->parameters[22]->setParameter(24,juce::sendNotification);
    p->parameters[25]->setParameter(0,juce::sendNotification);auto zero=isolatedModulation(h,*p,0);
    p->parameters[25]->setParameter(.37f,juce::sendNotification);auto blend=isolatedModulation(h,*p,0);
    p->parameters[25]->setParameter(1.62f,juce::sendNotification);auto upper=isolatedModulation(h,*p,0);
    acidFxCheck(audioDifference(zero,blend)>1 && audioDifference(blend,upper)>1,"fractional Shape does not change real audio");
    p->parameters[25]->setParameter(.37f,juce::sendNotification);const auto preset=folder.getChildFile("continuous.orbitpreset");
    acidFxCheck(h.saveGeneratorPresetFile(id,preset),"fractional preset save failed");p->parameters[25]->setParameter(2,juce::sendNotification);
    acidFxCheck(h.loadGeneratorPresetFile(id,preset) && std::abs(p->parameters[25]->getCurrentValue()-.37f)<1.e-6,"fractional preset recall failed");
    h.addInsert(0,9);auto* tube=dynamic_cast<orbit::mixing::DistortionPluginBase*>(h.getInsertPlugins(0)[0]);acidFxCheck(tube && !tube->parameters[3]->isDiscrete(),"Tube insert Shape still discrete");
    tube->parameters[3]->setParameter(1.62f,juce::sendNotification);acidFxCheck(std::abs(tube->parameters[3]->getCurrentValue()-1.62f)<1.e-6,"insert Shape rounded fractional value");
    h.saveToFile(file);acidFxCheck(h.project.getFile()==file && h.getSuggestedProjectFile("orbit")==file && h.getSuggestedProjectFile("orbittemplate")==file.withFileExtension("orbittemplate"),"Save As forgot current name/path/extension");
    const auto* chooser=h.fileChooser.get();h.saveProject();acidFxCheck(h.fileChooser.get()==chooser && h.project.getFile()==file,"ordinary Save reopened chooser");
    acidFxCheck(file.deleteFile(),"could not remove test file");acidFxCheck(h.getSuggestedProjectFile("orbit")==file,"missing current file forgot name");h.saveToFile(file);
    h.project.clearFileAssociation();h.project.setName("Template Copy");acidFxCheck(h.getSuggestedProjectFile("orbit")==folder.getChildFile("Template Copy.orbit"),"unassociated named document lost name");
    h.project.setName("Untitled Project");acidFxCheck(h.getSuggestedProjectFile("orbit")==file,"unassociated document forgot last saved name");h.applicationProperties.saveIfNeeded();
    std::cout<<"PASS: all eight FX/Advanced/LFO foldout combinations, restored height, continuous Tube Shape/audio/presets, current/last-name Save suggestions, legacy defaults, template naming and ordinary Save.\n";
    return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
