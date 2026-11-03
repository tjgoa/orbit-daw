static void interactionCheck(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
static juce::MouseEvent interactionMouse(juce::Component* c,juce::Point<float> p,int count=1)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),p,juce::Time::getCurrentTime(),count,false};}
static void interactionImage(juce::Component& c,const char* name)
{juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));juce::PNGImageFormat().writeImageToStream(c.createComponentSnapshot(c.getLocalBounds(),true,2),out);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1680,1080);h.createProjectDiscardingCurrent();
    for(int i=0;i<3;++i)h.addDrumRackTrack();h.addInstrumentTrack(true);const auto acid=h.selectedProjectTrackId();
    for(int row=0;row<4;++row){const auto id=h.project.getTrackId(row);for(int slot:{0,9}){h.project.setPatternCreated(id,slot,true);if(row!=3)h.project.appendPatternNoteTicks(id,slot,48+row,slot,64,100);h.project.setPlaylistClip(id,h.project.getPatternId(id,slot),slot==0?0:2,1,true);}}
    h.refreshEnginePlaylist();auto* grid=h.playlistWindow->grid;grid->setViewportArea({0,0,1500,880});
    for(bool draw:{true,false})for(int row:{0,1,2,3,0,3,1})for(double bar:{0.,2.,6.})
    {
        h.showPlaylist();grid->setDrawMode(draw);const auto id=h.project.getTrackId(row);
        auto p=juce::Point<float>(float(grid->labelWidth()+(bar+.4)*grid->barWidth()),float(grid->headerHeight()+(row+.5)*grid->rowHeight()));
        auto first=interactionMouse(grid,p);grid->mouseDown(first);grid->mouseUp(first);
        if(h.hostedPianoRollTrackId.isNotEmpty())
        {auto& old=*h.generatorControlWindows.at(h.hostedPianoRollTrackId).window;interactionCheck(h.getIndexOfChildComponent(h.playlistWindow.get())>h.getIndexOfChildComponent(&old),"first Playlist click raised previous Piano Roll over second click");}
        const auto source=h.project.getPlaylistPatternId(id,bar);const int expected=source.isEmpty()?h.project.getTrackSelectedPatternIndex(id):h.project.getPatternIndexForId(id,source);
        auto twice=interactionMouse(grid,p,2);grid->mouseDown(twice);grid->mouseDoubleClick(twice);grid->mouseUp(twice);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(15);
        interactionCheck(h.hostedPianoRollTrackId==id && h.selectedProjectTrackId()==id && h.pianoEditingTrackId()==id && h.pianoEditingPatternIndex()==expected,"double click opened wrong generator or pattern");
        interactionCheck(h.generatorControlWindows.at(id).window->isPatternView(),"double click displayed instrument instead of Piano Roll");
    }
    // Painting one rack leaves all MIDI clip instances intact, including other
    // patterns and generators. Native note content updates before mouse-up.
    h.releaseHostedPianoRoll();const auto drumID=h.project.getTrackId(0);h.selectGeneratorControlTrack(drumID);h.selectGeneratorControlPattern(drumID,0);h.openGeneratorControls(drumID,false);
    auto* rack=dynamic_cast<DrumRackWindow*>(h.generatorControlWindows.at(drumID).window->instrument.get());interactionCheck(rack!=nullptr,"drum controls absent");
    auto* steps=&rack->content.stepGrid;steps->setSize(640,640);h.project.setInitialTempoBpm(120);h.setPlaybackMode(ProjectView::PlaybackMode::song);h.refreshEnginePlaylist();
    std::vector<std::vector<juce::String>> before;for(auto* track:h.getGeneratorTracks()){std::vector<juce::String> ids;for(auto* clip:track->getClips())ids.push_back(clip->itemID.toString());before.push_back(ids);}
    const auto undo=h.undoHistory.size();const auto start=juce::Time::getMillisecondCounterHiRes();
    auto down=interactionMouse(steps,{20,220});steps->mouseDown(down);for(int step=1;step<16;++step)steps->mouseDrag(interactionMouse(steps,{float(step*40+20),220}));steps->mouseUp(down);
    std::cout<<"16-cell drum paint: "<<(juce::Time::getMillisecondCounterHiRes()-start)<<" ms\n";
    interactionCheck(h.undoHistory.size()==undo+1,"drum paint did not remain one Undo");
    for(int row=0;row<h.getGeneratorTracks().size();++row){std::vector<juce::String> ids;for(auto* clip:h.getGeneratorTracks()[row]->getClips())ids.push_back(clip->itemID.toString());interactionCheck(ids==before[size_t(row)],"drum painting rebuilt/replaced unrelated engine clips");}
    interactionCheck(h.project.getPatternNotes(drumID,0).size()>=15,"drum drag lost painted cells");
    h.undoProjectAction();h.redoProjectAction();interactionCheck(h.project.getPatternNotes(drumID,0).size()>=15,"drum drag failed Undo/Redo");
    h.openGeneratorControls(acid,false);auto* editor=dynamic_cast<orbit::synth::AcidBassEditor*>(h.generatorControlWindows.at(acid).window->instrument.get());interactionCheck(editor!=nullptr,"Acid editor absent");
    h.setKnobStyle(2);editor->refreshAppearance();editor->setSize(1040,190);interactionImage(*editor,"acid-labels.png");
    auto* label=dynamic_cast<juce::Label*>(editor->getChildComponent(0));interactionCheck(label!=nullptr && label->getY()>=30,"white Acid label was not moved nearer knob");
    h.setKnobStyle(1);editor->refreshAppearance();interactionCheck(label->getY()<30,"closer white label overlaps original knob style");h.setKnobStyle(2);
    h.showMixer();auto& mixer=*h.mixerWindow;mixer.setBounds(0,0,560,880);mixer.setMixerSkin(3);mixer.setKnobSize(400);mixer.resized();interactionImage(mixer,"deep-mixer-shadows.png");
    auto* strip=mixer.channelStrips[0];auto& knob=strip->eqControls[0][0];juce::Image shadow(juce::Image::ARGB,strip->getWidth(),strip->getHeight(),true);{juce::Graphics g(shadow);orbit::ui::drawMixerParentShadow(g,knob);}
    int outside=0;for(int y=0;y<shadow.getHeight();++y)for(int x=0;x<shadow.getWidth();++x)if(!knob.getBounds().contains(x,y) && shadow.getPixelAt(x,y).getAlpha()>0)++outside;
    interactionCheck(outside>0,"deep shadow still clipped to slider canvas");mixer.setMixerSkin(0);juce::Image dark(juce::Image::ARGB,strip->getWidth(),strip->getHeight(),true);{juce::Graphics g(dark);orbit::ui::drawMixerParentShadow(g,knob);}for(int y=0;y<dark.getHeight();++y)for(int x=0;x<dark.getWidth();++x)interactionCheck(dark.getPixelAt(x,y).getAlpha()==0,"deep shadow leaked onto dark palette");
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/interaction.orbit");h.saveToFile(file);interactionCheck(h.loadProjectFile(file),"edited project reopen failed");
    std::cout<<"PASS: 42 two-stage Playlist double clicks across Drum/Acid and empty cells, first-click stacking, pinned pattern ownership, in-place native drum clips/one Undo, Acid label styles, parent shadows beyond controls and dark isolation, native save/reopen.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
