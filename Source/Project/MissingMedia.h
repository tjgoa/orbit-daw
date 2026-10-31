#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "../Import/WaveMediaInfo.h"
#include <map>
#include <set>
#include <filesystem>

namespace orbit::media
{
inline juce::String leafName(juce::String path)
{return path.replaceCharacter('\\','/').fromLastOccurrenceOf("/",false,false);}
struct Missing
{
    tracktion::engine::WaveAudioClip* clip=nullptr;
    tracktion::engine::SamplerPlugin* sampler=nullptr;
    int sound=-1;
    juce::String reference;
};
inline std::vector<Missing> collect(tracktion::engine::Edit& edit)
{
    namespace te=tracktion::engine;std::vector<Missing> result;
    for(auto* track:te::getAudioTracks(edit))
    {
        for(int i=0;i<track->getNumTrackItems();++i)
            if(auto* clip=dynamic_cast<te::WaveAudioClip*>(track->getTrackItem(i)))
            {
                auto& source=clip->getSourceFileReference();
                if(source.getFile().existsAsFile())continue;
                const auto original=clip->state.getProperty("orbitOriginalReference",source.source.get()).toString();
                if(original.isNotEmpty())result.push_back({clip,nullptr,-1,original});
            }
        for(auto* plugin:track->pluginList)
            if(auto* sampler=dynamic_cast<te::SamplerPlugin*>(plugin))
                for(int sound=0;sound<sampler->getNumSounds();++sound)
                {
                    const auto reference=sampler->getSoundMedia(sound);
                    if(reference.isNotEmpty() && !te::SourceFileReference::findFileFromString(edit,reference).existsAsFile())
                        result.push_back({nullptr,sampler,sound,reference});
                }
    }
    return result;
}
inline bool matches(const Missing& missing,const juce::File& file)
{
    import::WaveMediaInfo info;std::string error;
    if(!import::inspectWaveMedia(std::filesystem::u8path(file.getFullPathName().toStdString()),info,error))return false;
    if(missing.clip==nullptr)return true;
    const auto& state=missing.clip->state;
    if(!state.hasProperty("orbitExpectedFrames"))
        return missing.clip->getPosition().offset.inSeconds()+missing.clip->getPosition().getLength().inSeconds()
            <=double(info.frames)/info.sampleRate+0.5/info.sampleRate;
    return int(state.getProperty("orbitExpectedRate"))==int(info.sampleRate)
        && int(state.getProperty("orbitExpectedChannels"))==info.channels
        && juce::int64(state.getProperty("orbitExpectedFrames"))==juce::int64(info.frames)
        && int(state.getProperty("orbitExpectedFormat"))==info.formatTag
        && int(state.getProperty("orbitExpectedBits"))==info.bitsPerSample;
}
using FileIndex=std::map<juce::String,std::vector<juce::File>>;
inline FileIndex scan(const juce::File& folder,const std::set<juce::String>& wanted,bool& truncated)
{
    FileIndex result;std::error_code error;truncated=false;std::size_t visited=0;
    std::filesystem::recursive_directory_iterator i(std::filesystem::u8path(folder.getFullPathName().toStdString()),
        std::filesystem::directory_options::skip_permission_denied,error),end;
    for(;!error && i!=end;i.increment(error))
    {
        if(++visited>200000){truncated=true;break;}
        if(!i->is_regular_file(error))continue;
        const auto utf8=i->path().u8string();
        const juce::File file(juce::String::fromUTF8(reinterpret_cast<const char*>(utf8.data()),static_cast<int>(utf8.size())));
        const auto key=file.getFileName().toLowerCase();
        if(wanted.count(key))result[key].push_back(file);
    }
    if(error)truncated=true;
    return result;
}
struct Match { Missing missing;juce::File file; };
inline std::vector<Match> plan(const std::vector<Missing>& missing,const FileIndex& files)
{
    std::vector<Match> result;
    for(const auto& item:missing)
    {
        const auto found=files.find(leafName(item.reference).toLowerCase());if(found==files.end())continue;
        std::vector<juce::File> valid;
        for(const auto& file:found->second)if(matches(item,file))valid.push_back(file);
        // Leave ambiguous same-name, same-metadata files unresolved.
        if(valid.size()==1)result.push_back({item,valid.front()});
    }
    return result;
}
inline void apply(const std::vector<Match>& matches)
{
    for(const auto& match:matches)
    {
        if(auto* clip=match.missing.clip)
        {
            clip->getSourceFileReference().setToDirectFileReference(match.file,false);
            clip->state.setProperty("orbitMissingMedia",false,nullptr);
            if(clip->getName().startsWith("Missing: "))clip->setName(clip->getName().substring(9));
        }
        else if(match.missing.sampler!=nullptr)
            match.missing.sampler->setSoundMedia(match.missing.sound,match.file.getFullPathName());
    }
}
}
