#pragma once

#include <JuceHeader.h>
#include <optional>

// Orbit's on-disk project document is kept separate from Tracktion Engine's
// Edit model so the project schema can remain stable if the engine changes.
class ProjectDocument
{
public:
    static constexpr int currentSchemaVersion = 4;
    static constexpr int ticksPerQuarterNote = 960;
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
        int position = 0;       // sixteenth-note steps (pattern or song timeline)
        float value = 0.5f;     // normalized 0..1 parameter value
    };

    struct AutomationLane
    {
        juce::String pluginName;
        juce::String parameterId;
        juce::String parameterName;
        juce::Array<AutomationPoint> points;
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
    void addInstrumentTrack();
    void setTracktionEditState(const juce::ValueTree& editState);
    juce::ValueTree getTracktionEditState() const;

    juce::String getName() const;
    juce::String getProjectId() const;
    juce::String getDescription() const;
    void setDescription(const juce::String& description);
    juce::String getCreatorUrl() const;
    void setCreatorUrl(const juce::String& url);
    juce::File getFile() const { return projectFile; }
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
    bool cloneTrackPatternsAndArrangement(const juce::String& sourceTrackId,
                                         const juce::String& destinationTrackId);
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
    bool isPatternCreated(const juce::String& trackId, int slotIndex) const;
    bool getPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    int getPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    int getPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step) const;
    int getPatternLengthSteps(const juce::String& trackId, int slotIndex) const;
    void setPatternCreated(const juce::String& trackId, int slotIndex, bool created);
    void setPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step, bool enabled);
    void setPatternNoteVelocity(const juce::String& trackId, int slotIndex, int pitch, int step, int velocity);
    void setPatternNoteLength(const juce::String& trackId, int slotIndex, int pitch, int step, int lengthSteps);
    void setPatternLengthSteps(const juce::String& trackId, int slotIndex, int lengthSteps);
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
    int getSongAutomationLaneCount(const juce::String& trackId) const;
    AutomationLane getSongAutomationLane(const juce::String& trackId, int laneIndex) const;
    void setSongAutomationPoint(const juce::String& trackId,
                                const juce::String& pluginName, const juce::String& parameterId,
                                const juce::String& parameterName, int songStep, float value);
    void moveTrack(int sourceIndex, int destinationIndex);
    juce::String getPlaylistPatternId(const juce::String& trackId, int bar) const;
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
