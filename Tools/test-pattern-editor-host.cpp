static void patternCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::MouseEvent patternMouse(juce::Component* grid,float x,float y,int flags=juce::ModifierKeys::leftButtonModifier,juce::Point<float> down={},bool drag=false)
{
    juce::Point<float> p{x,y};return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(flags),1,0,0,0,0,grid,grid,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};
}
struct PatternMidiObserver : te::AudioTrack::Listener
{
    std::vector<juce::MidiMessage> messages;
    void injectLiveMidiMessage(te::AudioTrack&,const te::MidiMessageWithSource& m,bool& used) override{messages.push_back(m);used=true;}
    void recordedMidiMessageSentToPlugins(te::AudioTrack&,const juce::MidiMessage&) override{}
};
struct PatternDummyEditor : juce::AudioProcessorEditor
{
    explicit PatternDummyEditor(juce::AudioProcessor& processor):AudioProcessorEditor(processor){setSize(500,500);}
    void paint(juce::Graphics& g) override{g.fillAll(juce::Colours::cornflowerblue);}
};
struct PatternNativeSizeEditor : PatternDummyEditor
{
    explicit PatternNativeSizeEditor(juce::AudioProcessor& p,bool ignores):PatternDummyEditor(p),ignoresScale(ignores){setSize(1200,1000);}
    void setScaleFactor(float scale) override {if(!ignoresScale)setSize(juce::roundToInt(1200*scale),juce::roundToInt(1000*scale));}
    bool ignoresScale;
};
#include "test-pattern-clipboard-host.cpp"
int main(int argc,char**)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;juce::AudioProcessorGraph processor;PatternMidiObserver observer;ProjectView host;host.stopTimer();host.setSize(1440,900);
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/pattern-section.orbit");
        if(argc>1)
        {
            patternCheck(host.loadProjectFile(file),"fresh-process project load failed");
            auto first=host.getGeneratorTracks()[0]->state.getChildWithName("PIANOROLL");
            auto second=host.getGeneratorTracks()[1]->state.getChildWithName("PIANOROLL");
            patternCheck(first.isValid() && second.isValid() && int(first.getProperty("divisions"))==8 && int(second.getProperty("divisions"))==32,"generator preferences did not survive fresh-process native reopen");
            std::cout<<"PASS: independent generator Piano Roll preferences restored from native project in fresh process.\n";return 0;
        }
        std::cout<<"CHECK: native container\n"<<std::flush;
        host.createProjectDiscardingCurrent();host.addInstrumentTrack();
        patternCheck(dynamic_cast<OrbitNativeInstrumentEditor*>(host.activeInstrumentEditor.get())!=nullptr,"built-in synth has no attached editor");
        if(!host.pianoRollWindow->isVisible())host.showPianoRoll();
        auto* window=host.pianoRollWindow.get();auto* grid=window->grid;
        const auto id=host.selectedProjectTrackId();
        patternCheck(host.pianoRollMaximized && host.generatorPanelBounds==juce::Rectangle<int>(0,82,1440,796),"maximized Piano Roll retained wallpaper padding");
        const auto floatingBounds=host.pianoRollRestoreBounds;
        host.setVisible(true); // Enable real hit-testing without adding a native window.
        host.setSize(1800,1100);
        patternCheck(host.generatorPanelBounds==juce::Rectangle<int>(0,82,1800,996),"maximized Piano Roll failed to follow app resize");
        host.generatorPanelBounds={14,92,1500,800};host.resized();
        patternCheck(host.generatorPanelBounds==juce::Rectangle<int>(0,82,1800,996),"old saved inset bounds overrode maximized coverage");
        host.wallpaperImage=juce::Image(juce::Image::RGB,4,4,true);{juce::Graphics g(host.wallpaperImage);g.fillAll(juce::Colours::lime);}host.setWallpaper(3);
        for(int skin:{5,6})
        {
            host.setSkin(skin);auto image=host.createComponentSnapshot(host.getLocalBounds());
            auto* bottom=host.getComponentAt(50,1060);
            patternCheck(bottom==&host.generatorBody || host.generatorBody.isParentOf(bottom),"generator dock overlaps maximized Piano Roll controls");
            for(auto point:{juce::Point<int>(0,83),{1799,500},{800,1077}})
                patternCheck(image.getPixelAt(point.x,point.y)!=juce::Colours::lime,"wallpaper visible around maximized Piano Roll");
            juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/piano-roll-maximized-")+juce::String(skin)+".png"));
            juce::PNGImageFormat png;patternCheck(png.writeImageToStream(image,out),"maximized Piano Roll render failed");
        }
        patternCheck(host.generatorMaximizeButton.isVisible(),"Piano Roll maximize/restore control hidden");
        host.generatorMaximizeButton.onClick();
        patternCheck(!host.pianoRollMaximized && window->isVisible() && !host.instrumentEditorClip.isVisible(),"Restore left Piano Roll or showed synth");
        auto restored=host.generatorPanelBounds;restored.setSize(restored.getWidth()-60,restored.getHeight()-40);host.generatorPanelBounds=restored;host.resized();host.savePianoRollState();
        host.generatorMaximizeButton.onClick();host.generatorMaximizeButton.onClick();
        patternCheck(host.generatorPanelBounds==restored,"maximize lost floating roll size");
        host.generatorMaximizeButton.onClick();
        std::cout<<"CHECK: generator window stacking and hosted drag\n"<<std::flush;
        patternCheck(host.generatorBody.getParentComponent()==&host.generatorSurface
            && host.generatorDragHandle.getParentComponent()==&host.generatorSurface
            && host.patternBankButtons[0]->getParentComponent()==&host.generatorSurface,"generator chrome and body are separate workspace siblings");
        host.generatorMaximizeButton.onClick();host.generatorPanelBounds={100,120,800,650};host.resized();
        host.showMixer();host.mixerPanelBounds={400,160,350,600};host.mixerBoundsInitialised=true;host.resized();
        host.showPlaylist();host.playlistPanelBounds={400,180,650,450};host.playlistWindow->setMaximized(false);host.resized();
        host.playlistWindow->toFront(false);host.resized();
        patternCheck(host.getIndexOfChildComponent(host.playlistWindow.get())>host.getIndexOfChildComponent(&host.generatorSurface),"layout stole Playlist focus");
        const auto startingPanel=host.generatorPanelBounds;
        const auto originalPointer=host.getLocalPoint(window,juce::Point<int>(170,12));
        auto downPanel=patternMouse(window,170,12);window->mouseDown(downPanel);
        const auto layoutsBeforeDrag=checkedWorkspaceLayouts;
        for(auto delta:{juce::Point<int>(100,20),{180,40},{80,-10}})
        {
            const auto local=window->getLocalPoint(&host,originalPointer+delta);
            const auto downLocal=window->getLocalPoint(&host,originalPointer);
            auto event=patternMouse(window,float(local.x),float(local.y),juce::ModifierKeys::leftButtonModifier,downLocal.toFloat(),true);window->mouseDrag(event);
            host.mouseDrag(event); // Recursive parent listener sees the same event after the child moved.
            patternCheck(host.generatorPanelBounds.getPosition()==startingPanel.getPosition()+delta,"Piano Roll title drag accumulated offsets or split from generator");
            patternCheck(window->getPosition()==juce::Point<int>(0,0) && host.generatorBody.getY()==86,"drag moved the Piano Roll inside its body");
            patternCheck(host.getIndexOfChildComponent(&host.generatorSurface)>host.getIndexOfChildComponent(host.playlistWindow.get())
                && host.getIndexOfChildComponent(&host.generatorSurface)>host.getIndexOfChildComponent(host.mixerWindow.get()),"generator interleaved with Mixer/Playlist during drag");
        }
        window->mouseUp(downPanel);
        patternCheck(checkedWorkspaceLayouts==layoutsBeforeDrag,"title dragging reflows the whole workspace");
        patternCheck(host.generatorSurface.getBounds()==host.generatorPanelBounds,"fast drag left the surface behind");
        patternCheck(host.pianoRollBounds==host.generatorBody.getBounds().translated(host.generatorPanelBounds.getX(),host.generatorPanelBounds.getY()),"fast drag lost persisted Piano Roll position");
        {
            auto* title=&host.generatorDragHandle;const auto start=host.generatorPanelBounds.getPosition();
            const auto pointer=host.getLocalPoint(title,juce::Point<int>(120,12));auto press=patternMouse(title,120,12);title->mouseDown(press);
            const auto delta=juce::Point<int>(65,23);const auto local=title->getLocalPoint(&host,pointer+delta);
            auto drag=patternMouse(title,float(local.x),float(local.y),juce::ModifierKeys::leftButtonModifier,{120,12},true);
            title->mouseDrag(drag);host.mouseDrag(drag);title->mouseUp(drag);host.mouseUp(drag);
            patternCheck(host.generatorPanelBounds.getPosition()==start+delta,"outer title and recursive listener double-moved generator");
            patternCheck(checkedWorkspaceLayouts==layoutsBeforeDrag,"outer title reflows workspace");
        }
        host.mixerWindow->toFront(false);host.resized();
        auto* mixerHit=host.getComponentAt(450,250);
        patternCheck(mixerHit==host.mixerWindow.get() || host.mixerWindow->isParentOf(mixerHit),"foreground mixer is behind part of generator");
        host.bringGeneratorToFront();auto* generatorHit=host.getComponentAt(450,250);
        patternCheck(generatorHit==&host.generatorSurface || host.generatorSurface.isParentOf(generatorHit),"activated generator is not one foreground window");
        host.mixerWindow->setVisible(false);host.playlistWindow->setVisible(false);host.pianoRollMaximized=true;host.resized();
        host.setSize(1440,900);host.closePianoRollView();
        patternCheck(host.generatorPanelBounds==floatingBounds && !host.pianoRollMaximized,"closing maximized Piano Roll lost instrument bounds");
        host.showPianoRoll();host.setSkin(5);
        patternCheck(window->getParentComponent()==&host.generatorBody && host.instrumentEditorClip.getParentComponent()==&host.generatorBody,"GUI/Piano Roll have separate parents");
        patternCheck(window->getBounds()==host.generatorBody.getLocalBounds() && !host.instrumentEditorClip.isVisible(),"Piano Roll not contained or instrument still visible");
        for(auto n:std::vector<orbit::editing::Note>{{60,0,128,60},{64,0,128,90},{67,0,128,110}})host.project.appendPatternNoteTicks(id,0,n.pitch,n.startTick,n.lengthTicks,n.velocity);
        host.project.setPatternCreated(id,0,true);host.readPatternSteps();grid->centrePitch(63);
        auto point=[&](int pitch,int step){return juce::Point<float>{float(grid->labelWidth()+step*grid->stepWidth()+grid->stepWidth()/4),float(grid->yForPitch(pitch)+grid->rowHeight()/2)};};
        auto click=[&](int pitch,int step,int flags)
        {auto p=point(pitch,step);auto e=patternMouse(grid,p.x,p.y,flags);grid->mouseDown(e);grid->mouseUp(e);};
        const auto original=host.capturePatternData(id,0).notes;const auto undo=host.undoHistory.size();
        patternCheck(window->velocityEditButton.isVisible() && window->velocityEditButton.getWidth()>0,"VEL control is hidden");
        window->velocityEditButton.setToggleState(true,juce::dontSendNotification);window->velocityEditButton.onClick();auto p=point(60,0);auto down=patternMouse(grid,p.x,p.y);grid->mouseDown(down);
        auto velocity=patternMouse(grid,p.x,p.y-20,juce::ModifierKeys::leftButtonModifier,p,true);grid->mouseDrag(velocity);grid->mouseUp(velocity);
        patternCheck(host.project.getPatternNoteVelocity(id,0,60,0)==80 && host.project.getPatternNoteVelocity(id,0,64,0)==90,"VEL changed all chord members");
        patternCheck(host.undoHistory.size()==undo+1,"VEL gesture has more than one Undo");host.undoProjectAction();patternCheck(host.capturePatternData(id,0).notes==original,"VEL Undo lost note data");
        window->velocityEditButton.setToggleState(false,juce::dontSendNotification);window->velocityEditButton.onClick();grid->centrePitch(63);
        for(int pitch:{60,64})click(pitch,0,juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::shiftModifier);
        patternCheck(host.selectedNoteKeys.size()==2,"chord selection failed");
        auto groupStroke=[&](bool copy)
        {
            auto start=point(60,0);const int flags=juce::ModifierKeys::leftButtonModifier|(copy?juce::ModifierKeys::ctrlModifier:0);
            auto press=patternMouse(grid,start.x,start.y,flags);grid->mouseDown(press);
            auto move=patternMouse(grid,start.x+grid->stepWidth(),point(61,1).y,flags,start,true);grid->mouseDrag(move);grid->mouseUp(move);
        };
        groupStroke(false);patternCheck(host.project.getPatternNote(id,0,61,1) && host.project.getPatternNote(id,0,65,1) && host.project.getPatternNote(id,0,67,0),"drag did not move selected group independently");
        host.undoProjectAction();patternCheck(host.capturePatternData(id,0).notes==original,"group Undo changed phrase");
        groupStroke(true);patternCheck(host.project.getPatternNotes(id,0).size()==5 && host.selectedNoteKeys.size()==2,"Ctrl-drag did not duplicate selected notes");host.undoProjectAction();
        host.clearNoteSelection();grid->centrePitch(63);window->rectangleButton.onClick();
        auto rectangleStart=point(65,3);auto rectangleDown=patternMouse(grid,rectangleStart.x,rectangleStart.y);grid->mouseDown(rectangleDown);
        auto rectangleEnd=patternMouse(grid,float(grid->labelWidth()),point(59,0).y,juce::ModifierKeys::leftButtonModifier,rectangleStart,true);grid->mouseDrag(rectangleEnd);grid->mouseUp(rectangleEnd);
        patternCheck(host.selectedNoteKeys.size()==2 && host.project.getPatternNotes(id,0).size()==3,"Rectangle tool edited notes or selected wrong group");
        window->drawButton.onClick();host.clearNoteSelection();
        host.selectPatternSlot(1);grid->centrePitch(60);auto brushStart=point(60,0);
        auto brushDown=patternMouse(grid,brushStart.x,brushStart.y,juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::altModifier);grid->mouseDown(brushDown);
        auto brushDrag=patternMouse(grid,brushStart.x+4*grid->stepWidth(),brushStart.y,juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::altModifier,brushStart,true);grid->mouseDrag(brushDrag);grid->mouseUp(brushDrag);
        patternCheck(host.project.getPatternNotes(id,1).size()==5,"Alt brush stretched one note or skipped cells");host.undoProjectAction();patternCheck(host.project.getPatternNotes(id,1).empty(),"brush Undo left notes");
        host.selectPatternSlot(0);host.clearNoteSelection();
        host.handleEditShortcut(juce::KeyPress(juce::KeyPress::upKey),true);patternCheck(host.project.getPatternNote(id,0,61,0),"arrow semitone shortcut failed");host.undoProjectAction();
        host.handleEditShortcut(juce::KeyPress('s',juce::ModifierKeys{},'s'),true);patternCheck(host.project.getPatternNotes(id,0).size()==6,"Scissor shortcut failed");host.undoProjectAction();
        host.handleEditShortcut(juce::KeyPress('x',juce::ModifierKeys{},'x'),true);patternCheck(host.project.getPatternNotes(id,0)[0].startTick==896,"horizontal flip shortcut failed");host.undoProjectAction();
        host.handleEditShortcut(juce::KeyPress(juce::KeyPress::rightKey),true);patternCheck(host.project.getPatternNote(id,0,60,1),"rotation shortcut failed");host.undoProjectAction();
        std::cout<<"CHECK: cell hitboxes and velocity limits\n"<<std::flush;
        {
            PianoRollGrid cells;int createdPitch=-1,createdTick=-1;
            cells.getActiveStepCount=[] {return 16;};cells.getNotes=[] {return std::vector<ProjectDocument::PatternNote>{};};
            cells.noteClicked=[&](int pitch,int tick,bool){createdPitch=pitch;createdTick=tick;};cells.setSize(3000,650);
            for(int divisions:{8,16,32,64})for(float zoom:{0.7f,1.12f,1.6f})
            {
                cells.setZoom(zoom,zoom);cells.setGridOptions(divisions,4);cells.setPitchScrollPosition(50.37);
                const int interval=1024/divisions;
                for(int pitch:{60,64,67,72})for(int cell:{0,3,6,10,15})for(double fraction:{0.01,0.51,0.99})
                {
                    const double tick=(cell+fraction)*interval;if(tick>=1024)continue;
                    for(int inset:{0,cells.rowHeight()-1})
                    {
                        const float x=float(cells.labelWidth()+tick/64.0*cells.stepWidth());
                        const float y=float(cells.yForPitch(pitch)+inset);createdTick=-1;createdPitch=-1;
                        auto e=patternMouse(&cells,x,y);cells.mouseDown(e);cells.mouseUp(e);
                        if(createdTick!=cell*interval || createdPitch!=pitch)std::cerr<<"cell failure divisions="<<divisions<<" zoom="<<zoom<<" pitch="<<pitch<<" cell="<<cell<<" fraction="<<fraction<<" inset="<<inset<<" x/y="<<x<<"/"<<y<<" got="<<createdPitch<<"/"<<createdTick<<" expected="<<cell*interval<<" row="<<cells.rowHeight()<<" scroll="<<cells.firstPitchRow<<"\n";
                        patternCheck(createdTick==cell*interval && createdPitch==pitch,"cell click rounded forward, missed or selected adjacent pitch");
                    }
                }
            }
        }
        patternCheck(window->drawButton.getY()<window->viewport.getY() && window->rectangleButton.getY()<window->viewport.getY(),"Draw/Select tools not above grid");
        host.selectPatternSlot(0);host.readPatternSteps();grid->centrePitch(63);window->setSelectionTool(false);
        window->velocityEditButton.setToggleState(true,juce::dontSendNotification);window->velocityEditButton.onClick();
        auto velStart=point(60,0);auto velDown=patternMouse(grid,velStart.x,velStart.y);grid->mouseDown(velDown);
        auto velLow=patternMouse(grid,velStart.x,velStart.y+500,juce::ModifierKeys::leftButtonModifier,velStart,true);grid->mouseDrag(velLow);
        patternCheck(host.project.getPatternNoteVelocity(id,0,60,0)==1,"note velocity cannot reach 1");
        auto velHigh=patternMouse(grid,velStart.x,velStart.y-500,juce::ModifierKeys::leftButtonModifier,velStart,true);grid->mouseDrag(velHigh);grid->mouseUp(velHigh);
        patternCheck(host.project.getPatternNoteVelocity(id,0,60,0)==127,"note velocity cannot reach 127");host.undoProjectAction();
        window->velocityButton.setToggleState(true,juce::dontSendNotification);window->velocityButton.onClick();
        window->velocityEditButton.setToggleState(true,juce::dontSendNotification);window->velocityEditButton.onClick();
        grid->setVelocityEditEnabled(false);
        auto handleFor=[&](int pitch,int tick)
        {
            for(const auto& handle:grid->velocityHandles(host.project.getPatternNotes(id,0)))
                if(handle.note.pitch==pitch && handle.note.startTick==tick)return handle;
            throw std::runtime_error("missing per-note velocity handle");
        };
        const auto lanePoint=juce::Point<float>{handleFor(60,0).x,float(grid->velocityPlotTop())};
        auto laneDown=patternMouse(grid,lanePoint.x,lanePoint.y);grid->mouseDown(laneDown);
        patternCheck(host.project.getPatternNoteVelocity(id,0,60,0)==127,"lane top not full velocity");
        patternCheck(host.project.getPatternNoteVelocity(id,0,64,0)==90 && host.project.getPatternNoteVelocity(id,0,67,0)==110,"lane edit changed other chord notes");
        patternCheck(std::abs(grid->velocityY(handleFor(60,0).note.velocity)-grid->velocityPlotTop())<0.01f,"chord averaging still suppresses 127 bar height");
        patternCheck(handleFor(60,0).x<handleFor(64,0).x && handleFor(64,0).x<handleFor(67,0).x,"chord velocity handles overlap instead of separating");
        auto laneLow=patternMouse(grid,lanePoint.x,float(grid->velocityPlotBottom()),juce::ModifierKeys::leftButtonModifier,lanePoint,true);grid->mouseDrag(laneLow);grid->mouseUp(laneLow);
        patternCheck(host.project.getPatternNoteVelocity(id,0,60,0)==1,"lane bottom cannot reach velocity 1");
        patternCheck(std::abs(grid->velocityY(handleFor(60,0).note.velocity)-grid->velocityPlotBottom())<0.01f,"velocity 1 bar is averaged with chord");
        patternCheck(grid->velocityFocusPitch==60 && grid->velocityFocusTick==0,"velocity readout follows wrong note");
        const auto laneUndoCount=host.undoHistory.size();host.undoProjectAction();
        patternCheck(host.capturePatternData(id,0).notes==original && host.undoHistory.size()+1==laneUndoCount,"per-note lane Undo changed chord");
        // Reproduce the user's video: one note at 127, another at 1 on the same step.
        for(int pitch:{60,64,67})
        {
            const auto handle=handleFor(pitch,0);const int value=pitch==60?127:pitch==64?1:64;
            auto e=patternMouse(grid,handle.x,grid->velocityY(value));grid->mouseDown(e);grid->mouseUp(e);
        }
        for(int skin:{5,6})
        {
            host.setSkin(skin);auto image=host.createComponentSnapshot(host.getLocalBounds());
            juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/velocity-chord-")+juce::String(skin)+".png"));
            juce::PNGImageFormat png;patternCheck(png.writeImageToStream(image,out),"velocity chord render failed");
        }
        patternCheck(handleFor(60,0).note.velocity==127 && handleFor(64,0).note.velocity==1 && handleFor(67,0).note.velocity==64,"per-note lane values were merged");
        host.undoProjectAction();host.undoProjectAction();host.undoProjectAction();
        patternCheck(host.capturePatternData(id,0).notes==original,"per-note lane strokes failed Undo");
        // Fractional onsets get independent handles rather than a rounded step average.
        {const auto handles=grid->velocityHandles({{60,16,16,1},{64,16,16,127},{67,32,16,64}});
        patternCheck(handles.size()==3 && handles[0].x<handles[1].x && handles[1].x<handles[2].x,"fractional-onset velocity layout lost notes");}

        window->velocityButton.setToggleState(false,juce::dontSendNotification);window->velocityButton.onClick();window->setSelectionTool(false);
        orbit::editing::HotkeyGuide guide;guide.search.setText("brush",false);guide.refresh();
        patternCheck(guide.text.getText().contains("Alt-drag") && !guide.text.getText().contains("Transpose"),"hotkey search wrong");
        guide.search.setText("",false);guide.refresh();patternCheck(guide.text.getText().contains("Ctrl-Q"),"hotkey guide missing implemented shortcut");
        std::cout<<"CHECK: preferences and ghosts\n"<<std::flush;
        auto prefs=window->getPreferences();prefs.setProperty("divisions",8,nullptr);prefs.setProperty("ghosts",true,nullptr);prefs.setProperty("audition",true,nullptr);prefs.setProperty("names",true,nullptr);window->setPreferences(prefs);host.saveSelectedPianoPreferences();
        host.addInstrumentTrack();const auto other=host.selectedProjectTrackId();host.selectPatternSlot(2);host.project.appendPatternNoteTicks(other,2,72,64,64,100);host.project.setPatternCreated(other,2,true);
        auto otherPrefs=window->getPreferences();otherPrefs.setProperty("divisions",32,nullptr);otherPrefs.setProperty("ghosts",false,nullptr);window->setPreferences(otherPrefs);host.saveSelectedPianoPreferences();
        host.trackSelector.setSelectedId(1,juce::dontSendNotification);host.selectInstrumentTrack();if(!host.pianoRollWindow->isVisible())host.showPianoRoll();
        patternCheck(grid->getSubdivisionsPerBar()==8 && grid->areGhostNotesVisible(),"generator switch lost Piano Roll preferences");
        const auto ghosts=grid->getGhostNotes();patternCheck(ghosts.size()==1 && ghosts[0].first.pitch==72,"ghosts did not use other generator's remembered pattern");
        auto navigation=window->getPreferences();navigation.setProperty("follow",true,nullptr);navigation.setProperty("cursor",false,nullptr);window->setPreferences(navigation);
        window->setSize(700,600);window->setPlayheadStep(40,true);
        patternCheck(window->viewport.getViewPositionX()>0 && !grid->playheadVisible,"follow/cursor settings are inactive");
        window->setPreferences(prefs);window->resized();grid->centrePitch(62);
        host.instrumentTrack->getListeners().add(&observer);
        patternCheck(host.prepareMidiRecording(0,juce::Time::getMillisecondCounterHiRes()),"audition recording fixture failed");
        click(62,4,juce::ModifierKeys::leftButtonModifier);
        patternCheck(std::any_of(observer.messages.begin(),observer.messages.end(),[](const auto& m){return m.isNoteOn() && m.getNoteNumber()==62;})
            && std::any_of(observer.messages.begin(),observer.messages.end(),[](const auto& m){return m.isNoteOff() && m.getNoteNumber()==62;}),"grid audition did not pair note on/off");
        patternCheck(host.midiTake.preview(id.toStdString(),100).empty(),"grid audition was recorded as duplicate input");
        host.finishMidiRecording();host.instrumentTrack->getListeners().remove(&observer);host.undoProjectAction();
        std::cout<<"CHECK: LFO preview\n"<<std::flush;
        const auto beforeLfo=host.project.getPatternAutomationLaneCount(id,0);auto points=orbit::editing::lfoPoints(16,orbit::editing::LfoShape::triangle,2,0,0.5,0.5);
        window->previewPatternLfo(points);patternCheck(host.project.getPatternAutomationLaneCount(id,0)==beforeLfo,"LFO preview edited document");window->previewPatternLfo({});
        window->applyPatternLfo(points);patternCheck(host.project.getPatternAutomationLaneCount(id,0)==beforeLfo+1,"LFO did not commit selected automation lane");
        te::AutomatableParameter* curveParameter=nullptr;
        for(auto* plugin:host.instrumentTrack->pluginList)if(plugin->getName()==host.automationPluginName)
            for(auto* parameter:plugin->getAutomatableParameters())if(parameter->paramID==host.automationParameterId)curveParameter=parameter;
        patternCheck(curveParameter!=nullptr && curveParameter->getCurve().getNumPoints()==16,"LFO engine curve was not applied");
        host.selectPatternSlot(6);patternCheck(curveParameter->getCurve().getNumPoints()==0,"previous pattern automation leaked into blank pattern");
        host.selectPatternSlot(0);patternCheck(curveParameter->getCurve().getNumPoints()==16,"pattern automation was not restored on return");
        host.undoProjectAction();
        patternCheck(host.project.getPatternAutomationLaneCount(id,0)==beforeLfo,"LFO Undo did not restore lanes");
        std::cout<<"CHECK: Rack switching\n"<<std::flush;
        host.addDrumRackTrack();patternCheck(host.drumRackWindow->getParentComponent()==&host.generatorBody && host.drumRackWindow->embedded,"Rack not contained");
        patternCheck(host.generatorSettingsButton.isVisible() && host.pianoRollButton.isVisible(),"Rack missing shared settings/Piano Roll controls");
        if(window->isVisible())host.closePianoRollView();if(!host.pianoRollWindow->isVisible())host.showPianoRoll();
        patternCheck(window->isVisible() && !host.drumRackWindow->isVisible() && !host.instrumentEditorClip.isVisible(),"Rack/GUI overlays Piano Roll");
        host.closePianoRollView();patternCheck(host.drumRackWindow->isVisible(),"return to Drum Rack failed");
        host.trackSelector.setSelectedId(1,juce::dontSendNotification);host.selectInstrumentTrack();
        patternCheck(!host.drumRackWindow->isVisible(),"old Rack remained over another instrument");
        std::cout<<"CHECK: long LFO persistence\n"<<std::flush;
        // Large patterns keep automation beyond the former 4095-step boundary.
        host.selectPatternSlot(1);host.project.setPatternLengthSteps(id,1,8192);
        const auto longLfo=orbit::editing::lfoPoints(8192,orbit::editing::LfoShape::sine,4,0,0.5,0.5);
        window->applyPatternLfo(longLfo);auto longLane=host.project.getPatternAutomationLane(id,1,0);
        patternCheck(longLane.points.size()==4096 && longLane.points.getLast().position==8191,"long LFO was clamped to 4095 steps");
        // Draw each supported non-note MIDI event into an initially blank slot.
        host.selectPatternSlot(4);host.auditionPatternPlayback=true;
        for(int target:{2,3,4})
        {
            window->automationTargetSelector.setSelectedId(target,juce::sendNotificationSync);
            window->onAutomationGestureStarted();window->automationPointChanged(0,0.25f);window->automationPointChanged(15,0.75f);window->onAutomationGestureEnded();
        }
        patternCheck(host.project.isPatternCreated(id,4) && host.hasPattern && host.patternClip!=nullptr
            && host.patternClip->getSequence().getNumControllerEvents()>=48,"automation-only MIDI pattern did not generate controller playback");
        host.auditionPatternPlayback=false;host.selectPatternSlot(0);
        patternCheck(window->automationTargetSelector.getSelectedId()==4,"pattern change reset the chosen MIDI event target");
        host.saveToFile(file);patternCheck(host.loadProjectFile(file),"Patterns native reopen failed");
        patternCheck(host.project.getPatternAutomationLane(id,1,0).points.getLast().position==8191,"native reopen lost long automation");
        if(!host.pianoRollWindow->isVisible())host.showPianoRoll();
        for(int skin:{5,6})
        {
            host.setSkin(skin);grid->centrePitch(65);auto image=window->createComponentSnapshot(window->getLocalBounds());
            juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/pattern-editor-")+juce::String(skin)+".png"));juce::PNGImageFormat png;patternCheck(png.writeImageToStream(image,out),"Patterns render failed");
        }
        std::cout<<"CHECK: exact editor frame\n"<<std::flush;
        host.closePianoRollView();host.instrumentEditorClip.setViewedComponent(nullptr,false);
        host.activeInstrumentEditor=std::make_unique<OrbitPluginEditorContent>(std::make_unique<PatternDummyEditor>(processor));
        host.instrumentEditorClip.setViewedComponent(host.activeInstrumentEditor.get(),false);host.fitGeneratorPanelToPluginEditor();host.resized();
        patternCheck(host.generatorBody.getWidth()==500 && host.generatorBody.getHeight()==500,"fixed-size GUI has host padding");
        auto* wrapper=dynamic_cast<OrbitPluginEditorContent*>(host.activeInstrumentEditor.get());
        patternCheck(wrapper->editor->getPosition()==juce::Point<int>{0,0},"native GUI is offset from body");
        {auto image=host.createComponentSnapshot(host.generatorPanelBounds);juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/generator-exact-frame.png"));juce::PNGImageFormat png;png.writeImageToStream(image,out);}
        host.generatorPanelBounds.translate(20,20);host.resized();
        patternCheck(wrapper->editor->getScreenPosition()==host.generatorBody.getScreenPosition(),"moving header detached native GUI");
        if(!host.pianoRollWindow->isVisible())host.showPianoRoll();patternCheck(!host.instrumentEditorClip.isVisible() && !wrapper->isVisible(),"native GUI overlays Piano Roll");
        host.closePianoRollView();patternCheck(host.instrumentEditorClip.isVisible(),"native GUI did not return");
        for(auto size:{juce::Point<int>{1200,1000},{400,400}})
        {
            host.instrumentEditorClip.setViewedComponent(nullptr,false);host.activeInstrumentEditor.reset();
            auto native=std::make_unique<PatternDummyEditor>(processor);native->setSize(size.x,size.y);
            host.activeInstrumentEditor=std::make_unique<OrbitPluginEditorContent>(std::move(native));
            host.instrumentEditorClip.setViewedComponent(host.activeInstrumentEditor.get(),false);host.fitGeneratorPanelToPluginEditor();host.resized();
            auto* fitted=dynamic_cast<OrbitPluginEditorContent*>(host.activeInstrumentEditor.get());
            const auto visible=fitted->editor->getLocalBounds().toFloat().transformedBy(fitted->editor->getTransform());
            patternCheck(std::abs(visible.getWidth()-host.generatorBody.getWidth())<1.5f
                && std::abs(visible.getHeight()-host.generatorBody.getHeight())<1.5f,"Auto Fit left margins after display clamping or minimum header size");
        }
        for(bool ignores:{false,true})
        {
            host.instrumentEditorClip.setViewedComponent(nullptr,false);host.activeInstrumentEditor.reset();
            host.activeInstrumentEditor=std::make_unique<OrbitPluginEditorContent>(std::make_unique<PatternNativeSizeEditor>(processor,ignores));
            host.instrumentEditorClip.setViewedComponent(host.activeInstrumentEditor.get(),false);host.fitGeneratorPanelToPluginEditor();host.resized();
            auto* fitted=dynamic_cast<OrbitPluginEditorContent*>(host.activeInstrumentEditor.get());
            for(bool grow:{false,true})
            {
                if(grow){fitted->editor->setSize(1800,1400);fitted->handleAsyncUpdate();}
                const auto visible=fitted->getLocalArea(fitted->editor.get(),fitted->editor->getLocalBounds());
                patternCheck(fitted->editor->getX()==0 && fitted->editor->getY()==0,"native editor has negative origin over toolbar");
                patternCheck(visible.getWidth()<=host.generatorBody.getWidth()+1 && visible.getHeight()<=host.generatorBody.getHeight()+1,"native size-based/ignored/late resize escapes body");
                patternCheck(host.generatorSettingsButton.isVisible() && host.pianoRollButton.isVisible() && host.generatorDragHandle.isVisible(),"generator header missing after editor change");
            }
        }
        checkMacPatternClipboard(host);
        std::cout<<"PASS: independent chord velocity handles and exact 1/127 heights, separate hit targets, active readout, fractional-onset layout and stroke Undo; cell-wide entry at fractional pitch scroll/fine subdivisions, full 1-127 note/lane velocity, top Draw/Select, floating maximize/restore, searchable Hotkeys, native-size/ignored/late-resize containment; exact fixed-editor container size/position and GUI/Piano Roll switching, long automation persistence; individual chord velocity/Undo, selected-group move/duplicate, Alt brush, focused musical shortcuts, per-generator preferences, remembered-pattern ghost notes, cancellable LFO preview/one Undo, attached Rack/GUI/Piano Roll visibility and native persistence. Hardware disabled.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
