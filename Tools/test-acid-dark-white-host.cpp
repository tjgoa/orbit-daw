static void darkCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool darkSame(const juce::Image& a,const juce::Image& b){if(a.getBounds()!=b.getBounds())return false;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)if(a.getPixelAt(x,y)!=b.getPixelAt(x,y))return false;return true;}
static juce::Image darkKnob(orbit::ui::AcidSilverLookAndFeel& look,juce::Slider& slider,bool old=false)
{juce::Image im(juce::Image::ARGB,140,140,true);juce::Graphics g(im);g.fillAll(look.skin==0?juce::Colour(230,234,240):juce::Colour(30,44,54));if(old)orbit::ui::drawSelectedKnob(g,10,10,120,120,.4f,3.9f,8.6f,slider,look.skin==1?1:2);else look.drawRotarySlider(g,10,10,120,120,.4f,3.9f,8.6f,slider);return im;}
int main(){try{
 juce::ScopedJuceInitialiser_GUI gui;orbit::ui::currentKnobStyle=0;orbit::ui::AcidSilverLookAndFeel look;juce::Slider slider;slider.setLookAndFeel(&look);
 look.skin=0;look.setDark(false);const auto silver=darkKnob(look,slider);darkCheck(darkSame(silver,darkKnob(look,slider,true)),"Silver white artwork changed");
 look.skin=1;look.setDark(true);const auto original=darkKnob(look,slider);darkCheck(darkSame(original,darkKnob(look,slider,true)),"Dark Original artwork changed");
 look.skin=2;auto before=darkKnob(look,slider,true),after=darkKnob(look,slider);darkCheck(!darkSame(before,after),"Dark White correction absent");
 double beforeHaze=0,afterHaze=0;const auto bg=juce::Colour(30,44,54);int whitePixels=0;
 for(int y=0;y<140;++y)for(int x=0;x<140;++x){const double radius=std::hypot(x-70.,y-70.);if(radius>40 && radius<57){beforeHaze+=std::abs(before.getPixelAt(x,y).getBrightness()-bg.getBrightness());afterHaze+=std::abs(after.getPixelAt(x,y).getBrightness()-bg.getBrightness());}if(radius<30 && after.getPixelAt(x,y).getBrightness()>.8f)++whitePixels;}
 darkCheck(afterHaze<beforeHaze*.1 && whitePixels>200,"outer haze remains or white face removed");
 slider.getProperties().set("orbitKnobOverride",1);darkCheck(darkSame(darkKnob(look,slider),darkKnob(look,slider,true)),"Original override changed");slider.getProperties().set("orbitKnobOverride",-1);
 look.skin=0;look.setDark(false);darkCheck(darkSame(silver,darkKnob(look,slider)),"Dark renderer contaminated silver/shared style");
 look.skin=1;look.setDark(true);darkCheck(darkSame(original,darkKnob(look,slider)),"Dark Original contaminated");slider.setLookAndFeel(nullptr);
 ProjectView h;h.stopTimer();h.setSize(1440,1000);h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);const auto id=h.selectedProjectTrackId();h.openGeneratorControls(id,false);auto* plugin=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(id));auto* editor=dynamic_cast<orbit::synth::AcidBassEditor*>(h.generatorControlWindows.at(id).window->instrument.get());darkCheck(plugin && editor,"Acid editor missing");plugin->state.setProperty("orbitAcidSkin",2,nullptr);editor->refreshAppearance();
 {juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/dark-white-clean.png"));juce::PNGImageFormat().writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),out);}
 for(auto action:{0,1,2}){if(action==0){editor->setAdvancedExpanded(true);editor->setAdvancedExpanded(false);}else if(action==1){editor->setEffectsExpanded(true);editor->setEffectsExpanded(false);}else{editor->setLfoExpanded(true);editor->setLfoExpanded(false);}darkCheck(editor->preferredHeight()==190,"foldout did not close independently");}
 h.setKnobStyle(2);h.addInstrumentTrack(true);h.showMixer();
 for(bool master:{false,true})for(int palette=0;palette<5;++palette)
 {
  h.showMixer(master);h.mixerWindow->setMixerSkin(palette);auto& mixer=*h.mixerWindow;auto* strip=master?&mixer.masterStrip:mixer.channelStrips[0];
  const auto panel=mixer.getBounds(),strips=mixer.strips.getBounds(),stripBounds=strip->getBounds(),graph=strip->eqGraph.getBounds(),fader=strip->gain.getBounds();
  const auto knobBounds=strip->eqControls[0][0].getBounds(),sendBounds=strip->sends[0].getBounds();
  juce::Image small,large;
  for(int size:{100,125,150,175,200,250,300,400,500})
  {
   mixer.handleMixerMenuResult(1000+size);
   darkCheck(mixer.getBounds()==panel && mixer.strips.getBounds()==strips && strip->getBounds()==stripBounds && strip->eqGraph.getBounds()==graph && strip->gain.getBounds()==fader,"knob size moved/resized mixer layout");
   darkCheck(strip->eqControls[0][0].getBounds()==knobBounds && strip->sends[0].getBounds()==sendBounds && mixer.stripStep()==132 && mixer.strips.getHeight()==mixer.viewport.getHeight(),"knob size changed cells/strip width or added vertical overflow");
   for(int band=0;band<4;++band){int count=band==1?3:2;for(int k=1;k<count;++k)darkCheck(!strip->eqControls[size_t(band)][size_t(k-1)].getBounds().intersects(strip->eqControls[size_t(band)][size_t(k)].getBounds()),"fixed knob cells overlap");}
   auto image=strip->createComponentSnapshot(strip->getLocalBounds());if(size==100)small=image;if(size==500)large=image;
  }
  darkCheck(!darkSame(small,large),"knob size changed no visible pixels");
  if(!master && palette==1){juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/mixer-navy-clean.png"));juce::PNGImageFormat().writeImageToStream(mixer.createComponentSnapshot(mixer.getLocalBounds()),out);}
  if(!master && palette==3){juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/mixer-silver-clean.png"));juce::PNGImageFormat().writeImageToStream(mixer.createComponentSnapshot(mixer.getLocalBounds()),out);}
 }
 std::cout<<"PASS nine mixer sizes across five palettes and Mixer/Master: exact panel/strip/graph/fader/cell geometry, no vertical overflow, visible size changes and no knob overlap.\n";
 std::cout<<"PASS Dark/White halo reduced from "<<beforeHaze<<" to "<<afterHaze<<"; white face retained; exact Silver and Dark/Original/reference renderer/override preservation; actual dark editor and independent Advanced/FX/LFO close.\n";return 0;
 }catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}}
