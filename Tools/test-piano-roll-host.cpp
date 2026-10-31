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
int main(int argc,char**)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;PianoMidiListener listener;
        ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.selectPatternSlot(0);
        const auto id=host.selectedProjectTrackId();host.instrumentTrack->getListeners().add(&listener);
        auto* window=host.pianoRollWindow.get();auto* grid=window->grid;window->setSize(1100,650);window->refresh("A1");
        if(argc>1)
        {
            const auto scale=grid->getScaleSnap();
            pianoCheck(scale.enabled && scale.root==9 && scale.scale==4,"scale preference did not restore in fresh process");
            std::cout<<"PASS: Piano Roll scale preferences restored in fresh process.\n";return 0;
        }
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
        // Right-hand white-key tails must remain white beside every black key,
        // and hit-testing must match the shorter black-key drawing.
        for(int skin:{5,6})for(float zoom:{0.6f,1.12f,2.2f})
        {
            host.setSkin(skin);grid->setZoom(1.12f,zoom);window->resizeGridToViewport();grid->centrePitch(61);
            auto image=grid->createComponentSnapshot(grid->getLocalBounds());
            const int y=grid->yForPitch(61),h=grid->rowHeight();
            pianoCheck(image.getPixelAt(52,y+h/4).getBrightness()>0.65f,"black key lost its white tail");
            pianoCheck(image.getPixelAt(10,y+h/2).getBrightness()<0.4f,"black key no longer dark");
            pianoCheck(grid->keyboardPitchAt(10,y+h/4)==61 && grid->keyboardPitchAt(52,y+h/4)==62
                && grid->keyboardPitchAt(52,y+3*h/4)==60,"key tails play the wrong pitch");
        }
        grid->setZoom(1.12f,1.12f);window->resizeGridToViewport();
        // Piano key audition must address both MIDI endpoints without painting notes.
        for(int pitch:{0,127})
        {
            grid->centrePitch(pitch);const float y=float(grid->yForPitch(pitch)+grid->rowHeight()/2);
            const auto count=host.capturePatternData(id,0).notes.size();auto event=pianoMouse(grid,12,y);grid->mouseDown(event);grid->mouseUp(event);
            pianoCheck(host.capturePatternData(id,0).notes.size()==count,"keyboard audition painted a note");
            pianoCheck(std::any_of(listener.messages.begin(),listener.messages.end(),[&](const auto& m){return m.isNoteOn() && m.getNoteNumber()==pitch;}),"endpoint keyboard audition failed");
        }
        // The actual keyboard mouse gesture now enters the live MIDI recorder.
        grid->centrePitch(60);const auto recordingBefore=host.capturePatternData(id,0).notes;
        const auto recordingUndo=host.undoHistory.size();
        pianoCheck(host.prepareMidiRecording(0,juce::Time::getMillisecondCounterHiRes()),"mouse recording did not start");
        auto keyDown=pianoMouse(grid,10,float(grid->yForPitch(60)+grid->rowHeight()/2));grid->mouseDown(keyDown);
        pianoCheck(!host.recordingNotesForPianoRoll().empty(),"held mouse note not visible during recording");
        auto keyDrag=pianoMouse(grid,10,float(grid->yForPitch(62)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier,keyDown.position,true);grid->mouseDrag(keyDrag);
        auto outside=pianoMouse(grid,100,keyDrag.y,juce::ModifierKeys::leftButtonModifier,keyDown.position,true);grid->mouseDrag(outside);
        pianoCheck(host.liveMidiNotes.empty(),"drag outside keyboard left a stuck note");
        grid->mouseDrag(keyDrag);grid->mouseUp(keyDrag);
        pianoCheck(host.capturePatternData(id,0).notes==recordingBefore && !host.recordingNotesForPianoRoll().empty(),"mouse take painted the document before commit");
        pianoCheck(host.finishMidiRecording() && host.undoHistory.size()==recordingUndo+1,"mouse take did not commit in one undo");
        pianoCheck(host.capturePatternData(id,0).notes.size()>=recordingBefore.size()+2,"mouse keys were only auditioned, not recorded");
        host.undoProjectAction();pianoCheck(host.capturePatternData(id,0).notes==recordingBefore,"mouse take undo changed existing notes");
        host.instrumentTrack->getListeners().add(&listener);
        // Scrollbar, wheel and zoom must reach both boundaries and remain independent of lanes.
        grid->setPitchScrollPosition(0);pianoCheck(grid->pitchAtY(grid->headerHeight()+grid->rowHeight()/2)==127,"top note unreachable");
        juce::MouseWheelDetails wheel{};wheel.deltaY=-0.5f;auto atGrid=pianoMouse(grid,200,120,0);grid->mouseWheelMove(atGrid,wheel);pianoCheck(grid->getPitchScrollPosition()>0,"wheel did not scroll pitches");
        window->pitchScrollbar.setCurrentRangeStart(0,juce::sendNotificationSync);pianoCheck(grid->getPitchScrollPosition()==0,"scrollbar did not scroll grid");
        window->velocityButton.setToggleState(true,juce::sendNotificationSync);window->velocityEditButton.setToggleState(true,juce::sendNotificationSync);window->resized();
        const int laneY=grid->laneTop();grid->setPitchScrollPosition(0);pianoCheck(grid->laneTop()==laneY,"lane moved while scrolling high pitches");grid->centrePitch(0);pianoCheck(grid->laneTop()==laneY,"lane moved while scrolling low pitches");
        pianoCheck(grid->laneTop()+grid->displayedLaneHeight()<=grid->getHeight(),"velocity lane clipped offscreen");
        const auto handles=grid->velocityHandles(host.project.getPatternNotes(id,0));
        const auto lowHandle=std::find_if(handles.begin(),handles.end(),[](const auto& h){return h.note.pitch==0 && h.note.startTick==0;});
        pianoCheck(lowHandle!=handles.end(),"low-range note has no velocity handle");
        auto velocity=pianoMouse(grid,lowHandle->x,float(grid->laneTop()+30));grid->mouseDown(velocity);grid->mouseUp(velocity);
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
        using orbit::editing::ScaleSnap;
        for(int scale=0;scale<int(orbit::editing::scaleDefinitions().size());++scale)
            for(int root=0;root<12;++root)for(int pitch=0;pitch<128;++pitch)
            {
                ScaleSnap setting{true,root,scale};const auto snapped=setting.snap(pitch);
                pianoCheck(snapped>=0 && snapped<=127 && setting.contains(snapped),"scale mapper returned out-of-key/range pitch");
                pianoCheck(setting.snap(snapped)==snapped,"scale mapping not stable");
                for(int other=0;other<128;++other)if(setting.contains(other))
                    pianoCheck(std::abs(snapped-pitch)<=std::abs(other-pitch),"scale mapper skipped a nearer note");
            }
        host.selectPatternSlot(2);window->applyScaleSnap({true,0,0});
        click(61,1);pianoCheck(host.project.getPatternNote(id,2,60,1) && !host.project.getPatternNote(id,2,61,1),"new note did not snap C# to C major");
        auto beforeScaleDrag=host.capturePatternData(id,2).notes;grid->centrePitch(60);
        auto scaleDown=pianoMouse(grid,float(grid->labelWidth()+grid->stepWidth()+grid->stepWidth()/4),float(grid->yForPitch(60)+grid->rowHeight()/2));
        grid->mouseDown(scaleDown);auto scaleDrag=pianoMouse(grid,scaleDown.x+grid->stepWidth(),float(grid->yForPitch(63)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier,scaleDown.position,true);
        grid->mouseDrag(scaleDrag);grid->mouseUp(scaleDrag);
        pianoCheck(host.project.getPatternNote(id,2,62,2),"vertical note move did not snap D# to D");
        host.undoProjectAction();pianoCheck(host.capturePatternData(id,2).notes==beforeScaleDrag,"scale drag undo altered phrase");
        // Existing out-of-scale material remains independently erasable.
        host.project.appendPatternNoteTicks(id,2,61,4*64,64,90);host.readPatternSteps();
        click(61,4,juce::ModifierKeys::rightButtonModifier);
        pianoCheck(!host.project.getPatternNote(id,2,61,4) && host.project.getPatternNote(id,2,60,1),"scale snap prevented deleting an existing accidental");
        host.selectPatternSlot(3);grid->centrePitch(61);
        const auto began=juce::Time::getMillisecondCounterHiRes();pianoCheck(host.prepareMidiRecording(0,began),"scale recording setup failed");
        auto key=pianoMouse(grid,12,float(grid->yForPitch(61)+grid->rowHeight()/2));grid->mouseDown(key);
        pianoCheck(grid->keyboardPitch==60,"Piano Roll keyboard audition did not snap");
        auto adjacent=pianoMouse(grid,12,float(grid->yForPitch(60)+grid->rowHeight()/2),juce::ModifierKeys::leftButtonModifier,key.position,true);grid->mouseDrag(adjacent);
        pianoCheck(host.midiTake.preview(id.toStdString(),100).size()==1 && host.midiTake.notes.empty(),"moving across equivalent scale keys retriggered held note");
        grid->mouseUp(adjacent);
        // Hardware/virtual input keeps its original pitch, per the user's choice.
        host.processLiveMidi(juce::MidiMessage::noteOn(1,61,juce::uint8(90)),"hardware-test",began+100);
        host.processLiveMidi(juce::MidiMessage::noteOff(1,61),"hardware-test",began+200);
        pianoCheck(host.finishMidiRecording(began+300),"scale take commit failed");
        const auto scaleNotes=host.project.getPatternNotes(id,3);
        pianoCheck(scaleNotes.size()==2 && std::any_of(scaleNotes.begin(),scaleNotes.end(),[](const auto& n){return n.pitch==60;})
            && std::any_of(scaleNotes.begin(),scaleNotes.end(),[](const auto& n){return n.pitch==61;}),"Piano Roll recording or independent external MIDI pitch wrong");
        window->applyScaleSnap({false,0,0});click(61,7);
        pianoCheck(host.project.getPatternNote(id,3,61,7),"disabled scale snapping still constrained drawing");
        window->applyScaleSnap({true,9,4});
        for(int skin:{5,6})
        {
            host.setSkin(skin);grid->centrePitch(60);auto image=window->createComponentSnapshot(window->getLocalBounds());
            juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/scale-snap-")+juce::String(skin)+".png"));juce::PNGImageFormat png;pianoCheck(png.writeImageToStream(image,out),"scale preview render failed");
        }
        std::cout<<"PASS: all 17 scales/12 keys/MIDI pitches map to nearest legal pitch; real note draw/move/Undo and existing accidental erase; Piano Roll keyboard recording/glissando; independent hardware MIDI; disabled mode; preference persistence; existing full-range keyboard, recording and editing regressions. Hardware/native display disabled.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
