#define main mixerWorkspaceRegression
#include "test-mixer-workspace-host.cpp"
#undef main
static void skinCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void skinImage(juce::Component& c,const char* name,bool crop=false)
{auto image=c.createComponentSnapshot(crop?c.getLocalBounds().withTrimmedBottom(22):c.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));juce::PNGImageFormat().writeImageToStream(image,out);}
static void requireSilverSkin(ProjectView& h,int skin)
{
    skinCheck(h.headerStyle()==8 && h.optionInt("skin")==skin && h.skinIsSelected(skin),"complete skin selection/checkmark was lost");
    skinCheck(h.mixerWindow->getMixerSkin()==(skin==8?4:3) && orbit::options::compact::variant(*h.playlistWindow)==(skin==8?2:1),"complete skin did not coordinate console and Playlist");
    skinCheck(h.playlistWindow->isLightTheme()==(skin==7),"second skin did not use the reference dark Playlist");
}
int main(int argc,char** argv)
{
 try
 {
    if(argc>1 && juce::String(argv[1])=="--workspace")return mixerWorkspaceRegression();
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    if(argc>1)
    {
        const juce::String mode(argv[1]);
        skinCheck(!host.mixerWindow->isVisible() && !host.playlistWindow->isVisible(),"new skins reopened document windows at startup");
        if(mode=="--silver1"){requireSilverSkin(host,7);host.handleMenuResult(4,5508);}
        else if(mode=="--silver2"){requireSilverSkin(host,8);host.mixerWindow->handleMixerMenuResult(12);}
        else if(mode=="--override")
        {skinCheck(host.optionInt("skin")==8 && host.mixerWindow->getMixerSkin()==2 && !host.skinIsSelected(8),"manual console override did not persist or kept a misleading full-skin checkmark");host.setSkin(8);}
        else if(mode=="--native")
        {
            host.createProjectDiscardingCurrent();juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,GENERATOR_VST_PATH_LITERAL);
            const juce::PluginDescription* synth=nullptr;for(auto* d:found)if(d->isInstrument){synth=d;break;}
            skinCheck(synth!=nullptr,"JC303 VST3 fixture missing");host.addVst3Instrument(*synth);
            auto* external=dynamic_cast<te::ExternalPlugin*>(host.selectedInstrumentPlugin());skinCheck(external!=nullptr,"native instrument was not inserted");
            host.clearGeneratorControlWindows();host.independentControlsMode=false;host.allGeneratorsMinimized=false;host.generatorCollapsed=false;
            host.activeInstrumentEditor=std::make_unique<OrbitPluginEditorContent>(*external);
            host.instrumentEditorClip.setViewedComponent(host.activeInstrumentEditor.get(),false);host.generatorPanelBounds={110,82,900,650};
            struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Native generator edge verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
            shell.setContentNonOwned(&host,false);shell.setSize(1440,1030);shell.setVisible(true);host.resized();
            auto* wrapper=dynamic_cast<OrbitPluginEditorContent*>(host.activeInstrumentEditor.get());
            for(int pass=0;pass<80 && !wrapper->editorReady();++pass){wrapper->ensureEditorCreated();juce::MessageManager::getInstance()->runDispatchLoopUntil(25);}
            skinCheck(wrapper->editorReady(),"native editor did not initialise");host.fitGeneratorPanelToPluginEditor();
            for(int pass=0;pass<3;++pass)
            {
                const auto size=host.generatorPanelBounds;const auto anchor=size.getPosition()+juce::Point<int>(100,12);
                host.beginGeneratorDrag(anchor);host.moveGeneratorPanel({anchor.x,-1000});host.draggingGenerator=false;
                skinCheck(host.generatorSurface.getY()==82 && host.generatorPanelBounds.getY()==82,"native/shared generator drag still leaves a top gap");
                host.resized();host.fitGeneratorPanelToPluginEditor();skinCheck(host.generatorSurface.getY()==82,"layout or native Auto Fit reintroduced the top gap");
                host.beginGeneratorResize(host.generatorPanelBounds.getBottomRight());host.resizeGeneratorPanel(host.generatorPanelBounds.getBottomRight()+juce::Point<int>(20,20));host.resizingGenerator=false;
                skinCheck(host.generatorSurface.getY()==82,"resizing the native generator reintroduced the top gap");
            }
            for(juce::Point<int> target:std::array<juce::Point<int>,3>{{{-180,82},{1320,82},{100,930}}})
            {
                const auto anchor=host.generatorPanelBounds.getPosition()+juce::Point<int>(100,12);host.beginGeneratorDrag(anchor);host.moveGeneratorPanel(target+juce::Point<int>(100,12));host.draggingGenerator=false;
                host.resized();skinCheck(host.generatorSurface.getPosition()==target,"native/shared generator lost side/bottom overflow");
                host.fitGeneratorPanelToPluginEditor();skinCheck(host.generatorSurface.getPosition()==target,"native Auto Fit boxed in an off-edge generator");
            }
            host.generatorPanelBounds.setPosition(100,82);host.resized();skinImage(host,"jc303-top-edge.png",true);
            host.playButton.onClick();host.stopButton.onClick();skinCheck(host.generatorSurface.isVisible() && host.generatorSurface.getY()==82,"transport hid or moved the native generator");
            std::cout<<"PASS: real JC303 shared-host drag, layout, native Auto Fit and resize reach y=82; side/bottom overflow and transport visibility retained.\n";return 0;
        }
        std::cout<<"PASS: fresh process "<<mode<<".\n";return 0;
    }
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Silver skins verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
    shell.setContentNonOwned(&host,false);shell.setSize(1440,1030);shell.setVisible(true);
    host.headerArtwork.setFolder(juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork"));
    host.setSkin(0);host.setHeaderStyle(1);host.mixerWindow->handleMixerMenuResult(12);host.applicationProperties.getUserSettings()->setValue("playlistLightTheme",false);host.playlistWindow->setLightTheme(false);
    host.createProjectDiscardingCurrent();
    for(int row=0;row<3;++row)
    {
        host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();for(int n=0;n<8;++n)host.project.appendPatternNoteTicks(id,0,40+row*12+(n%4)*2,n*128,96,100);
        for(int bar=0;bar<8;++bar)host.project.setPlaylistClip(id,host.project.getPatternId(id,0),bar,1,true);
    }
    host.refreshEnginePlaylist();host.showPlaylist();host.showMixer();const int wallpaper=host.optionInt("wallpaper");
    auto options=host.makeOptionsMenu();bool first=false,second=false;
    for(juce::PopupMenu::MenuItemIterator it(options);it.next();)if(it.getItem().text=="Skins" && it.getItem().subMenu!=nullptr)
        for(juce::PopupMenu::MenuItemIterator skin(*it.getItem().subMenu);skin.next();){if(skin.getItem().itemID==5507)first=skin.getItem().text=="Compact Silver";if(skin.getItem().itemID==5508)second=skin.getItem().text=="Compact Silver 2";}
    skinCheck(first && second,"both complete skins are not directly selectable in Options/Skins");
    for(int pass=0;pass<2;++pass)for(int skin:{7,8})
    {
        host.handleMenuResult(4,5500+skin);requireSilverSkin(host,skin);
        skinCheck(host.optionInt("wallpaper")==wallpaper,"skin changed wallpaper preference");
        for(auto& [id,entry]:host.generatorControlWindows)
        {
            entry.window->setTopLeftPosition(100,-500);skinCheck(entry.window->getY()==82,"independent generator cannot reach toolbar edge");
            const auto floating=entry.window->getBounds();entry.window->maximiseButtonPressed();skinCheck(entry.window->getBounds()==host.maximizedPlaylistBounds(),"independent generator maximize leaves padding");
            entry.window->minimiseButtonPressed();entry.window->minimiseButtonPressed();skinCheck(entry.window->getBounds()==host.maximizedPlaylistBounds(),"maximized generator minimize/restore lost flush edges");
            entry.window->maximiseButtonPressed();skinCheck(entry.window->getBounds()==floating,"generator floating restore changed");
            entry.window->setVisible(false);
        }
        host.playlistPanelBounds={445,108,980,840};host.mixerPanelBounds={14,108,416,840};host.resized();
        host.mixerWindow->toFront(false);skinImage(host,skin==7?"silver1-workspace.png":"silver2-workspace.png",true);
        if(skin==8)
        {
            auto& grid=*host.playlistWindow->grid;skinCheck(orbit::options::compact::colour(grid,20,29,38).getBrightness()<.3f,"reference Playlist canvas is too light");
            skinCheck(orbit::options::compact::colour(grid,39,125,165).getBrightness()>.6f,"reference pattern fill is not pale blue");
            skinCheck(orbit::options::compact::accent(*host.mixerWindow).getBlue()>orbit::options::compact::accent(*host.mixerWindow).getGreen(),"Mixer accent is not cool blue");
            host.showMixer(true);host.mixerPanelBounds={14,108,680,840};host.mixerWindow->setBounds(host.mixerPanelBounds);skinImage(*host.mixerWindow,"silver2-master.png");host.showMixer();
        }
        for(auto& [key,button]:host.playlistWindow->grid->cellButtons)skinCheck(button->findColour(juce::TextButton::buttonColourId).isTransparent(),"full skin refresh made Playlist hit targets opaque");
    }
    host.setSkin(0);skinCheck(host.headerStyle()==0 && host.mixerWindow->getMixerSkin()==0 && !host.playlistWindow->isLightTheme() && orbit::options::compact::variant(*host.playlistWindow)==0,"selecting Default failed to restore its independent stock appearance");
    host.setSkin(8);host.mixerWindow->handleMixerMenuResult(13);skinCheck(!host.skinIsSelected(8),"manual console choice did not clear complete-skin checkmark");host.mixerWindow->handleMixerMenuResult(14);requireSilverSkin(host,8);
    host.showMixer(true);skinCheck(host.mixerWindow->getMixerSkin()==4,"Master did not retain second silver palette");
    host.playButton.onClick();host.stopButton.onClick();skinCheck(host.playlistWindow->isVisible() && host.mixerWindow->isVisible(),"skin regressed transport visibility");
    host.setSkin(7);
    std::cout<<"PASS: two complete Skins menu entries; coordinated v1/v2 colors, dark reference Playlist, silver/chrome console, independent console choices, previous appearance restore, transparent hit targets, wallpaper, generator flush drag/maximize/minimize/restore and transport visibility.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
