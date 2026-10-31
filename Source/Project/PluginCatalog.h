#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include <map>

namespace orbit::project
{
// Keep the plugin's identity/format with the project, independently of the
// machine-wide scan cache. Opaque processor state remains in the native plugin.
inline void capturePluginCatalog(tracktion::engine::Edit& edit)
{
    namespace te=tracktion::engine;
    auto old=edit.state.getChildWithName("ORBITPLUGINCATALOG");
    auto catalog=old.isValid()?old.createCopy():juce::ValueTree("ORBITPLUGINCATALOG");
    for(auto* plugin:te::getAllPlugins(edit,true))if(auto* external=dynamic_cast<te::ExternalPlugin*>(plugin))
    {
        auto description=external->desc;
        if(auto* instance=external->getAudioPluginInstance())description=instance->getPluginDescription();
        if(description.pluginFormatName!="VST3" || description.fileOrIdentifier.isEmpty())continue;
        const auto key=description.createIdentifierString();auto existing=catalog.getChildWithProperty("key",key);
        if(existing.isValid())catalog.removeChild(existing,nullptr);
        juce::ValueTree entry("ENTRY");entry.setProperty("key",key,nullptr);
        if(auto xml=description.createXml())entry.addChild(juce::ValueTree::fromXml(*xml),-1,nullptr);
        catalog.addChild(entry,-1,nullptr);
    }
    if(old.isValid())edit.state.removeChild(old,nullptr);edit.state.addChild(catalog,-1,nullptr);
}
inline void restorePluginCatalog(tracktion::engine::Engine& engine,const juce::ValueTree& state)
{
    auto& known=engine.getPluginManager().knownPluginList;
    for(auto entry:state.getChildWithName("ORBITPLUGINCATALOG"))if(auto xml=entry.getChild(0).createXml())
    {
        juce::PluginDescription description;
        if(description.loadFromXml(*xml) && description.pluginFormatName=="VST3"
            && juce::File::createFileWithoutCheckingPath(description.fileOrIdentifier).exists())known.addType(description);
    }
    // Older projects have native filename/UID fields but no catalog. Match
    // exact class IDs (or one unambiguous name); never guess a shell's class.
    std::vector<juce::ValueTree> pending{state};std::map<juce::String,juce::Array<juce::PluginDescription>> scanned;
    while(!pending.empty())
    {
        auto node=pending.back();pending.pop_back();
        if(node.hasType("PLUGIN") && node.getProperty("type").toString()=="vst")
        {
            const auto path=node.getProperty("filename").toString();
            const auto unique=int(node.getProperty("uniqueId").toString().getHexValue32());
            const auto deprecated=int(node.getProperty("uid").toString().getHexValue32());
            bool found=false;
            for(const auto& d:known.getTypes())if(d.fileOrIdentifier==path && ((unique!=0 && d.uniqueId==unique) || (deprecated!=0 && d.deprecatedUid==deprecated))){found=true;break;}
            if(found || !path.endsWithIgnoreCase(".vst3") || !juce::File::createFileWithoutCheckingPath(path).exists())continue;
            auto [scan,inserted]=scanned.try_emplace(path);
            if(inserted){juce::OwnedArray<juce::PluginDescription> descriptions;juce::VST3PluginFormat format;format.findAllTypesForFile(descriptions,path);for(auto* d:descriptions)scan->second.add(*d);}
            const juce::PluginDescription* match=nullptr;int matches=0;
            for(const auto& d:scan->second)
                if((unique!=0 && d.uniqueId==unique) || (deprecated!=0 && d.deprecatedUid==deprecated)
                    || (unique==0 && deprecated==0 && d.name==node.getProperty("name").toString())){match=&d;++matches;}
            if(matches==1)known.addType(*match);
            continue;
        }
        for(auto child:node)pending.push_back(child);
    }
}
}
