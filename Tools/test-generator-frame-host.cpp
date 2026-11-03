static void frameCheck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void frameImage(juce::Component& c,const char* file)
{auto image=c.createComponentSnapshot(c.getLocalBounds());juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+file));juce::PNGImageFormat().writeImageToStream(image,out);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Orbit frame verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
    shell.setContentNonOwned(&host,false);shell.setSize(1440,1030);shell.setVisible(true);
    frameCheck(!host.playlistWindow->isVisible() && !host.mixerWindow->isVisible() && !host.generatorSurface.isVisible() && !host.generatorDockViewport.isVisible(),"startup exposed a document window");
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto acid=host.selectedProjectTrackId();host.addDrumRackTrack();const auto drums=host.selectedProjectTrackId();
    auto window=[&](const auto& id)->orbit::generator::ControlWindow&{return *host.generatorControlWindows.at(id).window;};
    window(acid).setBounds(70,110,900,340);window(drums).setBounds(180,200,1100,700);const auto instrumentBounds=window(acid).getBounds();
    juce::Image chrome(juce::Image::RGB,900,340,true);juce::Graphics chromeGraphics(chrome);
    window(acid).getContentComponent()->paint(chromeGraphics);
    frameCheck(chrome.getPixelAt(500,10)==orbit::options::colour(22,32,42) && chrome.getPixelAt(500,80)==orbit::options::colour(45,65,77),"navy leaked from toolbar into generator backing");
    // Built-in frames retain their size beyond all three free workspace edges.
    for(const auto& id:{acid,drums})
    {
        auto& w=window(id);const auto original=w.getBounds();
        for(const juce::Point<int> position:std::array<juce::Point<int>,3>{{{-200,130},{1300,160},{100,900}}})
        {
            w.setTopLeftPosition(position);host.resized();
            frameCheck(w.getPosition()==position && w.getWidth()==original.getWidth() && w.getHeight()==original.getHeight(),"floating generator was boxed in or shrunk");
            w.minimiseButtonPressed();frameCheck(!w.instrument->isShowing(),"overflow minimize leaked generator body");
            w.showControls(false);frameCheck(w.getPosition()==position,"restore lost off-edge generator position");
        }
        w.setTopLeftPosition(-10000,10000);const auto area=w.workspaceArea();
        frameCheck(w.getRight()==area.getX()+64 && w.getY()==area.getBottom()-24,"overflow lost reachable title bar");
        w.setBounds(original);w.maximiseButtonPressed();frameCheck(w.getBounds()==w.workspaceArea(),"maximize left workspace padding");
        w.maximiseButtonPressed();frameCheck(w.getBounds()==original,"maximize restore changed floating size");
    }
    host.showPlaylist();host.playlistPanelBounds={60,140,1000,600};host.resized();const auto playlistFloating=host.playlistWindow->getBounds();
    for(int pass=0;pass<3;++pass)
    {
        host.togglePlaylistMaximized();frameCheck(host.playlistWindow->getBounds()==host.maximizedPlaylistBounds(),"Playlist maximize left padding");
        frameCheck(host.playlistWindow->maximizeButton.getTooltip()=="Restore Playlist","maximized Playlist uses wrong control");
        host.playlistWindow->minimizeButton.onClick();host.resized();
        frameCheck(host.playlistWindow->isMinimized() && host.playlistWindow->getWidth()==220 && host.playlistWindow->getBottom()==host.generatorDockViewport.getY() && !host.playlistWindow->surface->isShowing() && !host.playlistWindow->minimizeButton.isVisible(),"Playlist minimize left full-width header at top");
        frameImage(host,"playlist-minimized.png");host.playlistWindow->maximizeButton.onClick();
        frameCheck(!host.playlistWindow->isMinimized() && host.playlistWindow->isMaximized() && host.playlistWindow->getBounds()==host.maximizedPlaylistBounds(),"Playlist restore lost maximized state");
        host.togglePlaylistMaximized();frameCheck(host.playlistWindow->getBounds()==playlistFloating,"minimize overwrote Playlist floating restore bounds");
        host.playlistWindow->minimizeButton.onClick();host.togglePlaylistView();
        frameCheck(host.playlistWindow->getBounds()==playlistFloating && !host.playlistWindow->isMaximized(),"toolbar restore lost floating Playlist geometry");
    }
    host.playlistWindow->setMinimized(true);host.savePlaylistPanelState();
    host.playlistBeforeMinimizeBounds={};host.playlistBoundsInitialised=false;host.initialisePlaylistPanelBounds();
    host.showPlaylist();frameCheck(host.playlistWindow->getBounds()==playlistFloating,"saved minimized Playlist reopened with compact geometry as its normal size");
    const int wallpaper=host.optionInt("wallpaper");host.applicationProperties.getUserSettings()->setValue("wallpaper",juce::var(0));
    juce::Image wallpaperImage(juce::Image::RGB,2,2,true);juce::Graphics wallpaperGraphics(wallpaperImage);host.paintWallpaper(wallpaperGraphics);
    frameCheck(wallpaperImage.getPixelAt(0,0)==orbit::options::colour(72,96,108),"default workspace color is still navy");
    host.applicationProperties.getUserSettings()->setValue("wallpaper",wallpaper);
    host.project.appendPatternNoteTicks(acid,0,48,0,64,100);host.project.appendPatternNoteTicks(drums,0,65,0,64,50);host.refreshEnginePlaylist();
    host.openGeneratorPianoRoll(acid);
    frameCheck(window(acid).isPatternView() && host.pianoRollWindow->getParentComponent()==window(acid).getContentComponent() && host.pianoRollWindow->isShowing() && !window(acid).instrument->isVisible() && !host.generatorSurface.isVisible(),"Piano Roll is not the sole body of its generator");
    frameCheck(window(drums).isVisible() && window(acid).isMaximized(),"opening Piano Roll closed another generator or failed to fill workspace");
    host.resized();host.selectGeneratorControlTrack(drums);host.resized();
    frameCheck(host.pianoRollWindow->isShowing() && host.pianoRollWindow->grid->getNotes().size()==1 && host.pianoRollWindow->grid->getNotes()[0].pitch==48,"selection or workspace layout hid or redirected the hosted editor");
    host.pianoRollWindow->grid->noteGestureStarted();host.togglePianoRollNote(50,128,false);host.pianoRollWindow->grid->noteGestureEnded();
    frameCheck(host.selectedProjectTrackId()==acid && host.project.getPatternNote(acid,0,50,2) && !host.project.getPatternNote(drums,0,50,2),"first editor gesture mutated another generator");
    window(acid).maximiseButtonPressed();window(acid).setTopLeftPosition(100,150);const auto floating=window(acid).getBounds();
    frameImage(window(acid),"generator-piano-frame.png");
    for(int pass=0;pass<3;++pass)
    {
        window(acid).minimiseButtonPressed();host.resized();
        frameCheck(window(acid).isMinimised() && window(acid).isVisible() && window(acid).getHeight()==24 && !host.pianoRollWindow->isShowing() && !window(acid).instrument->isShowing(),"minimized frame leaked a child view");
        window(acid).showControls();frameCheck(window(acid).isPatternView() && host.pianoRollWindow->isShowing() && window(acid).getBounds()==floating,"restore lost Piano Roll or floating geometry");
    }
    host.closeMinimizeAllWindows();
    frameCheck(window(acid).getBottom()<=host.generatorDockViewport.getY() && !window(acid).getBounds().intersects(window(drums).getBounds()) && !window(acid).getBounds().intersects(host.playlistWindow->getBounds()),"minimized bars overlap each other or generator tabs");
    frameCheck(window(acid).isMinimised() && !host.pianoRollWindow->isShowing(),"minimize all leaked hosted editor");window(acid).showControls();frameCheck(host.pianoRollWindow->isShowing(),"minimize all discarded hosted editor visibility");
    host.rememberUndo("frame undo fixture");host.project.appendPatternNoteTicks(acid,0,52,256,64,100);host.undoProjectAction();
    frameCheck(window(acid).isPatternView() && host.pianoRollWindow->isShowing() && !window(acid).instrument->isVisible() && window(drums).isVisible(),"Undo split or closed hosted editor");
    window(acid).closeButtonPressed();
    frameCheck(!window(acid).isPatternView() && window(acid).instrument->isShowing() && !host.pianoRollWindow->isVisible() && window(acid).getWidth()==instrumentBounds.getWidth() && window(acid).getHeight()==instrumentBounds.getHeight(),"first X failed to return to instrument in same frame");
    window(acid).hideControls();frameCheck(!window(acid).isVisible() && window(drums).isVisible(),"hiding a generator closed unrelated generator");host.openGeneratorControls(acid);
    orbit::workspace::OwnedAlert alert("Clone Options","Owned modal fixture",juce::MessageBoxIconType::NoIcon);alert.addButton("Clone",1);alert.addButton("Cancel",0);alert.setSize(480,180);
    int result=-1;alert.enterModalState(true,juce::ModalCallbackFunction::create([&](int r){result=r;}),false);
    frameImage(host,"owned-clone-popup.png");
    frameCheck(alert.getParentComponent()==&host && !alert.isOnDesktop(),"modal has incorrect workspace owner");
    frameCheck(host.getLocalBounds().contains(alert.getBounds()),"modal bounds escape workspace");
    frameCheck(juce::Component::getCurrentlyModalComponent()==&alert,"modal does not block workspace focus");
    auto* hit=host.getComponentAt(alert.getBounds().getCentre());frameCheck(hit==&alert || alert.isParentOf(hit),"another child intercepts modal clicks");alert.getButton(0)->triggerClick();for(int pass=0;pass<40 && result<0;++pass)juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    frameCheck(result==1 && !alert.isCurrentlyModal(),"owned modal button could not complete");
    alert.setVisible(false);
    host.showPlaylist();host.showMixer();host.showSongEventEditor(acid);host.openGeneratorPianoRoll(acid);
    host.playButton.onClick();frameCheck(window(acid).isVisible() && window(acid).isPatternView() && host.songEventWindow->isVisible(),"Play closed a view");host.stopButton.onClick();frameCheck(window(acid).isVisible() && window(acid).isPatternView() && host.songEventWindow->isVisible(),"Stop closed a view");
    host.createProjectDiscardingCurrent();frameCheck(host.generatorControlWindows.empty() && !host.playlistWindow->isVisible() && !host.mixerWindow->isVisible() && !host.songEventWindow->isVisible() && !host.pianoRollWindow->isVisible() && !host.generatorSurface.isVisible() && host.mixerWindow->openEffectWindows().empty() && !host.generatorDockViewport.isVisible(),"New Project left document windows behind");
    frameImage(host,"blank-workspace.png");frameCheck(orbit::workspace::uiTraceFile.loadFileAsString().contains("modal result: Clone Options result=1"),"UI diagnostics omitted modal completion");
    std::cout<<"PASS: free built-in generator edge placement/size/minimize/maximize recovery, compact bottom Playlist minimize and full/floating restore, prior workspace color, blank startup/New, one generator frame, Piano Roll owner pinning, repeated move/minimize/restore, Piano Roll X and explicit view hide, Undo without splitting, owned modal bounds/z-order/button completion, transport preserves windows and UI diagnostics.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
