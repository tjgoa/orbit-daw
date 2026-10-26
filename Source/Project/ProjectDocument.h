#pragma once

#include <JuceHeader.h>
#include <optional>

// Orbit's on-disk project document is kept separate from Tracktion Engine's
// Edit model so the project schema can remain stable if the engine changes.
class ProjectDocument
{
public:
    static constexpr int currentSchemaVersion = 2;
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

    ProjectDocument();

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
    juce::File getFile() const { return projectFile; }
    bool hasFile() const { return projectFile.existsAsFile(); }
    int getTrackCount() const;
    juce::String getTrackId(int index) const;
    juce::String getTrackName(int index) const;
    void setTrackName(int index, const juce::String& name);
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
    void setPatternCreated(const juce::String& trackId, int slotIndex, bool created);
    void setPatternNote(const juce::String& trackId, int slotIndex, int pitch, int step, bool enabled);
    juce::String getPlaylistPatternId(const juce::String& trackId, int bar) const;
    void setPlaylistClip(const juce::String& trackId, const juce::String& patternId,
                         int startBar, int lengthBars, bool enabled);
    int getTicksPerQuarterNote() const;
    double getInitialTempoBpm() const;
    void setInitialTempoBpm(double bpm);
    TimeSignature getInitialTimeSignature() const;

private:
    explicit ProjectDocument(juce::ValueTree state, juce::File file = {});

    juce::ValueTree state;
    juce::File projectFile;
};
