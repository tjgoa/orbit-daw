#ifndef SKIN_TEST_OUTPUT_DIR
#define SKIN_TEST_OUTPUT_DIR "OPTIONS_CHECK_FOLDER"
#endif
#define main originalSkinIsolationMain
#include "test-skin-isolation-host.cpp"
#undef main
static bool skinMenuHas(const juce::PopupMenu& menu,int id)
{
    for(juce::PopupMenu::MenuItemIterator it(menu);it.next();)
    {const auto& item=it.getItem();if(item.itemID==id || (item.subMenu && skinMenuHas(*item.subMenu,id)))return true;}
    return false;
}
static void canvasOverrideCheck(ProjectView& h,bool light)
{
    skinIsolationCheck(h.optionInt("skin")==7 && h.headerStyle()==8 && h.mixerWindow->getMixerSkin()==3,"canvas override changed silver chrome/Mixer");
    skinIsolationCheck(h.playlistWindow->isLightTheme()==light && orbit::options::compact::variant(*h.playlistWindow->grid)==0,"canvas override retained silver palette");
    skinIsolationCheck(!h.skinIsSelected(7),"custom canvas incorrectly marked as stock profile");
}
int main(int argc,char** argv){try{
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1440,1000);
    if(argc>1){const juce::String mode(argv[1]);
        if(mode=="--dark"){canvasOverrideCheck(h,false);h.playlistWindow->grid->onThemeChanged(true);}
        else if(mode=="--light"){canvasOverrideCheck(h,true);h.setSkin(7);requireProfile(h,7);h.applicationProperties.getUserSettings()->setValue("options.skin",3);h.applicationProperties.saveIfNeeded();}
        else if(mode=="--retired-grey"){requireProfile(h,0);h.applicationProperties.getUserSettings()->setValue("options.skin",4);h.applicationProperties.saveIfNeeded();}
        else if(mode=="--retired-mixer")requireProfile(h,0);
        else throw std::runtime_error("unknown canvas mode");
        skinIsolationCheck(!h.playlistWindow->isVisible() && !h.mixerWindow->isVisible(),"restart reopened panels");
        std::cout<<"PASS: fresh-process canvas/menu "<<mode<<".\n";return 0;
    }
    const auto menu=h.makeOptionsMenu();
    for(int id:{0,1,2,5,6,7,8})skinIsolationCheck(skinMenuHas(menu,5500+id),"remaining skin missing");
    for(int id:{3,4})skinIsolationCheck(!skinMenuHas(menu,5500+id),"retired skin still in menu");
    for(int id=0;id<9;++id)skinIsolationCheck(skinMenuHas(menu,5520+id),"header experiment missing");
    h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);auto id=h.selectedProjectTrackId();
    h.project.appendPatternNoteTicks(id,0,48,0,128,100);h.project.setPatternCreated(id,0,true);h.project.setPlaylistClip(id,h.project.getPatternId(id,0),0,0,true);
    for(auto& [key,entry]:h.generatorControlWindows)entry.window->setVisible(false);
    h.refreshEnginePlaylist();h.showPlaylist();h.showMixer();h.playlistPanelBounds={430,82,1010,896};h.mixerPanelBounds={0,82,416,896};h.resized();
    auto& grid=*h.playlistWindow->grid;grid.showPatternGrid=false;grid.showPatternNotes=true;grid.showPatternVelocities=false;grid.getTrackColour=[](int){return juce::Colour{};};grid.getTrackGradientHighlight=[](int){return false;};
    std::array<juce::Image,2> original;
    for(int i=0;i<2;++i){h.setSkin(5+i);original[size_t(i)]=skinGridImage(grid);}
    for(int skin:{7,8})for(bool light:{false,true}){
        h.setSkin(skin);const auto mixerBefore=h.mixerWindow->createComponentSnapshot(h.mixerWindow->getLocalBounds());
        grid.onThemeChanged(light);
        skinIsolationCheck(sameSkinImage(original[light?1:0],skinGridImage(grid)),"manual canvas differs from original Navy/Light pixels");
        skinIsolationCheck(h.headerStyle()==8 && sameSkinImage(mixerBefore,h.mixerWindow->createComponentSnapshot(h.mixerWindow->getLocalBounds())),"manual canvas altered silver header or Mixer pixels");
        skinIsolationCheck(h.applicationProperties.getUserSettings()->getBoolValue("playlistCanvasOverride",false),"canvas override not saved");
        h.setHeaderStyle(3);h.setHeaderStyle(8);skinIsolationCheck(sameSkinImage(original[light?1:0],skinGridImage(grid)),"header changed custom canvas");
        h.setSkin(skin);requireProfile(h,skin);skinIsolationCheck(!h.applicationProperties.getUserSettings()->getBoolValue("playlistCanvasOverride",true),"stock selection retained override");
    }
    for(int retired:{3,4}){h.setSkin(retired);requireProfile(h,0);}
    h.setSkin(7);grid.onThemeChanged(false);canvasOverrideCheck(h,false);
    h.headerArtwork.setFolder(juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork"));isolationImage(h,"silver-original-navy-canvas.png");
    std::cout<<"PASS: exact original Dark/Light Playlist pixels on both silver skins; silver header and Mixer unchanged; profile reset; 7 skin choices and all 9 headers retained; retired IDs map safely to Default.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
