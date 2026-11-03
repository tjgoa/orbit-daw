#include "KnobExplorer/KnobStyleRenderer.h"
static void sharedCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::MouseEvent sharedMouse(juce::Component* c,juce::Point<float> p,juce::Point<float> down={},bool drag=false)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};}
int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI gui;
 if(argc>1 && juce::String(argv[1])=="--gallery"){
  sharedCheck(orbit::knobs::homeAtlas().isValid(),"Labyrinth atlas failed to decode");
  juce::Array<juce::var> presets;
  for(int style=0;style<20;++style){
   juce::Image atlas(juce::Image::ARGB,96*16,96*8,true);juce::Graphics g(atlas);
   for(int frame=0;frame<128;++frame)orbit::knobs::draw(g,{float((frame%16)*96),float((frame/16)*96),96,96},style,frame/127.f);
   const auto name="knob-"+juce::String(style)+".png";juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name));
   sharedCheck(juce::PNGImageFormat().writeImageToStream(atlas,out),"native atlas export failed");
   auto* obj=new juce::DynamicObject;obj->setProperty("id",style);obj->setProperty("name",orbit::knobs::styles()[size_t(style)].name);obj->setProperty("file",name);presets.add(juce::var(obj));
  }
  juce::File("OPTIONS_CHECK_FOLDER/knobs.json").replaceWithText(juce::JSON::toString(juce::var(presets)));
  std::cout<<"PASS: 20 native JUCE knob atlases, each 128 frames; original Labyrinth atlas decoded.\n";return 0;
 }
 ProjectView h;h.stopTimer();h.setSize(1440,1000);
 const auto file=juce::File("OPTIONS_CHECK_FOLDER/shared-pattern.orbit");
 if(argc>1){sharedCheck(h.loadProjectFile(file),"fresh reopen failed");const auto id=h.project.getTrackId(0);sharedCheck(h.project.getPatternLengthSteps(id,0)==160,"fresh reopen lost shared length");for(const auto& c:h.project.getPlaylistClips(id))if(!c.exactLength && c.patternId==h.project.getPatternId(id,0))sharedCheck(c.lengthBars==10,"fresh reopen lost linked occurrence length");std::cout<<"PASS: shared length, overlaps and exact trims reopen in fresh process.\n";return 0;}
 h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);h.addInstrumentTrack(true);h.showPlaylist();h.setPlaybackMode(ProjectView::PlaybackMode::song);
 const auto id=h.project.getTrackId(0),other=h.project.getTrackId(1),a=h.project.getPatternId(id,0);h.selectGeneratorControlPattern(id,0);
 h.project.setPatternCreated(id,0,true);h.project.appendPatternNoteTicks(id,0,48,0,128,100);
 for(int bar:{0,1,4})h.project.setPlaylistClip(id,a,bar,0,true);
 h.project.setPlaylistClip(other,h.project.getPatternId(other,0),0,0,true);
 h.refreshEnginePlaylist();auto undo=h.undoHistory.size();h.resizePlaylistPatternClip(0,0,2);
 auto ordinary=[&](int bars,int expected){int n=0;for(const auto& c:h.project.getPlaylistClips(id))if(c.patternId==a && !c.exactLength){sharedCheck(c.lengthBars==bars,"an existing ordinary occurrence kept its old length");sharedCheck(h.project.playlistClipFollowsPatternLength(id,c.startBar),"occurrence is not linked");++n;}sharedCheck(n==expected,"resize removed or duplicated overlapping occurrences");};
 ordinary(2,3);sharedCheck(h.project.getPatternLengthSteps(id,0)==32 && h.undoHistory.size()==undo+1,"two bars did not update A1 in one Undo");
 sharedCheck(h.project.getPatternLengthSteps(other,0)==16,"resized another generator's A1");
 sharedCheck(h.project.getPlaylistClipStartBar(id,1.5)==1,"overlap hit chose an earlier hidden occurrence");
 int nativeCount=0;for(auto* c:h.getGeneratorTracks()[0]->getClips())if(auto* midi=dynamic_cast<te::MidiClip*>(c)){sharedCheck(std::abs(midi->getEditTimeRange().getLength().inSeconds()-2*h.playlistSecondsPerBar())<1.e-6,"native clip kept old playback duration");++nativeCount;}
 sharedCheck(nativeCount==3,"native overlap rebuild removed occurrences");
 h.undoProjectAction();sharedCheck(h.project.getPatternLengthSteps(id,0)==16 && h.project.getPlaylistClips(id).size()==3,"Undo did not restore pattern length and placements");h.redoProjectAction();ordinary(2,3);
 h.paintPlaylistClip(0,8);h.finishPlaylistPaintGesture();ordinary(2,4);
 h.paintPlaylistClipRange(0,12,18);h.finishPlaylistPaintGesture();ordinary(2,8);
 h.resizePlaylistPatternClip(0,18,10);ordinary(10,8);
 // Real right-edge mouse gesture can shrink the shared source again.
 auto* grid=h.playlistWindow->grid;grid->setHorizontalZoom(.25f);grid->setViewportArea({0,0,1440,700});grid->setDrawMode(true);grid->setSnapToGrid(true);grid->setSnapIntervalSteps(16);
 const juce::Point<float> p{float(grid->labelWidth()+28*grid->barWidth()-2),float(grid->headerHeight()+grid->rowHeight()/2)};
 auto down=sharedMouse(grid,p);grid->mouseDown(down);auto end=sharedMouse(grid,{p.x-float(8*grid->barWidth()),p.y},down.position,true);grid->mouseDrag(end);grid->mouseUp(end);ordinary(2,8);
 // A fine trim stays local; exact recorder segments do not acquire the full source length.
 h.project.setPlaylistClipExactBars(id,a,40,.5);h.resizePlaylistPatternClip(0,40,.75);sharedCheck(h.project.getPlaylistClipLengthBars(id,40)==.75 && h.project.getPatternLengthSteps(id,0)==32,"exact trim changed shared source");
 h.resizePlaylistPatternClip(0,18,10);ordinary(10,8);sharedCheck(h.project.getPlaylistClipLengthBars(id,40)==.75,"source resize stretched an exact performance segment");
 h.project.setPlaylistClip(id,{},1.25,1,false);ordinary(10,7);sharedCheck(h.project.getPlaylistClipStartBar(id,.5)==0,"deleting top overlap erased underlying occurrence");
 h.saveToFile(file);sharedCheck(file.existsAsFile(),"save failed");sharedCheck(h.loadProjectFile(file),"reopen failed");ordinary(10,7);
 std::cout<<"PASS: whole-bar edge resize shares A1 length; all old/new/range-painted copies update, overlaps survive, topmost hit/delete, native durations, shrink gesture, one Undo/Redo, independent generators, exact trims and save/reopen.\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
