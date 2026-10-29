#include "OrionSfsImportDialog.h"
#include "OrionSfsConversion.h"
#include "OrionSfsReport.h"

#include <charconv>
#include <limits>

namespace orbit::import
{
namespace
{
class Preview final : public juce::Component, private juce::ListBoxModel
{
public:
    Preview(const juce::File& file, std::function<bool(const ProjectImportPlan&, std::string&)> callback)
        : commit(std::move(callback))
    {
        addAndMakeVisible(summary);
        summary.setText("Experimental SFS recovery: choose source content and enter the original timing settings.", juce::dontSendNotification);
        auto field = [&](juce::Label& label, const char* text, juce::TextEditor& editor)
        {
            addAndMakeVisible(label); label.setText(text, juce::dontSendNotification);
            addAndMakeVisible(editor); editor.setTitle(text); editor.onTextChange = [this] { invalidate(); };
        };
        field(ppqLabel, "Source PPQ", ppq);
        field(tempoLabel, "Tempo BPM", tempo);
        field(meterLabel, "Meter N/D", meter);
        addAndMakeVisible(patterns);
        patterns.setName("Source patterns and audio tracks");
        patterns.setTooltip("Use Command/Ctrl-click to select multiple source rows.");
        patterns.setModel(this); patterns.setMultipleSelectionEnabled(true); patterns.setRowHeight(26);
        addAndMakeVisible(tentative); tentative.setButtonText("Use tentative note mapping");
        addAndMakeVisible(partial); partial.setButtonText("Accept omitted tempo changes, automation, routing and instrument/effect state");
        addAndMakeVisible(arrangement); arrangement.setButtonText("Recover tentative playlist ranges for selected patterns (4/4, whole bars)");
        addAndMakeVisible(audio); audio.setButtonText("Recover tentative dry audio from relinked WAVs; omit clip processing");
        addAndMakeVisible(mediaChoice); mediaChoice.setTextWhenNothingSelected("No decoded audio pool");
        mediaChoice.onChange=[this]{updateMediaPath();};
        addAndMakeVisible(mediaPath); mediaPath.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(relinkButton); relinkButton.setButtonText("Locate WAV...");
        relinkButton.onClick=[this]{chooseMedia();};
        addAndMakeVisible(omitEvents); omitEvents.setButtonText("Omit unsupported events from playback; keep their exact bytes in recovery notes");
        addAndMakeVisible(rounding); rounding.setButtonText("Allow rounding to Orbit's note resolution");
        for (auto* button : { &tentative, &partial, &rounding, &arrangement, &audio, &omitEvents }) button->onClick = [this] { invalidate(); };
        addAndMakeVisible(report); report.setMultiLine(true); report.setReadOnly(true); report.setScrollbarsShown(true);
        report.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(previewButton); previewButton.setButtonText("Preview recovery"); previewButton.onClick = [this] { prepare(); };
        addAndMakeVisible(importButton); importButton.setButtonText("Merge selected content"); importButton.setEnabled(false);
        importButton.onClick = [this]
        {
            if (! prepared) return;
            std::string error;
            if (! commit(*prepared, error)) { showFailure(error); return; }
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>()) window->exitModalState(1);
        };
        addAndMakeVisible(cancelButton); cancelButton.setButtonText("Cancel"); cancelButton.onClick = [this]
        { if (auto* window = findParentComponentOfClass<juce::DialogWindow>()) window->exitModalState(0); };
        setSize(940, 760);
        try
        {
            orion::SfsInspection inspection;
            std::string error;
            const std::filesystem::path path(file.getFullPathName().toStdString());
            if (! orion::OrionSfsReader::readDocument(path, source, inspection, error))
            { showFailure(error); previewButton.setEnabled(false); return; }
            inspectionText = orion::createInspectionReport(source, inspection);
            if (source.metadata.initialTempoBpm && *source.metadata.initialTempoBpm>=20 && *source.metadata.initialTempoBpm<=300)
            {
                tempo.setText(juce::String(*source.metadata.initialTempoBpm,6).trimCharactersAtEnd("0").trimCharactersAtEnd("."),false);
                tempoLabel.setText("Tempo BPM (candidate)",juce::dontSendNotification);
                tempo.setTooltip("Tentative value read from GLOB+12. Confirm the original BPM before import.");
            }
            source.metadata.title = file.getFileNameWithoutExtension().toStdString() + " (recovered patterns)";
            for (const auto& track : source.tracks)
            {
                if (track.kind == orion::TrackKind::audio)
                {
                    rows.push_back({track.sourceTrackIndex,0,true});
                    rowNames.add(juce::String::fromUTF8(track.name.c_str())+" / Audio / "+juce::String(track.audioClips.size())+" decoded clips");
                    continue;
                }
                for (const auto& pattern : track.patterns)
                {
                    const auto referenced = std::any_of(track.playlist.begin(), track.playlist.end(), [&](const orion::PlaylistPlacement& placement)
                        { return placement.patternIndex == pattern.sourcePatternIndex; });
                    if (pattern.events.empty() && !pattern.opaqueEventData && !referenced) continue;
                    rows.push_back({track.sourceTrackIndex, pattern.sourcePatternIndex});
                    const auto slot = pattern.sourcePatternIndex;
                    rowNames.add(juce::String::fromUTF8(track.name.c_str()) + " / "
                        + juce::String::charToString(static_cast<juce::juce_wchar>('A' + slot / 8))
                        + juce::String(slot % 8 + 1) + " / "
                        + (pattern.opaqueEventData ? juce::String("undecoded event payload")
                            : juce::String(pattern.events.size()) + " event records")
                        + (track.patternLayoutRecognized ? juce::String() : juce::String(" / incomplete track layout")));
                }
            }
            for (std::size_t index=0;index<source.audioSources.size();++index)
                mediaChoice.addItem(juce::String::fromUTF8(source.audioSources[index].displayName.c_str()),static_cast<int>(index)+1);
            if (!source.audioSources.empty()) mediaChoice.setSelectedId(1,juce::dontSendNotification);
            updateMediaPath();
            report.setText(juce::String::fromUTF8(inspectionText.c_str()), false);
            patterns.updateContent();
            if (rows.empty()) previewButton.setEnabled(false);
        }
        catch (const std::exception& error)
        { showFailure(error.what()); previewButton.setEnabled(false); }
    }

    ~Preview() override { patterns.setModel(nullptr); }
    void paint(juce::Graphics& graphics) override { graphics.fillAll(juce::Colour::fromRGB(27, 39, 49)); }
    void resized() override
    {
        auto area = getLocalBounds().reduced(14);
        summary.setBounds(area.removeFromTop(38));
        auto fields = area.removeFromTop(54);
        auto nextField = [&](juce::Label& label, juce::TextEditor& editor)
        {
            auto cell = fields.removeFromLeft(210).reduced(0, 2);
            label.setBounds(cell.removeFromTop(22)); editor.setBounds(cell.reduced(0, 1));
            fields.removeFromLeft(12);
        };
        nextField(ppqLabel, ppq); nextField(tempoLabel, tempo); nextField(meterLabel, meter);
        area.removeFromTop(8);
        patterns.setBounds(area.removeFromTop(130));
        tentative.setBounds(area.removeFromTop(28)); partial.setBounds(area.removeFromTop(28));
        arrangement.setBounds(area.removeFromTop(28)); audio.setBounds(area.removeFromTop(28));
        auto mediaRow=area.removeFromTop(28);
        relinkButton.setBounds(mediaRow.removeFromRight(130)); mediaRow.removeFromRight(8);
        mediaChoice.setBounds(mediaRow);
        mediaPath.setBounds(area.removeFromTop(24));
        omitEvents.setBounds(area.removeFromTop(28));
        rounding.setBounds(area.removeFromTop(28));
        auto buttons = area.removeFromBottom(36);
        cancelButton.setBounds(buttons.removeFromRight(100)); buttons.removeFromRight(8);
        importButton.setBounds(buttons.removeFromRight(185)); buttons.removeFromRight(8);
        previewButton.setBounds(buttons.removeFromRight(160));
        area.removeFromBottom(8); report.setBounds(area);
    }
private:
    int getNumRows() override { return rowNames.size(); }
    juce::String getNameForRow(int row) override { return rowNames[row]; }
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override
    {
        if (selected) graphics.fillAll(juce::Colour::fromRGB(42, 86, 112));
        graphics.setColour(juce::Colours::white);
        graphics.drawText(rowNames[row], 8, 0, width - 16, height, juce::Justification::centredLeft);
    }
    void selectedRowsChanged(int) override { invalidate(); }
    void invalidate() { prepared.reset(); importButton.setEnabled(false); }
    void showFailure(const std::string& error)
    {
        report.setText(juce::String::fromUTF8(("Recovery unavailable: " + error + "\n\n" + inspectionText).c_str()), false);
        invalidate();
    }
    static bool readInteger(const juce::String& text, std::int64_t& result)
    {
        const auto value = text.trim().toStdString();
        const auto converted = std::from_chars(value.data(), value.data() + value.size(), result);
        return converted.ec == std::errc{} && converted.ptr == value.data() + value.size();
    }
    void prepare()
    {
        invalidate();
        orion::ConversionOptions options;
        std::int64_t ppqValue = 0, numerator = 0, denominator = 0;
        const auto meterText = meter.getText().trim();
        if (! readInteger(ppq.getText(), ppqValue) || ppqValue <= 0
            || ! meterText.containsChar('/') || ! readInteger(meterText.upToFirstOccurrenceOf("/", false, false), numerator)
            || ! readInteger(meterText.fromFirstOccurrenceOf("/", false, false), denominator)
            || numerator < 1 || numerator > 32 || denominator < 1 || denominator > 32)
        { showFailure("Enter a positive source PPQ and an explicit meter such as 4/4."); return; }
        options.sourceTicksPerQuarterNote = ppqValue;
        options.meterNumerator = static_cast<std::uint16_t>(numerator);
        options.meterDenominator = static_cast<std::uint16_t>(denominator);
        try
        {
            const auto text = tempo.getText().trim().toStdString();
            std::size_t consumed = 0;
            const auto bpm = std::stod(text, &consumed);
            if (consumed != text.size()) throw std::invalid_argument("tempo");
            options.tempoBpm = bpm;
        }
        catch (const std::exception&) { showFailure("Enter the original tempo in BPM."); return; }
        options.acceptTentativeNotes = tentative.getToggleState();
        options.acceptPatternOnlyRecovery = partial.getToggleState();
        options.allowTickRounding = rounding.getToggleState();
        options.recoverTentativePlaylist = arrangement.getToggleState();
        options.recoverTentativeDryAudio = audio.getToggleState();
        options.mediaRelinks = mediaRelinks;
        options.omitUnsupportedEvents = omitEvents.getToggleState();
        const auto selected = patterns.getSelectedRows();
        for (int i = 0; i < selected.size(); ++i)
        {
            const auto& row=rows[static_cast<std::size_t>(selected[i])];
            if (row.audio) options.selectedAudioTracks.push_back(row.track);
            else options.selectedPatterns.emplace_back(row.track,row.pattern);
        }
        if (options.selectedPatterns.empty() && options.selectedAudioTracks.empty()) { showFailure("Choose one or more pattern or audio rows to recover."); return; }
        ProjectImportPlan plan;
        std::string error;
        if (! orion::prepareConversion(source, options, plan, error)) { showFailure(error); return; }
        report.setText(juce::String::fromUTF8((orion::createConversionReport(plan) + "\n" + inspectionText).c_str()), false);
        prepared = std::move(plan);
        importButton.setEnabled(true);
    }
    void updateMediaPath()
    {
        const auto index=mediaChoice.getSelectedId()-1;
        const bool valid=juce::isPositiveAndBelow(index,static_cast<int>(source.audioSources.size()));
        relinkButton.setEnabled(valid);
        if (!valid) {mediaPath.setText("No structurally decoded external audio references.",juce::dontSendNotification);return;}
        const auto found=mediaRelinks.find(static_cast<std::uint32_t>(index));
        const auto text=found==mediaRelinks.end() ? "Unresolved: "+source.audioSources[static_cast<std::size_t>(index)].sourceReference : "Relinked: "+found->second;
        mediaPath.setText(juce::String::fromUTF8(text.c_str()),juce::dontSendNotification);
        mediaPath.setTooltip(juce::String::fromUTF8(text.c_str()));
    }
    void chooseMedia()
    {
        const auto index=mediaChoice.getSelectedId()-1;
        if (!juce::isPositiveAndBelow(index,static_cast<int>(source.audioSources.size()))) return;
        chooser=std::make_unique<juce::FileChooser>("Locate audio source: "+mediaChoice.getText(),juce::File{},"*.wav");
        const juce::Component::SafePointer<Preview> safe(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe,index](const juce::FileChooser& result)
            {
                if (!safe || !result.getResult().existsAsFile()) return;
                safe->mediaRelinks[static_cast<std::uint32_t>(index)]=result.getResult().getFullPathName().toStdString();
                safe->updateMediaPath(); safe->invalidate();
            });
    }
    orion::ImportDocument source;
    std::string inspectionText;
    struct SelectionRow {std::uint32_t track=0,pattern=0;bool audio=false;};
    std::vector<SelectionRow> rows;
    std::map<std::uint32_t,std::string> mediaRelinks;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::StringArray rowNames;
    std::optional<ProjectImportPlan> prepared;
    std::function<bool(const ProjectImportPlan&, std::string&)> commit;
    juce::Label summary, ppqLabel, tempoLabel, meterLabel;
    juce::TextEditor ppq, tempo, meter, report;
    juce::ListBox patterns;
    juce::ToggleButton tentative, partial, rounding, arrangement, audio, omitEvents;
    juce::ComboBox mediaChoice;
    juce::Label mediaPath;
    juce::TextButton relinkButton;
    juce::TextButton previewButton, importButton, cancelButton;
};
}

std::unique_ptr<juce::Component> createOrionSfsPreview(const juce::File& file,
                          std::function<bool(const ProjectImportPlan&, std::string&)> commit)
{
    return std::make_unique<Preview>(file, std::move(commit));
}

void launchOrionSfsPreview(const juce::File& file, juce::Component* parent,
                          std::function<bool(const ProjectImportPlan&, std::string&)> commit)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Orion SFS recovery";
    options.dialogBackgroundColour = juce::Colour::fromRGB(27, 39, 49);
    options.componentToCentreAround = parent;
    options.resizable = false;
    options.content = juce::OptionalScopedPointer<juce::Component>(createOrionSfsPreview(file, std::move(commit)).release(), true);
    options.launchAsync();
}
}
