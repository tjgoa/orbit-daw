// Native UI regression: edges must not depend on theme, window state or process lifetime.
#define main legacyMixerWorkspaceMain
#include "test-mixer-workspace-host.cpp"
#undef main
static void edgeCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void edgeImage(juce::Component& c,const juce::String& name)
{auto im=c.createComponentSnapshot(c.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));edgeCheck(out.openedOk(),"preview output failed");juce::PNGImageFormat().writeImageToStream(im,out);}
int main(int argc,char** argv)
{
 try
 {
    if(argc>1 && juce::String(argv[1])=="--legacy")return legacyMixerWorkspaceMain();
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1440,1000);
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Workspace edge verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
    shell.setContentNonOwned(&h,false);shell.setSize(1440,1030);shell.setVisible(true);h.setSize(1440,1000);
    auto& m=*h.mixerWindow;auto& p=*h.playlistWindow;const juce::Rectangle<int> normalMixer(120,82,780,620),normalPlaylist(70,82,1000,700);
    if(argc>1 && juce::String(argv[1])=="--read")
    {
        edgeCheck(!m.isVisible() && !p.isVisible(),"startup must stay an empty workspace");
        h.showMixer();h.showPlaylist();
        edgeCheck(m.isMaximized() && m.getBounds()==juce::Rectangle<int>(0,82,1440,896),"fresh launch lost maximized Mixer or its flush edges");
        edgeCheck(p.isMaximized() && p.getBounds()==juce::Rectangle<int>(0,82,1440,896),"fresh launch lost maximized Playlist or its flush edges");
        h.toggleMixerMaximized();h.togglePlaylistMaximized();
        edgeCheck(m.getBounds()==normalMixer && p.getBounds()==normalPlaylist,"fresh launch corrupted normal restore bounds");
        h.resized();edgeCheck(m.getY()==82 && p.getY()==82,"fresh restored bounds acquired padding during layout");
        std::cout<<"PASS: fresh process starts blank, restores both maximized panels flush, retains normal bounds and top edge.\n";return 0;
    }
    h.createProjectDiscardingCurrent();for(int i=0;i<3;++i)h.addInstrumentTrack(true);
    h.showMixer();h.showPlaylist();
    if(m.isMaximized())h.toggleMixerMaximized();if(p.isMaximized())h.togglePlaylistMaximized();
    if(argc>1 && juce::String(argv[1])=="--write")
    {
        h.mixerPanelBounds=normalMixer;m.setBounds(normalMixer);h.playlistPanelBounds=normalPlaylist;p.setBounds(normalPlaylist);
        h.toggleMixerMaximized();h.togglePlaylistMaximized();h.saveMixerPanelBounds();h.savePlaylistPanelState();
        edgeCheck(h.applicationProperties.getUserSettings()->getBoolValue("mixerMaximized",false),"Mixer maximized state was not saved");
        std::cout<<"PASS: saved maximized modes and separate normal geometry for fresh-process test.\n";return 0;
    }
    h.headerArtwork.setFolder(juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork"));
    for(auto& [id,entry]:h.generatorControlWindows)entry.window->setVisible(false);
    for(int skin=0;skin<9;++skin)
    {
        h.setSkin(skin);
        for(int header=0;header<int(orbit::options::header::names.size());++header)
        {
            h.setHeaderStyle(header);
            for(bool master:{false,true})
            {
                h.showMixer(master);h.mixerPanelBounds=normalMixer;m.setBounds(normalMixer);
                h.playlistPanelBounds=normalPlaylist;p.setBounds(normalPlaylist);
                m.onDragged({0,-1000});h.movePlaylistPanel({0,-1000});h.resized();
                edgeCheck(m.getPosition()==juce::Point<int>(0,82) && p.getPosition()==juce::Point<int>(0,82),"floating top/left padding depends on skin/header/bank");
                m.onResizeStarted();m.onResizedByDrag({0,2000});m.onResizeEnded();
                h.resizePlaylistPanel({0,82,1440,896});h.resized();
                edgeCheck(m.getY()==82 && m.getBottom()==978 && p.getBounds()==juce::Rectangle<int>(0,82,1440,896),"manual resize imposes outer padding");
                m.minimizeButton.onClick();m.minimizeButton.onClick();p.minimizeButton.onClick();p.minimizeButton.onClick();
                edgeCheck(m.getY()==82 && p.getY()==82,"floating minimize restore reintroduced top inset");
                h.toggleMixerMaximized();h.togglePlaylistMaximized();
                const auto full=juce::Rectangle<int>(0,82,1440,896);
                edgeCheck(m.getBounds()==full && p.getBounds()==full,"maximize left workspace padding");
                h.showMixer(!master);h.fitMixerWidthToStrips();h.showMixer(master);h.showPlaylist();
                edgeCheck(m.getBounds()==full && p.getBounds()==full,"reopening/bank switch/strip fit shrank a maximized panel");
                if(header==0){h.setSkin((skin+1)%9);h.setSkin(skin);edgeCheck(m.getBounds()==full && p.getBounds()==full,"skin switch changed maximized edges");}
                h.setHeaderStyle((header+1)%int(orbit::options::header::names.size()));
                h.setSize(1000,760);edgeCheck(m.getBounds()==juce::Rectangle<int>(0,82,1000,656) && p.getBounds()==m.getBounds(),"app resize or style switch introduced gaps");
                h.setSize(1440,1000);h.closeMinimizeAllWindows();h.showMixer(master);h.showPlaylist();
                edgeCheck(m.getBounds()==full && p.getBounds()==full,"minimize all/restore introduced gaps");
                h.toggleMixerMaximized();h.togglePlaylistMaximized();
            }
        }
    }
    // Flat console samples: all EQ sections, selected and unselected strips,
    // unused viewport space and Master use the same neutral surface.
    for(int skin:{7,8})
    {
        h.setSkin(skin);h.showMixer();h.mixerPanelBounds={0,82,600,896};h.resized();
        for(auto* strip:m.channelStrips)
        {
            const auto im=strip->createComponentSnapshot(strip->getLocalBounds());
            const auto expected=orbit::options::compact::surface(skin-6);
            for(const auto& band:strip->eqBandBounds)
                edgeCheck(im.getPixelAt(band.getX()+8,band.getBottom()-3)==expected,"silver EQ backgrounds still alternate");
        }
        const auto im=m.content.createComponentSnapshot(m.content.getLocalBounds());
        edgeCheck(im.getPixelAt(im.getWidth()-20,60)==orbit::options::compact::surface(skin-6),"unused Mixer background differs from the channel silver");
        edgeCheck(orbit::options::compact::accent(m)==juce::Colour(96,200,226),"warm/green accent still present");
        h.playlistPanelBounds={610,82,820,896};h.resized();m.toFront(false);
        edgeImage(h,skin==7?"silver1-uniform-workspace.png":"silver2-uniform-workspace.png");
        h.showMixer(true);h.mixerPanelBounds={0,82,680,896};h.resized();edgeImage(m,skin==7?"silver1-uniform-master.png":"silver2-uniform-master.png");
    }
    h.showMixer();h.toggleMixerMaximized();h.togglePlaylistMaximized();h.playButton.onClick();h.stopButton.onClick();
    edgeCheck(m.isVisible() && p.isVisible() && m.getY()==82 && p.getY()==82,"transport altered workspace visibility or edges");
    edgeImage(h,"maximized-workspace.png");
    std::cout<<"PASS: nine skins x nine headers x Mixer/Master; drag to x0/y82, full manual resize, maximize, bank switch, strip fit, app resize, minimize all, reopen and transport; uniform silver EQ/channel/unused surfaces and cool accents.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
