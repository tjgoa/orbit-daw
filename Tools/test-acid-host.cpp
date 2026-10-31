#include "Import/OrionSfsConversion.h"
#include "Import/OrionSfsReader.h"
#include <fstream>
static void acidCheck(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
static juce::MouseEvent acidMouse(juce::Component* target,float x,float y,juce::Point<float> down={},bool drag=false)
{
    juce::Point<float> p{x,y};return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,target,target,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};
}
static void fixtureWave(const juce::File& path,const juce::ValueTree& metadata)
{
    path.getParentDirectory().createDirectory();std::ofstream out(path.getFullPathName().toStdString(),std::ios::binary);
    const int channels=metadata.getProperty("orbitExpectedChannels"),rate=metadata.getProperty("orbitExpectedRate"),bits=metadata.getProperty("orbitExpectedBits"),format=metadata.getProperty("orbitExpectedFormat");
    const auto frames=static_cast<juce::int64>(metadata.getProperty("orbitExpectedFrames"));
    const auto bytes=static_cast<std::uint32_t>(frames*channels*(bits/8));
    auto u16=[&](unsigned value){out.put(char(value&255));out.put(char((value>>8)&255));};
    auto u32=[&](std::uint32_t value){for(int i=0;i<4;++i)out.put(char((value>>(8*i))&255));};
    out.write("RIFF",4);u32(bytes+36);out.write("WAVEfmt ",8);u32(16);u16(format);u16(channels);u32(rate);
    u32(rate*channels*(bits/8));u16(channels*(bits/8));u16(bits);out.write("data",4);u32(bytes);
    std::array<char,4096> zero{};for(std::uint32_t left=bytes;left>0;){auto count=std::min(left,4096u);out.write(zero.data(),count);left-=count;}
    acidCheck(bool(out),"could not create media fixture");
}
int main(int argc,char** argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,900);
        const juce::File folder("OPTIONS_CHECK_FOLDER"),acidFile=folder.getChildFile("acid.orbit"),mediaFile=folder.getChildFile("missing-restored.orbit");
        if(argc>1)
        {
            const bool acid=juce::String(argv[1]).contains("acid");acidCheck(host.loadProjectFile(acid?acidFile:mediaFile),"fresh-process reopen failed");
            if(acid)
            {
                auto* plugin=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.selectedInstrumentPlugin());
                acidCheck(plugin!=nullptr && std::abs(plugin->parameters[1]->getCurrentValue()-1200)<0.1,"Acid Bass type/knobs did not survive reopen");
                acidCheck(host.project.getPatternSwitches(host.selectedProjectTrackId(),0).size()==10,"switches did not survive reopen");
            }
            else acidCheck(host.missingMediaCount()==0 && te::getAudioTracks(*host.edit).size()==3,"resolved audio did not survive fresh reopen");
            std::cout<<"PASS: fresh-process "<<(acid?"Acid Bass parameters/switches":"resolved placeholder media")<<" persistence.\n";return 0;
        }
        host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);
        acidCheck(dynamic_cast<orbit::synth::AcidBassEditor*>(host.activeInstrumentEditor.get())!=nullptr,"compact Acid Bass editor missing");
        auto* plugin=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.selectedInstrumentPlugin());acidCheck(plugin!=nullptr,"Acid Bass factory did not insert synth");
        const auto id=host.selectedProjectTrackId();host.createAcidDemo();
        acidCheck(host.project.getPatternNotes(id,0).size()==14 && host.project.getPatternSwitches(id,0).size()==10,"demo did not create notes/Accent/Slide");
        auto* window=host.pianoRollWindow.get();auto& rows=window->acidRows;auto* grid=window->grid;
        acidCheck(rows.isVisible() && rows.getHeight()==44 && grid->getSubdivisionsPerBar()==16,"Acid rows absent or clipped");
        const auto original=host.project.getPatternSwitches(id,0);const auto undo=host.undoHistory.size();
        auto click=[&](bool slide,int cell){auto event=acidMouse(&rows,64+(cell+0.5f)*grid->stepWidth(),slide?30:9);rows.mouseDown(event);rows.mouseUp(event);};
        click(false,0);acidCheck(host.project.getPatternSwitches(id,0).size()==9 && host.undoHistory.size()==undo+1,"Accent click/one Undo failed");
        host.undoProjectAction();acidCheck(host.project.getPatternSwitches(id,0).size()==10,"Accent Undo lost switches");
        grid->setGridOptions(32,4);window->resized();
        auto half=acidMouse(&rows,64+grid->stepWidth()/4.0f,8);rows.mouseDown(half);rows.mouseUp(half);
        bool retained=false;for(const auto& item:host.project.getPatternSwitches(id,0))if(!item.slide && item.startTick==32 && item.lengthTicks==32)retained=true;
        acidCheck(retained,"subdivision edit erased neighbouring half-step");host.undoProjectAction();
        grid->setGridOptions(16,4);window->resized();
        auto start=acidMouse(&rows,64+2.5f*grid->stepWidth(),30);rows.mouseDown(start);
        auto travel=acidMouse(&rows,64+5.5f*grid->stepWidth(),30,start.position,true);rows.mouseDrag(travel);rows.mouseUp(travel);
        acidCheck(host.undoHistory.size()==undo+1,"switch brush made several Undo entries");host.undoProjectAction();
        host.copyOrCutPatternNotes(false);host.selectPatternSlot(1);host.pastePatternNotes();
        acidCheck(host.project.getPatternSwitches(id,1).size()==10,"whole-pattern copy lost Accent/Slide");host.selectPatternSlot(0);
        host.project.setPlaylistClip(id,host.project.getPatternId(id,0),0,0,true);host.refreshEnginePlaylist();int markers=0;
        acidCheck(host.patternClip!=nullptr,"test arrangement clip not generated");
        for(auto* event:host.patternClip->getSequence().getControllerEvents())if(event->getType()==119)++markers;
        acidCheck(markers>=14,"native playback omitted articulation markers");
        // Production audio processor: silence before a delayed note, actual
        // stereo output afterwards, MIDI consumption and sample-accurate panic.
        plugin=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.selectedInstrumentPlugin());
        te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;plugin->initialise(info);
        juce::AudioBuffer<float> audio(2,512);te::MidiMessageArray midi;
        midi.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,3),48.0/48000,{});
        midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),48.0/48000,{});
        te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),0,512,&midi,0,
            {tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512.0/48000)},true,false,false,false);
        plugin->applyToBuffer(context);float energy=0;
        for(int sample=0;sample<512;++sample)
        {
            acidCheck(std::isfinite(audio.getSample(0,sample)) && audio.getSample(0,sample)==audio.getSample(1,sample),"invalid stereo synth output");
            if(sample<48)acidCheck(audio.getSample(0,sample)==0,"note began before its timestamp");energy+=std::abs(audio.getSample(0,sample));
        }
        acidCheck(energy>0.01f && midi.isEmpty(),"production synth made no audio or leaked MIDI");
        midi.isAllNotesOff=true;plugin->applyToBuffer(context);acidCheck(audio.getMagnitude(0,512)==0,"synth panic left audio");
        auto loopTransition=[&](bool keepTie)
        {
            plugin->initialise(info);te::MidiMessageArray events;juce::AudioBuffer<float> block(2,512);
            host.edit->getTransport().setLoopRange({tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(0.5)});
            host.edit->getTransport().looping=keepTie;
            events.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,2),0,{});
            events.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),0,{});
            events.addMidiMessage(juce::MidiMessage::noteOff(1,36),256.0/48000,{});
            te::PluginRenderContext tail(&block,juce::AudioChannelSet::stereo(),0,512,&events,0,
                {tracktion::TimePosition::fromSeconds(0.5-512.0/48000),tracktion::TimePosition::fromSeconds(0.5)},true,false,false,false);
            plugin->applyToBuffer(tail);
            events.isAllNotesOff=true;events.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,0),0,{});
            events.addMidiMessage(juce::MidiMessage::noteOn(1,48,juce::uint8(100)),0,{});
            plugin->applyToBuffer(te::PluginRenderContext(&block,juce::AudioChannelSet::stereo(),0,512,&events,0,
                {tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512.0/48000)},true,false,false,false));
            std::vector<float> result(512);std::copy_n(block.getReadPointer(0),512,result.data());return result;
        };
        const auto tied=loopTransition(true),retriggered=loopTransition(false);double loopDifference=0;
        for(int n=0;n<512;++n)loopDifference+=std::abs(tied[n]-retriggered[n]);
        acidCheck(loopDifference>0.01,"loop reset discarded an outgoing Slide");host.edit->getTransport().looping=false;
        plugin->deinitialise();
        plugin->parameters[1]->setParameter(1200,juce::sendNotificationSync);
        for(int skin:{5,6})
        {
            host.setSkin(skin);auto image=host.createComponentSnapshot(host.getLocalBounds());juce::FileOutputStream out(folder.getChildFile("acid-roll-"+juce::String(skin)+".png"));juce::PNGImageFormat png;png.writeImageToStream(image,out);
        }
        host.closePianoRollView();
        acidCheck(host.generatorPanelBounds.getWidth()==880 && host.activeInstrumentEditor->getHeight()==190,"Undo lost the compact synth restore bounds");
        {auto image=host.createComponentSnapshot(host.getLocalBounds());juce::FileOutputStream out(folder.getChildFile("acid-synth.png"));juce::PNGImageFormat png;png.writeImageToStream(image,out);}
        {
            juce::ValueTree state("PLUGIN");state.setProperty("name","Orbit unavailable test effect",nullptr);
            state.setProperty("filename",folder.getChildFile("Unavailable.vst3").getFullPathName(),nullptr);
            state.setProperty("type",te::ExternalPlugin::xmlTypeName,nullptr);
            auto missingEffect=host.edit->getPluginCache().createNewPlugin(state);
            acidCheck(missingEffect!=nullptr && missingEffect->isMissing(),"missing-effect fixture did not remain unavailable");
            host.instrumentTrack->pluginList.insertPlugin(*missingEffect,-1,nullptr);
            const auto before=host.undoHistory.size();host.bypassMissingEffects();
            acidCheck(!missingEffect->isEnabled() && host.undoHistory.size()==before+1,"missing effect was not bypassed with Undo");
            missingEffect->deleteFromParent();
        }
        host.saveToFile(acidFile);acidCheck(host.loadProjectFile(acidFile),"native Acid Bass reopen failed");
        // Missing SFS media must no longer prevent the actual host transaction.
        host.createProjectDiscardingCurrent();orbit::import::orion::ImportDocument source;std::string error;
        acidCheck(orbit::import::orion::OrionSfsReader::readDocument(std::filesystem::path(SFS_CHECK_SOURCE_LITERAL),source,error),error.c_str());
        orbit::import::orion::ConversionOptions options;options.sourceTicksPerQuarterNote=96;options.tempoBpm=120;options.meterNumerator=4;options.meterDenominator=4;
        options.acceptPatternOnlyRecovery=options.acceptTentativeNotes=options.recoverTentativePlaylist=options.recoverTentativeDryAudio=options.allowMissingMedia=options.omitUnsupportedEvents=true;
        for(const auto& track:source.tracks)
            if(track.kind==orbit::import::orion::TrackKind::audio)options.selectedAudioTracks.push_back(track.sourceTrackIndex);
            else for(const auto& pattern:track.patterns)
                if(!pattern.events.empty() || std::any_of(track.playlist.begin(),track.playlist.end(),[&](const auto& place){return place.patternIndex==pattern.sourcePatternIndex;}))
                    options.selectedPatterns.emplace_back(track.sourceTrackIndex,pattern.sourcePatternIndex);
        orbit::import::ProjectImportPlan plan;acidCheck(orbit::import::orion::prepareConversion(source,options,plan,error),error.c_str());
        const auto beforeMerge=host.undoHistory.size();acidCheck(host.commitRecoveredPatterns(plan,error),error.c_str());
        acidCheck(host.project.getTrackCount()==3 && host.missingMediaCount()==3 && host.undoHistory.size()==beforeMerge+1,"missing media blocked/partially merged the session");
        auto missing=orbit::media::collect(*host.edit);acidCheck(missing.size()==3,"placeholder inventory mismatch");
        std::vector<double> starts;for(const auto& item:missing){starts.push_back(item.clip->getPosition().getStart().inSeconds());acidCheck(item.clip->state.getProperty("orbitMissingMedia",false),"clip is not marked missing");}
        host.saveToFile(mediaFile);acidCheck(host.loadProjectFile(mediaFile) && host.missingMediaCount()==3,"unresolved media lost on native reopen");
        missing=orbit::media::collect(*host.edit);
        const auto search=folder.getChildFile("sample-search");
        for(std::size_t i=0;i<missing.size();++i)fixtureWave(search.getChildFile("nested/"+orbit::media::leafName(missing[i].reference)),missing[i].clip->state);
        const auto duplicate=search.getChildFile("duplicate/"+orbit::media::leafName(missing[0].reference));fixtureWave(duplicate,missing[0].clip->state);
        std::set<juce::String> wanted;for(const auto& item:missing)wanted.insert(orbit::media::leafName(item.reference).toLowerCase());bool incomplete=false;
        auto files=orbit::media::scan(search,wanted,incomplete);acidCheck(!incomplete,"fixture folder scan incomplete");
        const auto beforeRelink=host.undoHistory.size();host.relinkMissingMediaFromFiles(files);
        acidCheck(host.missingMediaCount()==1 && host.undoHistory.size()==beforeRelink+1,"folder matching guessed an ambiguous file or missed unique sources");
        host.undoProjectAction();acidCheck(host.missingMediaCount()==3,"relink Undo did not restore placeholders");
        duplicate.deleteFile();files=orbit::media::scan(search,wanted,incomplete);host.relinkMissingMediaFromFiles(files);acidCheck(host.missingMediaCount()==0,"unique folder relink failed");
        auto* audioTrack=te::getAudioTracks(*host.edit)[1];int checked=0;
        for(int index=0;index<audioTrack->getNumTrackItems();++index)if(auto* clip=dynamic_cast<te::WaveAudioClip*>(audioTrack->getTrackItem(index)))
        {acidCheck(clip->getPosition().getStart().inSeconds()==starts[checked++],"relink moved the arrangement");acidCheck(!clip->getName().startsWith("Missing:"),"relink left stale missing label");}
        acidCheck(checked==3,"audio placeholder count changed");host.saveToFile(mediaFile);
        std::cout<<"PASS: native Acid Bass insertion/editor/demo, grid-relative Accent/Slide strokes and Undo/copy, sample-timed stereo processing/panic, native save/reopen, actual three-track SFS host merge with missing media, recursive folder matching, ambiguity refusal, one-Undo relink and unchanged clip timing. Hardware disabled.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
