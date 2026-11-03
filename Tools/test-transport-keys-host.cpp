void postNativeSpace(juce::Component&,bool,unsigned long);
bool focusNativeEditor(juce::Component&);
void activateNativeWindow(juce::Component&);
void installNativeTextInput(juce::Component&);
juce::String nativeTextInputValue();
void removeNativeTextInput(juce::Component&);
class TransportProbe final:public juce::JUCEApplication,private juce::Timer
{
    struct Shell:juce::DocumentWindow{Shell():DocumentWindow("Orbit transport verification",juce::Colours::black,closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<ProjectView> host;std::unique_ptr<Shell> shell;
    juce::String acid,vst;int phase=0;int originalCount=0;
    static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
    orbit::generator::ControlWindow& acidWindow(){return *host->generatorControlWindows.at(acid).window;}
    void space(bool repeat=false){postNativeSpace(*host,repeat,0);}
    void timerCallback()override
    {
      try
      {
        stopTimer();
        if(phase==0)
        {
            auto* native=dynamic_cast<OrbitPluginEditorContent*>(host->generatorControlWindows.at(vst).window->instrument.get());check(native!=nullptr && focusNativeEditor(*native),"VST native view could not receive keyboard focus");space();phase=1;
        }
        else if(phase==1){check(host->edit->getTransport().isPlaying(),"native VST swallowed Space Play");space(true);phase=2;}
        else if(phase==2){check(host->edit->getTransport().isPlaying(),"Space autorepeat toggled transport");space();phase=3;}
        else if(phase==3)
        {
            check(!host->edit->getTransport().isPlaying(),"native VST swallowed Space Stop");installNativeTextInput(*host);space();phase=4;
        }
        else if(phase==4)
        {
            std::cout<<"Native text input: playing="<<host->edit->getTransport().isPlaying()<<" value=["<<nativeTextInputValue()<<"]"<<'\n';
            check(!host->edit->getTransport().isPlaying() && nativeTextInputValue()=="Text ","Space stole native text entry or started playback while typing");removeNativeTextInput(*host);host->openGeneratorPianoRoll(acid);acidWindow().closeButtonPressed();
            check(!acidWindow().isPatternView() && host->project.getTrackCount()==originalCount,"Piano Roll X deleted the instrument");acidWindow().minimiseButtonPressed();check(acidWindow().isMinimised() && host->project.getTrackCount()==originalCount,"Minimize deleted the instrument");acidWindow().showControls();host->selectGeneratorControlTrack(vst);acidWindow().closeButtonPressed();phase=5;
        }
        else if(phase==5)
        {
            auto* alert=dynamic_cast<orbit::workspace::OwnedAlert*>(juce::Component::getCurrentlyModalComponent());check(alert!=nullptr && alert->getName()=="Delete Generator","generator X did not open its deletion confirmation");check(!host->handleEditShortcut(juce::KeyPress(juce::KeyPress::spaceKey)),"host consumed transport Space behind a modal");phase=6;
        }
        else if(phase==6)
        {
            check(!host->edit->getTransport().isPlaying(),"Space started playback behind a modal confirmation");auto* alert=dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());check(alert!=nullptr,"Space dismissed confirmation unexpectedly");alert->getButton(1)->triggerClick();phase=7;
        }
        else if(phase==7){check(host->project.getTrackCount()==originalCount && acidWindow().isVisible(),"Cancel deleted or hid generator");acidWindow().closeButtonPressed();phase=8;}
        else if(phase==8){auto* alert=dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());check(alert!=nullptr,"second delete confirmation missing");alert->getButton(0)->triggerClick();phase=9;}
        else
        {
            check(host->project.getTrackCount()==originalCount-1 && !host->generatorControlWindows.contains(acid) && host->generatorControlWindows.at(vst).window->isVisible(),"Delete removed wrong generator or affected unrelated VST");host->undoProjectAction();check(host->project.getTrackCount()==originalCount && host->project.getPatternNotes(acid,0).size()==1 && host->canOpenGeneratorControls(acid),"Undo failed to restore deleted generator and pattern");
            std::cout<<"PASS: native VST-focused Space Play/Stop, autorepeat suppression, normal Space text entry, modal protection; Piano Roll X returns to GUI, minimize retains generator, GUI X confirms deletion, Cancel preserves window, Delete targets owner and Undo restores notes.\n";quit();return;
        }
        startTimer(250);
      }
      catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';setApplicationReturnValue(1);quit();}
    }
public:
    const juce::String getApplicationName()override{return "Orbit transport verification";}
    const juce::String getApplicationVersion()override{return "test";}
    void initialise(const juce::String&)override
    {
        host=std::make_unique<ProjectView>();host->stopTimer();host->setSize(1440,900);shell=std::make_unique<Shell>();shell->setContentNonOwned(host.get(),false);shell->setSize(1440,930);shell->setVisible(true);activateNativeWindow(*host);
        juce::MessageManager::callAsync([this]
        {
          try
          {
            auto& audio=host->engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters p;p.inputChannels=0;p.outputChannels=2;audio.initialise(p);audio.prepareToPlay(44100,512);
            host->createProjectDiscardingCurrent();host->addInstrumentTrack(true);acid=host->selectedProjectTrackId();host->project.setPatternCreated(acid,0,true);host->project.appendPatternNoteTicks(acid,0,48,0,128,100);host->refreshEnginePlaylist();
            const std::vector<juce::String> paths{NATIVE_PLUGIN_PATHS};check(!paths.empty(),"Supply one instrument fixture");juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,paths[0]);check(!found.isEmpty(),"VST scan failed");host->addVst3Instrument(*found[0]);vst=host->selectedProjectTrackId();originalCount=host->project.getTrackCount();startTimer(800);
          }
          catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';setApplicationReturnValue(1);quit();}
        });
    }
    void shutdown()override{stopTimer();if(host)removeNativeTextInput(*host);if(shell)shell->clearContentComponent();host.reset();shell.reset();}
};
START_JUCE_APPLICATION(TransportProbe)
