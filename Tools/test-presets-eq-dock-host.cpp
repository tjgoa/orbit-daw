#define ORBIT_FREE_MAIN acidFreeLfoRegressionMain
#include "test-acid-free-lfo-host.cpp"
#undef ORBIT_FREE_MAIN
static void nextCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void nextPump(){juce::MessageManager::getInstance()->runDispatchLoopUntil(60);}
static void nextImage(juce::Component& c,const char* name){juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));juce::PNGImageFormat().writeImageToStream(c.createComponentSnapshot(c.getLocalBounds()),out);}
static juce::MouseEvent nextMouse(juce::Component* c,juce::Point<float> p,int clicks=1)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),p,juce::Time::getCurrentTime(),clicks,false};}
static double eqTone(double rate,float freq,orbit::synth::AcidBassEq::Controls controls)
{
 orbit::synth::AcidBassEq eq;eq.prepare(rate);eq.setControls(controls);juce::AudioBuffer<float> b(2,256);double energy=0;int measured=0;
 for(int block=0;block<200;++block){for(int i=0;i<256;++i){float x=float(.1*std::sin(juce::MathConstants<double>::twoPi*freq*(block*256+i)/rate));b.setSample(0,i,x);b.setSample(1,i,-x);}eq.process(b,0,256);for(int i=0;i<256;++i){nextCheck(std::isfinite(b.getSample(0,i)) && std::abs(b.getSample(0,i))<100,"EQ instability");nextCheck(std::abs(b.getSample(0,i)+b.getSample(1,i))<1.e-6,"EQ leaked stereo state");if(block>100){energy+=b.getSample(0,i)*b.getSample(0,i);++measured;}}}
 return 10*std::log10(energy/measured/.005);
}
int main(int argc,char** argv)
{try{
 if(argc>1 && juce::String(argv[1])=="--free")return acidFreeLfoRegressionMain(1,nullptr);
 if(argc>1 && juce::String(argv[1])=="--rack")return acidRackRegressionMain(1,nullptr);
 juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1680,1150);h.saveOption("presetLibraryRoot",juce::String("OPTIONS_CHECK_FOLDER/library"));
 struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Orbit native panel verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}}shell;
 shell.setContentNonOwned(&h,false);shell.setSize(1680,1180);shell.setVisible(true);h.setSize(1680,1150);
 h.headerArtwork.setFolder(juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork"));
 const juce::File projectFile("OPTIONS_CHECK_FOLDER/next.orbit");
 if(argc>1){nextCheck(h.loadProjectFile(projectFile),"fresh project reopen failed");const auto id=h.project.getTrackId(0);auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(id));nextCheck(p && p->parameters[47]->getCurrentValue()==1 && p->parameters[51]->getCurrentValue()==6,"fresh EQ state lost");nextCheck(h.generatorPresetChoices(id).contains("My Acid"),"fresh preset library unavailable");nextCheck(h.mixerWindow->knobSizePercent==250,"mixer size not remembered");std::cout<<"PASS fresh project EQ, preset library and mixer knob size.\n";return 0;}
 // Enabled/flat and default bypass, shelves and parametric response at 3 rates.
 for(double rate:{44100.,48000.,96000.}){
  orbit::synth::AcidBassEq eq;eq.prepare(rate);eq.setControls({});juce::AudioBuffer<float> b(2,1024);for(int c=0;c<2;++c)for(int i=0;i<1024;++i)b.setSample(c,i,float(std::sin(i*.17+c)));
  juce::AudioBuffer<float> before;before.makeCopyOf(b);eq.process(b,17,777);for(int c=0;c<2;++c)for(int i=0;i<1024;++i)nextCheck(b.getSample(c,i)==before.getSample(c,i),"default EQ not bit exact bypass");
  orbit::synth::AcidBassEq::Controls control;control.enabled=1;nextCheck(std::abs(eqTone(rate,1000,control))<.02,"enabled flat EQ not unity");control.midGain=6;nextCheck(std::abs(eqTone(rate,1000,control)-6)<.03,"mid EQ gain response wrong");control.midGain=0;control.lowGain=9;nextCheck(eqTone(rate,30,control)>8.5,"low shelf absent");control.lowGain=0;control.highGain=-9;nextCheck(eqTone(rate,float(rate*.4),control)<-8.5,"high shelf absent");
 }
 h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);const auto a=h.selectedProjectTrackId();h.addInstrumentTrack(true);const auto b=h.selectedProjectTrackId();h.openGeneratorControls(a,false);
 auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(a));auto* e=dynamic_cast<orbit::synth::AcidBassEditor*>(h.generatorControlWindows.at(a).window->instrument.get());nextCheck(e && p && p->parameters.size()==56,"EQ editor/parameter layout absent");
 e->setEqExpanded(true);nextCheck(e->preferredHeight()==330 && e->eqKnobs[0].isVisible(),"independent EQ foldout layout wrong");e->eqEnabled.setToggleState(true,juce::dontSendNotification);e->eqEnabled.onClick();e->eqKnobs[3].setValue(6,juce::sendNotificationSync);nextCheck(p->parameters[47]->getCurrentValue()==1 && p->parameters[51]->getCurrentValue()==6,"EQ UI binding wrong");
 const auto folder=h.generatorPresetFolder(a,true),preset=folder.getChildFile("My Acid.orbitpreset");nextCheck(h.saveGeneratorPresetFile(a,preset),"library save failed");nextCheck(h.generatorPresetChoices(a).contains("My Acid"),"saved preset missing in selector");p->parameters[51]->setParameter(-6,juce::dontSendNotification);h.chooseGeneratorPreset(a,0);nextCheck(p->parameters[51]->getCurrentValue()==6,"library selector failed to restore EQ");nextCheck(h.suggestedGeneratorPresetFile(a,true)==preset && h.suggestedGeneratorPresetFile(a,false)==folder,"preset chooser location/name wrong");nextImage(*e,"acid-eq.png");
 p->parameters[47]->setParameter(0,juce::dontSendNotification);auto dry=isolatedModulation(h,*p,0);p->parameters[47]->setParameter(1,juce::dontSendNotification);auto wet=isolatedModulation(h,*p,0);nextCheck(audioDifference(dry,wet)>1,"internal EQ not connected to synth audio");
 // Cross-generator, same generator, different bank, repeated and resumed piano view.
 for(int row=0;row<2;++row)for(int slot:{0,9,42}){const auto id=h.project.getTrackId(row);h.project.setPatternCreated(id,slot,true);h.project.setPatternNote(id,slot,36+slot%12,0,true);h.project.setPlaylistClip(id,h.project.getPatternId(id,slot),slot==0?0:slot==9?2:4,2,true);}
 h.refreshEnginePlaylist();h.showPlaylist();auto* grid=h.playlistWindow->grid;grid->setViewportArea({0,0,1500,900});
 for(bool draw:{true,false})for(int row:{0,1,0})for(int slot:{9,42,0,9,9}){
  grid->setDrawMode(draw);const double bar=slot==0?0:slot==9?2:4;
  auto point=juce::Point<float>(float(grid->labelWidth()+(bar+.4)*grid->barWidth()),float(grid->headerHeight()+(row+.5)*grid->rowHeight()));auto down=nextMouse(grid,point);grid->mouseDown(down);grid->mouseUp(down);auto twice=nextMouse(grid,point,2);grid->mouseDown(twice);grid->mouseDoubleClick(twice);grid->mouseUp(twice);
  const auto id=h.project.getTrackId(row);nextCheck(h.hostedPianoRollTrackId==id && h.pianoEditingPatternIndex()==slot && h.project.getTrackSelectedPatternIndex(id)==slot,"double click opened wrong pattern");h.showPlaylist();
 }
 for(int skin:{0,1,2,5,6,7,8}){h.setSkin(skin);h.showMixer();h.playlistButton.onClick();nextCheck(h.playlistWindow->isVisible() && h.getIndexOfChildComponent(h.playlistWindow.get())>h.getIndexOfChildComponent(h.mixerWindow.get()),"Playlist click did not bring forward");h.playlistButton.onClick();nextCheck(h.playlistWindow->isVisible(),"Playlist click hid panel");h.mixerButton.onClick();nextCheck(h.mixerWindow->isVisible() && h.getIndexOfChildComponent(h.mixerWindow.get())>h.getIndexOfChildComponent(h.playlistWindow.get()),"Mixer click did not bring forward");h.mixerButton.onClick();nextCheck(h.mixerWindow->isVisible(),"Mixer click hid panel");}
 h.setSkin(7);h.setKnobStyle(2);h.showMixer();h.mixerWindow->setBounds(0,82,1000,1000);
 const auto panelBounds=h.mixerWindow->getBounds(),stripBounds=h.mixerWindow->channelStrips[0]->getBounds();
 for(int size:{100,150,200,250,300}){h.mixerWindow->handleMixerMenuResult(1000+size);auto* strip=h.mixerWindow->channelStrips[0];nextCheck(h.mixerWindow->getBounds()==panelBounds && strip->getBounds()==stripBounds,"size option altered Mixer geometry");nextCheck(strip->eqControls[1][0].getWidth()<strip->eqControls[0][0].getWidth(),"HM three knobs not smaller");for(int band=0;band<4;++band){const int n=band==1?3:2;for(int k=1;k<n;++k)nextCheck(!strip->eqControls[band][k-1].getBounds().intersects(strip->eqControls[band][k].getBounds()),"enlarged knobs overlap");}if(size==200)nextImage(*h.mixerWindow,"mixer-knobs-200.png");}
 h.mixerWindow->handleMixerMenuResult(1250);h.showMixer(true);nextImage(*h.mixerWindow,"master-knobs-250.png");
 // Minimized documents have no duplicate title bar; each restores from a name tab.
 h.showPlaylist();if(!h.playlistWindow->isMaximized())h.togglePlaylistMaximized();if(!h.mixerWindow->isMaximized())h.toggleMixerMaximized();h.openGeneratorPianoRoll(a);h.closeMinimizeAllWindows();nextPump();
 nextCheck(!h.playlistWindow->isVisible() && !h.mixerWindow->isVisible(),"Playlist/Mixer mini frame still visible");for(auto& [id,entry]:h.generatorControlWindows)nextCheck(!entry.window->isVisible() && entry.window->isMinimised(),"generator mini frame still visible");nextImage(h,"name-only-dock.png");
 auto dock=[&](const juce::String& text){for(auto& button:h.generatorDockButtons)if(button->getButtonText()==text){auto action=button->onClick;action();nextPump();return;}throw std::runtime_error("missing dock tab");};
 dock("Playlist");nextCheck(h.playlistWindow->isVisible() && !h.playlistWindow->isMinimized() && h.playlistWindow->getBounds()==h.maximizedPlaylistBounds(),"Playlist dock restore lost full edges");dock("Master");nextCheck(h.mixerWindow->isVisible() && !h.mixerWindow->isMinimized() && h.mixerWindow->getBounds()==h.maximizedPlaylistBounds(),"Master dock restore failed");dock(h.project.getTrackName(0));nextCheck(h.generatorControlWindows.at(a).window->isVisible() && !h.generatorControlWindows.at(a).window->isMinimised(),"generator name tab restore failed");
 juce::Image brand(juce::Image::ARGB,140,40,true);{juce::Graphics g(brand);orbit::options::header::brand(g,{0,0,140,40},2);}bool light=false;for(int y=0;y<40;++y)for(int x=0;x<140;++x){auto c=brand.getPixelAt(x,y);light|=c.getAlpha()>200 && c.getBrightness()>.8f;}nextCheck(light,"Metallic ORBIT still dark");
 h.setHeaderStyle(2);h.showPlaylist();nextImage(h,"metallic-header.png");h.saveToFile(projectFile);
 h.closeMinimizeAllWindows();h.createProjectDiscardingCurrent();nextPump();std::cerr<<"New Project visible playlist="<<h.playlistWindow->isVisible()<<" mixer="<<h.mixerWindow->isVisible()<<" dock="<<h.generatorDockButtons.size()<<" tracks="<<h.project.getTrackCount()<<" effects="<<h.mixerWindow->openEffectWindows().size()<<"\n";for(auto& button:h.generatorDockButtons)std::cerr<<"Dock: "<<button->getButtonText()<<"\n";nextCheck(!h.playlistWindow->isVisible() && !h.mixerWindow->isVisible() && h.generatorDockButtons.empty(),"New Project revived minimized documents");
 std::cout<<"PASS EQ bypass/response at three rates, native EQ controls/library/preset/project, 30 actual double-click sequences across tracks/banks/tools, all skin one-click views, five knob sizes/no overlap, hidden mini frames/name dock restoration/max edges, Metallic brand contrast.\n";return 0;
 }catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}}
