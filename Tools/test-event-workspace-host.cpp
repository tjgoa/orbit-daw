static void testEventWorkspace(ProjectView& host)
{
    auto& window=*host.songEventWindow;auto& p=window.panel;
    eventCheck(window.getParentComponent()==&host && !window.isOnDesktop(),"Event Editor is not embedded");
    window.setBounds(20,100,1000,480);const auto floating=window.getBounds();
    window.maximiseButtonPressed();eventCheck(window.getBounds()==window.workspaceArea(),"Event maximize missed workspace edges");
    window.maximiseButtonPressed();eventCheck(window.getBounds()==floating,"Event restore lost floating bounds");
    window.minimiseButtonPressed();eventCheck(window.isMinimised() && window.getHeight()==24 && window.isVisible(),"Event minimize hid/closed editor");window.minimiseButtonPressed();
    eventCheck(!host.playButton.getWantsKeyboardFocus() && !host.stopButton.getWantsKeyboardFocus() && !host.recordButton.getWantsKeyboardFocus() && !host.recordModeButton.getWantsKeyboardFocus(),"transport steals the active event editor keyboard focus");
    host.stopPlayback();eventCheck(window.isVisible(),"Stop closed Event Editor");
    for(auto* c:p.getChildren())if(auto* b=dynamic_cast<juce::Button*>(c))eventCheck(!juce::StringArray{"Copy","Paste","Clear","Smooth","Undo","Redo","Follow","Record"}.contains(b->getButtonText()),"extra automation toolbar control remains visible");
    host.loopStartSlider.setValue(1,juce::dontSendNotification);host.loopLengthSlider.setValue(8,juce::dontSendNotification);host.loopButton.setToggleState(false,juce::dontSendNotification);host.updateLoopRange();
    const float bar=128;
    p.graph.mouseDown(eventMouse(&p.graph,bar*2+10,10));p.graph.mouseUp(eventMouse(&p.graph,bar*2+10,10));
    eventCheck(!host.edit->getTransport().looping.get(),"ruler seek enabled loop");
    p.graph.mouseDown(eventMouse(&p.graph,bar*8,10));p.graph.mouseDrag(eventMouse(&p.graph,bar*9,10,true,{bar*8,10}));p.graph.mouseUp(eventMouse(&p.graph,bar*9,10));
    eventCheck(host.project.getLoopLengthBars()==9 && host.project.isLoopEnabled(),"ruler end drag did not resize loop");
    p.graph.mouseDown(eventMouse(&p.graph,bar*3+30,10));p.graph.mouseDrag(eventMouse(&p.graph,bar*5+30,10,true,{bar*3+30,10}));p.graph.mouseUp(eventMouse(&p.graph,bar*5+30,10));
    eventCheck(host.project.getLoopStartBar()==3 && host.project.getLoopLengthBars()==9,"ruler body drag did not move intact loop");
    const auto target=p.targets[size_t(p.selectedTarget)];const auto before=host.songEventPoints(target);const auto undo=host.undoHistory.size();
    host.showAutomationLfo(false);eventCheck(host.lfoWindow!=nullptr && p.previewActive && !p.previewPoints.isEmpty(),"Song LFO preview absent");const auto preview=p.previewPoints;
    {auto image=host.lfoWindow->createComponentSnapshot(host.lfoWindow->getLocalBounds());juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/live-lfo-tool.png"));juce::PNGImageFormat().writeImageToStream(image,output);}
    host.lfoWindow->panel.values[1].setValue(3,juce::sendNotificationSync);eventCheck(!orbit::automation::samePoints(preview,p.previewPoints) && orbit::automation::samePoints(before,host.songEventPoints(target)),"speed preview did not update without mutating project");
    host.lfoWindow->panel.onCancel();eventCheck(!p.previewActive && host.undoHistory.size()==undo,"LFO cancel changed project/Undo");
    host.showAutomationLfo(false);host.lfoWindow->panel.apply.onClick();eventCheck(host.undoHistory.size()==undo+1 && !host.songEventPoints(target).isEmpty(),"LFO apply was not one transaction");host.undoProjectAction();eventCheck(orbit::automation::samePoints(before,host.songEventPoints(target)),"LFO Undo lost original events");
    // No note pattern is required for the main Edit menu's event clipboard.
    checkedSongEventFocus=true;orbit::automation::Points authored;authored.add({0,.4f});authored.add({16,.8f});host.commitSongEvents(host.project.getProjectId(),target,before,authored);
    auto enabled=[&](int id){const auto menu=host.makeEditMenu();juce::PopupMenu::MenuItemIterator it(menu);while(it.next())if(it.getItem().itemID==id)return it.getItem().isEnabled;return false;};
    eventCheck(enabled(204),"Edit Copy was disabled for automation without notes");host.handleMenuResult(1,204);host.handleMenuResult(1,203);
    eventCheck(host.songEventPoints(target).isEmpty() && enabled(205),"Edit Cut/Paste did not use the curve clipboard");host.handleMenuResult(1,205);eventCheck(orbit::automation::samePoints(authored,host.songEventPoints(target)),"Edit Paste changed the wrong document");
    host.undoProjectAction();host.undoProjectAction();host.undoProjectAction();checkedSongEventFocus=false;
    eventCheck(orbit::automation::samePoints(before,host.songEventPoints(target)),"Edit clipboard Undo did not restore source");
    p.setGradientStyle(1);eventCheck(p.gradientTop()!=p.gradientBottom(),"red-blue palette missing");if(p.onViewOptionsChanged)p.onViewOptionsChanged();
    window.setBounds(20,100,1000,480);
    {auto image=window.createComponentSnapshot(window.getLocalBounds());juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/embedded-event-editor.png"));juce::PNGImageFormat().writeImageToStream(image,output);}
    std::cout<<"PASS: embedded Event controls/persistence, ruler seek/resize/move, compact tools, live LFO preview/cancel/apply/Undo and colors.\n";
}
