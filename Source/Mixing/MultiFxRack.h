#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "DistortionPlugin.h"
#include "LookaheadLimiterPlugin.h"
#include "PeakCompressorPlugin.h"
namespace orbit::mixing
{
inline const std::array<const char*,10> effectNames{"Compressor","Reverb","Delay","Chorus","Phaser","Low-pass Filter","Look-ahead Brickwall Limiter","Peak Compressor","Tube Distortion","Two-Band Distortion"};
inline const std::array<const char*,10> effectTypes{tracktion::engine::CompressorPlugin::xmlTypeName,tracktion::engine::ReverbPlugin::xmlTypeName,tracktion::engine::DelayPlugin::xmlTypeName,tracktion::engine::ChorusPlugin::xmlTypeName,tracktion::engine::PhaserPlugin::xmlTypeName,tracktion::engine::LowPassPlugin::xmlTypeName,LookaheadLimiterPlugin::xmlTypeName,PeakCompressorPlugin::xmlTypeName,TubeDistortionPlugin::xmlTypeName,TwoBandDistortionPlugin::xmlTypeName};
inline bool isMultiFx(const tracktion::engine::RackInstance* instance)
{return instance && instance->type && bool(instance->type->state.getProperty("orbitMultiFx",false));}
inline void connectSerial(tracktion::engine::RackType& rack)
{
    const auto links=rack.getConnections();
    for(const auto* c:links)rack.removeConnection(c->sourceID,c->sourcePin,c->destID,c->destPin);
    auto plugins=rack.getPlugins();tracktion::engine::EditItemID previous;
    for(auto* plugin:plugins){for(int pin=0;pin<3;++pin)rack.addConnection(previous,pin,plugin->itemID,pin);previous=plugin->itemID;}
    for(int pin=0;pin<3;++pin)rack.addConnection(previous,pin,{},pin);
}
inline tracktion::engine::Plugin::Ptr createMultiFx(tracktion::engine::Edit& edit)
{
    auto rack=edit.getRackList().addNewRack();if(!rack)return {};
    rack->rackName="Multi FX";rack->state.setProperty("orbitMultiFx",true,&edit.getUndoManager());connectSerial(*rack);
    return edit.getPluginCache().createNewPlugin(tracktion::engine::RackInstance::create(*rack));
}
inline bool addRackEffect(tracktion::engine::RackType& rack,tracktion::engine::Plugin::Ptr plugin)
{
    namespace te=tracktion::engine;
    if(!te::RackType::isPluginAllowed(plugin) || plugin->isSynth() || plugin->isMissing())return false;
    // Native rack storage API, without RackType::addPlugin's stopIfRecording:
    // editing an effect chain must not disarm the user's active take.
    auto wrapper=juce::ValueTree(te::IDs::PLUGININSTANCE);wrapper.setProperty(te::IDs::x,.5f,nullptr);wrapper.setProperty(te::IDs::y,.5f,nullptr);
    plugin->removeFromParent();wrapper.addChild(plugin->state,-1,rack.getUndoManager());rack.state.addChild(wrapper,-1,rack.getUndoManager());connectSerial(rack);return true;
}
inline void removeRackEffect(tracktion::engine::RackType& rack,tracktion::engine::Plugin& plugin)
{auto wrapper=plugin.state.getParent();if(wrapper.getParent()==rack.state){plugin.hideWindowForShutdown();rack.state.removeChild(wrapper,rack.getUndoManager());connectSerial(rack);}}
inline void moveRackEffect(tracktion::engine::RackType& rack,int index,int delta)
{const auto plugins=rack.getPlugins();if(!juce::isPositiveAndBelow(index,plugins.size()) || !juce::isPositiveAndBelow(index+delta,plugins.size()))return;auto a=plugins[index]->state.getParent(),b=plugins[index+delta]->state.getParent();rack.state.moveChild(rack.state.indexOf(a),rack.state.indexOf(b),rack.getUndoManager());connectSerial(rack);}
inline void cleanRackClipboard(juce::ValueTree state)
{
    namespace te=tracktion::engine;
    if(state.hasProperty(te::IDs::sidechainTrigger))state.setProperty(te::IDs::sidechainTrigger,false,nullptr);
    state.removeProperty(te::IDs::sidechainSourceID,nullptr);state.removeProperty(te::IDs::masterPluginID,nullptr);
    for(int i=state.getNumChildren();--i>=0;){auto child=state.getChild(i);if(child.hasType("ORBITAUTOMATIONBASES"))state.removeChild(i,nullptr);else cleanRackClipboard(child);}
}
inline juce::Array<tracktion::engine::Plugin*> flattenedPlugins(const tracktion::engine::PluginList& list)
{
    juce::Array<tracktion::engine::Plugin*> result;
    for(auto* plugin:list){result.add(plugin);if(auto* rack=dynamic_cast<tracktion::engine::RackInstance*>(plugin);isMultiFx(rack))for(auto* child:rack->type->getPlugins())result.add(child);}
    return result;
}
}
