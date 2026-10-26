#include "ProjectDocument.h"

#include <cmath>

namespace
{
const juce::Identifier projectType { "OrbitProject" };
const juce::Identifier tempoMapType { "TempoMap" };
const juce::Identifier tempoPointType { "TempoPoint" };
const juce::Identifier meterMapType { "TimeSignatureMap" };
const juce::Identifier meterPointType { "TimeSignaturePoint" };
const juce::Identifier tracksType { "Tracks" };
const juce::Identifier trackType { "Track" };
const juce::Identifier patternsType { "Patterns" };
const juce::Identifier patternSlotType { "PatternSlot" };
const juce::Identifier patternNotesType { "Notes" };
const juce::Identifier patternNoteType { "Note" };
const juce::Identifier clipsType { "Clips" };
const juce::Identifier playlistClipType { "PlaylistClip" };
const juce::Identifier pluginsType { "Plugins" };
const juce::Identifier automationType { "AutomationLanes" };
const juce::Identifier mixerType { "MixerChannels" };
const juce::Identifier mixerChannelType { "MixerChannel" };
const juce::Identifier mediaType { "MediaReferences" };
const juce::Identifier tracktionStateType { "TracktionEditState" };

juce::String newId()
{
    return juce::Uuid().toString();
}

juce::ValueTree makeTempoPoint(double bpm)
{
    juce::ValueTree point(tempoPointType);
    point.setProperty("id", newId(), nullptr);
    point.setProperty("tick", juce::int64(0), nullptr);
    point.setProperty("bpm", bpm, nullptr);
    return point;
}

juce::ValueTree makeMeterPoint(int numerator, int denominator)
{
    juce::ValueTree point(meterPointType);
    point.setProperty("id", newId(), nullptr);
    point.setProperty("tick", juce::int64(0), nullptr);
    point.setProperty("numerator", numerator, nullptr);
    point.setProperty("denominator", denominator, nullptr);
    return point;
}

void ensureChild(juce::ValueTree& parent, const juce::Identifier& type)
{
    if (!parent.getChildWithName(type).isValid())
        parent.addChild(juce::ValueTree(type), -1, nullptr);
}

void ensurePatternBank(juce::ValueTree track)
{
    if (track.getProperty("kind").toString() != "instrument")
        return;

    auto patterns = track.getChildWithName(patternsType);
    for (int index = 0; index < ProjectDocument::patternSlotCount; ++index)
    {
        juce::ValueTree slot;
        for (int child = 0; child < patterns.getNumChildren(); ++child)
        {
            auto candidate = patterns.getChild(child);
            if (candidate.getType() == patternSlotType
                && static_cast<int>(candidate.getProperty("index", -1)) == index)
            {
                slot = candidate;
                break;
            }
        }

        if (!slot.isValid())
        {
            slot = juce::ValueTree(patternSlotType);
            slot.setProperty("id", newId(), nullptr);
            slot.setProperty("index", index, nullptr);
            slot.setProperty("bank", index / ProjectDocument::patternsPerBank, nullptr);
            slot.setProperty("slot", index % ProjectDocument::patternsPerBank, nullptr);
            slot.setProperty("name", juce::String::charToString(static_cast<juce::juce_wchar>('A' + index / 8))
                                           + juce::String(index % 8 + 1), nullptr);
            slot.setProperty("lengthSteps", 16, nullptr);
            slot.setProperty("created", false, nullptr);
            patterns.addChild(slot, -1, nullptr);
        }
        ensureChild(slot, patternNotesType);
    }
}

juce::ValueTree findPatternSlot(juce::ValueTree state, const juce::String& trackId, int slotIndex)
{
    if (trackId.isEmpty() || !juce::isPositiveAndBelow(slotIndex, ProjectDocument::patternSlotCount))
        return {};

    const auto tracks = state.getChildWithName(tracksType);
    for (int i = 0; i < tracks.getNumChildren(); ++i)
    {
        auto track = tracks.getChild(i);
        if (track.getProperty("id").toString() == trackId)
        {
            auto patterns = track.getChildWithName(patternsType);
            for (int j = 0; j < patterns.getNumChildren(); ++j)
            {
                auto slot = patterns.getChild(j);
                if (slot.getType() == patternSlotType
                    && static_cast<int>(slot.getProperty("index", -1)) == slotIndex)
                    return slot;
            }
        }
    }
    return {};
}

juce::ValueTree findProjectTrack(juce::ValueTree state, const juce::String& trackId)
{
    const auto tracks = state.getChildWithName(tracksType);
    for (int i = 0; i < tracks.getNumChildren(); ++i)
    {
        auto track = tracks.getChild(i);
        if (track.getProperty("id").toString() == trackId)
            return track;
    }
    return {};
}

juce::ValueTree findMixerChannel(juce::ValueTree mixer, const juce::String& channelId)
{
    if (channelId.isEmpty()) return {};
    for (int i = 0; i < mixer.getNumChildren(); ++i)
    {
        auto channel = mixer.getChild(i);
        if (channel.getProperty("id").toString() == channelId)
            return channel;
    }
    return {};
}

void ensureTrackShape(juce::ValueTree track, juce::ValueTree mixer)
{
    if (track.getProperty("id").toString().isEmpty())
        track.setProperty("id", newId(), nullptr);
    if (track.getProperty("name").toString().isEmpty())
        track.setProperty("name", "Instrument", nullptr);
    if (track.getProperty("kind").toString().isEmpty())
        track.setProperty("kind", "instrument", nullptr);

    ensureChild(track, patternsType);
    ensureChild(track, clipsType);
    ensureChild(track, pluginsType);
    ensureChild(track, automationType);
    ensurePatternBank(track);

    auto channelId = track.getProperty("mixerChannelId").toString();
    if (channelId.isEmpty())
    {
        channelId = newId();
        track.setProperty("mixerChannelId", channelId, nullptr);
    }

    bool foundChannel = false;
    for (int i = 0; i < mixer.getNumChildren(); ++i)
        foundChannel |= mixer.getChild(i).getProperty("id").toString() == channelId;

    if (!foundChannel)
    {
        juce::ValueTree channel(mixerChannelType);
        channel.setProperty("id", channelId, nullptr);
        channel.setProperty("kind", "track", nullptr);
        channel.setProperty("trackId", track.getProperty("id"), nullptr);
        channel.setProperty("name", track.getProperty("name"), nullptr);
        channel.setProperty("gainDb", 0.0, nullptr);
        channel.setProperty("pan", 0.0, nullptr);
        channel.setProperty("muted", false, nullptr);
        channel.setProperty("solo", false, nullptr);
        for (int bus = 0; bus < 4; ++bus)
            channel.setProperty("fx" + juce::String(bus + 1) + "SendDb", -60.0, nullptr);
        channel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
        mixer.addChild(channel, -1, nullptr);
    }
}

bool ensureSchemaShape(juce::ValueTree& state, juce::String& error)
{
    if (state.getProperty("projectId").toString().isEmpty())
        state.setProperty("projectId", newId(), nullptr);
    if (state.getProperty("name").toString().isEmpty())
        state.setProperty("name", "Untitled Project", nullptr);
    const auto storedPpq = static_cast<int>(state.getProperty("ticksPerQuarterNote", 0));
    if (storedPpq == 0)
        state.setProperty("ticksPerQuarterNote", ProjectDocument::ticksPerQuarterNote, nullptr);
    else if (storedPpq != ProjectDocument::ticksPerQuarterNote)
    {
        error = "This project uses a musical time resolution Orbit cannot read.";
        return false;
    }

    ensureChild(state, tempoMapType);
    ensureChild(state, meterMapType);
    ensureChild(state, tracksType);
    ensureChild(state, mixerType);
    ensureChild(state, mediaType);

    auto tempoMap = state.getChildWithName(tempoMapType);
    if (tempoMap.getNumChildren() == 0)
        tempoMap.addChild(makeTempoPoint(static_cast<double>(state.getProperty("tempoBpm", 120.0))),
                          -1, nullptr);

    auto meterMap = state.getChildWithName(meterMapType);
    if (meterMap.getNumChildren() == 0)
        meterMap.addChild(makeMeterPoint(static_cast<int>(state.getProperty("timeSignatureNumerator", 4)),
                                         static_cast<int>(state.getProperty("timeSignatureDenominator", 4))),
                          -1, nullptr);

    auto mixer = state.getChildWithName(mixerType);
    auto masterId = mixer.getProperty("masterChannelId").toString();
    if (masterId.isEmpty())
    {
        masterId = newId();
        mixer.setProperty("masterChannelId", masterId, nullptr);
    }

    bool hasMaster = false;
    for (int i = 0; i < mixer.getNumChildren(); ++i)
        hasMaster |= mixer.getChild(i).getProperty("id").toString() == masterId;

    if (!hasMaster)
    {
        juce::ValueTree master(mixerChannelType);
        master.setProperty("id", masterId, nullptr);
        master.setProperty("kind", "master", nullptr);
        master.setProperty("name", "Master", nullptr);
        master.setProperty("gainDb", 0.0, nullptr);
        master.setProperty("pan", 0.0, nullptr);
        master.setProperty("muted", false, nullptr);
        master.setProperty("outputId", juce::String{}, nullptr);
        mixer.addChild(master, -1, nullptr);
    }

    auto tracks = state.getChildWithName(tracksType);
    for (int i = 0; i < tracks.getNumChildren(); ++i)
        ensureTrackShape(tracks.getChild(i), mixer);

    for (int i = 0; i < mixer.getNumChildren(); ++i)
    {
        auto channel = mixer.getChild(i);
        if (!channel.hasProperty("muted")) channel.setProperty("muted", false, nullptr);
        if (channel.getProperty("kind").toString() == "track")
        {
            if (!channel.hasProperty("solo")) channel.setProperty("solo", false, nullptr);
            for (int bus = 0; bus < 4; ++bus)
            {
                const auto property = juce::Identifier("fx" + juce::String(bus + 1) + "SendDb");
                if (!channel.hasProperty(property)) channel.setProperty(property, -60.0, nullptr);
            }
        }
        else if (channel.getProperty("kind").toString() == "fx_return")
        {
            if (!channel.hasProperty("solo")) channel.setProperty("solo", false, nullptr);
            if (!channel.hasProperty("gainDb")) channel.setProperty("gainDb", 0.0, nullptr);
            if (!channel.hasProperty("pan")) channel.setProperty("pan", 0.0, nullptr);
            if (!channel.hasProperty("outputId"))
                channel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
        }
    }

    state.setProperty("schemaVersion", ProjectDocument::currentSchemaVersion, nullptr);
    error.clear();
    return true;
}

juce::String trackKindToString(ProjectDocument::TrackKind kind)
{
    switch (kind)
    {
        case ProjectDocument::TrackKind::instrument: return "instrument";
        case ProjectDocument::TrackKind::midi:        return "midi";
        case ProjectDocument::TrackKind::audio:       return "audio";
    }
    return "instrument";
}
}

ProjectDocument::ProjectDocument()
    : ProjectDocument(createNew().state)
{
}

ProjectDocument::ProjectDocument(juce::ValueTree projectState, juce::File file)
    : state(std::move(projectState)), projectFile(std::move(file))
{
}

ProjectDocument ProjectDocument::createNew()
{
    juce::ValueTree root(projectType);
    root.setProperty("schemaVersion", currentSchemaVersion, nullptr);
    root.setProperty("projectId", newId(), nullptr);
    root.setProperty("name", "Untitled Project", nullptr);
    root.setProperty("ticksPerQuarterNote", ticksPerQuarterNote, nullptr);
    root.addChild(juce::ValueTree(tempoMapType), -1, nullptr);
    root.addChild(juce::ValueTree(meterMapType), -1, nullptr);
    root.addChild(juce::ValueTree(tracksType), -1, nullptr);
    root.addChild(juce::ValueTree(mixerType), -1, nullptr);
    root.addChild(juce::ValueTree(mediaType), -1, nullptr);

    juce::String ignoredError;
    ensureSchemaShape(root, ignoredError);
    return ProjectDocument(std::move(root));
}

std::optional<ProjectDocument> ProjectDocument::loadFromFile(const juce::File& file,
                                                               juce::String& error)
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
    {
        error = "Orbit could not read this project file.";
        return std::nullopt;
    }

    auto loadedState = juce::ValueTree::fromXml(*xml);
    if (!loadedState.hasType(projectType))
    {
        error = "This file is not an Orbit project.";
        return std::nullopt;
    }

    const auto schemaVersion = static_cast<int>(loadedState.getProperty("schemaVersion", 0));
    if (schemaVersion < 0 || schemaVersion > currentSchemaVersion)
    {
        error = "This Orbit project uses an unsupported version.";
        return std::nullopt;
    }

    if (!ensureSchemaShape(loadedState, error))
        return std::nullopt;

    return ProjectDocument(std::move(loadedState), file);
}

bool ProjectDocument::saveToFile(const juce::File& file, juce::String& error)
{
    if (file == juce::File{})
    {
        error = "Choose a location for the project file.";
        return false;
    }

    auto xml = state.createXml();
    if (xml == nullptr)
    {
        error = "Orbit could not serialize this project.";
        return false;
    }

    if (!file.getParentDirectory().createDirectory())
    {
        error = "Orbit could not create the project folder.";
        return false;
    }

    auto temporaryFile = file.getSiblingFile(file.getFileName() + ".tmp");
    if (!temporaryFile.replaceWithText(xml->toString()))
    {
        error = "Orbit could not write the project file.";
        return false;
    }

    const auto replaced = file.existsAsFile() ? temporaryFile.replaceFileIn(file)
                                              : temporaryFile.moveFileTo(file);
    if (!replaced)
    {
        temporaryFile.deleteFile();
        error = "Orbit could not finish saving the project file.";
        return false;
    }

    projectFile = file;
    error.clear();
    return true;
}

void ProjectDocument::setName(const juce::String& name)
{
    const auto trimmedName = name.trim();
    if (trimmedName.isNotEmpty())
        state.setProperty("name", trimmedName, nullptr);
}

juce::String ProjectDocument::addTrack(TrackKind kind, const juce::String& requestedName)
{
    auto tracks = state.getChildWithName(tracksType);
    auto mixer = state.getChildWithName(mixerType);
    const auto number = tracks.getNumChildren() + 1;
    const auto trackId = newId();

    juce::ValueTree track(trackType);
    track.setProperty("id", trackId, nullptr);
    track.setProperty("kind", trackKindToString(kind), nullptr);
    track.setProperty("name", requestedName.isNotEmpty()
                                   ? requestedName
                                   : trackKindToString(kind).substring(0, 1).toUpperCase()
                                         + trackKindToString(kind).substring(1) + " "
                                         + juce::String(number), nullptr);
    track.setProperty("order", number - 1, nullptr);
    ensureChild(track, patternsType);
    ensureChild(track, clipsType);
    ensureChild(track, pluginsType);
    ensureChild(track, automationType);
    ensurePatternBank(track);

    const auto channelId = newId();
    track.setProperty("mixerChannelId", channelId, nullptr);
    juce::ValueTree channel(mixerChannelType);
    channel.setProperty("id", channelId, nullptr);
    channel.setProperty("kind", "track", nullptr);
    channel.setProperty("trackId", trackId, nullptr);
    channel.setProperty("name", track.getProperty("name"), nullptr);
    channel.setProperty("gainDb", 0.0, nullptr);
    channel.setProperty("pan", 0.0, nullptr);
    channel.setProperty("muted", false, nullptr);
    channel.setProperty("solo", false, nullptr);
    for (int bus = 0; bus < 4; ++bus)
        channel.setProperty("fx" + juce::String(bus + 1) + "SendDb", -60.0, nullptr);
    channel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
    mixer.addChild(channel, -1, nullptr);
    tracks.addChild(track, -1, nullptr);
    return trackId;
}

void ProjectDocument::addInstrumentTrack()
{
    addTrack(TrackKind::instrument);
}

void ProjectDocument::setTracktionEditState(const juce::ValueTree& editState)
{
    auto existing = state.getChildWithName(tracktionStateType);
    if (existing.isValid())
        state.removeChild(existing, nullptr);
    if (editState.isValid())
    {
        juce::ValueTree wrapper(tracktionStateType);
        wrapper.addChild(editState.createCopy(), -1, nullptr);
        state.addChild(wrapper, -1, nullptr);
    }
}

juce::ValueTree ProjectDocument::getTracktionEditState() const
{
    const auto wrapper = state.getChildWithName(tracktionStateType);
    return wrapper.getNumChildren() > 0 ? wrapper.getChild(0) : juce::ValueTree{};
}

juce::String ProjectDocument::getName() const
{
    return state.getProperty("name", "Untitled Project").toString();
}

juce::String ProjectDocument::getProjectId() const
{
    return state.getProperty("projectId").toString();
}

int ProjectDocument::getTrackCount() const
{
    return state.getChildWithName(tracksType).getNumChildren();
}

juce::String ProjectDocument::getTrackId(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren()))
        return {};

    return tracks.getChild(index).getProperty("id").toString();
}

juce::String ProjectDocument::getTrackName(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren()))
        return {};

    return tracks.getChild(index).getProperty("name").toString();
}

void ProjectDocument::setTrackName(int index, const juce::String& requestedName)
{
    const auto name = requestedName.trim();
    auto tracks = state.getChildWithName(tracksType);
    if (name.isEmpty() || !juce::isPositiveAndBelow(index, tracks.getNumChildren()))
        return;

    auto track = tracks.getChild(index);
    track.setProperty("name", name, nullptr);

    const auto mixer = state.getChildWithName(mixerType);
    const auto channelId = track.getProperty("mixerChannelId").toString();
    auto channel = findMixerChannel(mixer, channelId);
    if (channel.isValid())
        channel.setProperty("name", name, nullptr);
}

void ProjectDocument::setTrackMixerState(int index, double gainDb, double pan, bool muted, bool solo)
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return;
    const auto track = tracks.getChild(index);
    const auto mixer = state.getChildWithName(mixerType);
    auto channel = findMixerChannel(mixer, track.getProperty("mixerChannelId").toString());
    if (channel.isValid())
    {
        channel.setProperty("gainDb", juce::jlimit(-60.0, 6.0, gainDb), nullptr);
        channel.setProperty("pan", juce::jlimit(-1.0, 1.0, pan), nullptr);
        channel.setProperty("muted", muted, nullptr);
        channel.setProperty("solo", solo, nullptr);
    }
}

void ProjectDocument::setTrackSendGain(int index, int bus, double gainDb)
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())
        || !juce::isPositiveAndBelow(bus, 4)) return;
    const auto track = tracks.getChild(index);
    const auto mixer = state.getChildWithName(mixerType);
    auto channel = findMixerChannel(mixer, track.getProperty("mixerChannelId").toString());
    if (channel.isValid())
        channel.setProperty("fx" + juce::String(bus + 1) + "SendDb",
                             juce::jlimit(-60.0, 6.0, gainDb), nullptr);
}

bool ProjectDocument::setFxReturnMixerState(int bus, const juce::String& channelId,
                                             const juce::String& name, double gainDb, double pan,
                                             bool muted, bool solo)
{
    if (!juce::isPositiveAndBelow(bus, 4) || channelId.isEmpty()) return false;

    auto mixer = state.getChildWithName(mixerType);
    juce::ValueTree channel;
    for (int i = 0; i < mixer.getNumChildren(); ++i)
    {
        auto candidate = mixer.getChild(i);
        if (candidate.getProperty("kind").toString() == "fx_return"
            && static_cast<int>(candidate.getProperty("bus", -1)) == bus)
        {
            channel = candidate;
            break;
        }
    }

    const auto boundedGain = juce::jlimit(-60.0, 6.0, gainDb);
    const auto boundedPan = juce::jlimit(-1.0, 1.0, pan);
    const bool changed = !channel.isValid()
        || channel.getProperty("id").toString() != channelId
        || channel.getProperty("name").toString() != name
        || static_cast<int>(channel.getProperty("bus", -1)) != bus
        || std::abs(static_cast<double>(channel.getProperty("gainDb", 0.0)) - boundedGain) > 0.0001
        || std::abs(static_cast<double>(channel.getProperty("pan", 0.0)) - boundedPan) > 0.0001
        || static_cast<bool>(channel.getProperty("muted", false)) != muted
        || static_cast<bool>(channel.getProperty("solo", false)) != solo
        || channel.getProperty("outputId").toString() != mixer.getProperty("masterChannelId").toString();

    if (!channel.isValid())
    {
        channel = juce::ValueTree(mixerChannelType);
        mixer.addChild(channel, -1, nullptr);
    }

    channel.setProperty("id", channelId, nullptr);
    channel.setProperty("kind", "fx_return", nullptr);
    channel.setProperty("bus", bus, nullptr);
    channel.setProperty("name", name, nullptr);
    channel.setProperty("gainDb", boundedGain, nullptr);
    channel.setProperty("pan", boundedPan, nullptr);
    channel.setProperty("muted", muted, nullptr);
    channel.setProperty("solo", solo, nullptr);
    channel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
    return changed;
}

void ProjectDocument::setMasterMixerState(double gainDb, double pan, bool muted)
{
    auto mixer = state.getChildWithName(mixerType);
    const auto masterId = mixer.getProperty("masterChannelId").toString();
    auto channel = findMixerChannel(mixer, masterId);
    if (channel.isValid())
    {
        channel.setProperty("gainDb", juce::jlimit(-60.0, 6.0, gainDb), nullptr);
        channel.setProperty("pan", juce::jlimit(-1.0, 1.0, pan), nullptr);
        channel.setProperty("muted", muted, nullptr);
    }
}

juce::String ProjectDocument::getPatternId(const juce::String& trackId, int slotIndex) const
{
    return findPatternSlot(state, trackId, slotIndex).getProperty("id").toString();
}

int ProjectDocument::getPatternIndexForId(const juce::String& trackId, const juce::String& patternId) const
{
    if (patternId.isEmpty()) return -1;
    const auto track = findProjectTrack(state, trackId);
    const auto patterns = track.getChildWithName(patternsType);
    for (int i = 0; i < patterns.getNumChildren(); ++i)
    {
        const auto slot = patterns.getChild(i);
        if (slot.getProperty("id").toString() == patternId)
            return static_cast<int>(slot.getProperty("index", -1));
    }
    return -1;
}

juce::String ProjectDocument::getPatternName(const juce::String& trackId, int slotIndex) const
{
    return findPatternSlot(state, trackId, slotIndex).getProperty("name").toString();
}

bool ProjectDocument::isPatternCreated(const juce::String& trackId, int slotIndex) const
{
    return static_cast<bool>(findPatternSlot(state, trackId, slotIndex).getProperty("created", false));
}

bool ProjectDocument::getPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step) const
{
    const auto slot = findPatternSlot(state, trackId, slotIndex);
    const auto notes = slot.getChildWithName(patternNotesType);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        const auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && static_cast<int>(note.getProperty("step", -1)) == step)
            return true;
    }
    return false;
}

void ProjectDocument::setPatternCreated(const juce::String& trackId, int slotIndex, bool created)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (slot.isValid())
        slot.setProperty("created", created, nullptr);
}

void ProjectDocument::setPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step, bool enabled)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (!slot.isValid())
        return;

    auto notes = slot.getChildWithName(patternNotesType);
    for (int i = notes.getNumChildren(); --i >= 0;)
    {
        auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && static_cast<int>(note.getProperty("step", -1)) == step)
        {
            if (enabled)
            {
                slot.setProperty("created", true, nullptr);
                return;
            }
            notes.removeChild(i, nullptr);
        }
    }

    if (enabled)
    {
        juce::ValueTree note(patternNoteType);
        note.setProperty("pitch", pitch, nullptr);
        note.setProperty("step", step, nullptr);
        note.setProperty("velocity", 100, nullptr);
        notes.addChild(note, -1, nullptr);
        slot.setProperty("created", true, nullptr);
    }
}

juce::String ProjectDocument::getPlaylistPatternId(const juce::String& trackId, int bar) const
{
    if (!juce::isPositiveAndBelow(bar, 4096)) return {};
    const auto track = findProjectTrack(state, trackId);
    const auto clips = track.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        const auto clip = clips.getChild(i);
        const auto start = static_cast<int>(clip.getProperty("startBar", -1));
        const auto length = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        if (bar >= start && bar < start + length)
            return clip.getProperty("patternId").toString();
    }
    return {};
}

void ProjectDocument::setPlaylistClip(const juce::String& trackId, const juce::String& patternId,
                                     int startBar, int lengthBars, bool enabled)
{
    if (!juce::isPositiveAndBelow(startBar, 4096)) return;
    auto track = findProjectTrack(state, trackId);
    if (!track.isValid()) return;
    auto clips = track.getChildWithName(clipsType);
    if (!clips.isValid())
    {
        clips = juce::ValueTree(clipsType);
        track.addChild(clips, -1, nullptr);
    }

    // The first Playlist pass is a bar grid: one pattern clip occupies one bar.
    // Replacing the cell removes an older clip at that position before adding the new reference.
    for (int i = clips.getNumChildren(); --i >= 0;)
    {
        const auto clip = clips.getChild(i);
        const auto start = static_cast<int>(clip.getProperty("startBar", -1));
        if (clip.getType() == playlistClipType && start == startBar)
            clips.removeChild(i, nullptr);
    }

    if (enabled && patternId.isNotEmpty())
    {
        juce::ValueTree clip(playlistClipType);
        clip.setProperty("id", newId(), nullptr);
        clip.setProperty("patternId", patternId, nullptr);
        clip.setProperty("startBar", startBar, nullptr);
        clip.setProperty("lengthBars", juce::jmax(1, lengthBars), nullptr);
        clips.addChild(clip, -1, nullptr);
    }
}

int ProjectDocument::getTicksPerQuarterNote() const
{
    return static_cast<int>(state.getProperty("ticksPerQuarterNote", ticksPerQuarterNote));
}

double ProjectDocument::getInitialTempoBpm() const
{
    const auto map = state.getChildWithName(tempoMapType);
    return map.getNumChildren() > 0
               ? static_cast<double>(map.getChild(0).getProperty("bpm", 120.0))
               : 120.0;
}

void ProjectDocument::setInitialTempoBpm(double bpm)
{
    auto map = state.getChildWithName(tempoMapType);
    if (map.getNumChildren() == 0)
        map.addChild(makeTempoPoint(bpm), -1, nullptr);
    else
        map.getChild(0).setProperty("bpm", juce::jlimit(20.0, 300.0, bpm), nullptr);
    state.setProperty("tempoBpm", juce::jlimit(20.0, 300.0, bpm), nullptr);
}

ProjectDocument::TimeSignature ProjectDocument::getInitialTimeSignature() const
{
    const auto map = state.getChildWithName(meterMapType);
    if (map.getNumChildren() == 0)
        return {};

    const auto point = map.getChild(0);
    return { static_cast<int>(point.getProperty("numerator", 4)),
             static_cast<int>(point.getProperty("denominator", 4)) };
}
