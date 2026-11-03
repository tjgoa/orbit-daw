static void uiCheck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static juce::MouseEvent uiDouble(juce::Component* c,juce::Point<float> at)
{return {juce::Desktop::getInstance().getMainMouseSource(),at,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),at,juce::Time::getCurrentTime(),2,false};}
static void uiImage(juce::Component& component,const char* file)
{auto image=component.createComponentSnapshot(component.getLocalBounds());juce::FileOutputStream output(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+file));juce::PNGImageFormat().writeImageToStream(image,output);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto acidId=host.selectedProjectTrackId();host.addDrumRackTrack();const auto drumId=host.selectedProjectTrackId();
    auto window=[&](const auto& id)->orbit::generator::ControlWindow&{return *host.generatorControlWindows.at(id).window;};
    for(int width:{480,500,540,600,900})
    {const orbit::generator::PatternToolbarLayout fit({0,0,width,58});uiCheck(fit.save.getRight()<=width && fit.piano.getX()<fit.banks.getX() && fit.preset.getWidth()>=100,"narrow toolbar clipped its preset files");}
    auto& a=window(acidId);a.setBounds(30,110,900,380);window(drumId).setBounds(50,520,1000,400);
    uiCheck(a.solo.getX()==a.mute.getX() && a.solo.getBottom()<a.mute.getBottom() && a.piano.getX()>a.solo.getRight() && a.banks[0].getX()>a.piano.getRight() && a.steps.getX()>a.banks[7].getRight() && a.preset.getX()>a.steps.getRight() && a.open.getX()>a.preset.getRight(),"toolbar order differs from reference");
    a.steps.setText("032",false);a.commitSteps();uiCheck(host.project.getPatternLengthSteps(acidId,0)==32 && host.project.getPatternLengthSteps(drumId,0)==16 && host.selectedProjectTrackId()==drumId,"steps edited selected generator instead of owner");
    a.solo.setToggleState(true,juce::dontSendNotification);a.solo.onClick();uiCheck(host.engineTrackForProjectId(acidId)->isSolo(false) && !host.engineTrackForProjectId(drumId)->isSolo(false),"solo targeted wrong generator");a.solo.setToggleState(false,juce::dontSendNotification);a.solo.onClick();
    a.mute.setToggleState(true,juce::dontSendNotification);a.mute.onClick();uiCheck(host.engineTrackForProjectId(acidId)->isMuted(false),"mute failed");a.mute.setToggleState(false,juce::dontSendNotification);a.mute.onClick();
    auto* plugin=host.generatorControlPlugin(acidId);auto* cutoff=plugin->getAutomatableParameters()[1];const auto pluginId=plugin->itemID.toString();
    juce::Array<ProjectDocument::AutomationPoint> events;events.add({0,.2f});events.add({32,.8f});host.project.replaceSongAutomationPoints(acidId,plugin->getName(),cutoff->paramID,cutoff->getParameterName(),events,pluginId);host.updateEngineAutomation();
    cutoff->setNormalisedParameter(.4f,juce::sendNotificationSync);const auto file=juce::File("OPTIONS_CHECK_FOLDER/Acid Test.orbitpreset");uiCheck(host.saveGeneratorPresetFile(acidId,file),"preset save failed");cutoff->setNormalisedParameter(.8f,juce::sendNotificationSync);
    const auto undo=host.undoHistory.size();uiCheck(!host.loadGeneratorPresetFile(drumId,file) && host.undoHistory.size()==undo,"incompatible preset changed document");uiCheck(host.loadGeneratorPresetFile(acidId,file),"preset load failed");
    uiCheck(pluginId==host.generatorControlPlugin(acidId)->itemID.toString() && std::abs(cutoff->getCurrentNormalisedValue()-.4f)<.001 && host.project.getSongAutomationLaneCount(acidId)==1 && host.project.getPatternLengthSteps(acidId,0)==32,"preset lost identity, sound, automation or pattern");
    host.undoProjectAction();uiCheck(std::abs(host.generatorControlPlugin(acidId)->getAutomatableParameters()[1]->getCurrentNormalisedValue()-.8f)<.001,"preset Undo failed");host.redoProjectAction();
    host.project.setPatternLengthSteps(acidId,0,16);host.project.setPatternCreated(acidId,0,true);host.project.setPatternCreated(acidId,1,true);
    host.project.appendPatternNoteTicks(acidId,0,12,0,2,100);host.project.appendPatternNoteTicks(acidId,0,18,6*64,2,100);
    host.project.appendPatternNoteTicks(acidId,1,108,0,2,100);host.project.appendPatternNoteTicks(acidId,1,114,6*64,2,100);
    host.project.setPlaylistClip(acidId,host.project.getPatternId(acidId,0),0,0,true);host.project.setPlaylistClip(acidId,host.project.getPatternId(acidId,1),1,0,true);host.refreshEnginePlaylist();host.showPlaylist();
    auto* grid=host.playlistWindow->grid;grid->showPatternNotes=true;auto* cell=grid->cellButtons.at(1).get();cell->mouseDoubleClick(uiDouble(cell,{20,20}));
    uiCheck(host.pianoRollWindow->isVisible() && host.selectedProjectTrackId()==acidId && host.selectedPatternIndex()==1,"pattern double click did not open its Piano Roll");cell=host.playlistWindow->grid->cellButtons.at(1).get();cell->mouseDoubleClick(uiDouble(cell,{20,20}));uiCheck(host.pianoRollWindow->isVisible(),"double-click toggled open Piano Roll off");
    uiImage(window(acidId),"shared-toolbar.png");host.closePianoRollView();host.openGeneratorControls(acidId);uiImage(window(acidId),"generator-toolbar.png");
    host.showSongEventEditor(acidId);host.refreshSongEventEditor();auto& panel=host.songEventWindow->panel;
    int populated=0;for(const auto& target:panel.targets)if(target.hasEvents){++populated;uiCheck(panel.parameterHasEvents(target.label),"populated parameter lost menu highlight");}
    uiCheck(populated==1,"automation marker leaked into empty parameters");
    const auto lane=host.project.getSongAutomationLane(acidId,0);host.project.replaceSongAutomationPoints(acidId,lane.pluginName,lane.parameterId,lane.parameterName,{},lane.pluginId);host.refreshSongEventEditor();uiCheck(std::none_of(panel.targets.begin(),panel.targets.end(),[](const auto& target){return target.hasEvents;}),"deleted events left stale menu highlights");
    host.project.replaceSongAutomationPoints(acidId,lane.pluginName,lane.parameterId,lane.parameterName,lane.points,lane.pluginId);host.refreshSongEventEditor();
    for(auto* child:panel.getChildren())if(auto* combo=dynamic_cast<juce::ComboBox*>(child))
    {
        juce::Image image(juce::Image::RGB,500,70,true);juce::Graphics g(image);auto& theme=combo->getLookAndFeel();theme.drawPopupMenuBackground(g,500,70);
        juce::String used,unused;for(const auto& target:panel.targets){if(target.hasEvents)used=target.label;else if(unused.isEmpty())unused=target.label;}
        theme.drawPopupMenuItem(g,{0,0,500,30},false,true,false,false,false,unused,{},nullptr,nullptr);
        theme.drawPopupMenuItem(g,{0,32,500,30},false,true,false,false,false,used,{},nullptr,nullptr);
        juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/parameter-menu.png"));juce::PNGImageFormat().writeImageToStream(image,output);
    }
    auto menu=host.makeWindowMenu();juce::StringArray menuNames;for(juce::PopupMenu::MenuItemIterator it(menu);it.next();)if(!it.getItem().isSeparator)menuNames.add(it.getItem().text);
    uiCheck(menuNames[0]=="Close/Minimize All" && menuNames[1]=="Cascade" && !menuNames.joinIntoString("|").contains("Compact Mixer"),"Window menu retained clutter");
    const int tracks=host.project.getTrackCount(),points=host.project.getSongAutomationLaneCount(acidId);const auto bounds=window(acidId).getBounds();
    host.closeMinimizeAllWindows();host.closeMinimizeAllWindows();uiCheck(window(acidId).isMinimised() && window(drumId).isMinimised() && window(acidId).getWidth()==180 && host.songEventWindow->isVisible() && host.songEventWindow->isMinimised() && host.playlistWindow->isMinimized(),"Close/Minimize All lost or failed to minimize a window");
    host.cascadeOpenWindows();uiCheck(!window(acidId).isMinimised() && !window(drumId).isMinimised() && !host.songEventWindow->isMinimised() && host.project.getTrackCount()==tracks && host.project.getSongAutomationLaneCount(acidId)==points,"Cascade lost windows or project data");
    host.openGeneratorControls(acidId);uiImage(window(acidId),"generator-toolbar.png");host.showPlaylist();host.playlistWindow->grid->setHorizontalZoom(1.f);uiImage(*host.playlistWindow,"playlist-previews.png");
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/orion-ui.orbit"));uiCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/orion-ui.orbit")),"native reopen failed");uiCheck(host.project.getSongAutomationLaneCount(acidId)==1,"reopen lost automation");
    std::cout<<"PASS: fixed Orion toolbar order, owner-pinned Steps/Solo/Mute, compatible preset file save/load with sound/identity/automation preservation and Undo/Redo, idempotent pattern double-click, populated lane highlighting/clear refresh, concise Window menu, repeated minimize-all and Cascade without data loss, native reopen.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
