// Included by test-options-host.py --piano-roll after the production host source.
static void pianoCheck(bool condition,const char* reason){if(!condition)throw std::runtime_error(reason);}
static juce::MouseEvent pianoMouse(PianoRollGrid* grid,float x,float y,int flags=juce::ModifierKeys::leftButtonModifier,
                                   juce::Point<float> down={},bool drag=false)
{
    const juce::Point<float> position{x,y};
    return {juce::Desktop::getInstance().getMainMouseSource(),position,juce::ModifierKeys(flags),1,0,0,0,0,grid,grid,
            juce::Time::getCurrentTime(),drag?down:position,juce::Time::getCurrentTime(),1,drag};
}
struct PianoMidiListener : te::AudioTrack::Listener
{
    std::vector<juce::MidiMessage> messages;
    void injectLiveMidiMessage(te::AudioTrack&,const te::MidiMessageWithSource& event,bool& used) override {messages.push_back(event);used=true;}
    void recordedMidiMessageSentToPlugins(te::AudioTrack&,const juce::MidiMessage&) override {}
};
int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;PianoMidiListener listener;
        ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.selectPatternSlot(0);
        const auto id=host.selectedProjectTrackId();host.instrumentTrack->getListeners().add(&listener);
        auto* window=host.pianoRollWindow.get();auto* grid=window->grid;window->setSize(1100,650);window->refresh("A1");
        pianoCheck(ProjectView::minimumPitch==0 && ProjectView::maximumPitch==127,"host editing cache still restricted");
        pianoCheck(grid->noteAreaHeight()>100 && grid->visiblePitchRows()<128,"MIDI range squeezed into viewport");
        pianoCheck(window->pitchScrollbar.getMaximumRangeLimit()==128,"pitch scrollbar range wrong");
        auto click=[&](int pitch,int step,int modifiers=juce::ModifierKeys::leftButtonModifier)
        {
            grid->centrePitch(pitch);const float y=float(grid->yForPitch(pitch)+grid->rowHeight()/2),x=float(grid->labelWidth()+step*grid->stepWidth()+grid->stepWidth()/4);
            pianoCheck(grid->pitchAtY(int(y))==pitch,"scrolled hit-test pitch mismatch");
            auto event=pianoMouse(grid,x,y,modifiers);grid->mouseDown(event);grid->mouseUp(event);
        };
        for(int pitch:{0,12,36,60,96,120,127})
        {
            click(pitch,pitch%7);pianoCheck(host.project.getPatternNote(id,0,pitch,pitch%7),"outlying pitch could not be drawn");
        }
        const auto authored=host.capturePatternData(id,0).notes;pianoCheck(authored.size()==7,"note entry lost records");
        host.undoProjectAction();pianoCheck(!host.project.getPatternNote(id,0,127,127%7) && host.capturePatternData(id,0).notes.size()==6,"single high-note undo failed");host.redoProjectAction();
        pianoCheck(host.capturePatternData(id,0).notes==authored,"redo did not restore full-range notes");
        host.instrumentTrack->getListeners().add(&listener);
        // Piano key audition must address both MIDI endpoints without painting notes.
        for(int pitch:{0,127})
        {
            grid->centrePitch(pitch);const float y=float(grid->yForPitch(pitch)+grid->rowHeight()/2);
            const auto count=host.capturePatternData(id,0).notes.size();auto event=pianoMouse(grid,12,y);grid->mouseDown(event);grid->mouseUp(event);
            pianoCheck(host.capturePatternData(id,0).notes.size()==count,"keyboard audition painted a note");
            pianoCheck(std::any_of(listener.messages.begin(),listener.messages.end(),[&](const auto& m){return m.isNoteOn() && m.getNoteNumber()==pitch;}),"endpoint keyboard audition failed");
        }
        // Scrollbar, wheel and zoom must reach both boundaries and remain independent of lanes.
        grid->setPitchScrollPosition(0);pianoCheck(grid->pitchAtY(grid->headerHeight()+grid->rowHeight()/2)==127,"top note unreachable");
        juce::MouseWheelDetails wheel{};wheel.deltaY=-0.5f;auto atGrid=pianoMouse(grid,200,120,0);grid->mouseWheelMove(atGrid,wheel);pianoCheck(grid->getPitchScrollPosition()>0,"wheel did not scroll pitches");
        window->pitchScrollbar.setCurrentRangeStart(0,juce::sendNotificationSync);pianoCheck(grid->getPitchScrollPosition()==0,"scrollbar did not scroll grid");
        window->velocityButton.setToggleState(true,juce::sendNotificationSync);window->velocityEditButton.setToggleState(true,juce::sendNotificationSync);window->resized();
        const int laneY=grid->laneTop();grid->setPitchScrollPosition(0);pianoCheck(grid->laneTop()==laneY,"lane moved while scrolling high pitches");grid->centrePitch(0);pianoCheck(grid->laneTop()==laneY,"lane moved while scrolling low pitches");
        pianoCheck(grid->laneTop()+grid->displayedLaneHeight()<=grid->getHeight(),"velocity lane clipped offscreen");
        auto velocity=pianoMouse(grid,float(grid->labelWidth()+grid->stepWidth()/2),float(grid->laneTop()+30));grid->mouseDown(velocity);grid->mouseUp(velocity);
        pianoCheck(host.project.getPatternNoteVelocity(id,0,0,0)>100,"velocity edit ignored low-range notes");
        window->automationToggleButton.setToggleState(true,juce::sendNotificationSync);window->resized();grid->centrePitch(127);
        pianoCheck(grid->laneTop()+grid->displayedLaneHeight()<=grid->getHeight(),"automation lane clipped offscreen");
        window->automationToggleButton.setToggleState(false,juce::sendNotificationSync);window->resized();
        for(float zoom:{0.6f,1.12f,2.2f})
        {
            grid->setZoom(1.12f,zoom);window->resizeGridToViewport();
            for(int pitch:{0,127}){grid->centrePitch(pitch);pianoCheck(grid->pitchAtY(grid->yForPitch(pitch)+grid->rowHeight()/2)==pitch,"zoom broke pitch mapping");}
        }
        grid->setZoom(1.12f,1.12f);window->resizeGridToViewport();
        const auto beforeResize=host.capturePatternData(id,0).notes;
        grid->centrePitch(127);auto edge=pianoMouse(grid,float(grid->labelWidth()+(127%7+1)*grid->stepWidth()-3),float(grid->yForPitch(127)+grid->rowHeight()/2));
        grid->mouseDown(edge);auto stretch=pianoMouse(grid,edge.x+grid->stepWidth(),edge.y,juce::ModifierKeys::leftButtonModifier,edge.position,true);grid->mouseDrag(stretch);grid->mouseUp(stretch);
        pianoCheck(host.project.getPatternNoteLength(id,0,127,127%7)==2,"high-note edge resize failed");host.undoProjectAction();pianoCheck(host.capturePatternData(id,0).notes==beforeResize,"resize undo altered other notes");
        grid->centrePitch(12);auto lassoStart=pianoMouse(grid,float(grid->labelWidth()+4*grid->stepWidth()),float(grid->yForPitch(13)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::shiftModifier);
        grid->mouseDown(lassoStart);auto lassoEnd=pianoMouse(grid,float(grid->labelWidth()+6*grid->stepWidth()),float(grid->yForPitch(11)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::shiftModifier,lassoStart.position,true);grid->mouseDrag(lassoEnd);grid->mouseUp(lassoEnd);
        pianoCheck(host.selectedNoteKeys==std::set<std::pair<int,int>>{{12,5*64}},"scrolled lasso selected wrong pitch range");host.clearNoteSelection();
        // Actual drag and lasso gestures use the scrolled pitch coordinate, too.
        const auto beforeDrag=host.capturePatternData(id,0).notes;
        grid->centrePitch(36);auto down=pianoMouse(grid,float(grid->labelWidth()+36%7*grid->stepWidth()+grid->stepWidth()/4),float(grid->yForPitch(36)+grid->rowHeight()/2));
        grid->mouseDown(down);auto drag=pianoMouse(grid,down.x+grid->stepWidth(),float(grid->yForPitch(35)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier,down.position,true);grid->mouseDrag(drag);grid->mouseUp(drag);
        pianoCheck(host.project.getPatternNote(id,0,35,36%7+1),"low-pitch drag did not move note");host.undoProjectAction();pianoCheck(host.capturePatternData(id,0).notes==beforeDrag,"drag undo altered other notes");
        grid->centrePitch(127);click(127,127%7,juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::shiftModifier);pianoCheck(!host.selectedNoteKeys.empty(),"high-range note selection failed");
        host.clearNoteSelection();click(127,127%7,juce::ModifierKeys::rightButtonModifier);pianoCheck(!host.project.getPatternNote(id,0,127,127%7),"right erase failed at MIDI 127");host.undoProjectAction();pianoCheck(host.capturePatternData(id,0).notes==beforeDrag,"erase undo altered other notes");
        host.clearNoteSelection();host.copyOrCutPatternNotes(false);pianoCheck(host.patternClipboard.notes.size()==7,"full-range clipboard lost pitches");
        // Save/reopen exact native data, then focus a pattern containing only high notes.
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/full-range.orbit");const auto beforeSave=host.capturePatternData(id,0).notes;host.saveToFile(file);pianoCheck(host.loadProjectFile(file),"native reopen failed");pianoCheck(host.capturePatternData(id,0).notes==beforeSave,"native reopen altered notes");
        host.project.appendPatternNoteTicks(id,1,120,1,17,100);host.project.setPatternCreated(id,1,true);host.selectPatternSlot(1);window->refresh("A2");
        pianoCheck(grid->yForPitch(120)>=grid->headerHeight() && grid->yForPitch(120)<grid->headerHeight()+grid->noteAreaHeight(),"new high pattern did not focus its notes");
        for(int pitch:{0,60,127})for(int skin:{5,6})
        {
            host.setSkin(skin);grid->centrePitch(pitch);auto image=window->createComponentSnapshot(window->getLocalBounds());
            juce::FileOutputStream output(juce::File(juce::String("OPTIONS_CHECK_FOLDER/piano-")+juce::String(pitch)+"-skin"+juce::String(skin)+".png"));juce::PNGImageFormat png;pianoCheck(png.writeImageToStream(image,output),"range render failed");
        }
        std::cout<<"PASS: full MIDI 0-127 drawing, scrolling/scrollbar, zoom, audition, scrolled drag/resize/lasso/selection/erase, full-range velocity and clipboard, fixed lanes, single-gesture undo/redo, exact native reopen, high-pattern focus, six dark/light range renders. Hardware/native display disabled.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
