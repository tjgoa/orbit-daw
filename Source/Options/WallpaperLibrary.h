#pragma once
#include <JuceHeader.h>

namespace orbit::options
{
// User-owned images live outside the application bundle, so updates preserve them.
class WallpaperLibrary
{
public:
    explicit WallpaperLibrary(juce::File directory) : folder(std::move(directory)) {}
    juce::File folder;

    juce::Array<juce::File> images() const
    {
        auto files=folder.findChildFiles(juce::File::findFiles,false,"*");
        files.removeIf([](const auto& f){return !f.hasFileExtension("png;jpg;jpeg;gif");});
        struct Order {int compareElements(const juce::File& a,const juce::File& b) const
            {return a.getFileName().compareNatural(b.getFileName());}} order;
        files.sort(order);return files;
    }
    static juce::Result load(const juce::File& file,juce::Image& image)
    {
        if(!file.existsAsFile() || !file.hasFileExtension("png;jpg;jpeg;gif"))
            return juce::Result::fail("Choose a PNG, JPEG or GIF image.");
        if(file.getSize()>32*1024*1024)return juce::Result::fail("Choose an image smaller than 32 MiB.");
        image=juce::ImageFileFormat::loadFrom(file);
        if(!image.isValid() || image.getWidth()>8192 || image.getHeight()>8192)
            return juce::Result::fail("Choose a supported image no larger than 8192 pixels per side.");
        return juce::Result::ok();
    }
    juce::Result addImage(const juce::File& source,juce::File& stored,juce::Image& image) const
    {
        auto result=load(source,image);if(result.failed())return result;
        result=folder.createDirectory();if(result.failed())return result;
        if(source.getParentDirectory()==folder){stored=source;return juce::Result::ok();}
        const auto named=folder.getChildFile(source.getFileName());
        if(named.existsAsFile() && named.hasIdenticalContentTo(source))
            {stored=named;return juce::Result::ok();}
        // Never replace another image with the same filename.
        stored=named.getNonexistentSibling();
        if(!source.copyFileTo(stored))return juce::Result::fail("Could not copy the image into "+folder.getFullPathName());
        return juce::Result::ok();
    }
};
}
