// Isolated production host check: header modes, real picker and existing surfaces.
static void headerCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void headerImage(juce::Component& c,const juce::String& name)
{auto image=c.createComponentSnapshot(name.startsWith("header-")?c.getLocalBounds().withHeight(82):c.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));juce::PNGImageFormat().writeImageToStream(image,out);}
int main(int argc,char** argv)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();
    const auto sourceArtwork=juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork");
    host.headerArtwork.setFolder(sourceArtwork);
    if(argc>1 && juce::String(argv[1])=="--fresh")
    {headerCheck(host.headerStyle()==8,"header style did not persist across processes");std::cout<<"PASS: independent header preference restored in a fresh process.\n";return 0;}
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Header verification",juce::Colours::black,allButtons){}void closeButtonPressed()override{}} shell;
    shell.setContentNonOwned(&host,false);shell.setSize(1440,930);shell.setVisible(true);host.setSize(1440,900);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();host.showPlaylist();host.showMixer();
    auto& generator=*host.generatorControlWindows.at(id).window;generator.setBounds(120,150,900,340);
    const auto toolbarBefore=generator.getContentComponent()->createComponentSnapshot({0,0,900,58});
    host.saveOption("wallpaper",2);const auto skin=host.optionInt("skin"),wallpaper=host.optionInt("wallpaper");
    const auto generatorBounds=generator.getBounds();
    juce::Image comparison(juce::Image::RGB,1440,600,true);juce::Graphics comparisonGraphics(comparison);
    comparisonGraphics.fillAll(juce::Colour(12,27,39));
    for(int variant=0;variant<orbit::options::header::names.size();++variant)
    {
        host.handleMenuResult(4,5520+variant);
        headerCheck(host.headerStyle()==variant,"style menu dispatch failed");
        for(int width:{1000,1100,1220,1440,1800})
        {
            host.setSize(width,900);
            std::vector<juce::Component*> controls{&host.fileMenuButton,&host.editMenuButton,&host.viewMenuButton,&host.insertMenuButton,&host.optionsMenuButton,&host.windowMenuButton,&host.helpMenuButton,
                &host.playButton,&host.stopButton,&host.rewindButton,&host.previousButton,&host.nextButton,&host.recordButton,&host.recordModeButton,&host.tempoLabel,&host.tempoSlider,&host.timeSignatureLabel,&host.loopButton,&host.barReadoutLabel,&host.loopStartSlider,&host.loopLengthSlider,&host.metronomeButton,
                &host.patternModeButton,&host.songModeButton,&host.playlistButton,&host.mixerButton,&host.masterButton,&host.masterVolumeKnob,&host.outputMeter};
            if(variant>=3)
            {
                auto cached=host.headerArtwork.image(variant,width);
                headerCheck(cached.isValid() && cached.getWidth()==width*2 && cached.getHeight()==164,"artwork absent or cache has wrong scale");
                headerCheck(cached==host.headerArtwork.image(variant,width),"unchanged paint regenerated artwork cache");
                if(variant!=8)
                {
                    const auto primary=(variant==4 || variant==6)?host.playlistButton.getBounds().getUnion(host.masterButton.getBounds()):host.fileMenuButton.getBounds().getUnion(host.helpMenuButton.getBounds());
                    const int virtualWidth=juce::jmax(1120,width);
                    const int expected=(variant==6 || variant==7)?juce::roundToInt((256+juce::roundToInt((virtualWidth-512)*(645.-334.)/1004.))*width/double(virtualWidth)):width/2;
                    headerCheck(std::abs(primary.getCentreX()-expected)<=3,"primary top panel misses its intended recess");
                }
            }
            if(variant>0)
            {
                for(size_t a=0;a<controls.size();++a)
                {
                    headerCheck(juce::Rectangle<int>(0,0,width,82).contains(controls[a]->getBounds()),"header control clipped");
                    for(size_t b=a+1;b<controls.size();++b)headerCheck(!controls[a]->getBounds().intersects(controls[b]->getBounds()),"header controls overlap");
                }
                headerCheck(host.loopButton.getY()>=42 && host.loopStartSlider.getY()>=42 && host.loopLengthSlider.getY()>=42,"loop/time controls left the transport row");
                headerCheck(host.loopButton.getX()>host.recordModeButton.getX(),"loop controls are not beside transport");
                headerCheck(host.playlistButton.getY()<35 || variant==8,"views did not move to the top row");
                headerCheck(!host.headerBrandBounds.isEmpty(),"Orbit branding is absent");
                for(auto* c:controls)headerCheck(!host.headerBrandBounds.intersects(c->getBounds()),"Orbit branding overlaps a control");
            }
        }
        host.setSize(1440,900);
        headerCheck(host.optionInt("skin")==skin && host.optionInt("wallpaper")==wallpaper && generator.getBounds()==generatorBounds,"header changed workspace or generator appearance/bounds");
        const auto toolbar=generator.getContentComponent()->createComponentSnapshot({0,0,900,58});
        for(int y=0;y<58;++y)for(int x=0;x<900;++x)headerCheck(toolbar.getPixelAt(x,y)==toolbarBefore.getPixelAt(x,y),"header style leaked into generator toolbar");
        headerCheck(generator.isVisible() && host.playlistWindow->isVisible() && host.mixerWindow->isVisible(),"header switch closed a window");
        generator.setVisible(false);host.playlistWindow->setVisible(false);host.mixerWindow->setVisible(false);
        headerImage(host,"header-"+juce::String(variant)+".png");
        const std::vector<int> comparisons{3,4,6,7,8};
        auto position=std::find(comparisons.begin(),comparisons.end(),variant);
        if(position!=comparisons.end())
        {
            const int y=int(position-comparisons.begin())*120;
            comparisonGraphics.setColour(juce::Colour(221,231,237));comparisonGraphics.setFont(juce::FontOptions(14.f));
            comparisonGraphics.drawText(orbit::options::header::names[variant]+" — loop beside transport",12,y,1400,26,juce::Justification::centredLeft);
            comparisonGraphics.drawImageAt(host.createComponentSnapshot({0,0,1440,82}),0,y+27);
        }
        generator.setVisible(true);host.playlistWindow->setVisible(true);host.mixerWindow->setVisible(true);
        host.songModeButton.onClick();headerCheck(host.playbackMode==ProjectView::PlaybackMode::song,"Song callback changed");
        host.patternModeButton.onClick();headerCheck(host.playbackMode==ProjectView::PlaybackMode::pattern,"Pattern callback changed");
        host.edit->state.setProperty("orbitTimeDisplay",2,nullptr);host.resized();
        if(variant>0)
        {
            std::vector<juce::Component*> row{&host.tempoSlider,&host.playButton,&host.stopButton,&host.recordButton,&host.recordModeButton,&host.rewindButton,&host.previousButton,&host.nextButton,&host.patternModeButton,&host.songModeButton,&host.loopButton,&host.barReadoutLabel,&host.loopStartSlider,&host.loopLengthSlider,&host.metronomeButton,&host.outputMeter,&host.masterVolumeKnob,&host.playlistButton,&host.mixerButton,&host.masterButton};
            for(size_t a=0;a<row.size();++a){headerCheck(juce::Rectangle<int>(0,0,1440,82).contains(row[a]->getBounds()),"extended display clips a control");for(size_t b=a+1;b<row.size();++b)headerCheck(!row[a]->getBounds().intersects(row[b]->getBounds()),"extended display overlaps controls");}
        }
        host.edit->state.setProperty("orbitTimeDisplay",0,nullptr);
    }
    {juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/header-comparison.png"));juce::PNGImageFormat().writeImageToStream(comparison,out);}
    orbit::options::header::Artwork missing;missing.setFolder(juce::File("OPTIONS_CHECK_FOLDER/no-artwork"));
    headerCheck(!missing.image(3,1440).isValid(),"missing artwork fallback is not safe");
    host.project.setPlaylistClip(id,host.project.getPatternId(id,0),0,1,true);
    host.project.setPatternName(id,63,"Last named pattern");
    auto menu=host.makePlaylistPatternMenu(0,0);int count=0,breaks=0;std::set<int> ids;
    for(juce::PopupMenu::MenuItemIterator it(menu);it.next();)
    {
        const auto& item=it.getItem();headerCheck(item.subMenu==nullptr,"picker still nests pattern banks");
        headerCheck(item.itemID==++count,"pattern IDs/order changed");ids.insert(item.itemID);
        if(item.shouldBreakAfter){++breaks;headerCheck(count==32,"column break is not after D8");}
        if(count==1)headerCheck(item.isTicked,"current pattern checkmark absent");
    }
    headerCheck(count==64 && ids.size()==64 && breaks==1,"picker does not contain two columns of 32");
    host.setPlaylistPattern(0,0,63);headerCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,63),"H8 picker result did not assign actual pattern");
    host.undoProjectAction();headerCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,0),"picker assignment cannot Undo");
    host.setHeaderStyle(1);host.setSize(1440,900);
    host.playlistWindow->setVisible(false);host.mixerWindow->setVisible(false);for(auto& entry:host.generatorControlWindows)entry.second.window->setVisible(false);
    juce::Process::makeForegroundProcess();shell.toFront(true);host.grabKeyboardFocus();
    host.showPlaylistPatternMenu(0,0,&host.fileMenuButton);
    // Capture/activate synchronously: the disposable console harness is not a
    // registered foreground Mac application, unlike the product bundle.
    bool renderedPicker=false;juce::Component* lastSlot=nullptr;int visibleSlots=0;
    for(int i=0;i<juce::Desktop::getInstance().getNumComponents();++i)
    {
        auto* c=juce::Desktop::getInstance().getComponent(i);
        if(!c || c==&shell || !c->isShowing())continue;
        std::vector<juce::Component*> slots;
        for(auto* child:c->getChildren())if(auto* a=child->getAccessibilityHandler())
        {
            const auto title=a->getTitle();if(title.length()>1 && title[0]>='A' && title[0]<='H' && title[1]>='1' && title[1]<='8')
            {slots.push_back(child);if(title.startsWith("H8"))lastSlot=child;}
        }
        std::cout<<"Desktop surface: "<<c->getName()<<" size="<<c->getWidth()<<"x"<<c->getHeight()<<" slots="<<slots.size()<<'\n';
        if(slots.empty())continue;
        visibleSlots+=int(slots.size());
        headerCheck(c->getWidth()>250 && c->getHeight()<700,"picker panel shape is not two compact columns");
        for(auto* child:slots)headerCheck(c->getLocalBounds().contains(child->getBounds()),"pattern is outside popup panel");
        headerImage(*c,"pattern-picker.png");renderedPicker=true;
    }
    headerCheck(renderedPicker,"real picker was not displayed");
    headerCheck(visibleSlots==64 && lastSlot!=nullptr,"not all 64 slots are visible in one panel");
    headerCheck(lastSlot->getAccessibilityHandler()->getActions().invoke(juce::AccessibilityActionType::press),"H8 popup cannot activate");
    for(int pass=0;pass<4;++pass)juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    headerCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,63),"real picker callback did not assign H8");
    juce::PopupMenu::dismissAllActiveMenus();for(int pass=0;pass<4;++pass)juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    host.setHeaderStyle(8);
    std::cout<<"PASS: nine header styles, loop beside transport, alternative primary top panels, reference crops and compact silver, bundled-artwork decoding and 2x width cache, independent palette/wallpaper/generator toolbar, existing mode callbacks/windows, bounded nonoverlapping controls at five widths and expanded time display; all 64 patterns in two direct columns, named H8 assignment and Undo, real popup render.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
