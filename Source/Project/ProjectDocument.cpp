#include "ProjectDocument.h"

#include <cmath>
#include <algorithm>
#include <map>

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
const juce::Identifier patternAutomationLanesType { "PatternAutomationLanes" };
const juce::Identifier automationLaneType { "AutomationLane" };
const juce::Identifier automationPointType { "AutomationPoint" };
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
        ensureChild(slot, patternAutomationLanesType);
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

int patternNoteStartTick(const juce::ValueTree& note)
{
    return note.hasProperty("startTick")
        ? static_cast<int>(note.getProperty("startTick"))
        : static_cast<int>(note.getProperty("step", 0)) * 64;
}

int patternNoteLengthTicks(const juce::ValueTree& note)
{
    return note.hasProperty("lengthTicks")
        ? juce::jmax(1, static_cast<int>(note.getProperty("lengthTicks")))
        : juce::jmax(1, static_cast<int>(note.getProperty("lengthSteps", 1))) * 64;
}

int patternLengthInBars(int lengthSteps)
{
    const auto safeLength = juce::jmax(1, lengthSteps);
    return safeLength / 16 + (safeLength % 16 != 0 ? 1 : 0);
}

int stepsForTickEnd(juce::int64 endTick)
{
    const auto safeEnd = std::max<juce::int64>(0, endTick);
    return static_cast<int>(std::min<juce::int64>(
        ProjectDocument::maximumPatternLengthSteps, safeEnd / 64 + (safeEnd % 64 != 0 ? 1 : 0)));
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
    if (!track.hasProperty("selectedPatternIndex"))
        track.setProperty("selectedPatternIndex", 0, nullptr);

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
        for (int bus = 0; bus < 8; ++bus)
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
    if (!state.hasProperty("pluginDelayCompensation"))
        state.setProperty("pluginDelayCompensation", true, nullptr);
    if (!state.hasProperty("multiCoreAudio"))
        state.setProperty("multiCoreAudio", true, nullptr);
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
            for (int bus = 0; bus < 8; ++bus)
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

ProjectDocument::ProjectDocument(const ProjectDocument& other)
    : state(other.state.createCopy()), projectFile(other.projectFile)
{
}

ProjectDocument& ProjectDocument::operator=(const ProjectDocument& other)
{
    if (this != &other)
    {
        state = other.state.createCopy();
        projectFile = other.projectFile;
    }
    return *this;
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
    track.setProperty("selectedPatternIndex", 0, nullptr);
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
    for (int bus = 0; bus < 8; ++bus)
        channel.setProperty("fx" + juce::String(bus + 1) + "SendDb", -60.0, nullptr);
    channel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
    mixer.addChild(channel, -1, nullptr);
    tracks.addChild(track, -1, nullptr);
    return trackId;
}

juce::String ProjectDocument::importTrackFrom(const ProjectDocument& source, int sourceTrackIndex,
                                               bool removeUnusedFx)
{
    const auto sourceTracks = source.state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(sourceTrackIndex, sourceTracks.getNumChildren()))
        return {};

    auto importedTrack = sourceTracks.getChild(sourceTrackIndex).createCopy();
    const auto sourceChannelId = importedTrack.getProperty("mixerChannelId").toString();
    const auto sourceMixer = source.state.getChildWithName(mixerType);
    const auto sourceChannel = findMixerChannel(sourceMixer, sourceChannelId);

    auto tracks = state.getChildWithName(tracksType);
    auto mixer = state.getChildWithName(mixerType);
    const auto importedTrackId = newId();
    const auto importedChannelId = newId();
    const auto trackName = importedTrack.getProperty("name").toString();

    importedTrack.setProperty("id", importedTrackId, nullptr);
    importedTrack.setProperty("order", tracks.getNumChildren(), nullptr);
    importedTrack.setProperty("mixerChannelId", importedChannelId, nullptr);

    juce::ValueTree importedChannel;
    if (sourceChannel.isValid())
        importedChannel = sourceChannel.createCopy();
    else
        importedChannel = juce::ValueTree(mixerChannelType);

    importedChannel.setProperty("id", importedChannelId, nullptr);
    importedChannel.setProperty("kind", "track", nullptr);
    importedChannel.setProperty("trackId", importedTrackId, nullptr);
    importedChannel.setProperty("name", trackName, nullptr);
    importedChannel.setProperty("outputId", mixer.getProperty("masterChannelId"), nullptr);
    if (!importedChannel.hasProperty("gainDb")) importedChannel.setProperty("gainDb", 0.0, nullptr);
    if (!importedChannel.hasProperty("pan")) importedChannel.setProperty("pan", 0.0, nullptr);
    if (!importedChannel.hasProperty("muted")) importedChannel.setProperty("muted", false, nullptr);
    if (!importedChannel.hasProperty("solo")) importedChannel.setProperty("solo", false, nullptr);
    for (int bus = 0; bus < 8; ++bus)
    {
        const auto sendName = "fx" + juce::String(bus + 1) + "SendDb";
        if (removeUnusedFx || !importedChannel.hasProperty(sendName))
            importedChannel.setProperty(sendName, -60.0, nullptr);
    }

    std::vector<std::pair<juce::String, juce::String>> patternIdMap;
    auto patterns = importedTrack.getChildWithName(patternsType);
    for (int i = 0; i < patterns.getNumChildren(); ++i)
    {
        auto slot = patterns.getChild(i);
        if (slot.getType() != patternSlotType) continue;
        const auto previousId = slot.getProperty("id").toString();
        const auto newPatternId = newId();
        slot.setProperty("id", newPatternId, nullptr);
        patternIdMap.emplace_back(previousId, newPatternId);
    }

    auto clips = importedTrack.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        auto clip = clips.getChild(i);
        const auto oldPatternId = clip.getProperty("patternId").toString();
        for (const auto& mapping : patternIdMap)
            if (mapping.first == oldPatternId)
            {
                clip.setProperty("patternId", mapping.second, nullptr);
                break;
            }
    }

    ensureTrackShape(importedTrack, mixer);
    tracks.addChild(importedTrack, -1, nullptr);
    mixer.addChild(importedChannel, -1, nullptr);
    return importedTrackId;
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

juce::String ProjectDocument::getDescription() const
{
    return state.getProperty("description").toString();
}

void ProjectDocument::setDescription(const juce::String& description)
{
    state.setProperty("description", description.trim(), nullptr);
}

juce::String ProjectDocument::getCreatorUrl() const
{
    return state.getProperty("creatorUrl").toString();
}

void ProjectDocument::setCreatorUrl(const juce::String& url)
{
    state.setProperty("creatorUrl", url.trim(), nullptr);
}

bool ProjectDocument::isPluginDelayCompensationEnabled() const
{
    return static_cast<bool>(state.getProperty("pluginDelayCompensation", true));
}

void ProjectDocument::setPluginDelayCompensationEnabled(bool enabled)
{
    state.setProperty("pluginDelayCompensation", enabled, nullptr);
}

bool ProjectDocument::isMultiCoreAudioEnabled() const
{
    return static_cast<bool>(state.getProperty("multiCoreAudio", true));
}

void ProjectDocument::setMultiCoreAudioEnabled(bool enabled)
{
    state.setProperty("multiCoreAudio", enabled, nullptr);
}

int ProjectDocument::getTrackCount() const
{
    return state.getChildWithName(tracksType).getNumChildren();
}

ProjectDocument::TrackKind ProjectDocument::getTrackKind(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren()))
        return TrackKind::instrument;

    const auto kind = tracks.getChild(index).getProperty("kind").toString();
    if (kind == "audio") return TrackKind::audio;
    if (kind == "midi") return TrackKind::midi;
    return TrackKind::instrument;
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

bool ProjectDocument::removeTrack(int index)
{
    auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return false;
    const auto track = tracks.getChild(index);
    const auto channelId = track.getProperty("mixerChannelId").toString();
    auto mixer = state.getChildWithName(mixerType);
    for (int i = mixer.getNumChildren(); --i >= 0;)
        if (mixer.getChild(i).getProperty("id").toString() == channelId)
            mixer.removeChild(i, nullptr);
    tracks.removeChild(index, nullptr);
    return true;
}

int ProjectDocument::getLoopStartBar() const
{
    return juce::jmax(1, static_cast<int>(state.getProperty("loopStartBar", 1)));
}

int ProjectDocument::getLoopLengthBars() const
{
    return juce::jlimit(1, 512, static_cast<int>(state.getProperty("loopLengthBars", 4)));
}

bool ProjectDocument::isLoopEnabled() const
{
    return static_cast<bool>(state.getProperty("loopEnabled", false));
}

bool ProjectDocument::setLoopSettings(int startBar, int lengthBars, bool enabled)
{
    startBar = juce::jlimit(1, 999, startBar);
    lengthBars = juce::jlimit(1, 512, lengthBars);
    const bool changed = getLoopStartBar() != startBar || getLoopLengthBars() != lengthBars || isLoopEnabled() != enabled;
    if (!changed) return false;
    state.setProperty("loopStartBar", startBar, nullptr);
    state.setProperty("loopLengthBars", lengthBars, nullptr);
    state.setProperty("loopEnabled", enabled, nullptr);
    return true;
}

juce::Colour ProjectDocument::getTrackColour(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return {};
    const auto track = tracks.getChild(index);
    const auto colour = static_cast<juce::uint32>(static_cast<juce::int64>(track.getProperty("colour", 0)));
    return colour == 0 ? juce::Colour{} : juce::Colour(colour);
}

void ProjectDocument::setTrackColour(int index, juce::Colour colour)
{
    auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return;
    tracks.getChild(index).setProperty("colour", static_cast<juce::int64>(colour.getARGB()), nullptr);
}

bool ProjectDocument::getTrackGradientHighlight(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    return juce::isPositiveAndBelow(index, tracks.getNumChildren())
        && static_cast<bool>(tracks.getChild(index).getProperty("gradientHighlight", false));
}

void ProjectDocument::setTrackGradientHighlight(int index, bool enabled)
{
    auto tracks = state.getChildWithName(tracksType);
    if (juce::isPositiveAndBelow(index, tracks.getNumChildren()))
        tracks.getChild(index).setProperty("gradientHighlight", enabled, nullptr);
}

bool ProjectDocument::getTrackShowGridNotes(int index) const
{
    const auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return false;
    return static_cast<bool>(tracks.getChild(index).getProperty("showGridNotes", false));
}

void ProjectDocument::setTrackShowGridNotes(int index, bool enabled)
{
    auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(index, tracks.getNumChildren())) return;
    tracks.getChild(index).setProperty("showGridNotes", enabled, nullptr);
}

bool ProjectDocument::copyPatternFrom(const ProjectDocument& source,const juce::String& sourceTrackId,int sourceSlot,
                                     const juce::String& destinationTrackId,int destinationSlot)
{
    const auto from=findPatternSlot(source.state,sourceTrackId,sourceSlot);
    auto to=findPatternSlot(state,destinationTrackId,destinationSlot);
    if(!from.isValid() || !to.isValid())return false;
    const auto id=to.getProperty("id");const auto bank=to.getProperty("bank"),slot=to.getProperty("slot");
    auto copy=from.createCopy();copy.setProperty("id",id,nullptr);
    copy.setProperty("index",destinationSlot,nullptr);
    if(to.hasProperty("bank"))copy.setProperty("bank",bank,nullptr);
    if(to.hasProperty("slot"))copy.setProperty("slot",slot,nullptr);
    const auto slotName=[](int i){return juce::String::charToString(juce::juce_wchar('A'+i/8))+juce::String(i%8+1);};
    if(from.getProperty("name").toString()==slotName(sourceSlot))copy.setProperty("name",slotName(destinationSlot),nullptr);
    to.copyPropertiesAndChildrenFrom(copy,nullptr);return true;
}

bool ProjectDocument::cloneTrackPatternsAndArrangement(const juce::String& sourceTrackId,
                                                        const juce::String& destinationTrackId)
{
    if (sourceTrackId.isEmpty() || destinationTrackId.isEmpty() || sourceTrackId == destinationTrackId)
        return false;
    auto source = findProjectTrack(state, sourceTrackId);
    auto destination = findProjectTrack(state, destinationTrackId);
    if (!source.isValid() || !destination.isValid()
        || source.getProperty("kind").toString() != "instrument"
        || destination.getProperty("kind").toString() != "instrument")
        return false;

    auto sourcePatterns = source.getChildWithName(patternsType);
    for (int index = 0; index < patternSlotCount; ++index)
    {
        auto from = findPatternSlot(state, sourceTrackId, index);
        auto to = findPatternSlot(state, destinationTrackId, index);
        if (!from.isValid() || !to.isValid()) continue;
        to.setProperty("name", from.getProperty("name"), nullptr);
        to.setProperty("created", from.getProperty("created", false), nullptr);
        to.setProperty("lengthSteps", from.getProperty("lengthSteps", 16), nullptr);
        for (const auto& childType : { patternNotesType, patternAutomationLanesType })
        {
            auto fromChild = from.getChildWithName(childType);
            auto toChild = to.getChildWithName(childType);
            if (!toChild.isValid())
            {
                toChild = juce::ValueTree(childType);
                to.addChild(toChild, -1, nullptr);
            }
            toChild.removeAllChildren(nullptr);
            if (fromChild.isValid())
                for (int child = 0; child < fromChild.getNumChildren(); ++child)
                    toChild.addChild(fromChild.getChild(child).createCopy(), -1, nullptr);
        }
    }
    destination.setProperty("selectedPatternIndex", source.getProperty("selectedPatternIndex", 0), nullptr);

    // Pattern slots have track-specific IDs. Rebuild each copied clip against the
    // matching destination slot so the copied arrangement remains self-contained.
    auto destinationClips = destination.getChildWithName(clipsType);
    auto sourceClips = source.getChildWithName(clipsType);
    if (!destinationClips.isValid())
    {
        destinationClips = juce::ValueTree(clipsType);
        destination.addChild(destinationClips, -1, nullptr);
    }
    destinationClips.removeAllChildren(nullptr);
    if (sourceClips.isValid())
    {
        for (int i = 0; i < sourceClips.getNumChildren(); ++i)
        {
            const auto sourceClip = sourceClips.getChild(i);
            const auto patternIndex = getPatternIndexForId(sourceTrackId, sourceClip.getProperty("patternId").toString());
            if (patternIndex < 0) continue;
            juce::ValueTree copiedClip(playlistClipType);
            copiedClip.setProperty("id", newId(), nullptr);
            copiedClip.setProperty("patternId", getPatternId(destinationTrackId, patternIndex), nullptr);
            copiedClip.setProperty("startBar", sourceClip.getProperty("startBar", 0), nullptr);
            copiedClip.setProperty("lengthBars", sourceClip.getProperty("lengthBars", 1), nullptr);
            copiedClip.setProperty("followsPatternLength",
                sourceClip.getProperty("followsPatternLength", true), nullptr);
            if (sourceClip.hasProperty("exactLength"))
                copiedClip.setProperty("exactLength", sourceClip.getProperty("exactLength"), nullptr);
            destinationClips.addChild(copiedClip, -1, nullptr);
        }
    }

    auto destinationSongAutomation = destination.getChildWithName(automationType);
    const auto sourceSongAutomation = source.getChildWithName(automationType);
    if (!destinationSongAutomation.isValid())
    {
        destinationSongAutomation = juce::ValueTree(automationType);
        destination.addChild(destinationSongAutomation, -1, nullptr);
    }
    destinationSongAutomation.removeAllChildren(nullptr);
    if (sourceSongAutomation.isValid())
        for (int i = 0; i < sourceSongAutomation.getNumChildren(); ++i)
            destinationSongAutomation.addChild(sourceSongAutomation.getChild(i).createCopy(), -1, nullptr);
    return true;
}

int ProjectDocument::getTrackSelectedPatternIndex(const juce::String& trackId) const
{
    const auto track = findProjectTrack(state, trackId);
    if (!track.isValid()) return 0;
    return juce::jlimit(0, patternSlotCount - 1,
                        static_cast<int>(track.getProperty("selectedPatternIndex", 0)));
}

void ProjectDocument::setTrackSelectedPatternIndex(const juce::String& trackId, int patternIndex)
{
    auto track = findProjectTrack(state, trackId);
    if (!track.isValid()) return;
    const auto clamped = juce::jlimit(0, patternSlotCount - 1, patternIndex);
    if (getTrackSelectedPatternIndex(trackId) == clamped) return;
    track.setProperty("selectedPatternIndex", clamped, nullptr);
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
        || !juce::isPositiveAndBelow(bus, 8)) return;
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
    if (!juce::isPositiveAndBelow(bus, 8) || channelId.isEmpty()) return false;

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

void ProjectDocument::setPatternName(const juce::String& trackId, int slotIndex, const juce::String& name)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (slot.isValid() && name.isNotEmpty()) slot.setProperty("name", name, nullptr);
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
            && patternNoteStartTick(note) / 64 == step)
            return true;
    }
    return false;
}

int ProjectDocument::getPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step) const
{
    const auto notes = findPatternSlot(state, trackId, slotIndex).getChildWithName(patternNotesType);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        const auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) / 64 == step)
            return juce::jlimit(1, 127, static_cast<int>(note.getProperty("velocity", 100)));
    }
    return 100;
}

int ProjectDocument::getPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step) const
{
    const auto slot = findPatternSlot(state, trackId, slotIndex);
    const auto notes = slot.getChildWithName(patternNotesType);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        const auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) / 64 == step)
            return stepsForTickEnd(patternNoteLengthTicks(note));
    }
    return 1;
}

std::vector<ProjectDocument::PatternNote> ProjectDocument::getPatternNotes(
    const juce::String& trackId, int slotIndex) const
{
    std::vector<PatternNote> result;
    const auto notes = findPatternSlot(state, trackId, slotIndex).getChildWithName(patternNotesType);
    result.reserve(static_cast<size_t>(notes.getNumChildren()));
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        const auto note = notes.getChild(i);
        result.push_back({ static_cast<int>(note.getProperty("pitch", 60)),
                           patternNoteStartTick(note), patternNoteLengthTicks(note),
                           juce::jlimit(1, 127, static_cast<int>(note.getProperty("velocity", 100))) });
    }
    return result;
}

int ProjectDocument::getPatternLengthSteps(const juce::String& trackId, int slotIndex) const
{
    const auto slot = findPatternSlot(state, trackId, slotIndex);
    return juce::jlimit(1, maximumPatternLengthSteps, static_cast<int>(slot.getProperty("lengthSteps", 16)));
}

void ProjectDocument::setPatternCreated(const juce::String& trackId, int slotIndex, bool created)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (slot.isValid())
        slot.setProperty("created", created, nullptr);
}

void ProjectDocument::setPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step, bool enabled)
{
    if (pitch < 0 || !juce::isPositiveAndBelow(step, maximumPatternLengthSteps)) return;
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (!slot.isValid())
        return;

    auto notes = slot.getChildWithName(patternNotesType);
    for (int i = notes.getNumChildren(); --i >= 0;)
    {
        auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) / 64 == step)
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
        note.setProperty("startTick", step * 64, nullptr);
        note.setProperty("velocity", 100, nullptr);
        note.setProperty("lengthSteps", 1, nullptr);
        note.setProperty("lengthTicks", 64, nullptr);
        notes.addChild(note, -1, nullptr);
        slot.setProperty("created", true, nullptr);
    }
}

void ProjectDocument::setPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step, int velocity)
{
    const auto notes = findPatternSlot(state, trackId, slotIndex).getChildWithName(patternNotesType);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) / 64 == step)
        {
            const auto onset = patternNoteStartTick(note);
            for (int member = 0; member < notes.getNumChildren(); ++member)
            {
                auto stacked = notes.getChild(member);
                if (static_cast<int>(stacked.getProperty("pitch", -1)) == pitch
                    && patternNoteStartTick(stacked) == onset)
                    stacked.setProperty("velocity", juce::jlimit(1, 127, velocity), nullptr);
            }
            return;
        }
    }
}

void ProjectDocument::setPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step, int lengthSteps)
{
    if (!juce::isPositiveAndBelow(step, maximumPatternLengthSteps)) return;
    auto slot = findPatternSlot(state, trackId, slotIndex);
    auto notes = slot.getChildWithName(patternNotesType);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) == step * 64)
        {
            const auto length = juce::jlimit(1, maximumPatternLengthSteps - step, lengthSteps);
            note.setProperty("lengthSteps", length, nullptr);
            note.setProperty("lengthTicks", length * 64, nullptr);
            return;
        }
    }
}

void ProjectDocument::setPatternNoteTicks(const juce::String& trackId, int slotIndex, int pitch,
                                          int startTick, int lengthTicks, int velocity)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (!slot.isValid() || pitch < 0 || startTick < 0
        || static_cast<juce::int64>(startTick) >= static_cast<juce::int64>(maximumPatternLengthSteps) * 64) return;
    auto notes = slot.getChildWithName(patternNotesType);
    const auto remainingTicks = static_cast<juce::int64>(maximumPatternLengthSteps) * 64 - startTick;
    const auto boundedLength = static_cast<int>(juce::jlimit<juce::int64>(1, remainingTicks, lengthTicks));
    const auto requiredSteps = stepsForTickEnd(static_cast<juce::int64>(startTick) + boundedLength);
    for (int i = 0; i < notes.getNumChildren(); ++i)
    {
        auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) == startTick)
        {
            note.setProperty("startTick", startTick, nullptr);
            note.setProperty("step", startTick / 64, nullptr);
            note.setProperty("lengthTicks", boundedLength, nullptr);
            note.setProperty("lengthSteps", (boundedLength + 63) / 64, nullptr);
            note.setProperty("velocity", juce::jlimit(1, 127, velocity), nullptr);
            slot.setProperty("lengthSteps", juce::jmax(getPatternLengthSteps(trackId, slotIndex), requiredSteps), nullptr);
            return;
        }
    }
    juce::ValueTree note(patternNoteType);
    note.setProperty("pitch", pitch, nullptr);
    note.setProperty("step", startTick / 64, nullptr);
    note.setProperty("startTick", startTick, nullptr);
    note.setProperty("velocity", juce::jlimit(1, 127, velocity), nullptr);
    note.setProperty("lengthSteps", (boundedLength + 63) / 64, nullptr);
    note.setProperty("lengthTicks", boundedLength, nullptr);
    notes.addChild(note, -1, nullptr);
    slot.setProperty("created", true, nullptr);
    slot.setProperty("lengthSteps", juce::jmax(getPatternLengthSteps(trackId, slotIndex), requiredSteps), nullptr);
}

bool ProjectDocument::appendPatternNoteTicks(const juce::String& trackId, int slotIndex, int pitch,
                                             int startTick, int lengthTicks, int velocity)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    auto notes = slot.getChildWithName(patternNotesType);
    constexpr auto maximumTicks = static_cast<juce::int64>(maximumPatternLengthSteps) * 64;
    if (!slot.isValid() || !notes.isValid() || pitch < 0 || pitch > 127 || startTick < 0
        || startTick >= maximumTicks || lengthTicks <= 0 || lengthTicks > maximumTicks - startTick
        || velocity < 1 || velocity > 127)
        return false;
    juce::ValueTree note(patternNoteType);
    note.setProperty("pitch", pitch, nullptr);
    note.setProperty("step", startTick / 64, nullptr);
    note.setProperty("startTick", startTick, nullptr);
    note.setProperty("velocity", velocity, nullptr);
    note.setProperty("lengthSteps", (lengthTicks + 63) / 64, nullptr);
    note.setProperty("lengthTicks", lengthTicks, nullptr);
    notes.addChild(note, -1, nullptr);
    slot.setProperty("created", true, nullptr);
    slot.setProperty("lengthSteps", juce::jmax(getPatternLengthSteps(trackId, slotIndex),
        stepsForTickEnd(static_cast<juce::int64>(startTick) + lengthTicks)), nullptr);
    return true;
}

bool ProjectDocument::movePatternNoteStack(const juce::String& trackId, int slotIndex,
                                           int fromPitch, int fromTick, int toPitch, int toTick)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    const auto notes = slot.getChildWithName(patternNotesType);
    constexpr auto maximumTicks = static_cast<juce::int64>(maximumPatternLengthSteps) * 64;
    if (!slot.isValid() || fromPitch < 0 || fromPitch > 127 || fromTick < 0 || fromTick >= maximumTicks
        || toPitch < 0 || toPitch > 127 || toTick < 0 || toTick >= maximumTicks
        || (fromPitch == toPitch && fromTick == toTick)) return false;
    std::vector<juce::ValueTree> stack;
    for (int index = 0; index < notes.getNumChildren(); ++index)
    {
        const auto note = notes.getChild(index);
        const auto pitch = static_cast<int>(note.getProperty("pitch", -1));
        const auto start = patternNoteStartTick(note);
        if (pitch == toPitch && start == toTick) return false;
        if (pitch == fromPitch && start == fromTick)
        {
            if (patternNoteLengthTicks(note) > maximumTicks - toTick) return false;
            stack.push_back(note);
        }
    }
    if (stack.empty()) return false;
    for (auto& note : stack)
    {
        note.setProperty("pitch", toPitch, nullptr);
        note.setProperty("startTick", toTick, nullptr);
        note.setProperty("step", toTick / 64, nullptr);
        slot.setProperty("lengthSteps", juce::jmax(getPatternLengthSteps(trackId, slotIndex),
            stepsForTickEnd(static_cast<juce::int64>(toTick) + patternNoteLengthTicks(note))), nullptr);
    }
    return true;
}

bool ProjectDocument::setPatternNoteStackLengthTicks(const juce::String& trackId, int slotIndex,
                                                     int pitch, int startTick, int lengthTicks)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    auto notes = slot.getChildWithName(patternNotesType);
    constexpr auto maximumTicks = static_cast<juce::int64>(maximumPatternLengthSteps) * 64;
    if (!slot.isValid() || pitch < 0 || pitch > 127 || startTick < 0 || startTick >= maximumTicks
        || lengthTicks <= 0 || lengthTicks > maximumTicks - startTick) return false;
    bool changed = false;
    for (int index = 0; index < notes.getNumChildren(); ++index)
    {
        auto note = notes.getChild(index);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch && patternNoteStartTick(note) == startTick)
        {
            note.setProperty("lengthTicks", lengthTicks, nullptr);
            note.setProperty("lengthSteps", (lengthTicks + 63) / 64, nullptr);
            changed = true;
        }
    }
    if (changed)
        slot.setProperty("lengthSteps", juce::jmax(getPatternLengthSteps(trackId, slotIndex),
            stepsForTickEnd(static_cast<juce::int64>(startTick) + lengthTicks)), nullptr);
    return changed;
}

void ProjectDocument::removePatternNoteTicks(const juce::String& trackId, int slotIndex,
                                             int pitch, int startTick)
{
    auto notes = findPatternSlot(state, trackId, slotIndex).getChildWithName(patternNotesType);
    for (int i = notes.getNumChildren(); --i >= 0;)
    {
        const auto note = notes.getChild(i);
        if (static_cast<int>(note.getProperty("pitch", -1)) == pitch
            && patternNoteStartTick(note) == startTick)
            notes.removeChild(i, nullptr);
    }
}

void ProjectDocument::setPatternLengthSteps(const juce::String& trackId, int slotIndex, int lengthSteps)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (slot.isValid())
        slot.setProperty("lengthSteps", juce::jlimit(1, maximumPatternLengthSteps, lengthSteps), nullptr);
}

namespace
{
juce::ValueTree findSongAutomationOwner(juce::ValueTree state, const juce::String& id, bool create = false)
{
    if (auto track = findProjectTrack(state, id); track.isValid()) return track;
    const bool valid = id == "mixer:master" || (id.startsWith("mixer:fx:") || id.startsWith("mixer:bus:"))
        && id.getLastCharacters(1).containsOnly("01234567") && id.length() == (id.startsWith("mixer:fx:") ? 10 : 11);
    if (!valid) return {};
    auto modules = state.getChildWithName("SongAutomationModules");
    if (!modules.isValid() && create) { modules = juce::ValueTree("SongAutomationModules"); state.addChild(modules, -1, nullptr); }
    for (auto module : modules) if (module.getProperty("id").toString() == id) return module;
    if (!create) return {};
    juce::ValueTree module("Module"); module.setProperty("id", id, nullptr); modules.addChild(module, -1, nullptr); return module;
}

juce::ValueTree findAutomationLane(juce::ValueTree lanes, const juce::String& pluginName,
                                   const juce::String& parameterId)
{
    for (int i = 0; i < lanes.getNumChildren(); ++i)
    {
        auto lane = lanes.getChild(i);
        if (lane.getType() == automationLaneType
            && lane.getProperty("pluginId").toString().isEmpty()
            && lane.getProperty("pluginName").toString() == pluginName
            && lane.getProperty("parameterId").toString() == parameterId)
            return lane;
    }
    return {};
}

ProjectDocument::AutomationLane readAutomationLane(const juce::ValueTree& lane)
{
    ProjectDocument::AutomationLane result;
    if (!lane.isValid()) return result;
    result.pluginName = lane.getProperty("pluginName").toString();
    result.pluginId = lane.getProperty("pluginId").toString();
    result.parameterId = lane.getProperty("parameterId").toString();
    result.parameterName = lane.getProperty("parameterName").toString();
    for (int i = 0; i < lane.getNumChildren(); ++i)
    {
        const auto point = lane.getChild(i);
        if (point.getType() != automationPointType) continue;
        result.points.add({ static_cast<double>(point.getProperty("position", 0)),
                            static_cast<float>(point.getProperty("value", 0.5)) });
    }
    std::stable_sort(result.points.begin(), result.points.end(), [](const auto& a, const auto& b) { return a.position < b.position; });
    return result;
}

void setAutomationPoint(juce::ValueTree lanes, const juce::String& pluginName,
                        const juce::String& parameterId, const juce::String& parameterName,
                        int position, float value, int maxPosition)
{
    if (!lanes.isValid() || pluginName.isEmpty() || parameterId.isEmpty()) return;
    auto lane = findAutomationLane(lanes, pluginName, parameterId);
    if (!lane.isValid())
    {
        lane = juce::ValueTree(automationLaneType);
        lane.setProperty("id", juce::Uuid().toString(), nullptr);
        lane.setProperty("pluginName", pluginName, nullptr);
        lane.setProperty("parameterId", parameterId, nullptr);
        lane.setProperty("parameterName", parameterName, nullptr);
        lanes.addChild(lane, -1, nullptr);
    }
    lane.setProperty("parameterName", parameterName, nullptr);
    const auto boundedPosition = juce::jlimit(0, maxPosition, position);
    for (int i=lane.getNumChildren();--i>=0;)
    {
        const auto p=lane.getChild(i);const double at=p.getProperty("position",0);
        if(at>boundedPosition && at<boundedPosition+1)lane.removeChild(i,nullptr);
    }
    for (int i = 0; i < lane.getNumChildren(); ++i)
    {
        auto point = lane.getChild(i);
        if (point.getType() == automationPointType
            && static_cast<double>(point.getProperty("position", -1)) == boundedPosition)
        {
            point.setProperty("value", juce::jlimit(0.0f, 1.0f, value), nullptr);
            return;
        }
    }
    juce::ValueTree point(automationPointType);
    point.setProperty("position", boundedPosition, nullptr);
    point.setProperty("value", juce::jlimit(0.0f, 1.0f, value), nullptr);
    lane.addChild(point, -1, nullptr);
}
}

int ProjectDocument::getPatternAutomationLaneCount(const juce::String& trackId, int slotIndex) const
{
    const auto slot = findPatternSlot(state, trackId, slotIndex);
    return slot.getChildWithName(patternAutomationLanesType).getNumChildren();
}

ProjectDocument::AutomationLane ProjectDocument::getPatternAutomationLane(const juce::String& trackId,
                                                                           int slotIndex, int laneIndex) const
{
    const auto lanes = findPatternSlot(state, trackId, slotIndex).getChildWithName(patternAutomationLanesType);
    return juce::isPositiveAndBelow(laneIndex, lanes.getNumChildren())
        ? readAutomationLane(lanes.getChild(laneIndex)) : AutomationLane{};
}

void ProjectDocument::setPatternAutomationTarget(const juce::String& trackId, int slotIndex,
                                                  const juce::String& pluginName,
                                                  const juce::String& parameterId,
                                                  const juce::String& parameterName)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (!slot.isValid()) return;
    auto lanes = slot.getChildWithName(patternAutomationLanesType);
    if (!lanes.isValid())
    {
        lanes = juce::ValueTree(patternAutomationLanesType);
        slot.addChild(lanes, -1, nullptr);
    }
    auto lane = findAutomationLane(lanes, pluginName, parameterId);
    if (!lane.isValid())
    {
        lane = juce::ValueTree(automationLaneType);
        lane.setProperty("id", juce::Uuid().toString(), nullptr);
        lane.setProperty("pluginName", pluginName, nullptr);
        lane.setProperty("parameterId", parameterId, nullptr);
        lane.setProperty("parameterName", parameterName, nullptr);
        lanes.addChild(lane, -1, nullptr);
    }
    lane.setProperty("parameterName", parameterName, nullptr);
}

std::vector<ProjectDocument::PatternSwitch> ProjectDocument::getPatternSwitches(const juce::String& trackId,int slotIndex) const
{
    std::vector<PatternSwitch> result;
    const auto slot=findPatternSlot(state,trackId,slotIndex);const auto switches=slot.getChildWithName("Articulations");
    const int ticks=getPatternLengthSteps(trackId,slotIndex)*64;
    for(const auto& child:switches)
    {
        const int start=child.getProperty("startTick",-1),length=child.getProperty("lengthTicks",0);
        if(start>=0 && start<ticks && length>0 && length<=ticks-start)
            result.push_back({start,length,bool(child.getProperty("slide",false))});
    }
    return result;
}
void ProjectDocument::replacePatternSwitches(const juce::String& trackId,int slotIndex,const std::vector<PatternSwitch>& values)
{
    auto slot=findPatternSlot(state,trackId,slotIndex);if(!slot.isValid())return;
    const int ticks=getPatternLengthSteps(trackId,slotIndex)*64;
    juce::ValueTree replacement("Articulations");
    for(const auto& value:values)
        if(value.startTick>=0 && value.startTick<ticks && value.lengthTicks>0 && value.lengthTicks<=ticks-value.startTick)
        {
            juce::ValueTree child("Switch");child.setProperty("startTick",value.startTick,nullptr);
            child.setProperty("lengthTicks",value.lengthTicks,nullptr);child.setProperty("slide",value.slide,nullptr);
            replacement.addChild(child,-1,nullptr);
        }
    const auto old=slot.getChildWithName("Articulations");if(old.isValid())slot.removeChild(old,nullptr);
    slot.addChild(replacement,-1,nullptr);
    if(!values.empty())slot.setProperty("created",true,nullptr);
}
void ProjectDocument::setPatternSwitch(const juce::String& trackId,int slotIndex,bool slide,int tick,int lengthTicks,bool enabled)
{
    const int ticks=getPatternLengthSteps(trackId,slotIndex)*64;
    if(tick<0 || tick>=ticks || lengthTicks<=0)return;
    const int end=tick+juce::jmin(lengthTicks,ticks-tick);std::vector<PatternSwitch> next;
    for(const auto& item:getPatternSwitches(trackId,slotIndex))
    {
        const int itemEnd=item.startTick+item.lengthTicks;
        if(item.slide!=slide || itemEnd<=tick || item.startTick>=end){next.push_back(item);continue;}
        if(item.startTick<tick)next.push_back({item.startTick,tick-item.startTick,slide});
        if(itemEnd>end)next.push_back({end,itemEnd-end,slide});
    }
    if(enabled)next.push_back({tick,end-tick,slide});
    replacePatternSwitches(trackId,slotIndex,next);
}

void ProjectDocument::setPatternAutomationPoint(const juce::String& trackId, int slotIndex,
                                                const juce::String& pluginName,
                                                const juce::String& parameterId,
                                                const juce::String& parameterName,
                                                int step, float value)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    if (!slot.isValid() || pluginName.isEmpty() || parameterId.isEmpty()) return;
    slot.setProperty("created", true, nullptr);
    auto lanes = slot.getChildWithName(patternAutomationLanesType);
    if (!lanes.isValid())
    {
        lanes = juce::ValueTree(patternAutomationLanesType);
        slot.addChild(lanes, -1, nullptr);
    }
    setAutomationPoint(lanes, pluginName, parameterId, parameterName, step, value,
                       juce::jmax(0, getPatternLengthSteps(trackId, slotIndex) - 1));
}

void ProjectDocument::replacePatternAutomationPoints(const juce::String& trackId,int slotIndex,
    const juce::String& pluginName,const juce::String& parameterId,const juce::String& parameterName,
    const juce::Array<AutomationPoint>& points)
{
    auto slot=findPatternSlot(state,trackId,slotIndex);
    if(!slot.isValid() || pluginName.isEmpty() || parameterId.isEmpty())return;
    auto lanes=slot.getChildWithName(patternAutomationLanesType);
    auto old=findAutomationLane(lanes,pluginName,parameterId);
    auto replacement=old.isValid()?old.createCopy():juce::ValueTree(automationLaneType);
    replacement.removeAllChildren(nullptr);
    if(!old.isValid())replacement.setProperty("id",juce::Uuid().toString(),nullptr);
    replacement.setProperty("pluginName",pluginName,nullptr);replacement.setProperty("parameterId",parameterId,nullptr);
    replacement.setProperty("parameterName",parameterName,nullptr);
    std::map<double,float> ordered;
    const double last=std::nextafter(double(getPatternLengthSteps(trackId,slotIndex)),0.0);
    for(const auto& point:points)if(std::isfinite(point.position) && std::isfinite(point.value))ordered[juce::jlimit(0.0,last,point.position)]=juce::jlimit(0.0f,1.0f,point.value);
    for(auto [position,value]:ordered)
    {
        juce::ValueTree point(automationPointType);point.setProperty("position",position,nullptr);point.setProperty("value",value,nullptr);
        replacement.addChild(point,-1,nullptr);
    }
    if(!lanes.isValid()){lanes=juce::ValueTree(patternAutomationLanesType);slot.addChild(lanes,-1,nullptr);}
    const int index=old.isValid()?lanes.indexOf(old):-1;
    lanes.addChild(replacement,index,nullptr);if(old.isValid())lanes.removeChild(old,nullptr);
    slot.setProperty("created",true,nullptr);
}

void ProjectDocument::removePatternAutomationPoint(const juce::String& trackId, int slotIndex,
                                                    const juce::String& pluginName,
                                                    const juce::String& parameterId, int step)
{
    auto slot = findPatternSlot(state, trackId, slotIndex);
    auto lanes = slot.getChildWithName(patternAutomationLanesType);
    auto lane = findAutomationLane(lanes, pluginName, parameterId);
    if (!lane.isValid()) return;
    const auto boundedPosition = juce::jlimit(0, maximumPatternLengthSteps - 1, step);
    for (int i = lane.getNumChildren(); --i >= 0;)
    {
        const auto point = lane.getChild(i);
        if (point.getType() == automationPointType
            && std::floor(static_cast<double>(point.getProperty("position", -1))) == boundedPosition)
            lane.removeChild(i, nullptr);
    }
}

int ProjectDocument::getSongAutomationLaneCount(const juce::String& trackId) const
{
    return findSongAutomationOwner(state, trackId).getChildWithName(automationType).getNumChildren();
}

ProjectDocument::AutomationLane ProjectDocument::getSongAutomationLane(const juce::String& trackId,
                                                                         int laneIndex) const
{
    const auto lanes = findSongAutomationOwner(state, trackId).getChildWithName(automationType);
    return juce::isPositiveAndBelow(laneIndex, lanes.getNumChildren())
        ? readAutomationLane(lanes.getChild(laneIndex)) : AutomationLane{};
}

void ProjectDocument::setSongAutomationPoint(const juce::String& trackId,
                                             const juce::String& pluginName,
                                             const juce::String& parameterId,
                                             const juce::String& parameterName,
                                             int songStep, float value)
{
    auto track = findSongAutomationOwner(state, trackId, true);
    if (!track.isValid()) return;
    auto lanes = track.getChildWithName(automationType);
    if (!lanes.isValid())
    {
        lanes = juce::ValueTree(automationType);
        track.addChild(lanes, -1, nullptr);
    }
    setAutomationPoint(lanes, pluginName, parameterId, parameterName, songStep, value, 65535);
}

void ProjectDocument::replaceSongAutomationPoints(const juce::String& trackId,
    const juce::String& pluginName, const juce::String& parameterId, const juce::String& parameterName,
    const juce::Array<AutomationPoint>& points, const juce::String& pluginId)
{
    auto track = findSongAutomationOwner(state, trackId, true);
    if (!track.isValid() || pluginName.isEmpty() || parameterId.isEmpty()) return;
    auto lanes = track.getChildWithName(automationType);
    if (!lanes.isValid()) { lanes = juce::ValueTree(automationType); track.addChild(lanes, -1, nullptr); }
    juce::ValueTree lane;
    for (auto child : lanes)
        if (child.getProperty("parameterId").toString() == parameterId
            && (pluginId.isNotEmpty() ? child.getProperty("pluginId").toString() == pluginId
                : child.getProperty("pluginId").toString().isEmpty()
                    && child.getProperty("pluginName").toString() == pluginName))
            { lane = child; break; }
    if (!lane.isValid())
    {
        lane = juce::ValueTree(automationLaneType);
        lane.setProperty("id", juce::Uuid().toString(), nullptr);
        lanes.addChild(lane, -1, nullptr);
    }
    lane.setProperty("pluginName", pluginName, nullptr);
    lane.setProperty("pluginId", pluginId, nullptr);
    lane.setProperty("parameterId", parameterId, nullptr);
    lane.setProperty("parameterName", parameterName, nullptr);
    lane.removeAllChildren(nullptr);
    std::map<double, float> ordered;
    for (const auto& point : points)
        if (std::isfinite(point.position) && std::isfinite(point.value)) ordered[juce::jlimit(0.0, std::nextafter(65536.0,0.0), point.position)] = juce::jlimit(0.0f, 1.0f, point.value);
    for (auto [position, value] : ordered)
    {
        juce::ValueTree point(automationPointType);
        point.setProperty("position", position, nullptr); point.setProperty("value", value, nullptr);
        lane.addChild(point, -1, nullptr);
    }
}

void ProjectDocument::remapSongAutomationPluginIds(const juce::String& trackId,
    const std::map<juce::String, juce::String>& ids)
{
    auto lanes = findSongAutomationOwner(state, trackId).getChildWithName(automationType);
    for (auto lane : lanes)
        if (auto i = ids.find(lane.getProperty("pluginId").toString()); i != ids.end())
            lane.setProperty("pluginId", i->second, nullptr);
}

void ProjectDocument::remapSongAutomationParameter(const juce::String& owner, const juce::String& pluginName,
    const juce::String& fromPlugin, const juce::String& fromParameter, const juce::String& toPlugin,
    const juce::String& toParameter, const juce::String& toName, bool includeLegacy)
{
    auto lanes=findSongAutomationOwner(state,owner).getChildWithName(automationType);
    for(auto lane:lanes)
    {
        const auto id=lane.getProperty("pluginId").toString();
        const bool matches=id==fromPlugin || (includeLegacy && id.isEmpty() && lane.getProperty("pluginName").toString()==pluginName);
        if(matches && lane.getProperty("parameterId").toString()==fromParameter)
        {
            lane.setProperty("pluginId",toPlugin,nullptr);lane.setProperty("parameterId",toParameter,nullptr);
            lane.setProperty("parameterName",toName,nullptr);
        }
    }
}

void ProjectDocument::moveTrack(int sourceIndex, int destinationIndex)
{
    auto tracks = state.getChildWithName(tracksType);
    if (!juce::isPositiveAndBelow(sourceIndex, tracks.getNumChildren())
        || !juce::isPositiveAndBelow(destinationIndex, tracks.getNumChildren())
        || sourceIndex == destinationIndex)
        return;
    tracks.moveChild(sourceIndex, destinationIndex, nullptr);
}

std::vector<ProjectDocument::PlaylistClipRecord> ProjectDocument::getPlaylistClips(const juce::String& trackId) const
{
    std::vector<PlaylistClipRecord> result;
    const auto clips = findProjectTrack(state, trackId).getChildWithName(clipsType);
    for (int index = 0; index < clips.getNumChildren(); ++index)
    {
        const auto clip = clips.getChild(index);
        if (!clip.hasType(playlistClipType)) continue;
        PlaylistClipRecord record;
        record.patternId = clip.getProperty("patternId").toString();
        record.startBar = static_cast<int>(clip.getProperty("startBar", -1));
        record.lengthBars = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        record.exactLength = static_cast<bool>(clip.getProperty("exactLength", false));
        const auto pattern = getPatternIndexForId(trackId, record.patternId);
        if (pattern < 0 || record.startBar < 0) continue;
        if (!record.exactLength)
            record.lengthBars = juce::jmax(record.lengthBars,
                patternLengthInBars(getPatternLengthSteps(trackId, pattern)));
        result.push_back(record);
    }
    std::stable_sort(result.begin(), result.end(), [](const auto& left, const auto& right)
        { return left.startBar < right.startBar; });
    return result;
}

bool ProjectDocument::setPlaylistClipExactBars(const juce::String& trackId, const juce::String& patternId,
                                               int startBar, int lengthBars)
{
    if (startBar < 0 || lengthBars < 1
        || static_cast<juce::int64>(startBar) + lengthBars > std::numeric_limits<int>::max()
        || getPatternIndexForId(trackId, patternId) < 0)
        return false;
    auto track = findProjectTrack(state, trackId);
    if (!track.isValid()) return false;
    setPlaylistClip(trackId, patternId, startBar, lengthBars, true);
    auto clips = track.getChildWithName(clipsType);
    for (int index = 0; index < clips.getNumChildren(); ++index)
    {
        auto clip = clips.getChild(index);
        if (static_cast<int>(clip.getProperty("startBar", -1)) == startBar)
        {
            clip.setProperty("exactLength", true, nullptr);
            clip.setProperty("followsPatternLength", false, nullptr);
            return true;
        }
    }
    return false;
}

juce::String ProjectDocument::getPlaylistPatternId(const juce::String& trackId, int bar) const
{
    if (bar < 0) return {};
    const auto track = findProjectTrack(state, trackId);
    const auto clips = track.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        const auto clip = clips.getChild(i);
        const auto start = static_cast<int>(clip.getProperty("startBar", -1));
        auto length = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        const auto patternIndex = getPatternIndexForId(trackId, clip.getProperty("patternId").toString());
        if (patternIndex >= 0 && !static_cast<bool>(clip.getProperty("exactLength", false)))
            length = juce::jmax(length, patternLengthInBars(getPatternLengthSteps(trackId, patternIndex)));
        if (start >= 0 && bar >= start && static_cast<juce::int64>(bar) < static_cast<juce::int64>(start) + length)
            return clip.getProperty("patternId").toString();
    }
    return {};
}

int ProjectDocument::getPlaylistClipStartBar(const juce::String& trackId, int bar) const
{
    if (bar < 0) return -1;
    const auto track = findProjectTrack(state, trackId);
    const auto clips = track.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        const auto clip = clips.getChild(i);
        const auto start = static_cast<int>(clip.getProperty("startBar", -1));
        auto length = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        const auto patternIndex = getPatternIndexForId(trackId, clip.getProperty("patternId").toString());
        if (patternIndex >= 0 && !static_cast<bool>(clip.getProperty("exactLength", false)))
            length = juce::jmax(length, patternLengthInBars(getPatternLengthSteps(trackId, patternIndex)));
        if (start >= 0 && bar >= start && static_cast<juce::int64>(bar) < static_cast<juce::int64>(start) + length)
            return start;
    }
    return -1;
}

bool ProjectDocument::playlistClipFollowsPatternLength(const juce::String& trackId, int bar) const
{
    const auto start = getPlaylistClipStartBar(trackId, bar);
    if (start < 0) return false;
    const auto track = findProjectTrack(state, trackId);
    const auto clips = track.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        const auto clip = clips.getChild(i);
        if (static_cast<int>(clip.getProperty("startBar", -1)) == start)
            return !clip.hasProperty("followsPatternLength")
                || static_cast<bool>(clip.getProperty("followsPatternLength", true));
    }
    return false;
}

int ProjectDocument::getPlaylistClipLengthBars(const juce::String& trackId, int bar) const
{
    const auto start = getPlaylistClipStartBar(trackId, bar);
    if (start < 0) return 0;
    const auto track = findProjectTrack(state, trackId);
    const auto clips = track.getChildWithName(clipsType);
    for (int i = 0; i < clips.getNumChildren(); ++i)
    {
        const auto clip = clips.getChild(i);
        if (static_cast<int>(clip.getProperty("startBar", -1)) != start) continue;
        const auto storedLength = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        const auto patternIndex = getPatternIndexForId(trackId, clip.getProperty("patternId").toString());
        if (patternIndex >= 0 && !static_cast<bool>(clip.getProperty("exactLength", false)))
            return juce::jmax(storedLength, patternLengthInBars(getPatternLengthSteps(trackId, patternIndex)));
        return storedLength;
    }
    return 0;
}

void ProjectDocument::setPlaylistClip(const juce::String& trackId, const juce::String& patternId,
                                     int startBar, int lengthBars, bool enabled)
{
    if (startBar < 0) return;
    auto track = findProjectTrack(state, trackId);
    if (!track.isValid()) return;
    auto clips = track.getChildWithName(clipsType);
    if (!clips.isValid())
    {
        clips = juce::ValueTree(clipsType);
        track.addChild(clips, -1, nullptr);
    }

    // A zero length follows the source pattern's step count. Positive values
    // represent an explicit arrangement length set by the Playlist resize tool.
    const bool followsPatternLength = lengthBars <= 0;
    const auto storedLength = juce::jmax(1, lengthBars);
    for (int i = clips.getNumChildren(); --i >= 0;)
    {
        const auto clip = clips.getChild(i);
        const auto start = static_cast<int>(clip.getProperty("startBar", -1));
        auto existingLength = juce::jmax(1, static_cast<int>(clip.getProperty("lengthBars", 1)));
        const auto existingPattern = getPatternIndexForId(trackId, clip.getProperty("patternId").toString());
        if (existingPattern >= 0 && !static_cast<bool>(clip.getProperty("exactLength", false)))
            existingLength = juce::jmax(existingLength,
                patternLengthInBars(getPatternLengthSteps(trackId, existingPattern)));
        const bool containsTarget = start >= 0 && startBar >= start
            && static_cast<juce::int64>(startBar) < static_cast<juce::int64>(start) + existingLength;
        if (clip.getType() == playlistClipType && (start == startBar || (!enabled && containsTarget)))
            clips.removeChild(i, nullptr);
    }

    if (enabled && patternId.isNotEmpty())
    {
        juce::ValueTree clip(playlistClipType);
        clip.setProperty("id", newId(), nullptr);
        clip.setProperty("patternId", patternId, nullptr);
        clip.setProperty("startBar", startBar, nullptr);
        clip.setProperty("lengthBars", storedLength, nullptr);
        clip.setProperty("followsPatternLength", followsPatternLength, nullptr);
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

void ProjectDocument::setInitialTimeSignature(int numerator, int denominator)
{
    numerator = juce::jlimit(1, 32, numerator);
    if (denominator != 1 && denominator != 2 && denominator != 4 && denominator != 8
        && denominator != 16 && denominator != 32)
        denominator = 4;

    auto map = state.getChildWithName(meterMapType);
    if (map.getNumChildren() == 0)
        map.addChild(makeMeterPoint(numerator, denominator), -1, nullptr);
    else
    {
        auto firstPoint = map.getChild(0);
        firstPoint.setProperty("numerator", numerator, nullptr);
        firstPoint.setProperty("denominator", denominator, nullptr);
    }
    state.setProperty("timeSignatureNumerator", numerator, nullptr);
    state.setProperty("timeSignatureDenominator", denominator, nullptr);
}
