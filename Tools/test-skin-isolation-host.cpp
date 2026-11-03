#ifndef SKIN_TEST_OUTPUT_DIR
#define SKIN_TEST_OUTPUT_DIR "OPTIONS_CHECK_FOLDER"
#endif
static void skinIsolationCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool sameSkinImage(const juce::Image& a,const juce::Image& b){if(a.getBounds()!=b.getBounds() || a.getFormat()!=b.getFormat())return false;juce::Image::BitmapData aa(a,juce::Image::BitmapData::readOnly),bb(b,juce::Image::BitmapData::readOnly);if(aa.pixelStride!=bb.pixelStride)return false;for(int y=0;y<a.getHeight();++y)if(std::memcmp(aa.getLinePointer(y),bb.getLinePointer(y),size_t(a.getWidth()*aa.pixelStride))!=0)return false;return true;}
static juce::Image skinGridImage(PlaylistGrid& grid){return grid.createComponentSnapshot(grid.getLocalBounds().withWidth(juce::jmin(960,grid.getWidth())).withHeight(juce::jmin(760,grid.getHeight())));}
static void isolationImage(juce::Component& c,const char* name){auto im=c.createComponentSnapshot(c.getLocalBounds());juce::FileOutputStream out(juce::File(SKIN_TEST_OUTPUT_DIR).getChildFile(name));skinIsolationCheck(juce::PNGImageFormat().writeImageToStream(im,out),"skin preview export failed");}
static void requireProfile(ProjectView& h,int index){const auto spec=orbit::options::workspaceAppearance(index);skinIsolationCheck(spec.has_value(),"missing complete skin profile");skinIsolationCheck(h.skinIsSelected(index),"selected profile is incomplete");skinIsolationCheck(h.headerStyle()==spec->headerStyle && h.mixerWindow->getMixerSkin()==spec->mixerSkin && h.playlistWindow->isLightTheme()==spec->lightCanvas && h.pianoRollWindow->isLightTheme()==spec->lightCanvas && orbit::options::compact::variant(*h.playlistWindow)==spec->playlistVariant,"skin inherited another profile's header or palette");}
int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1440,1000);
 if(argc>1){const juce::String mode(argv[1]);skinIsolationCheck(!h.playlistWindow->isVisible() && !h.mixerWindow->isVisible(),"skin startup reopened document windows");
  if(mode=="--silver2"){requireProfile(h,8);h.setSkin(5);}
  else if(mode=="--navy"){requireProfile(h,5);h.setHeaderStyle(3);h.mixerWindow->handleMixerMenuResult(12);}
  else if(mode=="--custom"){skinIsolationCheck(h.optionInt("skin")==5 && h.headerStyle()==3 && h.mixerWindow->getMixerSkin()==2 && orbit::options::compact::variant(*h.playlistWindow)==0 && !h.skinIsSelected(5),"explicit custom header/mixer override did not persist independently");h.setSkin(6);}
  else if(mode=="--light"){requireProfile(h,6);auto* s=h.applicationProperties.getUserSettings();s->setValue("options.skin",5);s->setValue("options.headerStyle",8);s->setValue("mixerSkin",4);s->setValue("playlistLightTheme",true);s->setValue("pianoRollLightTheme",true);s->setValue("options.independentSkinProfiles",false);s->saveIfNeeded();}
  else if(mode=="--migration")requireProfile(h,5);
  else throw std::runtime_error("unknown skin test mode");
  std::cout<<"PASS: fresh-process skin isolation "<<mode<<".\n";return 0;
 }
 h.headerArtwork.setFolder(juce::File::getCurrentWorkingDirectory().getChildFile("Source/Options/Artwork"));
 h.createProjectDiscardingCurrent();for(int i=0;i<2;++i){h.addInstrumentTrack(true);auto id=h.selectedProjectTrackId();h.project.appendPatternNoteTicks(id,0,48,0,128,100);h.project.appendPatternNoteTicks(id,0,52,384,128,100);h.project.setPatternCreated(id,0,true);h.project.setPlaylistClip(id,h.project.getPatternId(id,0),0,0,true);}
 for(auto& [id,entry]:h.generatorControlWindows)entry.window->setVisible(false);
 h.refreshEnginePlaylist();h.showPlaylist();h.showMixer();h.playlistPanelBounds={430,82,1010,896};h.mixerPanelBounds={0,82,416,896};h.resized();
 auto& grid=*h.playlistWindow->grid;grid.showPatternGrid=false;grid.showPatternNotes=true;grid.showPatternVelocities=false;grid.getTrackColour=[](int){return juce::Colour{};};grid.getTrackGradientHighlight=[](int){return false;};
 std::array<juce::Image,2> goldenGrid,goldenMixer;
 for(int i=0;i<2;++i){h.setSkin(5+i);requireProfile(h,5+i);goldenGrid[size_t(i)]=skinGridImage(grid);goldenMixer[size_t(i)]=h.mixerWindow->createComponentSnapshot(h.mixerWindow->getLocalBounds());skinIsolationCheck(goldenGrid[size_t(i)].getPixelAt(grid.labelWidth()+3*grid.barWidth()+grid.barWidth()/2,grid.headerHeight()+grid.rowHeight()/2)==(i==0?juce::Colour(25,38,49):juce::Colour(169,181,196)),"original Navy/Light canvas RGB drifted");skinIsolationCheck(goldenGrid[size_t(i)].getPixelAt(grid.labelWidth()+grid.barWidth()/2,grid.headerHeight()+grid.rowHeight()-20)==juce::Colour(39,125,165),"original blue clip RGB drifted");skinIsolationCheck(h.headerBrandBounds.isEmpty(),"original Classic header gained an Orbit brand panel");}
 for(int from:{0,1,2,5,6,7,8})for(int target:{0,5,6,7,8}){
  h.setSkin(from);h.setHeaderStyle(8);h.setSkin(target);requireProfile(h,target);
  skinIsolationCheck(h.optionInt("wallpaper")==0,"skin changed wallpaper selection");
  if(target==5 || target==6){const int i=target-5;skinIsolationCheck(sameSkinImage(goldenGrid[size_t(i)],skinGridImage(grid)),"returning to Navy/Light changed original Playlist pixels");skinIsolationCheck(sameSkinImage(goldenMixer[size_t(i)],h.mixerWindow->createComponentSnapshot(h.mixerWindow->getLocalBounds())),"returning to Navy/Light changed original Mixer pixels");}
  const auto variant=orbit::options::compact::variant(*h.playlistWindow);const auto light=h.playlistWindow->isLightTheme();const auto mixer=h.mixerWindow->getMixerSkin();const auto gridBefore=skinGridImage(grid);
  for(int header=0;header<9;++header){h.setHeaderStyle(header);skinIsolationCheck(orbit::options::compact::variant(*h.playlistWindow)==variant && h.playlistWindow->isLightTheme()==light && h.mixerWindow->getMixerSkin()==mixer,"Header Style recoloured another panel");skinIsolationCheck(sameSkinImage(gridBefore,skinGridImage(grid)),"header experiment altered Playlist pixels");}
 }
 for(int stock:{0,5,6}){h.setSkin(stock);for(auto rgb:std::array<std::array<juce::uint8,3>,4>{{{{20,29,38}},{{39,125,165}},{{220,244,250}},{{177,187,202}}}})skinIsolationCheck(orbit::options::compact::colour(grid,rgb[0],rgb[1],rgb[2])==juce::Colour(rgb[0],rgb[1],rgb[2]),"original base Playlist colour was remapped");isolationImage(h,stock==0?"default-classic.png":stock==5?"navy-classic.png":"light-classic.png");}
 h.setSkin(8);requireProfile(h,8);skinIsolationCheck(orbit::options::compact::colour(grid,220,244,250)==juce::Colours::white,"Compact Silver 2 notes are not white");
 for(bool showGrid:{false,true}){grid.showPatternGrid=showGrid;auto im=skinGridImage(grid);int white=0;for(int y=grid.headerHeight()+5;y<grid.headerHeight()+grid.rowHeight()-19;++y)for(int x=grid.labelWidth()+8;x<grid.labelWidth()+grid.barWidth()-12;++x){auto c=im.getPixelAt(x,y);if(c.getRed()>240 && c.getGreen()>240 && c.getBlue()>240)++white;}skinIsolationCheck(white>10,"white note previews missing from actual clip rendering");}
 grid.showPatternGrid=false;isolationImage(h,"silver2-white-notes.png");
 for(int i:{0,1,2,5,6,7,8}){h.setSkin(i);requireProfile(h,i);h.toggleMixerMaximized();h.togglePlaylistMaximized();skinIsolationCheck(h.mixerWindow->getBounds()==h.maximizedPlaylistBounds() && h.playlistWindow->getBounds()==h.maximizedPlaylistBounds(),"skin isolation regressed full workspace edges");h.setHeaderStyle(8);h.showMixer(true);skinIsolationCheck(h.mixerWindow->getBounds()==h.maximizedPlaylistBounds(),"header/bank changed maximized bounds");h.showMixer();h.toggleMixerMaximized();h.togglePlaylistMaximized();}
 h.setSkin(8);std::cout<<"PASS: all seven complete skin profiles; cross-profile transitions and all nine header overrides; exact Navy/Light Playlist+Mixer pixels restored; original RGB values unchanged; both Silver 2 preview modes have white notes; full workspace bounds and all header choices retained.\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
