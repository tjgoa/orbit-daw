// Production Playlist gesture/model checks, with isolated settings and no hardware.
static void playlistCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::MouseEvent playlistMouse(juce::Component* c,float x,float y,juce::Point<float> down={},bool drag=false)
{
    const juce::Point<float> p{x,y};return {juce::Desktop::getInstance().getMainMouseSource(),p,
        juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,
        juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};
}
#include "test-playlist-selection-host.cpp"

int main(int argc,char** argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        struct Cleanup {~Cleanup(){checkedApply={};checkedOptions.reset();}} cleanup;
        ProjectView host;host.stopTimer();
        if(argc>1)
        {playlistCheck(host.playlistWindow->scrollsExistingPatternsOnly(),"scroll policy did not restore in a fresh process");
            playlistCheck(host.playlistWindow->grid->labelWidth()==310,"Generator column width did not restore in fresh process");
            const auto view=host.playlistWindow->playlistViewPreferences();
            playlistCheck(bool(view.getProperty("follow")) && bool(view.getProperty("velocities")) && !bool(view.getProperty("navigation")) && !bool(view.getProperty("notes")),"Playlist view options did not restore in fresh process");
            std::cout<<"PASS: Playlist scroll policy and Generator column width restored in fresh process.\n";return 0;}
        host.createProjectDiscardingCurrent();host.setSize(1440,900);host.showPlaylist();
        auto* window=host.playlistWindow.get();auto* grid=window->grid;
        auto* preferences=host.applicationProperties.getUserSettings();
        auto resizeColumn=[&](int delta,int expected)
        {
            const int width=grid->labelWidth(),saved=preferences->getIntValue("playlistTrackHeaderWidth",250);
            const auto bounds=window->getBounds();
            const auto beforeUndo=host.undoHistory.size();const auto beforeCursor=host.edit->getTransport().getPosition();
            grid->setPlayheadPosition(4,120,true);const auto beforePlayhead=grid->playheadX;
            auto press=playlistMouse(grid,float(width-3),float(grid->headerHeight()/2));
            playlistCheck(grid->getComponentAt(press.getPosition())==grid,"visible three-line grip has an intercepting child");
            grid->mouseMove(press);playlistCheck(grid->getMouseCursor()==juce::MouseCursor::LeftRightResizeCursor,"grip has no resize cursor");
            grid->mouseDown(press);playlistCheck(grid->resizingTrackHeader && !grid->rulerGestureActive,"visible header grip did not start column resize");
            auto move=playlistMouse(grid,press.x+float(delta),press.y,press.position,true);grid->mouseDrag(move);
            playlistCheck(grid->labelWidth()==expected,"header grip did not resize left/right or clamp width");
            playlistCheck(std::abs(grid->playheadX-beforePlayhead-(expected-width))<0.01,"cursor became misaligned during column resize");
            playlistCheck(preferences->getIntValue("playlistTrackHeaderWidth",250)==saved,"column resize saved before release");
            grid->mouseUp(move);
            playlistCheck(preferences->getIntValue("playlistTrackHeaderWidth")==expected,"column width did not save on release");
            playlistCheck(window->getBounds()==bounds && host.undoHistory.size()==beforeUndo
                && host.edit->getTransport().getPosition()==beforeCursor,"column resize moved Playlist, edited notes, or sought transport");
            playlistCheck(grid->cellButtons.empty() || grid->cellButtons.begin()->second->getX()==grid->labelWidth(),"first bar no longer follows column width");
        };
        // The screenshot has no generators: the visible grip must still work.
        resizeColumn(-60,190);resizeColumn(-1000,160);resizeColumn(1000,480);resizeColumn(-230,250);
        host.addInstrumentTrack();window->refresh();resizeColumn(-40,210);resizeColumn(40,250);
        // Reproduce an upward header drag with the mouse-down point transformed
        // into the moving component's NEW coordinates on each event (as JUCE does).
        host.showPlaylist();host.playlistPanelBounds={300,250,800,400};window->setBounds(host.playlistPanelBounds);
        auto headerDown=playlistMouse(window,200,12);const auto screenDown=headerDown.getScreenPosition();
        const auto original=window->getPosition();host.savePlaylistPanelState();
        const int savedY=preferences->getIntValue("playlistPanelY");window->mouseDown(headerDown);
        for(auto delta:{juce::Point<int>{30,-55},{70,-90},{-25,-110},{20,-65},{60,20}})
        {
            const auto local=window->getLocalPoint(nullptr,screenDown+delta);
            const auto downLocal=window->getLocalPoint(nullptr,screenDown);
            auto move=playlistMouse(window,float(local.x),float(local.y),downLocal.toFloat(),true);
            window->mouseDrag(move);
            playlistCheck(window->getPosition()==original+delta,"header drag lost capture while moving upward or reversing");
            playlistCheck(preferences->getIntValue("playlistPanelY")==savedY,"header drag wrote preferences before release");
        }
        window->mouseUp(headerDown);
        playlistCheck(preferences->getIntValue("playlistPanelY")==window->getY(),"header release failed to save position");
        window->mouseDrag(headerDown);playlistCheck(window->getPosition()==original+juce::Point<int>(60,20),"released header kept dragging");

        auto ruler=[&](int bar){return float(grid->labelWidth()+(bar-1)*grid->barWidth()+10);};
        auto loop=[&](bool enabled){host.loopButton.setToggleState(enabled,juce::dontSendNotification);host.setLoopEnabled(enabled);};
        host.setPlaybackMode(ProjectView::PlaybackMode::song);loop(false);
        const auto originalStart=host.loopStartSlider.getValue(),originalLength=host.loopLengthSlider.getValue();
        auto rulerDown=playlistMouse(grid,ruler(7),12);grid->mouseDown(rulerDown);
        playlistCheck(!host.loopButton.getToggleState(),"ruler mouse-down enabled loop");
        auto jitter=playlistMouse(grid,rulerDown.x+2,12,rulerDown.position,true);grid->mouseDrag(jitter);grid->mouseUp(jitter);
        playlistCheck(!host.loopButton.getToggleState() && host.loopStartSlider.getValue()==originalStart
            && host.loopLengthSlider.getValue()==originalLength,"plain ruler click changed loop settings");
        playlistCheck(std::abs(host.edit->getTransport().getPosition().inSeconds()-12.0)<0.001,"ruler click did not seek to bar 7");
        playlistCheck(grid->playheadVisible && std::abs(grid->playheadX-(grid->labelWidth()+6*grid->barWidth()))<0.01,"stopped ruler cursor not shown at click");
        host.project.setInitialTimeSignature(3,4);grid->mouseDown(rulerDown);grid->mouseUp(rulerDown);
        playlistCheck(std::abs(host.edit->getTransport().getPosition().inSeconds()-9.0)<0.001
            && std::abs(grid->playheadX-(grid->labelWidth()+6*grid->barWidth()))<0.01,"ruler seek/cursor ignored 3/4 meter");
        host.project.setInitialTimeSignature(4,4);
        loop(true);auto enabledClick=playlistMouse(grid,ruler(3),12);grid->mouseDown(enabledClick);grid->mouseUp(enabledClick);
        playlistCheck(host.loopButton.getToggleState() && host.loopLengthSlider.getValue()==originalLength,"ruler click changed an enabled loop");
        loop(false);auto rangeDown=playlistMouse(grid,ruler(5),12);grid->mouseDown(rangeDown);
        auto range=playlistMouse(grid,ruler(2),12,rangeDown.position,true);grid->mouseDrag(range);grid->mouseUp(range);
        playlistCheck(host.loopButton.getToggleState() && host.loopStartSlider.getValue()==2 && host.loopLengthSlider.getValue()==4,"reverse ruler drag did not create explicit loop");
        loop(false);host.setPlaybackMode(ProjectView::PlaybackMode::pattern);
        const auto id=host.selectedProjectTrackId();
        host.selectPatternBank(7);host.selectPatternSlot(3);host.project.appendPatternNoteTicks(id,59,60,0,64,100);
        host.paintPlaylistClip(0,0);host.finishPlaylistPaintGesture();
        playlistCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,59),"paint lost remembered H4 selection");
        host.setPlaylistPattern(0,0,0);window->refresh();grid->setViewportArea({0,0,1400,400});
        auto wheelAt=[&](float delta)
        {auto& button=*grid->cellButtons.at(0);juce::MouseWheelDetails wheel{};wheel.deltaY=delta;button.mouseWheelMove(playlistMouse(&button,button.getWidth()/2.0f,12),wheel);};
        playlistCheck(!window->scrollsExistingPatternsOnly(),"blank pattern scrolling is not the default");
        wheelAt(0.0005f);playlistCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,1)
            && host.project.getPatternNotes(id,1).empty(),"small upward wheel failed to reach blank A2");
        wheelAt(-0.001f);playlistCheck(host.selectedPatternIndex()==0,"downward wheel did not return to A1");
        const auto lowerUndo=host.undoHistory.size();wheelAt(-.01f);
        playlistCheck(host.selectedPatternIndex()==0 && host.undoHistory.size()==lowerUndo,"A1 lower boundary wrapped or created Undo");
        host.setPlaylistPattern(0,0,63);const auto upperUndo=host.undoHistory.size();wheelAt(.01f);
        playlistCheck(host.selectedPatternIndex()==63 && host.undoHistory.size()==upperUndo,"H8 upper boundary wrapped or created Undo");
        host.setPlaylistPattern(0,0,0);
        for(int s=0;s<64;++s)host.project.setPatternCreated(id,s,s==0 || s==2);
        window->setScrollExistingPatternsOnly(true);wheelAt(0.01f);
        playlistCheck(host.selectedPatternIndex()==2,"existing-only mode did not skip blank A2");
        wheelAt(-.01f);wheelAt(-.01f);playlistCheck(host.selectedPatternIndex()==0,"existing-only lower boundary wrapped");
        host.setPlaylistPattern(0,0,63);wheelAt(.01f);playlistCheck(host.selectedPatternIndex()==63,"unlisted blank upper boundary wrapped");
        wheelAt(-.01f);playlistCheck(host.selectedPatternIndex()==2,"existing-only navigation from an unlisted blank chose the wrong slot");
        window->setScrollExistingPatternsOnly(false);host.setPlaylistPattern(0,0,0);

        // The bottom tab opens all 64 slots, including blank patterns, and
        // must not leak a release into painting or a drag into pattern cycling.
        window->refresh();grid->setViewportArea({0,0,1600,400});
        {auto& tab=*grid->cellButtons.at(0);int menus=0;
         const auto callback=tab.onPatternMenu;tab.onPatternMenu=[&](int row,int bar,juce::Component*){playlistCheck(row==0 && bar==0,"selector target changed");++menus;};
         auto press=playlistMouse(&tab,8,float(tab.getHeight()-7));const auto before=host.undoHistory.size();
         tab.mouseDown(press);tab.mouseDrag(playlistMouse(&tab,8,-50,press.position,true));tab.mouseUp(press);
         playlistCheck(menus==1 && host.undoHistory.size()==before,"selector painted/cycled instead of opening menu");
         playlistCheck(!tab.isPatternSelector({tab.getWidth()-1,tab.getHeight()-5}),"selector stole right-edge resize");tab.onPatternMenu=callback;
         auto menu=host.makePlaylistPatternMenu(0,0);int slots=0;bool empty=false;
         for(juce::PopupMenu::MenuItemIterator bank(menu);bank.next();)if(bank.getItem().subMenu!=nullptr)
           for(juce::PopupMenu::MenuItemIterator item(*bank.getItem().subMenu);item.next();){++slots;empty|=item.getItem().text.contains("empty");}
         playlistCheck(slots==64 && empty,"pattern chooser omits empty slots");}

        // Up/down travel while holding an existing block is distinct from a
        // horizontal paint stroke and shares the wheel's pattern policy.
        grid->setViewportArea({0,0,1600,400});
        auto verticalChoice=[&](int distance)
        {
            auto& button=*grid->cellButtons.at(0);const auto before=host.undoHistory.size();
            auto press=playlistMouse(&button,button.getWidth()/2.0f,12);button.mouseDown(press);
            auto move=playlistMouse(&button,press.x,press.y-float(distance),press.position,true);
            button.mouseDrag(move);button.mouseUp(move);
            playlistCheck(button.verticalPatternGesture==false && !button.rangePaintGesture,"pattern travel became a paint stroke");
            playlistCheck(host.undoHistory.size()==before+1,"vertical pattern gesture did not have one Undo");
        };
        verticalChoice(24);playlistCheck(host.selectedPatternIndex()==2,"upward held travel failed to advance two slots");
        host.undoProjectAction();playlistCheck(host.project.getPlaylistPatternId(id,0)==host.project.getPatternId(id,0),"pattern travel Undo failed");
        window->setScrollExistingPatternsOnly(true);verticalChoice(12);
        playlistCheck(host.selectedPatternIndex()==2,"held travel ignored existing-only policy");
        host.undoProjectAction();window->setScrollExistingPatternsOnly(false);

        // A body drag paints five separate one-bar copies, not one five-bar clip.
        window->refresh();grid->setViewportArea({0,0,1600,400});
        const auto beforePaint=host.undoHistory.size();auto& cell=*grid->cellButtons.at(6);
        auto down=playlistMouse(&cell,cell.getWidth()/2.0f,12);cell.mouseDown(down);
        auto drag=playlistMouse(&cell,down.x+4*grid->barWidth(),down.y,down.position,true);
        cell.mouseDrag(drag);cell.mouseUp(drag);
        for(int bar=6;bar<=10;++bar)
            playlistCheck(host.project.getPlaylistClipStartBar(id,bar)==bar && host.project.getPlaylistClipLengthBars(id,bar)==1
                && host.project.playlistClipFollowsPatternLength(id,bar),"body drag stretched a pattern instead of stamping");
        playlistCheck(host.undoHistory.size()==beforePaint+1,"paint stroke did not have one undo");
        host.undoProjectAction();playlistCheck(host.project.getPlaylistPatternId(id,6).isEmpty() && host.project.getPlaylistPatternId(id,10).isEmpty(),"paint undo left copies");
        host.redoProjectAction();const auto noOp=host.undoHistory.size();host.paintPlaylistClipRange(0,6,10);host.finishPlaylistPaintGesture();
        playlistCheck(host.undoHistory.size()==noOp,"unchanged paint stroke created undo history");

        host.selectPatternBank(0);host.selectPatternSlot(4);host.project.setPatternLengthSteps(id,4,128);
        for(int bar:{20,40}){host.paintPlaylistClip(0,bar);host.finishPlaylistPaintGesture();}
        host.paintPlaylistClipRange(0,60,83);host.finishPlaylistPaintGesture();
        for(int bar:{20,40,60,68,76})playlistCheck(host.project.getPlaylistClipStartBar(id,bar)==bar
            && host.project.getPlaylistClipLengthBars(id,bar)==8,"eight-bar pattern painted at a different size");
        playlistCheck(host.project.getPlaylistClipStartBar(id,67)==60 && host.project.getPlaylistClipStartBar(id,75)==68,"multi-bar copies overlapped");
        const auto beforeEnd=host.undoHistory.size();host.paintPlaylistClipRange(0,510,511);host.finishPlaylistPaintGesture();
        playlistCheck(host.project.getPlaylistPatternId(id,510).isEmpty() && host.undoHistory.size()==beforeEnd,"end-of-song paint truncated a pattern");

        // Only a deliberate right-edge gesture extends a placement.
        grid->setHorizontalZoom(0.25f);window->refresh();grid->setViewportArea({0,0,1600,400});
        auto& edge=*grid->cellButtons.at(27);auto edgeDown=playlistMouse(&edge,float(edge.getWidth()-2),12);
        edge.mouseDown(edgeDown);playlistCheck(edge.resizeGesture,"right-edge handle was not recognized");
        auto edgeDrag=playlistMouse(&edge,edgeDown.x+2*grid->barWidth(),12,edgeDown.position,true);
        edge.mouseDrag(edgeDrag);edge.mouseUp(edgeDrag);
        playlistCheck(host.project.getPlaylistClipLengthBars(id,20)==10 && host.project.getPatternLengthSteps(id,4)==128,"edge extension changed the source pattern");
        host.paintPlaylistClip(0,100);host.finishPlaylistPaintGesture();
        playlistCheck(host.project.getPlaylistClipLengthBars(id,100)==8,"new paint inherited an instance extension");
        host.paintPlaylistClip(0,30);host.finishPlaylistPaintGesture();const auto collisionUndo=host.undoHistory.size();
        host.resizePlaylistPatternClip(0,20,11);
        playlistCheck(host.project.getPlaylistClipLengthBars(id,20)==10 && host.undoHistory.size()==collisionUndo,"resize overlapped another copy of the same pattern");
        host.project.setPatternLengthSteps(id,5,256);host.setPlaylistPattern(0,20,5);
        playlistCheck(host.project.getPlaylistPatternId(id,20)==host.project.getPatternId(id,4) && !host.project.isPatternCreated(id,5),"wheel pattern change overwrote adjacent clips");

        host.addInstrumentTrack();window->refresh();grid->showPatternNotes=false;grid->showPatternGrid=false;grid->showPatternVelocities=false;
        for(int skin:{5,6})
        {
            host.setSkin(skin);grid->setViewportArea({0,0,1600,400});
            auto image=grid->createComponentSnapshot({0,0,grid->labelWidth()+38*grid->barWidth(),grid->headerHeight()+2*grid->rowHeight()});
            const int x=grid->labelWidth()+21*grid->barWidth(),y=grid->headerHeight()+10;
            playlistCheck(image.getPixelAt(x,y)==image.getPixelAt(x-1,y) && image.getPixelAt(x,y)==image.getPixelAt(x+1,y),"Playlist bar border showed through pattern body");
            juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/playlist-clips-")+juce::String(skin)+".png"));
            juce::PNGImageFormat png;playlistCheck(png.writeImageToStream(image,out),"clip render failed");
        }
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/playlist.orbit");host.saveToFile(file);playlistCheck(host.loadProjectFile(file),"playlist reopen failed");
        playlistCheck(host.project.getPlaylistClipLengthBars(id,20)==10 && host.project.getPlaylistClipLengthBars(id,100)==8,"reopen lost explicit/default clip lengths");
        host.showPlaylist();const auto restore=host.playlistPanelBounds;host.togglePlaylistMaximized();
        playlistCheck(window->getBounds()==juce::Rectangle<int>(0,82,1440,796),"maximized Playlist retained wallpaper padding");
        host.setSize(1800,1100);playlistCheck(window->getBounds()==juce::Rectangle<int>(0,82,1800,996),"resized maximized Playlist lost edge coverage");
        host.wallpaperImage=juce::Image(juce::Image::RGB,4,4,true);{juce::Graphics g(host.wallpaperImage);g.fillAll(juce::Colours::lime);}host.setWallpaper(3);
        auto maximized=host.createComponentSnapshot(host.getLocalBounds());
        for(auto point:{juce::Point<int>(0,83),{1799,500},{800,1077}})
            playlistCheck(maximized.getPixelAt(point.x,point.y)!=juce::Colours::lime,"wallpaper visible around maximized Playlist");
        {juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/playlist-maximized.png"));juce::PNGImageFormat png;png.writeImageToStream(maximized,out);}
        host.togglePlaylistMaximized();playlistCheck(window->getBounds()==restore,"maximize restore lost floating bounds");
        window->setScrollExistingPatternsOnly(true);grid->onScrollExistingPatternsChanged(true);
        playlistCheck(host.optionBool("playlistScrollExistingPatternsOnly"),"scroll policy not saved");
        grid->setViewportArea({0,0,1600,400});resizeColumn(60,310);
        checkPlaylistSelection(host);
        std::cout<<"PASS: held vertical pattern travel and wheel share blank/existing policy with one gesture Undo; column grip resizes empty/populated Playlist, clamps, aligns and saves on release; continuous header drag; click-versus-loop ruler; horizontal paint/edge-resize, Undo/collisions, themes, native reopen, maximize/restore and wallpaper coverage. Hardware disabled.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
