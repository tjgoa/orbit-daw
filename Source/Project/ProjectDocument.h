#pragma once

#include <JuceHeader.h>
#include <limits>
#include <optional>
#include <map>
#include <vector>

// Orbit's on-disk project document is kept separate from Tracktion Engine's
// Edit model so the project schema can remain stable if the engine changes.
class ProjectDocument
{
public:
    static constexpr int currentSchemaVersion = 20;
    static constexpr int ticksPerQuarterNote = 960;
    // Pattern positions and durations are stored as signed integer ticks with
    // 64 ticks per step. This is the representational boundary, not a UI cap.
    static constexpr int maximumPatternLengthSteps = std::numeric_limits<int>::max() / 64;
    static constexpr int patternBankCount = 8;
    static constexpr int patternsPerBank = 8;
    static constexpr int patternSlotCount = patternBankCount * patternsPerBank;

    enum class TrackKind
    {
        instrument,
        midi,
        audio
    };

    struct TimeSignature
    {
        int numerator = 4;
        int denominator = 4;
    };

    struct AutomationPoint
    {
        AutomationPoint() = default;
        AutomationPoint(double at, float v) : position(at), value(v) {}
        AutomationPoint(int at, float v) : position(double(at)), value(v) {}
        double position = 0;    // sixteenth-note steps; fractional positions preserve recorded timing
        float value = 0.5f;     // normalized 0..1 parameter value
    };

    struct AutomationLane
    {
        juce::String pluginName;
        juce::String pluginId; // Optional engine instance ID for Song events; empty in legacy lanes.
        juce::String parameterId;
        juce::String parameterName;
        juce::Array<AutomationPoint> points;
    };

    struct PatternNote
    {
        int pitch = 60;
        int startTick = 0;   // 1/64 of a sixteenth-note step
        int lengthTicks = 64;
        int velocity = 100;
    };
    struct PatternSwitch
    {
        int startTick=0,lengthTicks=64;
        bool slide=false;
    };

    ProjectDocument();
    ProjectDocument(const ProjectDocument& other);
    ProjectDocument& operator=(const ProjectDocument& other);
    ProjectDocument(ProjectDocument&&) noexcept = default;
    ProjectDocument& operator=(ProjectDocument&&) noexcept = default;

    static ProjectDocument createNew();
    static std::optional<ProjectDocument> loadFromFile(const juce::File& file,
                                                        juce::String& error);

    bool saveToFile(const juce::File& file, juce::String& error);
    void setName(const juce::String& name);
    juce::String addTrack(TrackKind kind, const juce::String& name = {});
    juce::String importTrackFrom(const ProjectDocument& source, int sourceTrackIndex,
                                 bool removeUnusedFx);
    void addInstrumentTrack();
    void setTracktionEditState(const juce::ValueTree& editState);
    juce::ValueTree getTracktionEditState() const;

    juce::String getName() const;
    juce::String getProjectId() const;
    juce::String getDescription() const;
    void setDescription(const juce::String& description);
    juce::String getCreatorUrl() const;
    void setCreatorUrl(const juce::String& url);
    bool isPluginDelayCompensationEnabled() const;
    void setPluginDelayCompensationEnabled(bool enabled);
    bool isMultiCoreAudioEnabled() const;
    void setMultiCoreAudioEnabled(bool enabled);
    juce::File getFile() const { return projectFile; }
    void clearFileAssociation() { projectFile = juce::File{}; }
    bool hasFile() const { return projectFile.existsAsFile(); }
    int getTrackCount() const;
    TrackKind getTrackKind(int index) const;
    juce::String getTrackId(int index) const;
    juce::String getTrackName(int index) const;
    void setTrackName(int index, const juce::String& name);
    bool removeTrack(int index);
    int getLoopStartBar() const;
    int getLoopLengthBars() const;
    bool isLoopEnabled() const;
    bool setLoopSettings(int startBar, int lengthBars, bool enabled);
    juce::Colour getTrackColour(int index) const;
    void setTrackColour(int index, juce::Colour colour);
    bool getTrackGradientHighlight(int index) const;
    void setTrackGradientHighlight(int index, bool enabled);
    bool getTrackShowGridNotes(int index) const;
    void setTrackShowGridNotes(int index, bool enabled);
    bool cloneTrackPatternsAndArrangement(const juce::String& sourceTrackId,
                                         const juce::String& destinationTrackId);
    bool copyPatternFrom(const ProjectDocument& source,const juce::String& sourceTrackId,int sourceSlot,
                         const juce::String& destinationTrackId,int destinationSlot);
    int getTrackSelectedPatternIndex(const juce::String& trackId) const;
    void setTrackSelectedPatternIndex(const juce::String& trackId, int patternIndex);
    void setTrackMixerState(int index, double gainDb, double pan, bool muted, bool solo);
    void setTrackSendGain(int index, int bus, double gainDb);
    bool setFxReturnMixerState(int bus, const juce::String& channelId,
                               const juce::String& name, double gainDb, double pan,
                               bool muted, bool solo);
    void setMasterMixerState(double gainDb, double pan, bool muted);
    juce::String getPatternId(const juce::String& trackId, int slotIndex) const;
    int getPatternIndexForId(const juce::String& trackId, const juce::String& patternId) const;
    juce::String getPatternName(const juce::String& trackId, int slotIndex) const;
    void setPatternName(const juce::String& trackId, int slotIndex, const juce::String& name);
    bool isPatternCreated(const juce::String& trackId, int slotIndex) const;
    bool getPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    int getPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    int getPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    std::vector<PatternNote> getPatternNotes(const juce::String& trackId, int slotIndex) const;
    int getPatternLengthSteps(const juce::String& trackId, int slotIndex) const;
    void setPatternCreated(const juce::String& trackId, int slotIndex, bool created);
    void setPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step, bool enabled);
    void setPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step, int velocity);
    void setPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step, int lengthSteps);
    void setPatternNoteTicks(const juce::String& trackId, int slotIndex, int pitch,
                             int startTick, int lengthTicks, int velocity = 100);
    // Strict insertion preserves coincident records as separate notes. The
    // existing set operation remains an upsert for legacy single-note callers.
    bool appendPatternNoteTicks(const juce::String& trackId, int slotIndex, int pitch,
                                int startTick, int lengthTicks, int velocity = 100);
    bool movePatternNoteStack(const juce::String& trackId, int slotIndex, int fromPitch,
                              int fromTick, int toPitch, int toTick);
    bool setPatternNoteStackLengthTicks(const juce::String& trackId, int slotIndex,
                                        int pitch, int startTick, int lengthTicks);
    void removePatternNoteTicks(const juce::String& trackId, int slotIndex, int pitch, int startTick);
    void setPatternLengthSteps(const juce::String& trackId, int slotIndex, int lengthSteps);
    std::vector<PatternSwitch> getPatternSwitches(const juce::String& trackId,int slotIndex) const;
    void replacePatternSwitches(const juce::String& trackId,int slotIndex,const std::vector<PatternSwitch>&);
    void setPatternSwitch(const juce::String& trackId,int slotIndex,bool slide,int tick,int lengthTicks,bool enabled);
    int getPatternAutomationLaneCount(const juce::String& trackId, int slotIndex) const;
    AutomationLane getPatternAutomationLane(const juce::String& trackId, int slotIndex, int laneIndex) const;
    void setPatternAutomationTarget(const juce::String& trackId, int slotIndex,
                                    const juce::String& pluginName, const juce::String& parameterId,
                                    const juce::String& parameterName);
    void setPatternAutomationPoint(const juce::String& trackId, int slotIndex,
                                   const juce::String& pluginName, const juce::String& parameterId,
                                   const juce::String& parameterName, int step, float value);
    void removePatternAutomationPoint(const juce::String& trackId, int slotIndex,
                                      const juce::String& pluginName, const juce::String& parameterId,
                                      int step);
    void replacePatternAutomationPoints(const juce::String& trackId, int slotIndex,
                                         const juce::String& pluginName, const juce::String& parameterId,
                                         const juce::String& parameterName, const juce::Array<AutomationPoint>& points);
    int getSongAutomationLaneCount(const juce::String& trackId) const;
    AutomationLane getSongAutomationLane(const juce::String& trackId, int laneIndex) const;
    void setSongAutomationPoint(const juce::String& trackId,
                                const juce::String& pluginName, const juce::String& parameterId,
                                const juce::String& parameterName, int songStep, float value);
    void replaceSongAutomationPoints(const juce::String& trackId,
                                      const juce::String& pluginName, const juce::String& parameterId,
                                      const juce::String& parameterName, const juce::Array<AutomationPoint>& points,
                                      const juce::String& pluginId = {});
    void remapSongAutomationPluginIds(const juce::String& trackId,
                                      const std::map<juce::String, juce::String>& ids);
    void remapSongAutomationParameter(const juce::String& owner, const juce::String& pluginName,
                                      const juce::String& fromPlugin, const juce::String& fromParameter,
                                      const juce::String& toPlugin, const juce::String& toParameter,
                                      const juce::String& toName, bool includeLegacy);
    void moveTrack(int sourceIndex, int destinationIndex);
    struct PlaylistClipRecord
    {
        juce::String patternId;
        int startBar = 0;
        int lengthBars = 1;
        bool exactLength = false;
    };
    std::vector<PlaylistClipRecord> getPlaylistClips(const juce::String& trackId) const;
    // Exact ranges may be shorter than their pattern. This is a generic native
    // clip capability, not a source-format concept. No Playlist UI limit applies.
    bool setPlaylistClipExactBars(const juce::String& trackId, const juce::String& patternId,
                                  int startBar, int lengthBars);
    juce::String getPlaylistPatternId(const juce::String& trackId, int bar) const;
    int getPlaylistClipStartBar(const juce::String& trackId, int bar) const;
    int getPlaylistClipLengthBars(const juce::String& trackId, int bar) const;
    bool playlistClipFollowsPatternLength(const juce::String& trackId, int bar) const;
    void setPlaylistClip(const juce::String& trackId, const juce::String& patternId,
                         int startBar, int lengthBars, bool enabled);
    int getTicksPerQuarterNote() const;
    double getInitialTempoBpm() const;
    void setInitialTempoBpm(double bpm);
    TimeSignature getInitialTimeSignature() const;
    void setInitialTimeSignature(int numerator, int denominator);

private:
    explicit ProjectDocument(juce::ValueTree state, juce::File file = {});

    juce::ValueTree state;
    juce::File projectFile;
};
