#define ORBIT_FREE_MAIN acidFreeLfoRegressionMain
#include "test-acid-free-lfo-host.cpp"
#undef ORBIT_FREE_MAIN
static juce::Image knobSnapshot(juce::Component& c){return c.createComponentSnapshot(c.getLocalBounds());}
static bool sameKnobImage(const juce::Image& a,const juce::Image& b)
{if(a.getBounds()!=b.getBounds())return false;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)if(a.getPixelAt(x,y)!=b.getPixelAt(x,y))return false;return true;}
static void writeKnobImage(juce::Component& c,const char* name)
{juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));juce::PNGImageFormat().writeImageToStream(knobSnapshot(c),out);}
int main(int argc,char** argv)
{
 try
 {
    if(argc>1 && juce::String(argv[1])=="--free")return acidFreeLfoRegressionMain(1,nullptr);
    if(argc>1 && juce::String(argv[1])=="--rack")return acidRackRegressionMain(1,nullptr);
    juce::ScopedJuceInitialiser_GUI gui;
    struct Cleanup{~Cleanup(){checkedOptions.reset();checkedApply={};}}cleanup;
    ProjectView h;h.stopTimer();h.setSize(1680,1200);const juce::File file("OPTIONS_CHECK_FOLDER/knob-skins.orbit"),preset("OPTIONS_CHECK_FOLDER/skin.orbitpreset");
    auto acid=[&](int row){return dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(h.project.getTrackId(row)));};
    if(argc>1 && juce::String(argv[1])=="--settings")
    {
        h.setKnobStyle(2);h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);const auto a=h.selectedProjectTrackId();h.addInstrumentTrack(true);const auto b=h.selectedProjectTrackId();h.showGeneratorProperties(a);
        checkedOptions->choices.at("acidSkin")->setSelectedItemIndex(2,juce::dontSendNotification);checkedOptions->choices.at("knobStyle")->setSelectedItemIndex(2,juce::dontSendNotification);
        auto undo=h.undoHistory.size();acidFxCheck(checkedApply(*checkedOptions) && h.undoHistory.size()==undo+1 && int(acid(0)->state.getProperty("orbitAcidSkin"))==2 && int(acid(0)->state.getProperty("orbitKnobOverride"))==1 && int(acid(1)->state.getProperty("orbitAcidSkin",0))==0,"Settings followed selected B instead of owning A");
        h.undoProjectAction();acidFxCheck(int(acid(0)->state.getProperty("orbitAcidSkin",0))==0 && int(acid(0)->state.getProperty("orbitKnobOverride",-1))==-1,"appearance Undo failed");h.redoProjectAction();acidFxCheck(int(acid(0)->state.getProperty("orbitAcidSkin"))==2 && int(acid(0)->state.getProperty("orbitKnobOverride"))==1 && int(acid(1)->state.getProperty("orbitAcidSkin",0))==0,"appearance Redo/instance separation failed");
        h.addInstrumentTrack(false);const auto c=h.selectedProjectTrackId();h.openGeneratorControls(c,false);h.showGeneratorProperties(c);acidFxCheck(!checkedOptions->choices.count("acidSkin") && checkedOptions->choices.count("knobStyle"),"generic generator choices incorrect");checkedOptions->choices.at("knobStyle")->setSelectedItemIndex(3,juce::dontSendNotification);acidFxCheck(checkedApply(*checkedOptions),"generic knob Apply failed");
        auto* plugin=h.generatorControlPlugin(c);auto* editor=h.generatorControlWindows.at(c).window->instrument.get();acidFxCheck(int(plugin->state.getProperty("orbitKnobOverride"))==2 && orbit::ui::effectiveKnobStyle(*editor)==2,"generic generator override not applied");
        const auto generic=juce::File("OPTIONS_CHECK_FOLDER/generic.orbitpreset");acidFxCheck(h.saveGeneratorPresetFile(c,generic),"generic appearance preset save failed");plugin->state.setProperty("orbitKnobOverride",1,nullptr);h.refreshGeneratorKnobStyle(c);acidFxCheck(h.loadGeneratorPresetFile(c,generic) && int(plugin->state.getProperty("orbitKnobOverride"))==2 && orbit::ui::effectiveKnobStyle(*editor)==2,"generic appearance preset recall failed");
        std::cout<<"PASS: per-instance Settings ownership, appearance Undo/Redo and generic native generator knob Apply/preset recall.\n";return 0;
    }
    if(argc>1)
    {
        acidFxCheck(orbit::ui::currentKnobStyle==2,"global knob style lost after restart");acidFxCheck(h.loadProjectFile(file),"skin project fresh reopen failed");
        acidFxCheck(acid(0) && int(acid(0)->state.getProperty("orbitAcidSkin"))==2 && int(acid(0)->state.getProperty("orbitKnobOverride"))==0 && acid(0)->parameters[45]->getCurrentValue()==1 && acid(0)->parameters[46]->getCurrentValue()==0,"per-instance skin/knob/LFO trigger state lost");
        std::cout<<"PASS: fresh global knob preference and independent generator appearance/LFO trigger state.\n";return 0;
    }
    h.setKnobStyle(0);h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);const auto id=h.selectedProjectTrackId();h.openGeneratorControls(id,false);
    auto* p=acid(0);auto* e=dynamic_cast<orbit::synth::AcidBassEditor*>(h.generatorControlWindows.at(id).window->instrument.get());acidFxCheck(p && e && p->parameters.size()==orbit::synth::AcidBassPlugin::parameterCount,"new parameter tail/editor missing");
    acidFxCheck(p->parameters[45]->getCurrentValue()==0 && p->parameters[46]->getCurrentValue()==0,"free-running not default");
    auto legacy=p->state.createCopy();legacy.removeProperty("lfo1Retrigger",nullptr);legacy.removeProperty("lfo2Retrigger",nullptr);legacy.removeProperty("orbitAcidSkin",nullptr);p->restorePluginStateFromValueTree(legacy);acidFxCheck(p->parameters[45]->getCurrentValue()==0 && int(p->state.getProperty("orbitAcidSkin"))==0,"old preset defaults changed");
    h.showGeneratorProperties(id);acidFxCheck(checkedOptions->choices.count("acidSkin") && checkedOptions->choices.count("knobStyle"),"settings lack appearance selectors");
    auto undo=h.undoHistory.size();checkedOptions->choices.at("acidSkin")->setSelectedItemIndex(1,juce::dontSendNotification);acidFxCheck(checkedApply(*checkedOptions),"dark skin Apply failed");acidFxCheck(h.undoHistory.size()==undo+1 && int(p->state.getProperty("orbitAcidSkin"))==1,"skin Apply not atomic");
    e->refreshAppearance();auto dark=knobSnapshot(*e);acidFxCheck(dark.getPixelAt(2,2)==juce::Colour(30,44,54) && e->knobs[0].findColour(juce::Label::textColourId).getBrightness()>.7f,"original dark palette/contrast missing");auto original=knobSnapshot(e->knobs[0]);writeKnobImage(*e,"acid-dark-original.png");
    p->state.setProperty("orbitAcidSkin",2,nullptr);e->refreshAppearance();auto white=knobSnapshot(e->knobs[0]);acidFxCheck(!sameKnobImage(original,white) && knobSnapshot(*e).getPixelAt(2,2)==juce::Colour(30,44,54),"dark white knob variant missing");e->setLfoExpanded(true);writeKnobImage(*e,"acid-dark-white.png");
    // Actual mixer EQ/send/pan and Master controls must use the switch, retain
    // values and restore their precise skin-default image in every palette.
    h.showMixer();h.mixerWindow->setBounds(0,90,500,950);
    for(int skin=0;skin<=4;++skin)
    {
        h.mixerWindow->setMixerSkin(skin);auto baseline=knobSnapshot(*h.mixerWindow);h.mixerWindow->handleMixerMenuResult(32);auto changed=knobSnapshot(*h.mixerWindow);acidFxCheck(!sameKnobImage(baseline,changed),"mixer white override not drawn");h.mixerWindow->handleMixerMenuResult(30);acidFxCheck(sameKnobImage(baseline,knobSnapshot(*h.mixerWindow)),"mixer default art did not restore");
    }
    h.setKnobStyle(2);h.showMixer(true);writeKnobImage(*h.mixerWindow,"master-white-knobs.png");h.showMixer();writeKnobImage(*h.mixerWindow,"mixer-white-knobs.png");
    for(int skin:{0,1,2,5,6,7,8}){h.setSkin(skin);acidFxCheck(orbit::ui::currentKnobStyle==2,"DAW skin changed knob preference");}
    // An instance override wins over global without changing either skin.
    p->state.setProperty("orbitKnobOverride",1,nullptr);e->refreshAppearance();acidFxCheck(orbit::ui::effectiveKnobStyle(e->knobs[0],2)==1,"generator override ignored");p->state.setProperty("orbitKnobOverride",0,nullptr);e->refreshAppearance();acidFxCheck(orbit::ui::effectiveKnobStyle(e->knobs[0],2)==2,"instance skin-default override ignored");
    e->lfoTrigger[0].setSelectedId(2,juce::sendNotificationSync);e->lfoTrigger[1].setSelectedId(1,juce::sendNotificationSync);acidFxCheck(p->parameters[45]->getCurrentValue()==1 && p->parameters[46]->getCurrentValue()==0,"route trigger selectors not independent");
    for(int row=0;row<2;++row)
    {
        auto dry=freeLfoAudio(*p,false,row,0,true,false,123),free=freeLfoAudio(*p,true,row,0,true,false,123),reset=freeLfoAudio(*p,true,row,0,true,true,123);
        for(double time:{4.2,7.2}){const double rate=row==0?.05:.1;acidFxCheck(std::abs(measuredLfoDb(free,dry,time)-(-24+36*rate*time))<.08,"free mode retriggered");acidFxCheck(std::abs(measuredLfoDb(reset,dry,time)-(-24+36*rate*(.2-123./48000)))<.08,"note retrigger not sample-aligned");}
    }
    // Appearance changes are persisted with the native preset/project, not DSP IDs.
    p->state.setProperty("orbitAcidSkin",2,nullptr);p->state.setProperty("orbitKnobOverride",0,nullptr);p->parameters[45]->setParameter(1,juce::sendNotification);p->parameters[46]->setParameter(0,juce::sendNotification);
    acidFxCheck(h.saveGeneratorPresetFile(id,preset),"appearance preset save failed");p->state.setProperty("orbitAcidSkin",0,nullptr);p->state.setProperty("orbitKnobOverride",1,nullptr);p->parameters[45]->setParameter(0,juce::sendNotification);acidFxCheck(h.loadGeneratorPresetFile(id,preset) && int(p->state.getProperty("orbitAcidSkin"))==2 && int(p->state.getProperty("orbitKnobOverride"))==0 && p->parameters[45]->getCurrentValue()==1,"skin/trigger preset recall failed");h.saveToFile(file);h.setKnobStyle(2);
    std::cout<<"PASS: three Acid skins/contrast, Generator Settings Apply, independent knob overrides, five mixer palettes and Master, exact default-art restoration, all seven DAW skins retain global choice, two sample-aligned LFO trigger modes, presets and project save.\n";return 0;
 }
 catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}
}
