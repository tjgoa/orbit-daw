// Native-peer verification of the production multi-window host. Uses only
// explicitly supplied installed plugins, isolated prefs, no audio devices.
bool saveNativePeer(juce::Component&,const juce::File&);
bool nativeGeometryMatchesPeer(juce::Component&);
bool nativeHitIsJuce(juce::Component&,juce::Point<int>);
class LiveWorkspaceProbe final:public juce::JUCEApplication,private juce::Timer
{
    std::unique_ptr<ProjectView> host;
    class Shell final:public juce::DocumentWindow
    {public:Shell():DocumentWindow("Orbit workspace verification",juce::Colours::black,closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<Shell> shell;
    std::vector<juce::String> paths{NATIVE_PLUGIN_PATHS};
    juce::String firstGenerator,secondGenerator,firstFx,secondFx;int phase=0;
    juce::Rectangle<int> firstBounds,secondBounds;
    std::unique_ptr<orbit::workspace::OwnedAlert> modal;int modalResult=-1;juce::Component::SafePointer<juce::DialogWindow> childDialog;juce::Component::SafePointer<juce::TextButton> dialogDone;
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
                // Native Cocoa editors must remain aligned and clipped while their
                // complete owning frame extends beyond the parent on any edge.
                auto& w=*host->generatorControlWindows.at(firstGenerator).window;
                const auto original=w.getBounds();
                for(const juce::Point<int> position:std::array<juce::Point<int>,3>{{{-200,120},{1300,160},{100,w.workspaceArea().getBottom()-30}}})
                {
                    w.setTopLeftPosition(position);host->resized();verifyNative();
                    check(w.getPosition()==position && w.getWidth()==original.getWidth() && w.getHeight()==original.getHeight(),"Native generator was boxed in or shrunk");
                    check(nativeHitIsJuce(*host,{100,40}),"Off-edge native editor intercepts transport clicks");
                    w.minimiseButtonPressed();check(!w.instrument->isShowing(),"Off-edge native editor leaked while minimized");
                    w.showControls(false);verifyNative();check(w.getPosition()==position,"Native restore lost off-edge bounds");
                }
                w.setBounds(original);verifyNative();
                b->minimiseButtonPressed();check(!b->isVisible() && a->isVisible(),"Native minimize affected another FX");
                check(host->generatorDockViewport.isVisible(),"Native FX dock missing");phase=2;startTimer(500);return;
            }
            if(phase==2)
            {
                auto b=host->mixerWindow->openEffectWindows().at(secondFx);b->restoreEditor(false);
                host->openGeneratorForRow(0);host->openGeneratorForRow(1);host->stopPlayback();phase=3;startTimer(700);return;
            }
            if(phase==3)
            {
                verifyNative();check(host->mixerWindow->openEffectWindows().at(firstFx)->getBounds()==firstBounds,"Selection/Stop moved native FX");
                host->openGeneratorPianoRoll(firstGenerator);host->resized();phase=4;startTimer(500);return;
            }
            auto& frame=*host->generatorControlWindows.at(firstGenerator).window;
            if(phase==4)
            {
                host->syncGeneratorNativeViews();check(frame.isPatternView() && host->pianoRollWindow->isShowing() && !frame.instrument->isShowing(),"Native synth remains visible over Piano Roll");
                check(nativeHitIsJuce(*host,host->getLocalPoint(host->pianoRollWindow.get(),juce::Point<int>{200,180})),"Hidden native synth intercepts Piano Roll clicks");
                check(saveNativePeer(*host,juce::File("OPTIONS_CHECK_FOLDER/hosted-native-piano.png")),"Piano Roll capture failed");
                frame.minimiseButtonPressed();phase=5;startTimer(500);return;
            }
            if(phase==5)
            {
                host->syncGeneratorNativeViews();check(frame.isMinimised() && !host->pianoRollWindow->isShowing() && !frame.instrument->isShowing(),"Native minimized frame leaked one of its bodies");
                frame.showControls();check(frame.isPatternView() && host->pianoRollWindow->isShowing(),"Native restore lost Piano Roll");frame.closeButtonPressed();phase=6;startTimer(500);return;
            }
            if(phase==6)
            {
                verifyNative();check(!frame.isPatternView() && frame.instrument->isShowing(),"Piano Roll X failed to restore native instrument");
                modal=std::make_unique<orbit::workspace::OwnedAlert>("Clone Options","Native peer modal ownership fixture",juce::MessageBoxIconType::NoIcon);modal->addButton("Clone",1);modal->addButton("Cancel",0);modal->setSize(480,180);
                modal->enterModalState(true,juce::ModalCallbackFunction::create([this](int result){modalResult=result;}),false);phase=7;startTimer(500);return;
            }
            if(phase==7)
            {
                host->syncGeneratorNativeViews();check(modal->getParentComponent()==host.get() && modal->isShowing() && modal->isCurrentlyModal(),"Native modal escaped Orbit");
                check(nativeHitIsJuce(*host,modal->getBounds().getCentre()),"Native plugin intercepts modal clicks");
                check(saveNativePeer(*host,juce::File("OPTIONS_CHECK_FOLDER/hosted-native-modal.png")),"Modal capture failed");modal->getButton(0)->triggerClick();phase=8;startTimer(500);return;
            }
            if(phase==8)
            {
                check(modalResult==1 && !modal->isCurrentlyModal(),"Native modal button did not complete");modal.reset();
                juce::DialogWindow::LaunchOptions options;options.dialogTitle="Owned settings fixture";options.componentToCentreAround=host.get();
                auto* done=new juce::TextButton("Done");done->setSize(400,150);dialogDone=done;options.content.setOwned(done);
                childDialog=orbit::workspace::ownDialog(options.launchAsync(),host.get());done->onClick=[this]{if(childDialog!=nullptr)childDialog->exitModalState(1);};phase=9;startTimer(500);return;
            }
            if(phase==9)
            {
                check(childDialog!=nullptr && childDialog->getParentComponent()==host.get() && !childDialog->isOnDesktop() && childDialog->isCurrentlyModal(),"Settings form escaped Orbit");
                check(nativeHitIsJuce(*host,childDialog->getBounds().getCentre()),"Native editor intercepts settings form");dialogDone->triggerClick();phase=10;startTimer(500);return;
            }
            check(childDialog==nullptr,"Settings form did not close cleanly");
            host->createProjectDiscardingCurrent();check(host->generatorControlWindows.empty() && host->mixerWindow->openEffectWindows().empty() && !host->playlistWindow->isVisible() && !host->mixerWindow->isVisible(),"New Project left native document windows");
            std::cout<<"PASS: native generator left/right/bottom overflow with geometry/clipping/minimize recovery; two real instrument and two FX GUIs coexist; native geometry, foreground clipping, transport, one-frame Piano Roll, minimize/restore/X, owned modal hit testing/button completion and blank New Project. No audio devices.\n";
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
