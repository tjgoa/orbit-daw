#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

namespace orbit::ui::sprites
{
// An immutable, decoded filmstrip. Only the selected strip is held in memory.
// All file access and decoding happen before this reaches a paint callback.
struct Entry
{
    juce::String id,name;
    juce::File file,library;
    int width=0,height=0,frames=0;
    bool horizontal=false;
    int polarity=0; // 0 unknown, 1 unipolar, 2 bipolar; explicit metadata only.
};
struct Sprite
{
    Entry entry;
    juce::Image image;
    juce::Rectangle<int> artworkBounds;
    int frameIndex(float value)const
    {return juce::roundToInt(juce::jlimit(0.f,1.f,std::isfinite(value)?value:0.f)*float(entry.frames-1));}
    juce::Rectangle<int> frameBounds(float value)const
    {const int f=frameIndex(value);return {entry.horizontal?f*entry.width:0,entry.horizontal?0:f*entry.height,entry.width,entry.height};}
    void draw(juce::Graphics& g,juce::Rectangle<float> target,float value)const
    {
        const auto frame=frameBounds(value);
        const auto source=artworkBounds.isEmpty()?frame:artworkBounds.translated(frame.getX(),frame.getY());
        const float scale=juce::jmin(target.getWidth()/float(source.getWidth()),target.getHeight()/float(source.getHeight()));
        const auto dest=target.withSizeKeepingCentre(source.getWidth()*scale,source.getHeight()*scale).toNearestInt();
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(image,dest.getX(),dest.getY(),dest.getWidth(),dest.getHeight(),source.getX(),source.getY(),source.getWidth(),source.getHeight(),false);
    }
};
using Ptr=std::shared_ptr<const Sprite>;
struct Provider
{
    virtual ~Provider()=default;
    virtual const Sprite* mixerSprite()const=0;
    virtual const Sprite* mixerSpriteForRole(int)const{return mixerSprite();}
};
inline const Sprite* forComponent(const juce::Component& c)
{for(auto* p=&c;p!=nullptr;p=p->getParentComponent())if(auto* provider=dynamic_cast<const Provider*>(p))return provider->mixerSpriteForRole(int(c.getProperties().getWithDefault("orbitMixerKnobRole",0)));return nullptr;}

struct Catalogue
{
    juce::File folder;
    std::vector<Entry> entries;
    int skipped=0;
    juce::String error;
};
inline Catalogue scan(juce::File folder)
{
    Catalogue result;result.folder=folder;
    // Accept either the export root or its nested sprites directory.
    if(!folder.getChildFile("manifest.json").existsAsFile() && folder.getFileName()=="sprites")folder=folder.getParentDirectory();
    result.folder=folder;const auto manifest=folder.getChildFile("manifest.json");
    if(!manifest.existsAsFile()){result.error="Choose the export folder containing manifest.json and the sprites folder.";return result;}
    if(manifest.getSize()>16*1024*1024){result.error="The sprite manifest is too large (limit 16 MB).";return result;}
    const auto data=juce::JSON::parse(manifest.loadFileAsString());
    const auto* items=data["knobs"].getArray();
    if(data["format"].toString()!="orbit-sprite-manifest-v1" || items==nullptr || items->size()>4096)
    {result.error="This is not a supported Orbit KnobMan export manifest.";return result;}
    std::set<juce::String> ids;
    for(const auto& item:*items)
    {
        auto integer=[&](const char* key){const auto n=item[key];if(!(n.isInt()||n.isInt64()||n.isDouble()))return 0;const double d=double(n);return std::isfinite(d)&&d>=1&&d<=32767&&std::floor(d)==d?int(d):0;};
        Entry e;e.library=folder;e.id=item["id"].toString();const auto source=item["source"].toString().replaceCharacter('\\','/');e.name=source.fromLastOccurrenceOf("/",false,false).upToLastOccurrenceOf(".",false,false);
        const auto polarity=item["polarity"].toString().toLowerCase();e.polarity=polarity=="bipolar"?2:polarity=="unipolar"?1:0;
        e.width=integer("frame_width");e.height=integer("frame_height");e.frames=integer("frames");
        const auto orientation=item["orientation"].toString();e.horizontal=orientation=="horizontal";
        const auto relative=item["sprite"].toString().replaceCharacter('\\','/');
        // Treat manifest paths as data, never commands or arbitrary absolute files.
        const bool safePath=relative.startsWith("sprites/")&&!relative.contains("../")&&!relative.contains("//")&&!relative.containsChar(':');
        e.file=safePath?folder.getChildFile(relative):juce::File{};
        const int64_t pixels=int64_t(e.width)*e.height*e.frames;
        const int64_t stripWidth=int64_t(e.width)*(e.horizontal?e.frames:1),stripHeight=int64_t(e.height)*(e.horizontal?1:e.frames);
        if(item["status"].toString()!="ok" || e.id.isEmpty() || e.id.length()>200 || !ids.insert(e.id).second || e.name.isEmpty() || !safePath || !e.file.existsAsFile()
           || e.width==0 || e.height==0 || e.frames==0 || pixels>32*1024*1024 || stripWidth>32767 || stripHeight>32767 || (orientation!="vertical"&&orientation!="horizontal"))
        {++result.skipped;continue;}
        result.entries.push_back(std::move(e));
    }
    std::sort(result.entries.begin(),result.entries.end(),[](const auto& a,const auto& b){const auto order=a.name.compareNatural(b.name);return order==0?a.id<b.id:order<0;});
    if(result.entries.empty())result.error="No usable sprites were found in this export. Check its manifest and PNG files.";
    return result;
}
inline Ptr load(const Entry& e,juce::String& error)
{
    error.clear();
    if(!e.file.existsAsFile() || e.file.getSize()>64*1024*1024){error="Sprite PNG missing or too large (limit 64 MB).";return {};}
    // Check the PNG header before a decoder can allocate based on untrusted dimensions.
    auto stream=e.file.createInputStream();unsigned char header[24]{};
    const unsigned char signature[8]{137,80,78,71,13,10,26,10};
    if(stream==nullptr || stream->read(header,24)!=24 || std::memcmp(header,signature,8)!=0 || std::memcmp(header+12,"IHDR",4)!=0)
    {error="The selected file is not a PNG sprite strip.";return {};}
    auto word=[&](int i){return (uint32_t(header[i])<<24)|(uint32_t(header[i+1])<<16)|(uint32_t(header[i+2])<<8)|uint32_t(header[i+3]);};
    const int64_t w=int64_t(e.width)*(e.horizontal?e.frames:1),h=int64_t(e.height)*(e.horizontal?1:e.frames);
    if(e.width<1||e.height<1||e.frames<1||w>32767||h>32767||w*h>32*1024*1024||word(16)!=w||word(20)!=h)
    {error="PNG dimensions do not match the exported frame metadata.";return {};}
    auto image=juce::ImageFileFormat::loadFrom(e.file);
    if(!image.isValid() || image.getWidth()!=w || image.getHeight()!=h){error="The sprite PNG could not be decoded.";return {};}
    auto sprite=std::make_shared<Sprite>();sprite->entry=e;sprite->image=std::move(image);
    // Crop common transparent padding across all frames, never per-frame: the
    // knob must not wobble as it rotates. Keep soft shadows above 1/255 alpha.
    int left=e.width,top=e.height,right=0,bottom=0;
    juce::Image::BitmapData pixels(sprite->image,juce::Image::BitmapData::readOnly);
    for(int f=0;f<e.frames;++f)for(int y=0;y<e.height;++y)for(int x=0;x<e.width;++x)
        if(pixels.getPixelColour(x+(e.horizontal?f*e.width:0),y+(e.horizontal?0:f*e.height)).getAlpha()>1)
        {left=juce::jmin(left,x);top=juce::jmin(top,y);right=juce::jmax(right,x+1);bottom=juce::jmax(bottom,y+1);}
    if(right>left && bottom>top)sprite->artworkBounds={left,top,right-left,bottom-top};
    return sprite;
}
}
