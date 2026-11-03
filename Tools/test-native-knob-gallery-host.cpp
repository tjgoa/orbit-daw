#include "KnobExplorer/KnobStyleRenderer.h"
static void galleryCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool galleryEqual(const juce::Image& a,const juce::Image& b){if(a.getBounds()!=b.getBounds())return false;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)if(a.getPixelAt(x,y)!=b.getPixelAt(x,y))return false;return true;}
int main(){try{
 juce::ScopedJuceInitialiser_GUI gui;
 // Exercise the actual mixer palette property, including repeat transitions.
 ProjectView host;host.stopTimer();host.setSize(1440,1000);host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);host.setKnobStyle(2);host.showMixer();auto& mixer=*host.mixerWindow;auto* strip=mixer.channelStrips[0];
 const auto geometry=mixer.getBounds();
 for(int skin:{0,1,2,3,4,1,3}){mixer.setMixerSkin(skin);galleryCheck(orbit::ui::lightMixerSurface(strip->eqControls[0][0])==(skin>=3),"mixer light-surface scope incorrect");galleryCheck(mixer.getBounds()==geometry,"palette changed mixer geometry");}
 mixer.setMixerSkin(3);mixer.setKnobSize(200);auto withShadow=strip->createComponentSnapshot(strip->getLocalBounds());mixer.getProperties().set("orbitMixerLightSurface",false);auto dry=strip->createComponentSnapshot(strip->getLocalBounds());galleryCheck(!galleryEqual(withShadow,dry),"silver mixer shadow absent");mixer.getProperties().set("orbitMixerLightSurface",true);
 {juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/silver-mixer-shadow.png"));juce::PNGImageFormat().writeImageToStream(mixer.createComponentSnapshot(mixer.getLocalBounds()),out);}
 // Export native renders at actual 1x/2x frame sizes used by HTML. Validate PNG
 // readback against a separate direct call at the same position and dimensions.
 const int count=int(orbit::knobs::styles().size());galleryCheck(count==33,"style count wrong");juce::Array<juce::var> presets;
 juce::Image sheet(juce::Image::RGB,6*180,6*160,true);juce::Graphics preview(sheet);preview.fillAll(juce::Colour(0xff1e2c36));
 for(int style=0;style<count;++style){juce::var objectHolder(new juce::DynamicObject);auto* obj=objectHolder.getDynamicObject();obj->setProperty("id",style);obj->setProperty("name",orbit::knobs::styles()[size_t(style)].name);juce::var imagesHolder(new juce::DynamicObject);auto* images=imagesHolder.getDynamicObject();
  for(int pixels:{32,48,64,96,128}){
   juce::Image atlas(juce::Image::ARGB,pixels*16,pixels*8,true);
   for(int frame=0;frame<128;++frame){juce::Image single(juce::Image::ARGB,pixels,pixels,true);{juce::Graphics g(single);orbit::knobs::draw(g,single.getBounds().toFloat(),style,frame/127.f);}juce::Image::BitmapData src(single,juce::Image::BitmapData::readOnly),dst(atlas,juce::Image::BitmapData::writeOnly);for(int row=0;row<pixels;++row)std::memcpy(dst.getPixelPointer(frame%16*pixels,frame/16*pixels+row),src.getLinePointer(row),size_t(pixels*src.pixelStride));}
   const auto name="knob-"+juce::String(style)+"-"+juce::String(pixels)+".png";auto file=juce::File("OPTIONS_CHECK_FOLDER").getChildFile(name);{juce::FileOutputStream out(file);galleryCheck(juce::PNGImageFormat().writeImageToStream(atlas,out),"atlas PNG export failed");}auto decoded=juce::ImageFileFormat::loadFrom(file);galleryCheck(decoded.isValid(),"native PNG readback failed");
   for(int frame:{0,64,127}){juce::Image direct(juce::Image::ARGB,pixels,pixels,true);{juce::Graphics g(direct);orbit::knobs::draw(g,direct.getBounds().toFloat(),style,frame/127.f);}juce::MemoryOutputStream encoded;galleryCheck(juce::PNGImageFormat().writeImageToStream(direct,encoded),"independent frame encode failed");auto independent=juce::ImageFileFormat::loadFrom(encoded.getData(),encoded.getDataSize());for(int y=0;y<pixels;++y)for(int x=0;x<pixels;++x)if(direct.getPixelAt(x,y)!=atlas.getPixelAt(frame%16*pixels+x,frame/16*pixels+y) || independent.getPixelAt(x,y)!=decoded.getPixelAt(frame%16*pixels+x,frame/16*pixels+y)){std::cerr<<"Mismatch style="<<style<<" size="<<pixels<<" frame="<<frame<<" xy="<<x<<","<<y<<" direct="<<direct.getPixelAt(x,y).toDisplayString(true)<<" decoded="<<decoded.getPixelAt(frame%16*pixels+x,frame/16*pixels+y).toDisplayString(true)<<" atlas="<<atlas.getPixelAt(frame%16*pixels+x,frame/16*pixels+y).toDisplayString(true)<<"\n";throw std::runtime_error("HTML atlas differs from direct native render");}}
   images->setProperty(juce::String(pixels),name);
  }
  obj->setProperty("atlases",imagesHolder);presets.add(objectHolder);
  auto card=juce::Rectangle<float>(float(style%6*180),float(style/6*160),180,160);orbit::knobs::draw(preview,{card.getX()+42,card.getY()+10,96,96},style,.5f);preview.setColour(juce::Colours::white);preview.setFont(12);preview.drawFittedText(juce::String(style)+"  "+orbit::knobs::styles()[size_t(style)].name,card.withTrimmedTop(115).toNearestInt().reduced(6),juce::Justification::centred,2);
 }
 juce::File("OPTIONS_CHECK_FOLDER/knobs.json").replaceWithText(juce::JSON::toString(juce::var(presets)));
 {juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/native-contact-sheet.png"));juce::PNGImageFormat().writeImageToStream(sheet,out);}
 std::cout<<"PASS scoped silver shadow, fixed geometry, 33 compiled JUCE renderers x five native sizes x 128 frames; raw min/mid/max frames and independently PNG-encoded native frames match atlas output pixel-for-pixel.\n";return 0;
 }catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}}
