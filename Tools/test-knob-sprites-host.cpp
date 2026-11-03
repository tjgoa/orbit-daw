static void spriteCheck(bool good,const char* message){if(!good)throw std::runtime_error(message);}
static void waitSprite(orbit::mixing::SpriteBrowser& browser)
{const auto end=juce::Time::getMillisecondCounterHiRes()+15000;while(browser.loading && juce::Time::getMillisecondCounterHiRes()<end){juce::Thread::sleep(5);browser.poll();}spriteCheck(!browser.loading,"sprite worker timed out");}
static void spriteImage(juce::Component& c,const char* name)
{auto im=c.createComponentSnapshot(c.getLocalBounds(),true,2);juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name));juce::PNGImageFormat().writeImageToStream(im,out);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;
    auto root=juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Projects/SOURCE CODE MIT/Sprites");
    const auto catalog=orbit::ui::sprites::scan(root);
    spriteCheck(catalog.error.isEmpty() && catalog.entries.size()==156 && catalog.skipped==11,"real export catalogue count mismatch");
    spriteCheck(orbit::ui::sprites::scan(root.getChildFile("sprites")).entries.size()==catalog.entries.size(),"nested sprite folder not accepted");
    for(const auto& entry:catalog.entries)
    {
        juce::String error;auto s=orbit::ui::sprites::load(entry,error);spriteCheck(s!=nullptr && error.isEmpty(),"real exported PNG failed native decode");
        spriteCheck(s->frameIndex(0)==0 && s->frameIndex(1)==entry.frames-1 && s->frameIndex(-1)==0 && s->frameIndex(2)==entry.frames-1,"frame endpoints/clamping failed");
        spriteCheck(s->image.getBounds().contains(s->frameBounds(1)),"last frame outside image");
    }
    // Explicit horizontal strip, PNG header mismatch and traversal rejection.
    const auto fixtures=juce::File("OPTIONS_CHECK_FOLDER/fixture");fixtures.getChildFile("sprites").createDirectory();
    auto makeRecord=[](const char* id,const char* path,const char* orientation){auto* o=new juce::DynamicObject();o->setProperty("id",id);o->setProperty("source","fixture.knob");o->setProperty("status","ok");o->setProperty("sprite",path);o->setProperty("frame_width",12);o->setProperty("frame_height",8);o->setProperty("frames",3);o->setProperty("orientation",orientation);return juce::var(o);};
    juce::Image horizontal(juce::Image::ARGB,36,8,true);{juce::Graphics g(horizontal);g.setColour(juce::Colours::red);g.fillRect(0,0,12,8);g.setColour(juce::Colours::green);g.fillRect(12,0,12,8);g.setColour(juce::Colours::blue);g.fillRect(24,0,12,8);}
    {juce::FileOutputStream out(fixtures.getChildFile("sprites/test.png"));juce::PNGImageFormat().writeImageToStream(horizontal,out);}
    juce::Array<juce::var> records;records.add(makeRecord("valid","sprites/test.png","horizontal"));records.add(makeRecord("badpath","sprites/../../else.png","horizontal"));records.add(makeRecord("badmeta","sprites/test.png","diagonal"));
    auto* manifest=new juce::DynamicObject();manifest->setProperty("format","orbit-sprite-manifest-v1");manifest->setProperty("knobs",records);fixtures.getChildFile("manifest.json").replaceWithText(juce::JSON::toString(juce::var(manifest)));
    auto c=orbit::ui::sprites::scan(fixtures);spriteCheck(c.entries.size()==1 && c.skipped==2,"invalid manifest entries accepted");juce::String error;auto horizontalSprite=orbit::ui::sprites::load(c.entries[0],error);spriteCheck(horizontalSprite!=nullptr,"horizontal sprite load failed");
    for(int f=0;f<3;++f){juce::Image frame(juce::Image::ARGB,12,8,true);juce::Graphics g(frame);horizontalSprite->draw(g,{0,0,12,8},f/2.f);spriteCheck(frame.getPixelAt(6,4)==horizontal.getPixelAt(f*12+6,4),"horizontal frame rendering mismatch");}
    auto invalid=c.entries[0];invalid.frames=4;spriteCheck(orbit::ui::sprites::load(invalid,error)==nullptr,"mismatched PNG dimensions accepted");
    ProjectView host;host.stopTimer();host.setSize(1440,1000);host.createProjectDiscardingCurrent();for(int i=0;i<3;++i)host.addInstrumentTrack(true);host.showMixer(true);host.showMixer();
    auto& mixer=*host.mixerWindow;host.setKnobStyle(1);mixer.setMixerSkin(5);mixer.setBounds(0,82,660,880);mixer.resized();
    const auto bounds=mixer.getBounds();const auto layout=mixer.layoutSettings;const auto undo=host.undoHistory.size();const auto gain=host.getTrackGainDb(0),pan=host.getTrackPan(0);
    auto* strip=mixer.channelStrips[0];const auto control=strip->eqControls[0][0].getBounds();
    mixer.handleMixerMenuResult(41);spriteCheck(mixer.spritePanel!=nullptr && mixer.spriteWindow->isVisible(),"sprite menu did not open panel");waitSprite(mixer.spriteBrowser);
    auto& browser=mixer.spriteBrowser;browser.open(root);waitSprite(browser);spriteCheck(browser.catalogue.entries.size()==156,"live browser catalogue not populated");
    browser.select(catalog.entries.front());browser.select(catalog.entries.back());waitSprite(browser);
    spriteCheck(browser.active && browser.active->entry.id==catalog.entries.back().id,"rapid switching used a stale decode result");
    spriteCheck(orbit::ui::sprites::forComponent(strip->eqControls[0][0])==browser.active.get(),"EQ did not inherit selected sprite");spriteCheck(orbit::ui::sprites::forComponent(strip->sends[0])==browser.active.get(),"send knob did not inherit sprite");spriteCheck(orbit::ui::sprites::forComponent(strip->pan)==browser.active.get(),"pan did not inherit sprite");
    juce::Slider independent;spriteCheck(orbit::ui::sprites::forComponent(independent)==nullptr,"custom sprite leaked to non-mixer controls");
    spriteCheck(host.optionString("mixerSpriteID")==browser.active->entry.id && host.optionString("mixerSpriteFolder")==root.getFullPathName(),"sprite preference not saved");
    const auto chosen=browser.active;
    for(int skin=0;skin<=6;++skin){mixer.setMixerSkin(skin);spriteCheck(mixer.mixerSprite()==chosen.get(),"skin change lost explicit sprite");}
    mixer.setMixerSkin(5);spriteImage(*mixer.spritePanel,"sprite-panel.png");spriteImage(mixer,"sprite-mixer.png");
    mixer.showLayoutPanel();mixer.layoutPanel->sliders[4].setValue(500,juce::sendNotificationSync);mixer.layoutPanel->commit();spriteCheck(mixer.getBounds()==bounds && strip->eqControls[0][0].getBounds()==control,"sprite size resized mixer or control cells");
    host.showMixer(true);spriteCheck(orbit::ui::sprites::forComponent(mixer.masterStrip.pan)==chosen.get(),"Master bank did not inherit sprite");spriteImage(mixer,"sprite-master.png");host.showMixer();
    browser.select(invalid);waitSprite(browser);spriteCheck(browser.active==chosen && browser.status.contains("dimensions"),"bad sprite replaced valid selection");
    browser.open(fixtures.getChildFile("missing"));waitSprite(browser);spriteCheck(browser.active==chosen && browser.catalogue.entries.size()==156,"bad library destroyed current selection/library");
    {MixerWindow restored;restored.restoreSpritePreference(root,chosen->entry.id);waitSprite(restored.spriteBrowser);spriteCheck(restored.mixerSprite()!=nullptr && restored.mixerSprite()->entry.id==chosen->entry.id,"saved sprite did not restore without opening browser");}
    {MixerWindow missing;missing.restoreSpritePreference(fixtures.getChildFile("missing"),"gone");waitSprite(missing.spriteBrowser);spriteCheck(missing.mixerSprite()==nullptr,"missing saved library failed safe fallback");}
    mixer.showSpritePanel();auto* panel=mixer.spritePanel;panel->filter.setText("2000sMasterVolDial",false);panel->filter.onTextChange();spriteCheck(panel->getNumRows()==1,"search failed");panel->list.selectRow(0);waitSprite(browser);spriteCheck(browser.active->entry.name=="2000sMasterVolDial","search selection did not apply");
    panel->filter.setText({},false);panel->filter.onTextChange();panel->next.onClick();waitSprite(browser);const auto next=browser.active->entry.id;panel->previous.onClick();waitSprite(browser);spriteCheck(browser.active->entry.id!=next,"previous/next did not switch sprites");
    mixer.setMinimized(true);spriteCheck(!mixer.spriteWindow->isVisible(),"minimized mixer left sprite panel open");mixer.setMinimized(false);mixer.showSpritePanel();mixer.setVisible(false);spriteCheck(!mixer.spriteWindow->isVisible(),"hidden mixer left sprite panel open");mixer.setVisible(true);
    browser.useDefault();spriteCheck(mixer.mixerSprite()==nullptr && host.optionString("mixerSpriteID").isEmpty(),"normal knob fallback did not clear saved override");
    mixer.setLayout(layout);if(mixer.onLayoutSaved)mixer.onLayoutSaved(layout);
    // Host bank switching restores its own stored window bounds; independent knob sizing was checked before switching.
    spriteCheck(host.undoHistory.size()==undo && host.getTrackGainDb(0)==gain && host.getTrackPan(0)==pan,"sprite exploration changed audio parameters or Undo");
    {orbit::mixing::SpriteBrowser closing;closing.open(root);closing.select(catalog.entries.front());} // outstanding decode at destruction
    std::cout<<"PASS: all 156 native PNG decodes; vertical/horizontal endpoints; metadata/path limits; stale selection cancellation; live menu/search/previous/next; Mixer/Master and all seven skins; independent 500% sizing; restore/missing assets/default fallback; minimize/close cleanup; audio values and Undo unchanged.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
