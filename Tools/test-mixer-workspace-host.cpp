// Native regression check with isolated preferences and no audio hardware.
// Compile/run through: python3 Tools/test-options-host.py --sanitize
//     --test-file Tools/test-mixer-workspace-host.cpp --compile-only
// Then run the printed executable in a WindowServer-capable session.
static void mixerWorkspaceCheck(bool ok,const char* message)
{if(!ok)throw std::runtime_error(message);}
static void mixerWorkspaceImage(juce::Component& c,const char* name)
{auto image=c.createComponentSnapshot(c.getLocalBounds());juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name));juce::PNGImageFormat().writeImageToStream(image,out);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Orbit mixer workspace verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
    shell.setContentNonOwned(&host,false);shell.setSize(1440,1030);shell.setVisible(true);
    host.createProjectDiscardingCurrent();for(int i=0;i<6;++i)host.addInstrumentTrack(true);
    host.showPlaylist();host.showMixer();auto& mixer=*host.mixerWindow;
    mixerWorkspaceCheck(mixer.isOpaque(),"opaque mixer still repaints the wallpaper beneath its body");
    for(bool master:{false,true})
    {
        host.showMixer(master);host.mixerPanelBounds={120,140,780,620};host.resized();
        const auto normal=mixer.getBounds();
        for(int pass=0;pass<3;++pass)
        {
            mixer.maximizeButton.onClick();
            mixerWorkspaceCheck(mixer.getBounds()==host.maximizedPlaylistBounds(),"maximize leaves outer padding or fitted strip width");
            host.showMixer(!master);mixerWorkspaceCheck(mixer.getBounds()==host.maximizedPlaylistBounds(),"switching banks shrinks maximized mixer");
            host.showMixer(master);
            for(auto size:std::array<juce::Point<int>,3>{{{1000,760},{1680,1050},{1440,1000}}})
            {
                host.setSize(size.x,size.y);
                mixerWorkspaceCheck(mixer.getBounds()==juce::Rectangle<int>(0,82,size.x,size.y-82-host.workspaceBottomInset()),"workspace resize introduces maximize gaps");
            }
            auto* prefs=host.applicationProperties.getUserSettings();
            mixerWorkspaceCheck(prefs->getIntValue("mixerPanelWidth")==normal.getWidth() && prefs->getIntValue("mixerPanelHeight")==normal.getHeight(),"maximize overwrote saved normal size");
            mixer.minimizeButton.onClick();host.resized();
            mixerWorkspaceCheck(mixer.isMinimized() && !mixer.viewport.isVisible(),"minimize exposed mixer body");
            mixer.minimizeButton.onClick();host.resized();
            mixerWorkspaceCheck(mixer.isMaximized() && mixer.getBounds()==host.maximizedPlaylistBounds(),"minimize/restore lost full workspace bounds");
            mixer.maximizeButton.onClick();
            mixerWorkspaceCheck(mixer.getBounds()==normal,"minimize overwrote floating restore geometry");
        }
        host.toggleMixerMaximized();host.closeMinimizeAllWindows();host.showMixer(master);
        mixerWorkspaceCheck(mixer.isMaximized() && mixer.getBounds()==host.maximizedPlaylistBounds(),"minimize all lost maximize geometry");
        host.toggleMixerMaximized();mixerWorkspaceCheck(mixer.getBounds()==normal,"minimize all overwrote floating restore");
        host.setMixerPanelSize(true);host.showMixer(master);
        mixerWorkspaceCheck(mixer.getBounds()==host.maximizedPlaylistBounds(),"menu expansion leaves padding");
        mixerWorkspaceImage(host,master?"master-maximized.png":"mixer-maximized.png");
        mixer.maximizeButton.onClick();
        host.mixerPanelBounds=normal;mixer.setBounds(normal);
        auto downScreen=mixer.localPointToGlobal(juce::Point<int>(100,12));
        auto event=[&](juce::Point<int> screen,juce::Component* target=nullptr)
        {
            auto* component=target!=nullptr?target:&mixer;
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),component->getLocalPoint(nullptr,screen).toFloat(),
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,component,component,juce::Time::getCurrentTime(),
                component->getLocalPoint(nullptr,downScreen).toFloat(),juce::Time::getCurrentTime(),1,true);
        };
        mixer.mouseDown(event(downScreen));const auto layouts=checkedWorkspaceLayouts;const auto stripBounds=mixer.strips.getBounds();
        for(auto delta:std::array<juce::Point<int>,7>{{{0,-40},{100,-40},{220,170},{-500,200},{1200,250},{0,2000},{10,20}}})
        {
            const auto destination=host.constrainMixerPosition(normal.getPosition()+delta);
            for(int duplicate=0;duplicate<4;++duplicate)
            {
                mixer.mouseDrag(event(downScreen+delta));
                mixerWorkspaceCheck(mixer.getPosition()==destination,"title drag dropped or oscillated as local coordinates changed");
                mixerWorkspaceCheck(mixer.getWidth()==normal.getWidth() && mixer.getHeight()==normal.getHeight() && mixer.strips.getBounds()==stripBounds,"title drag resized or reflowed channel strips");
            }
        }
        mixerWorkspaceCheck(checkedWorkspaceLayouts==layouts,"title drag relaid out the complete workspace");
        mixer.mouseUp(event(downScreen+juce::Point<int>(10,20)));
        mixerWorkspaceCheck(!host.draggingMixer && !mixer.draggingHeader,"mouse release left drag active");
        const auto released=mixer.getBounds();mixer.mouseDrag(event(downScreen+juce::Point<int>(40,60)));
        mixerWorkspaceCheck(mixer.getBounds()==released,"late drag event moved released mixer");
        mixerWorkspaceCheck(host.applicationProperties.getUserSettings()->getIntValue("mixerPanelX")==released.getX(),"release did not persist final position");
        mixer.minimizeButton.onClick();host.resized();host.showMixer(master);
        mixerWorkspaceCheck(mixer.getPosition()==released.getPosition() && !mixer.isMinimized(),"toolbar restore lost floating position");
        mixer.maximizeButton.onClick();const auto full=mixer.getBounds();downScreen=mixer.localPointToGlobal(juce::Point<int>(100,12));
        mixer.mouseDown(event(downScreen));mixer.mouseDrag(event(downScreen+juce::Point<int>(100,100)));mixer.mouseUp(event(downScreen));
        mixerWorkspaceCheck(mixer.getBounds()==full && !host.draggingMixer,"maximized header drifted away from full workspace");
        mixer.maximizeButton.onClick();
    }
    host.showPlaylist();host.togglePlaylistMaximized();mixerWorkspaceCheck(host.playlistWindow->getBounds()==host.maximizedPlaylistBounds(),"Playlist workspace regressed");
    const auto id=host.selectedProjectTrackId();host.openGeneratorControls(id);auto& generator=*host.generatorControlWindows.at(id).window;
    generator.setTopLeftPosition(-180,180);host.resized();mixerWorkspaceCheck(generator.getX()==-180,"mixer fix boxed in floating generators");
    host.playButton.onClick();host.stopButton.onClick();mixerWorkspaceCheck(mixer.isVisible() && generator.isVisible() && host.playlistWindow->isVisible(),"transport closed an existing view");
    std::cout<<"PASS: Mixer/Master exact full-workspace bounds, bank switches, resize, menu expansion, repeated maximize/minimize/restore, saved normal geometry; screen-anchored title drag with duplicate/reversed/off-edge events, release and no layout/reflow; Playlist, generator freedom and transport visibility.\n";
    return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
