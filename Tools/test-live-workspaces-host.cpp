// Native-peer verification of the production multi-window host. Uses only
// explicitly supplied installed plugins, isolated prefs, no audio devices.
bool saveNativePeer(juce::Component&,const juce::File&);
bool nativeGeometryMatchesPeer(juce::Component&);
class LiveWorkspaceProbe final:public juce::JUCEApplication,private juce::Timer
{
    std::unique_ptr<ProjectView> host;
    class Shell final:public juce::DocumentWindow
    {public:Shell():DocumentWindow("Orbit workspace verification",juce::Colours::black,closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<Shell> shell;
    std::vector<juce::String> paths{NATIVE_PLUGIN_PATHS};
    juce::String firstGenerator,secondGenerator,firstFx,secondFx;int phase=0;
    juce::Rectangle<int> firstBounds,secondBounds;
    static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
    juce::PluginDescription description(const juce::String& path,bool instrument)
    {
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,path);
        for(auto* d:found)if(d->isInstrument==instrument)return *d;
        throw std::runtime_error("No matching native plugin class");
    }
    void load()
    {
        check(paths.size()==2,"Supply instrument and effect paths");host->createProjectDiscardingCurrent();
        auto synth=description(paths[0],true),effect=description(paths[1],false);
        host->addVst3Instrument(synth);firstGenerator=host->selectedProjectTrackId();
        host->addVst3Instrument(synth);secondGenerator=host->selectedProjectTrackId();
        check(host->addVst3Effect(0,effect) && host->addVst3Effect(1,effect),"Native effects failed to insert");
        firstFx=host->getInsertPlugins(0)[0]->itemID.toString();secondFx=host->getInsertPlugins(1)[0]->itemID.toString();
        startTimer(1200);
    }
    void verifyNative()
    {
        host->syncGeneratorNativeViews();
        for(const auto& id:{firstGenerator,secondGenerator})
        {
            const auto& w=*host->generatorControlWindows.at(id).window;
            auto* native=dynamic_cast<OrbitPluginEditorContent*>(w.instrument.get());
            check(w.isVisible() && native!=nullptr && native->editorReady() && native->nativeClip.attachedCount()>0,"Native generator editor missing/hidden");
            check(nativeGeometryMatchesPeer(*native),"Generator Cocoa frame detached from JUCE wrapper");
        }
        for(const auto& id:{firstFx,secondFx})
        {
            auto w=host->mixerWindow->openEffectWindows().at(id);auto* native=w->nativeEditor.get();
            check(w->isVisible() && native!=nullptr && native->editorReady() && native->nativeClip.attachedCount()>0,"Native FX editor missing/hidden");
            check(nativeGeometryMatchesPeer(*native),"FX Cocoa frame detached from JUCE wrapper");
            check(w->getParentComponent()==host.get() && !w->isOnDesktop(),"FX escaped parent app");
        }
    }
    void timerCallback()override
    {
        try
        {
            stopTimer();
            if(phase==0)
            {
                verifyNative();host->generatorControlWindows.at(firstGenerator).window->setTopLeftPosition(10,90);
                host->generatorControlWindows.at(secondGenerator).window->setTopLeftPosition(550,95);
                auto a=host->mixerWindow->openEffectWindows().at(firstFx),b=host->mixerWindow->openEffectWindows().at(secondFx);
                a->setTopLeftPosition(20,370);b->setTopLeftPosition(550,380);firstBounds=a->getBounds();secondBounds=b->getBounds();
                host->showPlaylist();host->showSongEventEditor();phase=1;startTimer(700);return;
            }
            if(phase==1)
            {
                verifyNative();auto a=host->mixerWindow->openEffectWindows().at(firstFx),b=host->mixerWindow->openEffectWindows().at(secondFx);
                check(a->getBounds()==firstBounds && b->getBounds()==secondBounds,"Opening unrelated panels moved FX windows");
                check(saveNativePeer(*host,juce::File("OPTIONS_CHECK_FOLDER/live-workspaces.png")),"Workspace capture failed");
                b->minimiseButtonPressed();check(!b->isVisible() && a->isVisible(),"Native minimize affected another FX");
                check(host->generatorDockViewport.isVisible(),"Native FX dock missing");phase=2;startTimer(500);return;
            }
            if(phase==2)
            {
                auto b=host->mixerWindow->openEffectWindows().at(secondFx);b->restoreEditor(false);
                host->openGeneratorForRow(0);host->openGeneratorForRow(1);host->stopPlayback();phase=3;startTimer(700);return;
            }
            verifyNative();check(host->mixerWindow->openEffectWindows().at(firstFx)->getBounds()==firstBounds,"Selection/Stop moved native FX");
            std::cout<<"PASS: two real instrument GUIs and two real FX GUIs coexist in Orbit's native peer, have valid Cocoa frames, retain placement across Playlist/Event/selection/Stop, and minimize/restore independently. No audio devices used.\n";
            quit();
        }
        catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}
    }
public:
    const juce::String getApplicationName()override{return "Orbit workspace verification";}
    const juce::String getApplicationVersion()override{return "test";}
    void initialise(const juce::String&)override
    {
        host=std::make_unique<ProjectView>();host->stopTimer();host->setSize(1440,880);shell=std::make_unique<Shell>();shell->setContentNonOwned(host.get(),false);shell->setBounds(20,35,1440,910);shell->setVisible(true);
        juce::MessageManager::callAsync([this]{try{load();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}});
    }
    void shutdown()override{stopTimer();if(shell)shell->clearContentComponent();host.reset();shell.reset();}
};
START_JUCE_APPLICATION(LiveWorkspaceProbe)
