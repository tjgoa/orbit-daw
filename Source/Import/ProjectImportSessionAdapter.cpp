#include "ProjectImportSessionAdapter.h"
#include "WaveMediaInfo.h"

namespace orbit::import
{
bool prepareProjectImportSession(tracktion::engine::Engine& engine, const ProjectDocument& base,
                                  tracktion::engine::Edit& sourceEdit, const ProjectImportPlan& plan,
                                  PreparedProjectImportSession& result, std::string& error)
{
    namespace te = tracktion::engine;
    PreparedProjectImportSession staged;
    if (!prepareProjectImport(base, plan, staged.document, error)) return false;
    sourceEdit.flushState();
    staged.edit = te::loadEditFromState(engine, sourceEdit.state.createCopy());
    if (!staged.edit)
    {
        error = "Could not stage a copy of the current audio session.";
        return false;
    }
    juce::Array<te::AudioTrack*> existing;
    for (auto* track : te::getAudioTracks(*staged.edit))
        if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1)) < 0
            && static_cast<int>(track->state.getProperty(juce::Identifier("orbitMixerBus"), -1)) < 0)
            existing.add(track);
    if ((base.getTrackCount() > 0 && existing.size() != base.getTrackCount())
        || (base.getTrackCount() == 0 && existing.size() > 1))
    {
        error = "The project and audio session have different track inventories; recovery was not applied.";
        return false;
    }
    const bool hasPlaylist = std::any_of(plan.tracks.begin(), plan.tracks.end(), [](const PlannedTrack& track)
        { return !track.playlist.empty() || !track.audioClips.empty(); });
    if (hasPlaylist && (staged.edit->tempoSequence.getNumTempos() != 1 || staged.edit->tempoSequence.getNumTimeSigs() != 1))
    {
        error = "Arrangement merge into a changing tempo/meter map is not supported; exact ranges were not applied.";
        return false;
    }
    if (auto* tempo = staged.edit->tempoSequence.getTempo(0)) tempo->setBpm(plan.tempoBpm);
    if (auto* meter = staged.edit->tempoSequence.getTimeSig(0))
        meter->setStringTimeSig(juce::String(plan.meterNumerator) + "/" + juce::String(plan.meterDenominator));
    for (std::size_t index = 0; index < plan.tracks.size(); ++index)
    {
        auto* track = base.getTrackCount() == 0 && index == 0 && existing.size() == 1 ? existing[0] : nullptr;
        if (!track) track = staged.edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*staged.edit), nullptr).get();
        if (!track)
        {
            error = "Could not stage a recovered audio-session track.";
            return false;
        }
        const auto trackIndex = base.getTrackCount() + static_cast<int>(index);
        const auto trackId = staged.document.getTrackId(trackIndex);
        track->setName(staged.document.getTrackName(trackIndex));
        track->getOutput().setOutputToDefaultDevice(false);
        const auto secondsPerBar = 240.0 / plan.tempoBpm;
        for (const auto& placement : staged.document.getPlaylistClips(trackId))
        {
            const auto pattern = staged.document.getPatternIndexForId(trackId, placement.patternId);
            const auto start = placement.startBar * secondsPerBar;
            const auto end = start + placement.lengthBars * secondsPerBar;
            auto clip = track->insertMIDIClip(staged.document.getPatternName(trackId, pattern),
                {tracktion::TimePosition::fromSeconds(start), tracktion::TimePosition::fromSeconds(end)}, nullptr);
            if (!clip)
            {
                error = "Could not stage an exact playlist clip; the live project was not changed.";
                return false;
            }
            clip->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0.0), tracktion::BeatPosition::fromBeats(
                staged.document.getPatternLengthSteps(trackId, pattern) * 0.25)});
            for (const auto& note : staged.document.getPatternNotes(trackId, pattern))
                clip->getSequence().addNote(note.pitch, tracktion::BeatPosition::fromBeats(note.startTick / 256.0),
                    tracktion::BeatDuration::fromBeats(note.lengthTicks / 256.0), note.velocity, 0, nullptr);
        }
        for (const auto& audio:plan.tracks[index].audioClips)
        {
            WaveMediaInfo info;
            if (!audio.missingMedia && (!inspectWaveMedia(std::filesystem::path(audio.mediaPath),info,error)
                || info.sampleRate!=audio.sampleRate || info.channels!=audio.channels || info.frames!=audio.sourceFrames
                || info.formatTag!=audio.waveFormatTag || info.bitsPerSample!=audio.bitsPerSample))
            { error="Relinked audio changed or could not be validated during session staging."; return false; }
            const auto file=audio.missingMedia?juce::File{}:juce::File(juce::String::fromUTF8(audio.mediaPath.c_str()));
            te::AudioFile media(engine,file);
            if (!audio.missingMedia && !media.isValid()) { error="The engine cannot read a relinked WAV file."; return false; }
            auto clip=track->insertWaveClip(juce::String::fromUTF8(audio.name.c_str()),file,
                {{tracktion::TimePosition::fromSeconds(audio.startSeconds),tracktion::TimePosition::fromSeconds(audio.startSeconds+audio.lengthSeconds)},
                    tracktion::TimeDuration::fromSeconds(audio.sourceOffsetSeconds)},false);
            if (!clip) { error="Could not stage a recovered audio clip; the live project was not changed."; return false; }
            clip->state.setProperty("orbitOriginalReference",juce::String::fromUTF8(audio.originalReference.c_str()),nullptr);
            clip->state.setProperty("orbitExpectedRate",static_cast<int>(audio.sampleRate),nullptr);
            clip->state.setProperty("orbitExpectedChannels",static_cast<int>(audio.channels),nullptr);
            clip->state.setProperty("orbitExpectedFrames",static_cast<juce::int64>(audio.sourceFrames),nullptr);
            clip->state.setProperty("orbitExpectedFormat",static_cast<int>(audio.waveFormatTag),nullptr);
            clip->state.setProperty("orbitExpectedBits",static_cast<int>(audio.bitsPerSample),nullptr);
            clip->state.setProperty("orbitMissingMedia",audio.missingMedia,nullptr);
            if(audio.missingMedia)clip->setName("Missing: "+juce::String::fromUTF8(audio.name.c_str()));
            clip->setAutoTempo(false); clip->setAutoPitch(false);
            clip->setSpeedRatio(1.0); clip->setPitchChange(0.0f); clip->setIsReversed(false);
            clip->setGainDB(0.0f); clip->setFadeIn({}); clip->setFadeOut({});
        }
        staged.selectedTrackIndex = trackIndex;
    }
    staged.edit->setLatencyCompensationEnabled(staged.document.isPluginDelayCompensationEnabled());
    staged.edit->flushState();
    staged.document.setTracktionEditState(staged.edit->state);
    result = std::move(staged);
    return true;
}
}
