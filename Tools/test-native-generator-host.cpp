// Isolated native-peer check using the production generator surface/editor host.
// Loads only explicitly supplied plugins; no Tracktion Engine, user projects or audio devices.
bool saveNativePeer(juce::Component&,const juce::File&);
bool nativeHitIsJuce(juce::Component&,juce::Point<int>);
bool nativeGeometryMatchesPeer(juce::Component&);
bool nativeMaskStable(juce::Component&,const std::function<void()>&);
static void nativeCheck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
class NativeProbeApp final : public juce::JUCEApplication,private juce::Timer
{
    struct Pane : juce::Component {void paint(juce::Graphics& g) override{g.fillAll(juce::Colours::steelblue);g.setColour(juce::Colours::white);g.drawText("Foreground Mixer / Playlist test",getLocalBounds().removeFromTop(30),juce::Justification::centred);}};
    struct Root : juce::Component
    {
        orbit::generator::GeneratorSurface surface;juce::Component body;juce::Label title;Pane overlay;
        juce::TextButton piano{"Piano Roll"};std::array<juce::TextButton,8> banks,slots;
        std::unique_ptr<juce::AudioPluginInstance> processor;
        std::unique_ptr<OrbitPluginEditorContent> wrapper;bool fitting=false;
        Root()
        {
            addAndMakeVisible(surface);surface.addAndMakeVisible(body);surface.addAndMakeVisible(title);surface.addAndMakeVisible(piano);
            for(int i=0;i<8;++i){banks[i].setButtonText(juce::String::charToString(char('A'+i)));slots[i].setButtonText(juce::String(i+1));surface.addAndMakeVisible(banks[i]);surface.addAndMakeVisible(slots[i]);}
            addChildComponent(overlay);setSize(1440,900);
        }
        ~Root() override{wrapper.reset();processor.reset();}
        void fit()
        {
            if(fitting || !wrapper)return;const juce::ScopedValueSetter<bool> guard(fitting,true);
            const float scale=juce::jmin(1.0f,juce::jmin((getWidth()-100.0f)/wrapper->getPreferredWidth(),(getHeight()-150.0f)/wrapper->getPreferredHeight()));
            const int w=juce::roundToInt(wrapper->getPreferredWidth()*scale),h=juce::roundToInt(wrapper->getPreferredHeight()*scale);
            surface.setBounds(40,40,w,h+86);title.setBounds(12,0,w-24,30);piano.setBounds(225,40,100,28);
            for(int i=0;i<8;++i){banks[i].setBounds(12+i*25,32,23,20);slots[i].setBounds(12+i*25,55,23,20);}
            body.setBounds(0,86,w,h);wrapper->setRequestedScale(scale);wrapper->setBounds(body.getLocalBounds());
        }
        void sync(bool occluded)
        {
            std::vector<juce::Rectangle<int>> blocked;
            if(occluded)blocked.push_back(wrapper->getLocalArea(&overlay,overlay.getLocalBounds()));
            wrapper->syncNativeViews(blocked);
        }
    };
    struct Window : juce::DocumentWindow
    {Window():DocumentWindow("Orbit native plugin verification",juce::Colours::darkgrey,juce::DocumentWindow::closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed() override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<Root> root;std::unique_ptr<Window> window;
    std::vector<juce::String> paths{NATIVE_PLUGIN_PATHS};int plugin=0,phase=0;
    void load()
    {
        const auto start=juce::Time::getMillisecondCounterHiRes();
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> descriptions;format.findAllTypesForFile(descriptions,paths[plugin]);
        nativeCheck(!descriptions.isEmpty(),"VST3 scan returned no descriptions");juce::String error;
        root->wrapper.reset();root->processor.reset();root->overlay.setVisible(false);
        root->processor=format.createInstanceFromDescription(*descriptions[0],44100,512,error);
        nativeCheck(root->processor!=nullptr,error.toRawUTF8());
        nativeCheck(root->getPeer()!=nullptr,"editor creation preceded peer attachment");
        auto editor=std::unique_ptr<juce::AudioProcessorEditor>(root->processor->createEditorIfNeeded());
        if(!editor)editor=std::make_unique<juce::GenericAudioProcessorEditor>(*root->processor);
        root->wrapper=std::make_unique<OrbitPluginEditorContent>(std::move(editor));root->body.addAndMakeVisible(*root->wrapper);
        root->wrapper->preferredSizeChanged=[this]{root->fit();};root->title.setText(descriptions[0]->name,juce::dontSendNotification);root->fit();root->surface.toFront(false);
        std::cout<<"Loaded native "<<descriptions[0]->name<<" in "<<juce::Time::getMillisecondCounterHiRes()-start<<" ms (scan, instance, editor, fit)\n"<<std::flush;phase=0;startTimer(1500);
    }
    void timerCallback() override
    {
        try
        {
            stopTimer();root->sync(phase==2);
            nativeCheck(root->wrapper->nativeClip.attachedCount()>0,"native view has no Cocoa clip host");
            nativeCheck(nativeGeometryMatchesPeer(*root->wrapper),"native frame differs from JUCE peer coordinates");
            nativeCheck(root->piano.getScreenBounds().getBottom()<=root->wrapper->getScreenY(),"toolbar overlaps native editor");
            if(phase==0){phase=1;startTimer(700);return;}
            const auto name="native-"+juce::String(plugin)+"-"+juce::String(phase)+".png";
            nativeCheck(saveNativePeer(*root,juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name)),"native peer capture failed");
            if(phase==1)
            {
                const auto body=root->getLocalArea(root->wrapper.get(),root->wrapper->getLocalBounds());
                root->overlay.setBounds(body.getCentreX(),body.getY()+40,juce::jmin(260,body.getWidth()/2),juce::jmin(300,body.getHeight()-40));
                root->overlay.setVisible(true);root->overlay.toFront(false);root->sync(true);phase=2;startTimer(700);return;
            }
            if(phase==2)
            {
                nativeCheck(nativeHitIsJuce(*root,root->overlay.getBounds().getCentre()),"masked plugin still intercepts foreground panel clicks");
                root->overlay.setVisible(false);root->sync(false);
                const auto start=juce::Time::getMillisecondCounterHiRes();
                for(int i=0;i<60;++i)
                {
                    root->surface.setTopLeftPosition(40+i*2,40+(i%8)*7);
                    // Check BEFORE the clipping adapter gets a chance to repair
                    // coordinates: this catches the old per-event native jump.
                    nativeCheck(nativeGeometryMatchesPeer(*root->wrapper),"JUCE movement temporarily double-offsets the native GUI");root->sync(false);
                }
                std::cout<<"60 native position changes: "<<juce::Time::getMillisecondCounterHiRes()-start<<" ms\n";
                nativeCheck(nativeMaskStable(*root->wrapper,[this]{for(int i=0;i<60;++i)root->sync(false);}),"idle sync rebuilds the native mask");
                root->setSize(1100,720);root->fit();root->sync(false);phase=3;startTimer(700);return;
            }
            nativeCheck(nativeHitIsJuce(*root,root->getLocalPoint(&root->piano,root->piano.getLocalBounds().getCentre())),"full-peer clip host intercepts toolbar clicks");
            std::cout<<"PASS native stage "<<plugin<<": clipped toolbar/body and foreground hit pass-through.\n"<<std::flush;
            if(++plugin<int(paths.size())){load();return;}
            std::cout<<"PASS: supplied VST3 native editors, peer-before-editor creation, Cocoa masks and native hit pass-through. No audio hardware.\n"<<std::flush;quit();
        }
        catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}
    }
public:
    const juce::String getApplicationName() override{return "Orbit UI Verification";}
    const juce::String getApplicationVersion() override{return "test";}
    void initialise(const juce::String&) override
    {
        root=std::make_unique<Root>();window=std::make_unique<Window>();window->setContentNonOwned(root.get(),false);window->setBounds(30,40,1440,925);window->setVisible(true);
        juce::MessageManager::callAsync([this]{try{load();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}});
    }
    void shutdown() override{stopTimer();if(window)window->clearContentComponent();root.reset();window.reset();}
};
START_JUCE_APPLICATION(NativeProbeApp)
