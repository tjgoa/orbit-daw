#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Project/ProjectDocument.h"

namespace te = tracktion::engine;

namespace
{
class AudioSettingsWindow final : public juce::DocumentWindow
{
public:
    explicit AudioSettingsWindow(juce::AudioDeviceManager& manager)
        : DocumentWindow("Orbit Audio Settings", juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new juce::AudioDeviceSelectorComponent(manager, 0, 0, 1, 2, true, true, true, true), true);
        setResizable(true, true);
        centreWithSize(520, 460);
    }

    void closeButtonPressed() override { setVisible(false); }
};

class PlaylistGrid final : public juce::Component
{
public:
    static constexpr int barCount = 64;
    std::function<int()> trackCount;
    std::function<juce::String(int)> trackName;
    std::function<juce::String(int, int)> clipName;
    std::function<void(int, int)> cellClicked;
    std::function<void(int, int, int)> clipDragged;

    class PlaylistCellButton final : public juce::TextButton
    {
    public:
        std::function<void()> onActivate;
        std::function<void(int)> onDragToBar;
        std::function<bool()> sourceHasClip;

        PlaylistCellButton()
        {
            setClickingTogglesState(false);
            setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
            onClick = [this]
            {
                if (!dragGesture && onActivate)
                    onActivate();
            };
        }

        void configure(int track, int bar, const juce::String& trackLabel,
                       const juce::String& currentClip,
                       std::function<void(int, int)> activate,
                       std::function<void(int, int, int)> drag)
        {
            barIndex = bar;
            dragGesture = false;
            onActivate = [activateCallback = std::move(activate), track, bar]
            {
                if (activateCallback) activateCallback(track, bar);
            };
            onDragToBar = [dragCallback = std::move(drag), track, bar](int destinationBar)
            {
                if (dragCallback) dragCallback(track, bar, destinationBar);
            };
            sourceHasClip = [currentClip] { return currentClip.isNotEmpty(); };
            const auto barLabel = "Bar " + juce::String(bar + 1);
            setButtonText(currentClip.isNotEmpty() ? currentClip : barLabel);
            setTitle(trackLabel + ", " + barLabel);
            setDescription(currentClip.isNotEmpty()
                ? "Contains pattern " + currentClip + ". Activate to remove it or drag it to another bar."
                : "Empty playlist cell. Activate to place the selected pattern here.");
            setTooltip(trackLabel + ", " + barLabel + (currentClip.isNotEmpty()
                ? " — " + currentClip : " — empty; activate to place the selected pattern"));
        }

        void mouseDown(const juce::MouseEvent& event) override
        {
            dragGesture = false;
            dragSourceHasClip = sourceHasClip && sourceHasClip();
            juce::TextButton::mouseDown(event);
        }

        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (dragSourceHasClip && event.getDistanceFromDragStart() >= 8)
                dragGesture = true;
            juce::TextButton::mouseDrag(event);
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            const bool shouldMove = dragGesture && dragSourceHasClip
                && std::abs(event.position.y) < rowHeight;
            const int destinationBar = barIndex + static_cast<int>(std::floor(event.position.x / barWidth));
            juce::TextButton::mouseUp(event);
            if (shouldMove && onDragToBar)
                onDragToBar(destinationBar);
            dragGesture = false;
            dragSourceHasClip = false;
        }

    private:
        static constexpr int rowHeight = 46;
        static constexpr int barWidth = 92;
        int barIndex = -1;
        bool dragGesture = false;
        bool dragSourceHasClip = false;
    };

    void paint(juce::Graphics& g) override
    {
        constexpr int labelWidth = 190;
        constexpr int headerHeight = 34;
        constexpr int rowHeight = 46;
        constexpr int barWidth = 92;
        g.fillAll(juce::Colour::fromRGB(20, 29, 38));
        g.setColour(juce::Colour::fromRGB(29, 43, 55));
        g.fillRect(0, 0, getWidth(), headerHeight);
        g.setColour(juce::Colour::fromRGB(178, 198, 211));
        g.setFont(14.0f);
        g.drawText("GENERATOR", 12, 0, labelWidth - 16, headerHeight, juce::Justification::centredLeft);
        for (int bar = 0; bar < barCount; ++bar)
        {
            if (bar % 4 == 0)
            {
                g.setColour(juce::Colour::fromRGB(178, 198, 211));
                g.drawText("Bar " + juce::String(bar + 1), labelWidth + bar * barWidth, 0, barWidth, headerHeight,
                           juce::Justification::centred);
            }
        }

        const auto count = trackCount ? trackCount() : 0;
        for (int row = 0; row < count; ++row)
        {
            const auto y = headerHeight + row * rowHeight;
            g.setColour(row % 2 == 0 ? juce::Colour::fromRGB(25, 38, 49) : juce::Colour::fromRGB(22, 33, 43));
            g.fillRect(0, y, getWidth(), rowHeight);
            g.setColour(juce::Colour::fromRGB(178, 198, 211));
            g.drawText(juce::String(row + 1) + "   " + (trackName ? trackName(row) : juce::String{}),
                       12, y, labelWidth - 16, rowHeight, juce::Justification::centredLeft);
            for (int bar = 0; bar < barCount; ++bar)
            {
                const auto x = labelWidth + bar * barWidth;
                g.setColour(juce::Colour::fromRGB(53, 75, 90));
                g.drawRect(x, y, barWidth, rowHeight, 1);
                const auto label = clipName ? clipName(row, bar) : juce::String{};
                if (label.isNotEmpty())
                {
                    g.setColour(juce::Colour::fromRGB(39, 125, 165));
                    g.fillRoundedRectangle(static_cast<float>(x + 3), static_cast<float>(y + 4),
                                           static_cast<float>(barWidth - 6), static_cast<float>(rowHeight - 8), 5.0f);
                    g.setColour(juce::Colours::white);
                    g.drawText(label, x + 3, y + 4, barWidth - 6, rowHeight - 8, juce::Justification::centred);
                }
            }
        }
        g.setColour(juce::Colour::fromRGB(65, 94, 112));
        g.drawRect(getLocalBounds(), 1);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        constexpr int labelWidth = 190;
        constexpr int headerHeight = 34;
        constexpr int rowHeight = 46;
        constexpr int barWidth = 92;
        const auto row = (event.y - headerHeight) / rowHeight;
        const auto bar = (event.x - labelWidth) / barWidth;
        if (event.x < labelWidth || event.y < headerHeight || !trackCount
            || !juce::isPositiveAndBelow(row, trackCount()) || !juce::isPositiveAndBelow(bar, barCount))
            return;
        dragSourceRow = row;
        dragSourceBar = bar;
        dragSourceHasClip = clipName && clipName(row, bar).isNotEmpty();
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (dragSourceRow < 0 || dragSourceBar < 0) return;
        constexpr int labelWidth = 190;
        constexpr int headerHeight = 34;
        constexpr int rowHeight = 46;
        constexpr int barWidth = 92;
        const auto destinationRow = (event.y - headerHeight) / rowHeight;
        const auto destinationBar = (event.x - labelWidth) / barWidth;
        const bool moved = dragSourceHasClip && event.getDistanceFromDragStart() >= 8
            && destinationRow == dragSourceRow && juce::isPositiveAndBelow(destinationBar, barCount);
        if (moved)
        {
            if (clipDragged && destinationBar != dragSourceBar)
                clipDragged(dragSourceRow, dragSourceBar, destinationBar);
        }
        else if (event.getDistanceFromDragStart() < 8 && cellClicked)
        {
            cellClicked(dragSourceRow, dragSourceBar);
        }
        dragSourceRow = dragSourceBar = -1;
        dragSourceHasClip = false;
        repaint();
    }

    void refreshCellButtons()
    {
        const auto rows = trackCount ? juce::jmax(0, trackCount()) : 0;
        const auto requiredCount = rows * barCount;
        if (static_cast<int>(cellButtons.size()) != requiredCount)
        {
            cellButtons.clear();
            cellButtons.reserve(static_cast<size_t>(requiredCount));
            for (int row = 0; row < rows; ++row)
                for (int bar = 0; bar < barCount; ++bar)
                {
                    auto button = std::make_unique<PlaylistCellButton>();
                    addAndMakeVisible(*button);
                    cellButtons.push_back(std::move(button));
                }
        }

        for (int row = 0; row < rows; ++row)
            for (int bar = 0; bar < barCount; ++bar)
            {
                const auto index = static_cast<size_t>(row * barCount + bar);
                auto& button = *cellButtons[index];
                const auto currentClip = clipName ? clipName(row, bar) : juce::String{};
                button.configure(row, bar, trackName ? trackName(row) : juce::String("Generator ") + juce::String(row + 1),
                                 currentClip, cellClicked, clipDragged);
                button.setBounds(190 + bar * 92, 34 + row * 46, 92, 46);
            }
    }

private:
    int dragSourceRow = -1;
    int dragSourceBar = -1;
    bool dragSourceHasClip = false;
    std::vector<std::unique_ptr<PlaylistCellButton>> cellButtons;
};

class PlaylistWindow final : public juce::DocumentWindow
{
public:
    PlaylistWindow(std::function<int()> getTrackCount,
                   std::function<juce::String(int)> getTrackName,
                   std::function<juce::String(int, int)> getClipName,
                   std::function<void(int, int)> onCellClicked,
                   std::function<void(int, int, int)> onClipDragged)
        : DocumentWindow("Orbit Playlist", juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        grid = new PlaylistGrid();
        grid->trackCount = std::move(getTrackCount);
        grid->trackName = std::move(getTrackName);
        grid->clipName = std::move(getClipName);
        grid->cellClicked = std::move(onCellClicked);
        grid->clipDragged = std::move(onClipDragged);
        auto* viewport = new juce::Viewport();
        viewport->setViewedComponent(grid, true);
        setContentOwned(viewport, true);
        setResizable(true, true);
        centreWithSize(1040, 460);
        setVisible(false);
    }

    void refresh()
    {
        if (grid == nullptr) return;
        grid->setSize(190 + PlaylistGrid::barCount * 92, 34 + juce::jmax(1, grid->trackCount ? grid->trackCount() : 0) * 46);
        grid->refreshCellButtons();
        grid->repaint();
    }
    void closeButtonPressed() override { setVisible(false); }

private:
    PlaylistGrid* grid = nullptr;
};

class EqResponseGraph final : public juce::Component
{
public:
    std::function<float(float)> responseAtFrequency;
    std::function<float(int)> bandFrequency;
    std::function<float(int)> bandGain;
    std::function<void(int, float, float)> setBand;
    std::function<void()> onClick;

    EqResponseGraph() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }

    void mouseDown(const juce::MouseEvent& event) override
    {
        selectedBand = -1;
        didDragBand = false;
        const auto bounds = getLocalBounds().toFloat();
        for (int band = 0; band < 4; ++band)
        {
            const auto frequency = bandFrequency ? bandFrequency(band) : 0.0f;
            if (frequency < 20.0f) continue;
            const auto gain = bandGain ? bandGain(band) : 0.0f;
            const juce::Point<float> point(frequencyToX(frequency, bounds), gainToY(gain, bounds));
            if (point.getDistanceFrom(event.position) <= 12.0f)
            {
                selectedBand = band;
                break;
            }
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (selectedBand < 0 || !setBand) return;
        if (event.getDistanceFromDragStart() > 2) didDragBand = true;
        if (!didDragBand) return;
        const auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f) return;
        const auto x = juce::jlimit(bounds.getX(), bounds.getRight(), event.position.x);
        const auto y = juce::jlimit(bounds.getY(), bounds.getBottom(), event.position.y);
        const auto frequency = xToFrequency(x, bounds);
        const auto gain = juce::jmap(y, bounds.getBottom(), bounds.getY(), -18.0f, 18.0f);
        setBand(selectedBand, frequency, gain);
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        // A simple click continues to open the full EQ editor; dragging a node
        // directly edits that band's frequency and gain in the project.
        if (!didDragBand && onClick) onClick();
        selectedBand = -1;
        didDragBand = false;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.fillAll(juce::Colour::fromRGB(22, 39, 52));
        if (bounds.getWidth() < 4.0f || bounds.getHeight() < 4.0f) return;

        constexpr float minDb = -18.0f;
        constexpr float maxDb = 18.0f;
        for (int db = -18; db <= 18; db += 6)
        {
            const auto y = juce::jmap(static_cast<float>(db), minDb, maxDb, bounds.getBottom(), bounds.getY());
            g.setColour(db == 0 ? juce::Colour::fromRGB(96, 132, 151) : juce::Colour::fromRGB(48, 72, 89));
            g.drawHorizontalLine(juce::roundToInt(y), bounds.getX(), bounds.getRight());
        }
        for (float frequency : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
        {
            const auto x = frequencyToX(frequency, bounds);
            g.setColour(juce::Colour::fromRGB(40, 62, 77));
            g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
        }

        juce::Path curve;
        constexpr int points = 180;
        for (int i = 0; i < points; ++i)
        {
            const auto proportion = static_cast<float>(i) / static_cast<float>(points - 1);
            const auto frequency = 20.0f * std::pow(1000.0f, proportion);
            const auto db = juce::jlimit(minDb, maxDb, responseAtFrequency ? responseAtFrequency(frequency) : 0.0f);
            const auto x = bounds.getX() + proportion * bounds.getWidth();
            const auto y = juce::jmap(db, minDb, maxDb, bounds.getBottom(), bounds.getY());
            if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
        }
        g.setColour(juce::Colour::fromRGB(93, 211, 241));
        g.strokePath(curve, juce::PathStrokeType(getHeight() < 90 ? 1.5f : 2.5f));
        for (int band = 0; band < 4; ++band)
        {
            const auto frequency = bandFrequency ? bandFrequency(band) : 0.0f;
            if (frequency < 20.0f) continue;
            const auto gain = bandGain ? bandGain(band) : 0.0f;
            const auto point = juce::Point<float>(frequencyToX(frequency, bounds), gainToY(gain, bounds));
            g.setColour(juce::Colour::fromRGB(18, 41, 53));
            g.fillEllipse(point.x - 5.0f, point.y - 5.0f, 10.0f, 10.0f);
            g.setColour(juce::Colour::fromRGB(174, 239, 255));
            g.fillEllipse(point.x - 3.5f, point.y - 3.5f, 7.0f, 7.0f);
        }
        g.setColour(juce::Colour::fromRGB(174, 207, 224));
        g.drawRect(getLocalBounds(), 1);
    }

private:
    int selectedBand = -1;
    bool didDragBand = false;

    static float gainToY(float gain, juce::Rectangle<float> bounds)
    {
        return juce::jmap(juce::jlimit(-18.0f, 18.0f, gain), -18.0f, 18.0f, bounds.getBottom(), bounds.getY());
    }

    static float frequencyToX(float frequency, juce::Rectangle<float> bounds)
    {
        return bounds.getX() + std::log(frequency / 20.0f) / std::log(1000.0f) * bounds.getWidth();
    }

    static float xToFrequency(float x, juce::Rectangle<float> bounds)
    {
        const auto proportion = juce::jlimit(0.0f, 1.0f, (x - bounds.getX()) / bounds.getWidth());
        return 20.0f * std::pow(1000.0f, proportion);
    }
};

class EqualiserWindow final : public juce::DocumentWindow
{
public:
    std::function<float(int, int)> getParameter;
    std::function<float(int, float)> getResponse;
    std::function<void(int, int, float)> setParameter;
    std::function<void(int, int, float, float)> setBandValues;
    std::function<juce::String(int)> getTrackName;

    EqualiserWindow()
        : DocumentWindow("Orbit Equaliser", juce::Colour::fromRGB(23, 41, 54), juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentNonOwned(&content, false);
        content.setOpaque(true);
        content.addAndMakeVisible(title);
        title.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(238, 248, 252));
        content.addAndMakeVisible(graph);

        static const std::array<const char*, 4> bandNames { "LOW SHELF", "LOW MID", "HIGH MID", "HIGH SHELF" };
        static const std::array<const char*, 3> parameterNames { "Frequency", "Gain", "Q" };
        for (int band = 0; band < 4; ++band)
        {
            bandTitles[static_cast<size_t>(band)].setText(bandNames[static_cast<size_t>(band)], juce::dontSendNotification);
            bandTitles[static_cast<size_t>(band)].setJustificationType(juce::Justification::centredLeft);
            bandTitles[static_cast<size_t>(band)].setFont(juce::FontOptions(12.0f, juce::Font::bold));
            bandTitles[static_cast<size_t>(band)].setColour(juce::Label::textColourId, juce::Colour::fromRGB(187, 221, 237));
            content.addAndMakeVisible(bandTitles[static_cast<size_t>(band)]);
            for (int parameter = 0; parameter < 3; ++parameter)
            {
                auto& label = parameterTitles[static_cast<size_t>(band)][static_cast<size_t>(parameter)];
                label.setText(parameterNames[static_cast<size_t>(parameter)], juce::dontSendNotification);
                label.setFont(juce::FontOptions(10.5f));
                label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 223, 232));
                content.addAndMakeVisible(label);

                auto& slider = controls[static_cast<size_t>(band)][static_cast<size_t>(parameter)];
                slider.setSliderStyle(juce::Slider::LinearHorizontal);
                slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                if (parameter == 0)
                {
                    slider.setRange(20.0, 20000.0, 1.0);
                    slider.setSkewFactorFromMidPoint(1000.0);
                    slider.setTextValueSuffix(" Hz");
                }
                else if (parameter == 1)
                {
                    slider.setRange(-20.0, 20.0, 0.1);
                    slider.setTextValueSuffix(" dB");
                }
                else
                {
                    slider.setRange(0.1, 4.0, 0.01);
                }
                slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(128, 222, 244));
                slider.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(69, 124, 152));
                slider.setPopupDisplayEnabled(true, true, &content);
                slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
                slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(33, 57, 73));
                slider.onValueChange = [this, band, parameter]
                {
                    if (trackIndex >= 0 && setParameter)
                        setParameter(trackIndex, band * 3 + parameter, static_cast<float>(controls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].getValue()));
                    graph.repaint();
                };
                content.addAndMakeVisible(slider);
            }
        }

        graph.responseAtFrequency = [this](float frequency)
        {
            return trackIndex >= 0 && getResponse ? getResponse(trackIndex, frequency) : 0.0f;
        };
        graph.bandFrequency = [this](int band)
        {
            return trackIndex >= 0 && getParameter ? getParameter(trackIndex, band * 3) : 0.0f;
        };
        graph.bandGain = [this](int band)
        {
            return trackIndex >= 0 && getParameter ? getParameter(trackIndex, band * 3 + 1) : 0.0f;
        };
        graph.setBand = [this](int band, float frequency, float gain)
        {
            if (trackIndex >= 0 && setBandValues) setBandValues(trackIndex, band, frequency, gain);
        };
        setResizable(true, true);
        centreWithSize(760, 610);
        setVisible(false);
    }

    void openForTrack(int index)
    {
        trackIndex = index;
        const auto trackName = getTrackName ? getTrackName(index) : "Generator";
        title.setText(trackName + " - 4-Band Equaliser", juce::dontSendNotification);
        refreshParameterValues(index);
        setVisible(true);
        toFront(true);
    }

    void refreshParameterValues(int index)
    {
        if (trackIndex != index) return;
        for (int band = 0; band < 4; ++band)
            for (int parameter = 0; parameter < 3; ++parameter)
                if (getParameter)
                    controls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setValue(
                        getParameter(index, band * 3 + parameter), juce::dontSendNotification);
        graph.repaint();
    }

    void closeButtonPressed() override { setVisible(false); }

    void resized() override
    {
        DocumentWindow::resized();
        auto area = content.getLocalBounds().reduced(18);
        title.setBounds(area.removeFromTop(28));
        area.removeFromTop(8);
        graph.setBounds(area.removeFromTop(190));
        area.removeFromTop(10);
        const int rowHeight = area.getHeight() / 4;
        for (int band = 0; band < 4; ++band)
        {
            auto row = area.removeFromTop(rowHeight);
            bandTitles[static_cast<size_t>(band)].setBounds(row.removeFromTop(19));
            const int columnWidth = row.getWidth() / 3;
            for (int parameter = 0; parameter < 3; ++parameter)
            {
                auto column = row.removeFromLeft(columnWidth).reduced(4, 0);
                parameterTitles[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(column.removeFromTop(15));
                controls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(column);
            }
        }
    }

private:
    juce::Component content;
    juce::Label title;
    EqResponseGraph graph;
    std::array<juce::Label, 4> bandTitles;
    std::array<std::array<juce::Label, 3>, 4> parameterTitles;
    std::array<std::array<juce::Slider, 3>, 4> controls;
    int trackIndex = -1;
};

class PluginParameterWindow final : public juce::DocumentWindow
{
    class ParameterRow final : public juce::Component
    {
    public:
        ParameterRow(te::AutomatableParameter::Ptr parameter, std::function<void()> onChanged)
            : parameterRef(std::move(parameter)), changed(std::move(onChanged))
        {
            addAndMakeVisible(label);
            label.setText(parameterRef->getParameterName(), juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(228, 241, 248));
            label.setTooltip(parameterRef->getCurrentValueAsStringWithLabel());
            addAndMakeVisible(value);
            value.setSliderStyle(juce::Slider::LinearHorizontal);
            const auto parameterRange = parameterRef->getValueRange();
            value.setRange(parameterRange.getStart(), parameterRange.getEnd(), parameterRef->valueRange.interval);
            value.setValue(parameterRef->getCurrentValue(), juce::dontSendNotification);
            value.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            value.setTextValueSuffix(parameterRef->getLabel().isNotEmpty() ? " " + parameterRef->getLabel() : juce::String{});
            value.textFromValueFunction = [parameterHandle = parameterRef](double v) { return parameterHandle->valueToString(static_cast<float>(v)); };
            value.valueFromTextFunction = [parameterHandle = parameterRef](const juce::String& s) { return static_cast<double>(parameterHandle->stringToValue(s)); };
            value.setPopupDisplayEnabled(true, true, this);
            value.onDragStart = [parameterHandle = parameterRef] { parameterHandle->parameterChangeGestureBegin(); };
            value.onDragEnd = [parameterHandle = parameterRef] { parameterHandle->parameterChangeGestureEnd(); };
            value.onValueChange = [this]
            {
                parameterRef->setParameter(static_cast<float>(value.getValue()), juce::sendNotification);
                label.setTooltip(parameterRef->getCurrentValueAsStringWithLabel());
                if (changed) changed();
            };
            value.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(106, 210, 236));
            value.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(44, 78, 100));
            value.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(242, 249, 252));
            value.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(33, 56, 71));
            value.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(97, 146, 170));
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(10, 3);
            label.setBounds(area.removeFromLeft(190));
            value.setBounds(area);
        }

    private:
        te::AutomatableParameter::Ptr parameterRef;
        std::function<void()> changed;
        juce::Label label;
        juce::Slider value;
    };

    class ParameterList final : public juce::Component
    {
    public:
        void setParameters(te::Plugin::Ptr plugin, std::function<void()> changed)
        {
            rows.clear();
            pluginRef = std::move(plugin);
            if (pluginRef != nullptr)
                for (int i = 0; i < pluginRef->getNumAutomatableParameters(); ++i)
                {
                    auto row = std::make_unique<ParameterRow>(pluginRef->getAutomatableParameter(i), changed);
                    addAndMakeVisible(*row);
                    rows.push_back(std::move(row));
                }
            setSize(500, juce::jmax(80, static_cast<int>(rows.size()) * 42 + 16));
            resized();
        }

        void clearParameters()
        {
            rows.clear();
            pluginRef = nullptr;
            setSize(500, 80);
        }

        void resized() override
        {
            auto y = 8;
            for (auto& row : rows)
            {
                row->setBounds(0, y, getWidth(), 38);
                y += 42;
            }
        }

        void paint(juce::Graphics& g) override { g.fillAll(juce::Colour::fromRGB(27, 43, 55)); }

    private:
        te::Plugin::Ptr pluginRef;
        std::vector<std::unique_ptr<ParameterRow>> rows;
    };

public:
    PluginParameterWindow()
        : DocumentWindow("Orbit Effect", juce::Colour::fromRGB(25, 44, 58), juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentNonOwned(&viewport, false);
        viewport.setViewedComponent(&parameters, false);
        setResizable(true, true);
        setVisible(false);
    }

    void showPlugin(te::Plugin::Ptr plugin, std::function<void()> onChanged)
    {
        if (plugin == nullptr) return;
        parameters.setParameters(plugin, std::move(onChanged));
        setName(plugin->getName());
        setName("Orbit - " + plugin->getName());
        viewport.setViewPosition(0, 0);
        centreWithSize(690, juce::jlimit(180, 640, parameters.getHeight() + 48));
        setVisible(true);
        toFront(true);
    }

    void clearPlugin()
    {
        setVisible(false);
        parameters.clearParameters();
    }

    void resized() override
    {
        DocumentWindow::resized();
        viewport.setBounds(getLocalBounds().reduced(8));
    }

    void closeButtonPressed() override { clearPlugin(); }

private:
    juce::Viewport viewport;
    ParameterList parameters;
};

class MixerWindow final : public juce::DocumentWindow
{
public:
    std::function<int()> getTrackCount;
    std::function<juce::String(int)> getTrackName;
    std::function<void(int, const juce::String&)> renameTrack;
    std::function<double(int)> getTrackGain;
    std::function<void(int, double)> setTrackGain;
    std::function<double(int)> getTrackPan;
    std::function<void(int, double)> setTrackPan;
    std::function<bool(int)> getTrackMuted;
    std::function<void(int, bool)> setTrackMuted;
    std::function<bool(int)> isGeneratorTrack;
    std::function<bool(int)> getTrackSolo;
    std::function<void(int, bool)> setTrackSolo;
    std::function<float(int, float)> getTrackEqResponse;
    std::function<float(int, int)> getTrackEqParameter;
    std::function<void(int, int, float)> setTrackEqParameter;
    std::function<void(int, int, float, float)> setTrackEqBand;
    std::function<bool(int)> getTrackEqEnabled;
    std::function<void(int, bool)> setTrackEqEnabled;
    std::function<void(int)> onOpenEq;
    std::function<juce::StringArray(int)> getInsertNames;
    std::function<bool(int, int)> getInsertEnabled;
    std::function<void(int, int)> toggleInsert;
    std::function<void(int, int)> editInsert;
    std::function<void(int, int)> removeInsert;
    std::function<void(int, int)> addInsert;
    std::function<double(int, int)> getTrackSendGain;
    std::function<void(int, int, double)> setTrackSendGain;
    std::function<bool(int)> hasFxReturn;
    std::function<void(int)> onOpenFxReturn;
    std::function<te::LevelMeterPlugin*(int)> getTrackMeter;
    std::function<te::EditPlaybackContext*()> getMasterMeterContext;
    std::function<double()> getMasterGain;
    std::function<void(double)> setMasterGain;
    std::function<double()> getMasterPan;
    std::function<void(double)> setMasterPan;
    std::function<bool()> getMasterMuted;
    std::function<void(bool)> setMasterMuted;
    MixerWindow()
        : DocumentWindow("Orbit Mixer", juce::Colour::fromRGB(34, 59, 78), juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentNonOwned(&content, false);
        content.setOpaque(true);
        content.addAndMakeVisible(title);
        title.setText("MIXER", juce::dontSendNotification);
        title.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(234, 246, 251));
        for (int bus = 0; bus < 4; ++bus)
        {
            auto& button = fxReturnButtons[static_cast<size_t>(bus)];
            content.addAndMakeVisible(button);
            button.setButtonText("FX " + juce::String(bus + 1));
            button.setTooltip("Open the shared FX " + juce::String(bus + 1) + " return effect");
            button.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(47, 91, 117));
            button.onClick = [this, bus] { if (onOpenFxReturn) onOpenFxReturn(bus); };
        }
        content.addAndMakeVisible(viewport);
        viewport.setViewedComponent(&strips, false);
        strips.setOpaque(false);
        strips.addAndMakeVisible(masterStrip);
        strips.setSize(640, 880);
        setResizable(true, true);
        effectWindow = std::make_unique<PluginParameterWindow>();
        // The mixer reference is a wide console with twelve channels and a
        // master strip visible together. Start wide enough to show that bank
        // on a typical desktop, while keeping the window resizable.
        centreWithSize(1600, 980);
        setVisible(false);
    }

    void refresh(int selectedTrack)
    {
        const auto count = getTrackCount ? getTrackCount() : 0;
        while (channelStrips.size() < count) channelStrips.add(new ChannelStrip());
        while (channelStrips.size() > count) channelStrips.removeLast();
        for (int i = 0; i < count; ++i)
        {
            auto* strip = channelStrips[i];
            if (strip->getParentComponent() != &strips) strips.addAndMakeVisible(strip);
            strip->setNameText(getTrackName ? getTrackName(i) : "Generator " + juce::String(i + 1));
            const bool generator = !isGeneratorTrack || isGeneratorTrack(i);
            strip->setNameEditable(generator);
            strip->setFooterText(juce::String(i + 1) + "  "
                                 + (getTrackName ? getTrackName(i) : "Generator " + juce::String(i + 1)));
            strip->setSelected(i == selectedTrack);
            strip->setMeterPlugin(getTrackMeter ? getTrackMeter(i) : nullptr);
            strip->setGainValue(getTrackGain ? getTrackGain(i) : 0.0);
            strip->setPanValue(getTrackPan ? getTrackPan(i) : 0.0);
            for (int bus = 0; bus < 4; ++bus)
                strip->setSendValue(bus, getTrackSendGain ? getTrackSendGain(i, bus) : -60.0);
            strip->setMuteState(getTrackMuted && getTrackMuted(i));
            strip->setSoloState(getTrackSolo && getTrackSolo(i));
            strip->setEqResponse([this, i](float frequency)
            {
                return getTrackEqResponse ? getTrackEqResponse(i, frequency) : 0.0f;
            });
            strip->setEqEnabled(getTrackEqEnabled && getTrackEqEnabled(i));
            strip->setEqBandCallbacks(
                [this, i](int band) { return getTrackEqParameter ? getTrackEqParameter(i, band * 3) : 0.0f; },
                [this, i](int band) { return getTrackEqParameter ? getTrackEqParameter(i, band * 3 + 1) : 0.0f; },
                [this, i](int band, float frequency, float gain)
                {
                    if (setTrackEqBand) setTrackEqBand(i, band, frequency, gain);
                });
            strip->getEqParameter = [this, i](int parameter)
            {
                return getTrackEqParameter ? getTrackEqParameter(i, parameter) : 0.0f;
            };
            strip->refreshEqParameterValues();
            strip->onEqParameter = [this, i](int parameter, float value)
            {
                if (setTrackEqParameter) setTrackEqParameter(i, parameter, value);
            };
            strip->onToggleEq = [this, i](bool enabled)
            {
                if (setTrackEqEnabled) setTrackEqEnabled(i, enabled);
            };
            strip->setMixControlsVisible(true);
            // setMixControlsVisible enables the channel's send controls too.
            // Apply track-kind visibility after it so FX return strips do not
            // expose dead send knobs that only accept generator-track indices.
            strip->setSendsVisible(!isGeneratorTrack || isGeneratorTrack(i));
            const auto insertNames = getInsertNames ? getInsertNames(i) : juce::StringArray{};
            for (int slot = 0; slot < 4; ++slot)
            {
                const auto name = slot < insertNames.size() ? insertNames[slot] : juce::String{};
                strip->setInsertRow(slot, name, name.isNotEmpty() && getInsertEnabled && getInsertEnabled(i, slot));
            }
            // Insert slots can become visible without changing the strip size.
            // Re-run layout so their buttons receive bounds immediately.
            strip->resized();
            strip->onGain = [this, i](double value) { if (setTrackGain) setTrackGain(i, value); };
            strip->onPan = [this, i](double value) { if (setTrackPan) setTrackPan(i, value); };
            strip->onSend = [this, i](int bus, double value)
            {
                if (setTrackSendGain) setTrackSendGain(i, bus, value);
            };
            strip->onMute = [this, i](bool value) { if (setTrackMuted) setTrackMuted(i, value); };
            strip->onSolo = [this, i](bool value) { if (setTrackSolo) setTrackSolo(i, value); };
            strip->onRename = [this, i](const juce::String& value)
            {
                if (renameTrack) renameTrack(i, value);
            };
            strip->onOpenEq = [this, i] { if (onOpenEq) onOpenEq(i); };
            strip->onToggleInsert = [this, i](int slot) { if (toggleInsert) toggleInsert(i, slot); };
            strip->onEditInsert = [this, i](int slot) { if (editInsert) editInsert(i, slot); };
            strip->onRemoveInsert = [this, i](int slot) { if (removeInsert) removeInsert(i, slot); };
            strip->onAddInsert = [this, i]
            {
                showInsertChooser([this, i](int kind)
                {
                    if (addInsert) addInsert(i, kind);
                });
            };
        }
        for (int bus = 0; bus < 4; ++bus)
            fxReturnButtons[static_cast<size_t>(bus)].setEnabled(hasFxReturn && hasFxReturn(bus));
        masterStrip.setNameText("MASTER");
        masterStrip.setFooterText("MASTER");
        masterStrip.setMasterMeterContextSupplier(getMasterMeterContext);
        masterStrip.setGainValue(getMasterGain ? getMasterGain() : 0.0);
        masterStrip.setPanValue(getMasterPan ? getMasterPan() : 0.0);
        masterStrip.setMuteState(getMasterMuted && getMasterMuted());
        masterStrip.setMixControlsVisible(false);
        masterStrip.setMasterControlsVisible(true);
        masterStrip.setSendsVisible(false);
        masterStrip.setEqResponse([this, count](float frequency)
        {
            return getTrackEqResponse ? getTrackEqResponse(count, frequency) : 0.0f;
        });
        masterStrip.setEqEnabled(getTrackEqEnabled && getTrackEqEnabled(count));
        masterStrip.setEqBandCallbacks(
            [this, count](int band) { return getTrackEqParameter ? getTrackEqParameter(count, band * 3) : 0.0f; },
            [this, count](int band) { return getTrackEqParameter ? getTrackEqParameter(count, band * 3 + 1) : 0.0f; },
            [this, count](int band, float frequency, float gain)
            {
                if (setTrackEqBand) setTrackEqBand(count, band, frequency, gain);
            });
        masterStrip.getEqParameter = [this, count](int parameter)
        {
            return getTrackEqParameter ? getTrackEqParameter(count, parameter) : 0.0f;
        };
        masterStrip.refreshEqParameterValues();
        masterStrip.onEqParameter = [this, count](int parameter, float value)
        {
            if (setTrackEqParameter) setTrackEqParameter(count, parameter, value);
        };
        masterStrip.onToggleEq = [this, count](bool enabled)
        {
            if (setTrackEqEnabled) setTrackEqEnabled(count, enabled);
        };
        masterStrip.onOpenEq = [this, count] { if (onOpenEq) onOpenEq(count); };
        const auto masterInserts = getInsertNames ? getInsertNames(count) : juce::StringArray{};
        for (int slot = 0; slot < 4; ++slot)
        {
            const auto insertName = slot < masterInserts.size() ? masterInserts[slot] : juce::String{};
            masterStrip.setInsertRow(slot, insertName, insertName.isNotEmpty()
                && getInsertEnabled && getInsertEnabled(count, slot));
        }
        masterStrip.resized();
        masterStrip.onToggleInsert = [this, count](int slot) { if (toggleInsert) toggleInsert(count, slot); };
        masterStrip.onEditInsert = [this, count](int slot) { if (editInsert) editInsert(count, slot); };
        masterStrip.onRemoveInsert = [this, count](int slot) { if (removeInsert) removeInsert(count, slot); };
        masterStrip.onAddInsert = [this, count]
        {
            showInsertChooser([this, count](int kind)
            {
                if (addInsert) addInsert(count, kind);
            });
        };
        masterStrip.onGain = [this](double value) { if (setMasterGain) setMasterGain(value); };
        masterStrip.onPan = [this](double value) { if (setMasterPan) setMasterPan(value); };
        masterStrip.onMute = [this](bool value) { if (setMasterMuted) setMasterMuted(value); };
        layoutStrips();
        content.repaint();
    }

    void resized() override
    {
        DocumentWindow::resized();
        auto area = content.getLocalBounds().reduced(18);
        auto header = area.removeFromTop(30);
        title.setBounds(header.removeFromLeft(150));
        header.removeFromLeft(12);
        for (auto& button : fxReturnButtons)
        {
            button.setBounds(header.removeFromLeft(54).reduced(2, 2));
            header.removeFromLeft(4);
        }
        area.removeFromTop(10);
        viewport.setBounds(area);
        layoutStrips();
    }

    void focusMaster() { masterStrip.gain.grabKeyboardFocus(); }
    void repaintEqGraphs()
    {
        for (auto* strip : channelStrips) strip->repaintEqGraph();
        masterStrip.repaintEqGraph();
    }
    void refreshEqControls()
    {
        for (auto* strip : channelStrips) strip->refreshEqParameterValues();
        masterStrip.refreshEqParameterValues();
    }
    void showEffectEditor(te::Plugin::Ptr plugin, std::function<void()> onChanged)
    {
        if (effectWindow != nullptr) effectWindow->showPlugin(std::move(plugin), std::move(onChanged));
    }
    void closeButtonPressed() override { setVisible(false); }
    void showInsertChooser(std::function<void(int)> onSelected)
    {
        auto* dialog = new juce::AlertWindow("Add Insert", "Choose an effect for this mixer strip.",
                                             juce::MessageBoxIconType::NoIcon, this);
        dialog->addButton("Compressor", 1);
        dialog->addButton("Reverb", 2);
        dialog->addButton("Delay", 3);
        dialog->addButton("Chorus", 4);
        dialog->addButton("Phaser", 5);
        dialog->addButton("Low-pass Filter", 6);
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        dialog->centreAroundComponent(this, 860, 270);
        dialog->enterModalState(true, juce::ModalCallbackFunction::create(
            [selectionCallback = std::move(onSelected)](int result)
            {
                if (result > 0 && selectionCallback) selectionCallback(result);
            }), true);
    }
    void detachMeters()
    {
        if (effectWindow != nullptr) effectWindow->clearPlugin();
        for (auto* strip : channelStrips) strip->detachMeter();
        masterStrip.detachMeter();
        masterStrip.setMasterMeterContextSupplier({});
    }

private:
    void layoutStrips()
    {
        // Expand each strip to use the available mixer width, up to a useful
        // console width. At higher track counts the 112 px minimum preserves
        // the current compact layout and the viewport scrolls horizontally.
        const int count = channelStrips.size();
        const int availableWidth = juce::jmax(0, viewport.getWidth() - 20);
        const int columns = count + 1; // generator strips plus Master
        const int columnStep = availableWidth > 0
            ? juce::jlimit(112, 138, availableWidth / juce::jmax(1, columns))
            : 112;
        const int columnWidth = columnStep - 6;
        const int stripHeight = juce::jmax(700, viewport.getHeight() - 20);

        for (int i = 0; i < count; ++i)
            channelStrips[i]->setBounds(10 + i * columnStep, 10, columnWidth, stripHeight);
        masterStrip.setBounds(10 + count * columnStep, 10, columnWidth, stripHeight);
        strips.setSize(20 + columns * columnStep, stripHeight + 20);
    }

    class SmallActionButton final : public juce::TextButton
    {
    public:
        void setDrawsBypassIndicator(bool shouldDrawBypass) { drawsBypass = shouldDrawBypass; repaint(); }
        void setActive(bool active) { isActive = active; repaint(); }

    protected:
        void paintButton(juce::Graphics& g, bool highlighted, bool down) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            auto background = findColour(juce::TextButton::buttonColourId);
            if (highlighted) background = background.brighter(0.16f);
            if (down) background = background.darker(0.12f);
            g.setColour(background);
            g.fillRoundedRectangle(bounds, 3.0f);
            g.setColour(juce::Colour::fromRGB(116, 157, 178));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            const auto centre = bounds.getCentre();
            if (drawsBypass)
            {
                const auto radius = juce::jmin(3.8f, bounds.getHeight() * 0.22f);
                g.setColour(isActive ? juce::Colour::fromRGB(72, 207, 229)
                                     : juce::Colour::fromRGB(34, 47, 56));
                g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
                if (isActive)
                {
                    g.setColour(juce::Colour::fromRGB(183, 242, 250).withAlpha(0.72f));
                    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 0.8f);
                }
            }
            else
            {
                g.setColour(juce::Colour::fromRGB(232, 244, 249));
                g.drawLine(centre.x - 3.0f, centre.y - 3.0f,
                           centre.x + 3.0f, centre.y + 3.0f, 1.2f);
                g.drawLine(centre.x + 3.0f, centre.y - 3.0f,
                           centre.x - 3.0f, centre.y + 3.0f, 1.2f);
            }
        }

    private:
        bool drawsBypass = false;
        bool isActive = false;
    };

    class EqHeaderButton final : public juce::TextButton
    {
    public:
        void setEnabledIndicator(bool enabled) { isEnabled = enabled; repaint(); }

    protected:
        void paintButton(juce::Graphics& g, bool highlighted, bool down) override
        {
            juce::TextButton::paintButton(g, highlighted, down);
            const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            const auto centreY = bounds.getCentreY();
            const auto radius = juce::jmin(3.5f, bounds.getHeight() * 0.18f);
            g.setColour(isEnabled ? juce::Colour::fromRGB(72, 207, 229)
                                  : juce::Colour::fromRGB(34, 47, 56));
            g.fillEllipse(bounds.getX() + 7.0f, centreY - radius, radius * 2.0f, radius * 2.0f);
            if (isEnabled)
            {
                g.setColour(juce::Colour::fromRGB(183, 242, 250).withAlpha(0.72f));
                g.drawEllipse(bounds.getX() + 7.0f, centreY - radius, radius * 2.0f, radius * 2.0f, 0.8f);
            }
            g.setColour(juce::Colour::fromRGB(225, 240, 248));
            g.drawLine(bounds.getRight() - 11.0f, centreY - 2.0f,
                       bounds.getRight() - 8.0f, centreY + 1.0f, 1.1f);
            g.drawLine(bounds.getRight() - 8.0f, centreY + 1.0f,
                       bounds.getRight() - 5.0f, centreY - 2.0f, 1.1f);
        }

    private:
        bool isEnabled = false;
    };

    class ChannelStrip final : public juce::Component, private juce::Timer
    {
    public:
        ChannelStrip()
        {
            startTimerHz(20);
            addAndMakeVisible(name);
            name.setJustificationType(juce::Justification::centred);
            name.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            name.setColour(juce::Label::textColourId, juce::Colour::fromRGB(244, 250, 253));
            name.setEditable(false, true, false);
            name.onTextChange = [this]
            {
                const auto cleaned = name.getText().trim();
                if (cleaned.isNotEmpty() && onRename) onRename(cleaned);
            };
            addAndMakeVisible(footer);
            footer.setJustificationType(juce::Justification::centred);
            footer.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            footer.setColour(juce::Label::textColourId, juce::Colour::fromRGB(242, 249, 252));
            footer.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(29, 69, 94));
            footer.setColour(juce::Label::outlineColourId, juce::Colour::fromRGB(106, 164, 193));
            addAndMakeVisible(mute);
            addAndMakeVisible(solo);
            addAndMakeVisible(eqButton);
            addAndMakeVisible(eqGraph);
            eqGraph.onClick = [this] { if (onOpenEq) onOpenEq(); };
            addAndMakeVisible(insertsTitle);
            insertsTitle.setText("INSERTS", juce::dontSendNotification);
            insertsTitle.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            insertsTitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 240, 248));
            addAndMakeVisible(sendsTitle);
            sendsTitle.setText("SENDS", juce::dontSendNotification);
            sendsTitle.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            sendsTitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 240, 248));
            eqButton.setButtonText("EQ");
            eqButton.setTitle("Toggle equaliser");
            eqButton.setClickingTogglesState(true);
            eqButton.setTooltip("Enable or bypass the Tracktion equaliser. Click the graph to open its full controls.");
            eqButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(53, 103, 130));
            eqButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(43, 118, 145));
            eqButton.onClick = [this] { if (onToggleEq) onToggleEq(eqButton.getToggleState()); };
            static const std::array<const char*, 4> bandNames { "HIGH SHELF", "HIGH MID", "LOW MID", "LOW SHELF" };
            static const std::array<const char*, 3> parameterNames { "Gain", "Frequency", "Q" };
            static const std::array<const char*, 3> compactParameterNames { "Gain", "Freq", "Q" };
            for (int band = 0; band < 4; ++band)
            {
                auto& bandTitle = eqBandTitles[static_cast<size_t>(band)];
                bandTitle.setText(bandNames[static_cast<size_t>(band)], juce::dontSendNotification);
                bandTitle.setFont(juce::FontOptions(9.0f, juce::Font::bold));
                bandTitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 240, 248));
                bandTitle.setJustificationType(juce::Justification::centredLeft);
                addAndMakeVisible(bandTitle);
                for (int parameter = 0; parameter < 3; ++parameter)
                {
                    auto& control = eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)];
                    addAndMakeVisible(control);
                    control.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                    control.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                    control.setPopupDisplayEnabled(true, true, this);
                    control.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(186, 225, 242));
                    control.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(62, 169, 205));
                    control.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(34, 58, 74));
                    if (parameter == 0)
                    {
                        control.setRange(-20.0, 20.0, 0.1);
                        control.setTextValueSuffix(" dB");
                    }
                    else if (parameter == 1)
                    {
                        control.setRange(20.0, 20000.0, 1.0);
                        control.setSkewFactorFromMidPoint(1000.0);
                        control.setTextValueSuffix(" Hz");
                    }
                    else
                    {
                        control.setRange(0.1, 4.0, 0.01);
                    }
                    control.setTooltip(juce::String(bandNames[static_cast<size_t>(band)]) + " "
                                       + parameterNames[static_cast<size_t>(parameter)]
                                       + " — drag to adjust; the value appears while you move the knob");
                    control.onValueChange = [this, band, parameter]
                    {
                        if (onEqParameter)
                        {
                            // The compact strip shows Gain, Frequency, Q while
                            // Tracktion stores each band's values as Frequency,
                            // Gain, Q. Translate the UI order to the engine order.
                            const auto engineKind = parameter == 0 ? 1 : parameter == 1 ? 0 : 2;
                            onEqParameter((3 - band) * 3 + engineKind,
                                static_cast<float>(eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].getValue()));
                        }
                    };

                    auto& controlLabel = eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)];
                    controlLabel.setText(compactParameterNames[static_cast<size_t>(parameter)], juce::dontSendNotification);
                    controlLabel.setFont(juce::FontOptions(8.5f));
                    controlLabel.setJustificationType(juce::Justification::centred);
                    controlLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 227, 237));
                    addAndMakeVisible(controlLabel);
                }
            }
            for (int slot = 0; slot < 4; ++slot)
            {
                auto& insert = insertButtons[static_cast<size_t>(slot)];
                auto& edit = editInsertButtons[static_cast<size_t>(slot)];
                auto& bypass = bypassInsertButtons[static_cast<size_t>(slot)];
                auto& remove = removeInsertButtons[static_cast<size_t>(slot)];
                addAndMakeVisible(insert);
                addAndMakeVisible(edit);
                addAndMakeVisible(bypass);
                addAndMakeVisible(remove);
                bypass.setDrawsBypassIndicator(true);
                insert.setClickingTogglesState(false);
                insert.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(47, 91, 117));
                remove.setButtonText("x");
                remove.setTitle("Remove insert");
                remove.setTooltip("Remove this effect insert");
                remove.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(63, 86, 101));
                insert.onClick = [this, slot] { if (onEditInsert) onEditInsert(slot); };
                edit.setButtonText("E");
                edit.setTooltip("Open effect controls");
                edit.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(44, 80, 101));
                edit.onClick = [this, slot] { if (onEditInsert) onEditInsert(slot); };
                bypass.setButtonText("B");
                bypass.setTitle("Bypass insert");
                bypass.setTooltip("Bypass or enable this insert");
                bypass.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(44, 80, 101));
                bypass.onClick = [this, slot] { if (onToggleInsert) onToggleInsert(slot); };
                remove.onClick = [this, slot] { if (onRemoveInsert) onRemoveInsert(slot); };
                insert.setVisible(false);
                edit.setVisible(false);
                bypass.setVisible(false);
                remove.setVisible(false);
            }
            addAndMakeVisible(addInsertButton);
            addInsertButton.setButtonText("+ Insert");
            addInsertButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(44, 80, 101));
            addInsertButton.onClick = [this] { if (onAddInsert) onAddInsert(); };
            mute.setButtonText("M");
            solo.setButtonText("S");
            mute.setClickingTogglesState(true);
            solo.setClickingTogglesState(true);
            mute.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(183, 87, 71));
            solo.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(193, 153, 71));
            mute.onClick = [this] { if (onMute) onMute(mute.getToggleState()); };
            solo.onClick = [this] { if (onSolo) onSolo(solo.getToggleState()); };
            addAndMakeVisible(pan);
            for (auto* label : { &panLeft, &panTitle, &panRight })
            {
                addAndMakeVisible(*label);
                label->setFont(juce::FontOptions(9.0f, juce::Font::bold));
                label->setColour(juce::Label::textColourId, juce::Colour::fromRGB(215, 234, 243));
                label->setJustificationType(juce::Justification::centred);
            }
            panLeft.setText("L", juce::dontSendNotification);
            panTitle.setText("Pan", juce::dontSendNotification);
            panRight.setText("R", juce::dontSendNotification);
            pan.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            pan.setRange(-1.0, 1.0, 0.01);
            pan.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            pan.setTextValueSuffix(" Pan");
            pan.setPopupDisplayEnabled(true, true, this);
            pan.onValueChange = [this] { if (onPan) onPan(pan.getValue()); };
            for (int bus = 0; bus < 4; ++bus)
            {
                auto& label = sendNames[static_cast<size_t>(bus)];
                auto& send = sends[static_cast<size_t>(bus)];
                addAndMakeVisible(label);
                label.setText("FX " + juce::String(bus + 1), juce::dontSendNotification);
                label.setJustificationType(juce::Justification::centred);
                label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
                label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 240, 248));
                addAndMakeVisible(send);
                send.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                send.setRange(-60.0, 6.0, 0.1);
                send.setValue(-60.0, juce::dontSendNotification);
                send.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                send.setTextValueSuffix(" dB");
                send.setPopupDisplayEnabled(true, true, this);
                send.setTooltip("FX " + juce::String(bus + 1) + " post-fader send level");
                send.onValueChange = [this, bus] { if (onSend) onSend(bus, sends[static_cast<size_t>(bus)].getValue()); };
                send.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(186, 225, 242));
                send.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(62, 169, 205));
                send.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(34, 58, 74));
                send.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(242, 249, 252));
                send.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(43, 73, 94));
                send.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(125, 169, 193));
            }
            addAndMakeVisible(gain);
            gain.setSliderStyle(juce::Slider::LinearVertical);
            gain.setRange(-60.0, 6.0, 0.1);
            gain.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            gain.setTextValueSuffix(" dB");
            gain.setPopupDisplayEnabled(true, true, this);
            gain.onValueChange = [this] { if (onGain) onGain(gain.getValue()); };
            for (auto* slider : { &gain, &pan })
            {
                slider->setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(186, 225, 242));
                slider->setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(44, 78, 100));
                slider->setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(242, 249, 252));
                slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(43, 73, 94));
                slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(125, 169, 193));
            }
        }

        ~ChannelStrip() override
        {
            stopTimer();
            detachMeter();
        }

        std::function<void(double)> onGain, onPan;
        std::function<void(const juce::String&)> onRename;
        std::function<void(int, double)> onSend;
        std::function<void(bool)> onMute, onSolo, onToggleEq;
        std::function<void()> onOpenEq;
        std::function<void(int, float)> onEqParameter;
        std::function<float(int)> getEqParameter;
        std::function<void(int)> onEditInsert;
        std::function<void(int)> onToggleInsert, onRemoveInsert;
        std::function<void()> onAddInsert;
        juce::Slider gain;
        juce::Label footer;
        juce::Label insertsTitle;
        juce::Label sendsTitle;
        std::array<juce::Label, 4> eqBandTitles;
        std::array<std::array<juce::Slider, 3>, 4> eqControls;
        std::array<juce::Label, 4> sendNames;
        std::array<juce::Slider, 4> sends;
        void setNameText(const juce::String& text) { name.setText(text, juce::dontSendNotification); }
        void setNameEditable(bool editable)
        {
            name.setEditable(false, editable, false);
            name.setTooltip(editable ? "Double-click to rename this channel" : juce::String{});
        }
        void setFooterText(const juce::String& text) { footer.setText(text, juce::dontSendNotification); }
        void setGainValue(double value) { gain.setValue(value, juce::dontSendNotification); }
        void setPanValue(double value) { pan.setValue(value, juce::dontSendNotification); }
        void refreshEqParameterValues()
        {
            if (!getEqParameter) return;
            for (int band = 0; band < 4; ++band)
                for (int parameter = 0; parameter < 3; ++parameter)
                    eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setValue(
                        getEqParameter((3 - band) * 3 + (parameter == 0 ? 1 : parameter == 1 ? 0 : 2)),
                        juce::dontSendNotification);
        }
        void setSendValue(int bus, double value)
        {
            if (juce::isPositiveAndBelow(bus, 4))
                sends[static_cast<size_t>(bus)].setValue(value, juce::dontSendNotification);
        }
        void setMuteState(bool value) { mute.setToggleState(value, juce::dontSendNotification); }
        void setSoloState(bool value) { solo.setToggleState(value, juce::dontSendNotification); }
        void setEqEnabled(bool value)
        {
            eqButton.setToggleState(value, juce::dontSendNotification);
            eqButton.setEnabledIndicator(value);
            eqButton.setTooltip(value
                ? "Equaliser enabled — click to bypass; click the graph to open full controls."
                : "Equaliser bypassed — click to enable; click the graph to open full controls.");
        }
        void setPanVisible(bool shouldShow) { pan.setVisible(shouldShow); }
        void setMixControlsVisible(bool shouldShow)
        {
            mute.setVisible(shouldShow);
            solo.setVisible(shouldShow);
            sendsTitle.setVisible(shouldShow);
            for (int bus = 0; bus < 4; ++bus)
            {
                sendNames[static_cast<size_t>(bus)].setVisible(shouldShow);
                sends[static_cast<size_t>(bus)].setVisible(shouldShow);
            }
            eqButton.setVisible(shouldShow);
            eqGraph.setVisible(shouldShow);
            for (int band = 0; band < 4; ++band)
            {
                eqBandTitles[static_cast<size_t>(band)].setVisible(shouldShow);
                for (int parameter = 0; parameter < 3; ++parameter)
                {
                    eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(shouldShow);
                    eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(shouldShow);
                }
            }
            insertsTitle.setVisible(shouldShow);
            addInsertButton.setVisible(shouldShow);
            if (!shouldShow)
                for (int slot = 0; slot < 4; ++slot)
                {
                    insertButtons[static_cast<size_t>(slot)].setVisible(false);
                    editInsertButtons[static_cast<size_t>(slot)].setVisible(false);
                    bypassInsertButtons[static_cast<size_t>(slot)].setVisible(false);
                    removeInsertButtons[static_cast<size_t>(slot)].setVisible(false);
                }
        }
        void setMasterControlsVisible(bool shouldShow)
        {
            masterEqView = shouldShow;
            mute.setVisible(shouldShow);
            mute.setEnabled(shouldShow);
            solo.setVisible(shouldShow);
            solo.setEnabled(false);
            solo.setTooltip("The master output cannot be soloed");
            eqButton.setVisible(shouldShow);
            eqGraph.setVisible(shouldShow);
            for (int band = 0; band < 4; ++band)
            {
                eqBandTitles[static_cast<size_t>(band)].setVisible(false);
                for (int parameter = 0; parameter < 3; ++parameter)
                {
                    eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(false);
                    eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(false);
                }
            }
            insertsTitle.setVisible(shouldShow);
            addInsertButton.setVisible(shouldShow);
            resized();
        }
        void setSendsVisible(bool shouldShow)
        {
            sendsTitle.setVisible(shouldShow);
            for (int bus = 0; bus < 4; ++bus)
            {
                sendNames[static_cast<size_t>(bus)].setVisible(shouldShow);
                sends[static_cast<size_t>(bus)].setVisible(shouldShow);
            }
        }
        void setEqResponse(std::function<float(float)> callback)
        {
            eqGraph.responseAtFrequency = std::move(callback);
            eqGraph.repaint();
        }
        void setEqBandCallbacks(std::function<float(int)> frequency,
                                std::function<float(int)> gainValue,
                                std::function<void(int, float, float)> setBand)
        {
            eqGraph.bandFrequency = std::move(frequency);
            eqGraph.bandGain = std::move(gainValue);
            eqGraph.setBand = std::move(setBand);
            eqGraph.repaint();
        }
        void repaintEqGraph() { eqGraph.repaint(); }
        void setMeterPlugin(te::LevelMeterPlugin* plugin)
        {
            if (meterPlugin == plugin) return;
            detachMeter();
            if (plugin == nullptr) return;
            meterPluginRef = plugin;
            meterPlugin = plugin;
            meterPlugin->measurer.addClient(meterClient);
            meterVisible = true;
        }
        void setMasterMeterContextSupplier(std::function<te::EditPlaybackContext*()> supplier)
        {
            masterMeterContextSupplier = std::move(supplier);
            isMasterMeter = static_cast<bool>(masterMeterContextSupplier);
        }
        void detachMeter()
        {
            if (meterPlugin != nullptr)
                meterPlugin->measurer.removeClient(meterClient);
            meterPlugin = nullptr;
            meterPluginRef = nullptr;
            if (auto* context = masterMeterContext.get())
                context->masterLevels.removeClient(meterClient);
            masterMeterContext = {};
            leftPeakDb = rightPeakDb = -100.0f;
            leftPeakHoldDb = rightPeakHoldDb = -100.0f;
            leftPeakHoldTicks = rightPeakHoldTicks = clipHoldTicks = 0;
            meterVisible = false;
        }
        void setInsertRow(int slot, const juce::String& insertName, bool enabled)
        {
            if (!juce::isPositiveAndBelow(slot, 4)) return;
            auto& insert = insertButtons[static_cast<size_t>(slot)];
            auto& edit = editInsertButtons[static_cast<size_t>(slot)];
            auto& bypass = bypassInsertButtons[static_cast<size_t>(slot)];
            auto& remove = removeInsertButtons[static_cast<size_t>(slot)];
            const bool visible = insertName.isNotEmpty();
            insert.setVisible(visible);
            remove.setVisible(visible);
            // The effect-name button already opens its editor. Hiding the
            // duplicate E control gives the name enough room to read at the
            // reference mixer’s narrow channel width.
            edit.setVisible(false);
            bypass.setVisible(visible);
            insert.setButtonText(insertName);
            insert.setColour(juce::TextButton::buttonColourId,
                             enabled ? juce::Colour::fromRGB(47, 91, 117) : juce::Colour::fromRGB(55, 69, 78));
            insert.setTooltip("Open " + insertName + " controls");
            bypass.setButtonText("B");
            bypass.setTooltip(enabled ? "Bypass this effect" : "Enable this effect");
            bypass.setActive(enabled);
            bypass.setColour(juce::TextButton::buttonColourId,
                             enabled ? juce::Colour::fromRGB(44, 80, 101) : juce::Colour::fromRGB(55, 69, 78));
        }
        void setSelected(bool selected) { isSelected = selected; repaint(); }

        void paint(juce::Graphics& g) override
        {
            auto box = getLocalBounds().toFloat().reduced(1.0f);
            const auto base = isSelected ? juce::Colour::fromRGB(91, 143, 171) : juce::Colour::fromRGB(71, 117, 146);
            g.setGradientFill(juce::ColourGradient(base.brighter(0.12f), 0.0f, 0.0f,
                                                   base.darker(0.16f), 0.0f, getHeight(), false));
            g.fillRoundedRectangle(box, 5.0f);
            g.setColour(juce::Colour::fromRGB(161, 197, 216));
            g.drawRoundedRectangle(box, 5.0f, 1.0f);
            g.setColour(juce::Colour::fromRGB(43, 81, 105));
            g.fillRect(5.0f, 35.0f, box.getWidth() - 10.0f, 1.0f);
            paintMeter(g);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(6, 6);
            footer.setBounds(area.removeFromBottom(25));
            area.removeFromBottom(4);
            name.setBounds(area.removeFromTop(28));
            area.removeFromTop(3);
            if (sends[0].isVisible())
            {
                // Four compact send controls in the 2 x 2 arrangement used by
                // the reference mixer, ahead of inserts and EQ.
                sendsTitle.setBounds(area.removeFromTop(16));
                auto sendArea = area.removeFromTop(98);
                for (int bus = 0; bus < 4; ++bus)
                {
                    const auto row = bus / 2;
                    const auto column = bus % 2;
                    const auto cellX = area.getX() + column * (area.getWidth() / 2);
                    auto cell = juce::Rectangle<int>(cellX, sendArea.getY() + row * 49,
                                                     area.getWidth() / 2, 49).reduced(1, 0);
                    sendNames[static_cast<size_t>(bus)].setBounds(cell.removeFromTop(14));
                    sends[static_cast<size_t>(bus)].setBounds(cell.reduced(4, 0));
                }
                area.removeFromTop(2);
            }
            if (eqButton.isVisible())
            {
                insertsTitle.setBounds(area.removeFromTop(16));
                for (int slot = 0; slot < 4; ++slot)
                {
                    // Keep the insert section compact. A mixer strip with no
                    // effects should move straight from its + Insert control
                    // into the EQ, instead of reserving four empty effect
                    // rows and leaving a large dead gap in the channel.
                    if (!insertButtons[static_cast<size_t>(slot)].isVisible())
                    {
                        insertButtons[static_cast<size_t>(slot)].setBounds({});
                        editInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        bypassInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        removeInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        continue;
                    }
                    auto row = area.removeFromTop(23);
                    auto removeArea = row.removeFromRight(20);
                    auto bypassArea = row.removeFromRight(22);
                    row.removeFromRight(2);
                    insertButtons[static_cast<size_t>(slot)].setBounds(row);
                    editInsertButtons[static_cast<size_t>(slot)].setBounds({});
                    bypassInsertButtons[static_cast<size_t>(slot)].setBounds(bypassArea);
                    removeInsertButtons[static_cast<size_t>(slot)].setBounds(removeArea);
                    area.removeFromTop(1);
                }
                addInsertButton.setBounds(area.removeFromTop(22));
                area.removeFromTop(4);
                eqButton.setBounds(area.removeFromTop(22));
                area.removeFromTop(2);
                if (masterEqView)
                {
                    // The reference Master strip gives its EQ response graph
                    // a dedicated display area instead of repeating the four
                    // compact channel EQ control rows.
                    const auto graphHeight = juce::jmin(210, juce::jmax(110, area.getHeight() / 3));
                    eqGraph.setBounds(area.removeFromTop(graphHeight));
                    area.removeFromTop(3);
                }
                else
                {
                    eqGraph.setBounds(area.removeFromTop(42));
                    area.removeFromTop(3);
                    for (int band = 0; band < 4; ++band)
                    {
                        eqBandTitles[static_cast<size_t>(band)].setBounds(area.removeFromTop(12));
                        auto controls = area.removeFromTop(30);
                        const auto knobWidth = controls.getWidth() / 3;
                        for (int parameter = 0; parameter < 3; ++parameter)
                        {
                            auto knobCell = controls.removeFromLeft(knobWidth);
                            eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(knobCell.reduced(1, 0));
                        }
                        auto labels = area.removeFromTop(11);
                        const auto labelWidth = labels.getWidth() / 3;
                        for (int parameter = 0; parameter < 3; ++parameter)
                            eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(
                                labels.removeFromLeft(labelWidth));
                        area.removeFromTop(2);
                    }
                }
                area.removeFromTop(2);
            }
            if (mute.isVisible())
            {
                auto modeButtons = area.removeFromTop(26);
                mute.setBounds(modeButtons.removeFromLeft((modeButtons.getWidth() - 4) / 2));
                modeButtons.removeFromLeft(4);
                solo.setBounds(modeButtons);
                area.removeFromTop(2);
                auto panArea = area.removeFromTop(70);
                auto panLabelRow = panArea.removeFromTop(14);
                panLeft.setBounds(panLabelRow.removeFromLeft(18));
                panRight.setBounds(panLabelRow.removeFromRight(18));
                panTitle.setBounds(panLabelRow);
                const int panWidth = juce::jmin(64, panArea.getWidth());
                pan.setBounds(panArea.withSizeKeepingCentre(panWidth, panArea.getHeight()));
                area.removeFromTop(4);
            }
            auto meterArea = area.removeFromRight(34);
            area.removeFromRight(3);
            gain.setBounds(area);
            meterBounds = meterArea.reduced(2, 0);
        }


    private:
        void timerCallback() override
        {
            if (isMasterMeter && masterMeterContextSupplier)
            {
                auto* currentContext = masterMeterContextSupplier();
                if (masterMeterContext.get() != currentContext)
                {
                    detachMeter();
                    if (currentContext != nullptr)
                    {
                        masterMeterContext = currentContext;
                        currentContext->masterLevels.addClient(meterClient);
                        meterVisible = true;
                    }
                }
            }

            const auto left = meterClient.getAndClearAudioLevel(0).dB;
            const auto right = meterClient.getAndClearAudioLevel(1).dB;
            leftPeakDb = left > -100.0f ? juce::jmax(left, leftPeakDb - 1.1f) : juce::jmax(-100.0f, leftPeakDb - 1.1f);
            rightPeakDb = right > -100.0f ? juce::jmax(right, rightPeakDb - 1.1f) : juce::jmax(-100.0f, rightPeakDb - 1.1f);
            updatePeakHold(left, leftPeakHoldDb, leftPeakHoldTicks);
            updatePeakHold(right, rightPeakHoldDb, rightPeakHoldTicks);
            // The level measurer reports peaks in dBFS, where full scale is 0 dB.
            // Treat a sample that reaches 0 dBFS as a clip as well as any overs.
            if (left >= 0.0f || right >= 0.0f)
                clipHoldTicks = 40;
            else if (clipHoldTicks > 0)
                --clipHoldTicks;
            repaint(meterBounds.expanded(2));
        }

        static void updatePeakHold(float incomingDb, float& heldDb, int& holdTicks)
        {
            if (incomingDb > heldDb)
            {
                heldDb = incomingDb;
                holdTicks = 40;
            }
            else if (holdTicks > 0)
            {
                --holdTicks;
            }
            else
            {
                heldDb = juce::jmax(-100.0f, heldDb - 0.1f);
            }
        }

        void paintMeter(juce::Graphics& g)
        {
            if (!meterVisible || meterBounds.isEmpty()) return;
            g.setColour(juce::Colour::fromRGB(19, 33, 43));
            g.fillRoundedRectangle(meterBounds.toFloat(), 2.0f);
            const int gap = 2;
            constexpr int scaleWidth = 12;
            const int channelWidth = (meterBounds.getWidth() - scaleWidth - gap - 2) / 2;
            static constexpr std::array<int, 7> scaleValues { 0, -6, -12, -18, -24, -36, -60 };
            const auto meterTop = meterBounds.getY() + 2;
            const auto meterHeight = meterBounds.getHeight() - 4;
            g.setFont(7.5f);
            for (const auto db : scaleValues)
            {
                const auto level = static_cast<float>(db + 60) / 60.0f;
                const auto y = meterTop + juce::roundToInt((1.0f - level) * static_cast<float>(meterHeight));
                g.setColour(juce::Colour::fromRGB(194, 216, 225));
                g.drawFittedText(db == 0 ? "0" : juce::String(db),
                                 meterBounds.getX(), y - 4, scaleWidth - 2, 9,
                                 juce::Justification::centredRight, 1);
                g.setColour(juce::Colour::fromRGB(125, 158, 173));
                g.drawHorizontalLine(y, static_cast<float>(meterBounds.getX() + scaleWidth - 2),
                                     static_cast<float>(meterBounds.getX() + scaleWidth + 2));
            }
            const auto drawChannel = [&g, this, channelWidth](int x, float db, float heldDb)
            {
                const auto channel = juce::Rectangle<int>(x, meterBounds.getY() + 2,
                                                           channelWidth, meterBounds.getHeight() - 4);
                g.setColour(juce::Colour::fromRGB(31, 49, 60));
                g.fillRect(channel);
                constexpr int segmentCount = 30; // 2 dB per LED over the -60 to 0 dB range
                constexpr int segmentGap = 2;
                const int segmentHeight = juce::jmax(2,
                    (channel.getHeight() - (segmentCount - 1) * segmentGap) / segmentCount);
                for (int segment = 0; segment < segmentCount; ++segment)
                {
                    const float thresholdDb = -60.0f + (static_cast<float>(segment) + 1.0f)
                        * (60.0f / static_cast<float>(segmentCount));
                    if (db < thresholdDb) continue;

                    if (thresholdDb > -6.0f)
                        g.setColour(juce::Colour::fromRGB(227, 101, 81));
                    else if (thresholdDb > -18.0f)
                        g.setColour(juce::Colour::fromRGB(204, 231, 241));
                    else
                        g.setColour(juce::Colour::fromRGB(86, 203, 231));

                    const int y = channel.getBottom() - (segment + 1) * (segmentHeight + segmentGap)
                        + segmentGap;
                    g.fillRect(x, y, channelWidth, segmentHeight);
                }

                if (heldDb > -60.0f)
                {
                    const auto heldLevel = juce::jlimit(0.0f, 1.0f, (heldDb + 60.0f) / 60.0f);
                    const auto markerY = channel.getBottom()
                        - juce::roundToInt(heldLevel * static_cast<float>(channel.getHeight()));
                    g.setColour(juce::Colour::fromRGB(232, 247, 252));
                    g.fillRect(x, juce::jlimit(channel.getY(), channel.getBottom() - 2, markerY - 1),
                               channelWidth, 2);
                }
            };
            drawChannel(meterBounds.getX() + scaleWidth, leftPeakDb, leftPeakHoldDb);
            drawChannel(meterBounds.getX() + scaleWidth + channelWidth + gap, rightPeakDb, rightPeakHoldDb);
            if (clipHoldTicks > 0)
            {
                g.setColour(juce::Colour::fromRGB(235, 75, 61));
                g.fillRect(meterBounds.getX() + scaleWidth, meterBounds.getY() + 2,
                           meterBounds.getWidth() - scaleWidth - 2, 3);
            }
            g.setColour(juce::Colour::fromRGB(147, 185, 202));
            g.drawRoundedRectangle(meterBounds.toFloat(), 2.0f, 1.0f);
        }

        juce::Label name;
        juce::Slider pan;
        juce::TextButton mute, solo;
        EqHeaderButton eqButton;
        EqResponseGraph eqGraph;
        std::array<juce::TextButton, 4> insertButtons;
        std::array<juce::TextButton, 4> editInsertButtons;
        std::array<SmallActionButton, 4> bypassInsertButtons;
        std::array<SmallActionButton, 4> removeInsertButtons;
        juce::TextButton addInsertButton;
        te::LevelMeterPlugin* meterPlugin = nullptr;
        te::Plugin::Ptr meterPluginRef;
        te::LevelMeasurer::Client meterClient;
        juce::WeakReference<te::EditPlaybackContext> masterMeterContext;
        std::function<te::EditPlaybackContext*()> masterMeterContextSupplier;
        juce::Rectangle<int> meterBounds;
        juce::Label panLeft, panTitle, panRight;
        std::array<std::array<juce::Label, 3>, 4> eqControlLabels;
        float leftPeakDb = -100.0f, rightPeakDb = -100.0f;
        float leftPeakHoldDb = -100.0f, rightPeakHoldDb = -100.0f;
        int leftPeakHoldTicks = 0, rightPeakHoldTicks = 0, clipHoldTicks = 0;
        bool meterVisible = false, isMasterMeter = false;
        bool masterEqView = false;
        bool isSelected = false;
    };

    class MixerPanel final : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override { g.fillAll(juce::Colour::fromRGB(68, 112, 139)); }
    };

    MixerPanel content;
    juce::Label title;
    std::array<juce::TextButton, 4> fxReturnButtons;
    juce::Viewport viewport;
    juce::Component strips;
    juce::OwnedArray<ChannelStrip> channelStrips;
    ChannelStrip masterStrip;
    std::unique_ptr<PluginParameterWindow> effectWindow;
};

class PianoRollGrid final : public juce::Component
{
public:
    std::function<bool(int, int)> noteIsOn;
    std::function<void(int, int)> noteClicked;

    void paint(juce::Graphics& g) override
    {
        constexpr int labelWidth = 58;
        constexpr int headerHeight = 30;
        constexpr int rowHeight = 22;
        constexpr int stepWidth = 46;
        g.fillAll(juce::Colour::fromRGB(20, 29, 38));
        g.setColour(juce::Colour::fromRGB(29, 43, 55));
        g.fillRect(0, 0, getWidth(), headerHeight);
        g.setColour(juce::Colour::fromRGB(178, 198, 211));
        g.setFont(12.0f);
        for (int step = 0; step < 16; ++step)
            g.drawText(juce::String(step + 1), labelWidth + step * stepWidth, 0, stepWidth, headerHeight,
                       juce::Justification::centred);

        for (int pitch = 84; pitch >= 48; --pitch)
        {
            const auto row = 84 - pitch;
            const auto y = headerHeight + row * rowHeight;
            const bool blackKey = ((pitch % 12) == 1 || (pitch % 12) == 3 || (pitch % 12) == 6
                                   || (pitch % 12) == 8 || (pitch % 12) == 10);
            g.setColour(blackKey ? juce::Colour::fromRGB(24, 34, 43) : juce::Colour::fromRGB(32, 46, 58));
            g.fillRect(0, y, labelWidth, rowHeight);
            g.setColour(juce::Colour::fromRGB(178, 198, 211));
            g.drawText(pitchLabel(pitch), 4, y, labelWidth - 8, rowHeight, juce::Justification::centredRight);

            for (int step = 0; step < 16; ++step)
            {
                const auto x = labelWidth + step * stepWidth;
                const bool active = noteIsOn && noteIsOn(pitch, step);
                g.setColour(active ? juce::Colour::fromRGB(48, 158, 199)
                                   : (blackKey ? juce::Colour::fromRGB(27, 41, 52) : juce::Colour::fromRGB(36, 51, 63)));
                g.fillRect(x + 1, y + 1, stepWidth - 2, rowHeight - 2);
                g.setColour(step % 4 == 0 ? juce::Colour::fromRGB(82, 113, 130) : juce::Colour::fromRGB(53, 75, 90));
                g.drawRect(x, y, stepWidth, rowHeight, 1);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        constexpr int labelWidth = 58;
        constexpr int headerHeight = 30;
        constexpr int rowHeight = 22;
        constexpr int stepWidth = 46;
        const auto pitch = 84 - (event.y - headerHeight) / rowHeight;
        const auto step = (event.x - labelWidth) / stepWidth;
        if (event.x < labelWidth || event.y < headerHeight || pitch < 48 || pitch > 84
            || !juce::isPositiveAndBelow(step, 16))
            return;
        if (noteClicked) noteClicked(pitch, step);
    }

private:
    static juce::String pitchLabel(int pitch)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(names[pitch % 12]) + juce::String(pitch / 12 - 1);
    }
};

class PianoRollWindow final : public juce::DocumentWindow
{
public:
    PianoRollWindow(std::function<bool(int, int)> isNoteOn,
                    std::function<void(int, int)> onNoteClicked)
        : DocumentWindow("Orbit Piano Roll", juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        grid = new PianoRollGrid();
        grid->noteIsOn = std::move(isNoteOn);
        grid->noteClicked = std::move(onNoteClicked);
        auto* viewport = new juce::Viewport();
        viewport->setViewedComponent(grid, true);
        setContentOwned(viewport, true);
        setResizable(true, true);
        centreWithSize(850, 820);
        setVisible(false);
    }

    void refresh(const juce::String& patternName)
    {
        setName("Piano Roll - " + patternName);
        if (grid == nullptr) return;
        grid->setSize(58 + 16 * 46, 30 + 37 * 22);
        grid->repaint();
    }

    void closeButtonPressed() override { setVisible(false); }

private:
    PianoRollGrid* grid = nullptr;
};

class ProjectView final : public juce::Component, private juce::Timer
{
public:
    ProjectView() : engine("Orbit")
    {
        title.setText("Orbit", juce::dontSendNotification);
        title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
        title.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title);

        configureButton(newProjectButton, "New Project", [this] { createProject(); });
        configureButton(openButton, "Open", [this] { openProject(); });
        configureButton(saveButton, "Save", [this] { saveProject(); });
        configureButton(midiExportButton, "Export MIDI", [this] { exportMidi(); });
        configureButton(audioSettingsButton, "Audio Settings", [this] { showAudioSettings(); });
        configureButton(addTrackButton, "Add Instrument", [this] { addInstrumentTrack(); });
        configureButton(patternButton, "New Pattern", [this] { addPattern(); });
        configureButton(playlistButton, "Playlist", [this] { showPlaylist(); });
        configureButton(playButton, "Play", [this] { togglePlayback(); });
        configureButton(stopButton, "Stop", [this] { stopPlayback(); });
        configureButton(mixerButton, "Mixer", [this] { showMixer(); });
        configureButton(masterButton, "Master", [this] { showMixer(true); });
        for (auto* button : { &newProjectButton, &openButton, &saveButton, &midiExportButton, &audioSettingsButton, &addTrackButton,
                              &patternButton, &playlistButton, &playButton, &stopButton, &mixerButton, &masterButton })
            addAndMakeVisible(*button);

        tempoLabel.setText("Tempo", juce::dontSendNotification);
        tempoLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 207, 221));
        addAndMakeVisible(tempoLabel);
        tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        tempoSlider.setRange(20.0, 300.0, 0.1);
        tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 24);
        tempoSlider.setValue(120.0, juce::dontSendNotification);
        tempoSlider.onValueChange = [this]
        {
            if (edit == nullptr || !hasProject) return;
            const auto bpm = tempoSlider.getValue();
            if (auto* tempo = edit->tempoSequence.getTempo(0)) tempo->setBpm(bpm);
            project.setInitialTempoBpm(bpm);
            isDirty = true;
            musicalDetails.setText("Tracks: " + juce::String(project.getTrackCount()) + "   |   "
                                       + juce::String(bpm, 1) + " BPM   |   4/4   |   Built-in 4OSC   |   64 patterns per generator",
                                   juce::dontSendNotification);
            status.setText("Tempo changed. Save to keep the change.", juce::dontSendNotification);
        };
        tempoLabel.setEnabled(false);
        tempoSlider.setEnabled(false);
        addAndMakeVisible(tempoSlider);

        trackSelector.setTextWhenNothingSelected("Select generator");
        trackSelector.setTooltip("Choose the generator whose patterns, piano roll, and mixer settings you are editing.");
        trackSelector.setEnabled(false);
        trackSelector.onChange = [this] { selectInstrumentTrack(); };
        addAndMakeVisible(trackSelector);

        status.setText("Create a project, add an instrument, then add a pattern.", juce::dontSendNotification);
        status.setColour(juce::Label::textColourId, juce::Colour::fromRGB(170, 204, 222));
        addAndMakeVisible(status);
        projectDetails.setColour(juce::Label::textColourId, juce::Colours::white);
        musicalDetails.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 199, 210));
        audioDetails.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 199, 210));
        addAndMakeVisible(projectDetails);
        addAndMakeVisible(musicalDetails);
        addAndMakeVisible(audioDetails);

        generatorTitle.setText("Pattern Generator", juce::dontSendNotification);
        generatorTitle.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        generatorTitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(213, 229, 239));
        generatorTitle.setTitle("Selected instrument pattern editor");
        addAndMakeVisible(generatorTitle);

        patternTitle.setText("Select an instrument generator to edit patterns", juce::dontSendNotification);
        patternTitle.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(patternTitle);
        generatorTitle.setVisible(false);
        patternTitle.setVisible(false);

        pitchSelector.addItemList(makePitchNames(), 1);
        pitchSelector.setSelectedId(pitchToItemId(60), juce::dontSendNotification);
        pitchSelector.setTooltip("Choose a note pitch to edit in the 16-step pattern.");
        pitchSelector.onChange = [this] { loadSelectedPitchSteps(); };
        pitchSelector.setEnabled(false);
        addAndMakeVisible(pitchSelector);
        pitchSelector.setVisible(false);

        pianoRollButton.setButtonText("PR");
        pianoRollButton.setTooltip("Open the selected pattern in the piano roll");
        pianoRollButton.setTitle("Open piano roll");
        pianoRollButton.onClick = [this] { showPianoRoll(); };
        pianoRollButton.setEnabled(false);
        addAndMakeVisible(pianoRollButton);
        pianoRollButton.setVisible(false);

        for (int i = 0; i < 8; ++i)
        {
            patternBankButtons[static_cast<size_t>(i)] = std::make_unique<juce::TextButton>(juce::String::charToString(static_cast<juce::juce_wchar>('A' + i)));
            auto* bankButton = patternBankButtons[static_cast<size_t>(i)].get();
            bankButton->setClickingTogglesState(true);
            bankButton->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
            bankButton->setTooltip("Select pattern bank " + juce::String::charToString(static_cast<juce::juce_wchar>('A' + i)));
            bankButton->onClick = [this, i] { selectPatternBank(i); };
            bankButton->setEnabled(false);
            addAndMakeVisible(*bankButton);
            bankButton->setVisible(false);

            patternSlotButtons[static_cast<size_t>(i)] = std::make_unique<juce::TextButton>(juce::String(i + 1));
            auto* slotButton = patternSlotButtons[static_cast<size_t>(i)].get();
            slotButton->setClickingTogglesState(true);
            slotButton->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(55, 114, 131));
            slotButton->setTooltip("Select pattern slot " + juce::String(i + 1));
            slotButton->onClick = [this, i] { selectPatternSlot(i); };
            slotButton->setEnabled(false);
            addAndMakeVisible(*slotButton);
            slotButton->setVisible(false);
        }
        patternBankButtons[0]->setToggleState(true, juce::dontSendNotification);
        patternSlotButtons[0]->setToggleState(true, juce::dontSendNotification);

        playlistWindow = std::make_unique<PlaylistWindow>(
            [this] { return project.getTrackCount(); },
            [this](int row) { return project.getTrackName(row); },
            [this](int row, int bar) { return getPlaylistClipLabel(row, bar); },
            [this](int row, int bar) { togglePlaylistClip(row, bar); },
            [this](int row, int sourceBar, int destinationBar)
            {
                movePlaylistClip(row, sourceBar, destinationBar);
            });

        pianoRollWindow = std::make_unique<PianoRollWindow>(
            [this](int pitch, int step)
            {
                if (pitch < minimumPitch || pitch > maximumPitch || step < 0 || step >= 16) return false;
                return noteGrid[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)];
            },
            [this](int pitch, int step) { togglePianoRollNote(pitch, step); });

        mixerWindow = std::make_unique<MixerWindow>();
        mixerWindow->getTrackCount = [this] { return edit != nullptr ? getMixerTracks().size() : 0; };
        mixerWindow->getTrackName = [this](int index)
        {
            const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
            return juce::isPositiveAndBelow(index, tracks.size()) ? tracks.getUnchecked(index)->getName() : juce::String{};
        };
        mixerWindow->renameTrack = [this](int index, const juce::String& value)
        {
            const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
            const auto cleaned = value.trim();
            if (!juce::isPositiveAndBelow(index, tracks.size()) || cleaned.isEmpty()) return;
            auto* track = tracks.getUnchecked(index);
            if (track->getName() == cleaned && project.getTrackName(index) == cleaned) return;
            if (track->getName() != cleaned) track->setName(cleaned);
            project.setTrackName(index, cleaned);
            isDirty = true;
            refreshTrackSelector();
            refreshMixerControls();
            if (mixerWindow != nullptr) mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
            status.setText("Renamed mixer channel to " + cleaned + ". Save to keep the change.", juce::dontSendNotification);
        };
        mixerWindow->getTrackGain = [this](int index) { return getTrackGainDb(index); };
        mixerWindow->setTrackGain = [this](int index, double value) { setTrackGainDb(index, value); };
        mixerWindow->getTrackPan = [this](int index) { return getTrackPan(index); };
        mixerWindow->setTrackPan = [this](int index, double value) { setTrackPan(index, value); };
        mixerWindow->getTrackMuted = [this](int index) { return getTrackMute(index); };
        mixerWindow->setTrackMuted = [this](int index, bool value) { setTrackMute(index, value); };
        mixerWindow->isGeneratorTrack = [this](int index) { return index < getGeneratorTracks().size(); };
        mixerWindow->getTrackSolo = [this](int index) { return getTrackSolo(index); };
        mixerWindow->setTrackSolo = [this](int index, bool value) { setTrackSolo(index, value); };
        mixerWindow->getTrackEqResponse = [this](int index, float frequency) { return getTrackEqResponse(index, frequency); };
        mixerWindow->getTrackEqParameter = [this](int index, int parameter) { return getTrackEqParameter(index, parameter); };
        mixerWindow->setTrackEqParameter = [this](int index, int parameter, float value)
        {
            setTrackEqParameter(index, parameter, value);
        };
        mixerWindow->setTrackEqBand = [this](int index, int band, float frequency, float gain)
        {
            setTrackEqBand(index, band, frequency, gain);
        };
        mixerWindow->getTrackEqEnabled = [this](int index)
        {
            const auto* eq = getTrackEqualiser(index);
            return eq != nullptr && eq->isEnabled();
        };
        mixerWindow->setTrackEqEnabled = [this](int index, bool enabled)
        {
            setTrackEqEnabled(index, enabled);
        };
        mixerWindow->onOpenEq = [this](int index) { openTrackEqualiser(index); };
        mixerWindow->getInsertNames = [this](int index) { return getInsertNames(index); };
        mixerWindow->getInsertEnabled = [this](int index, int slot) { return getInsertEnabled(index, slot); };
        mixerWindow->toggleInsert = [this](int index, int slot) { toggleInsert(index, slot); };
        mixerWindow->editInsert = [this](int index, int slot) { openInsertEditor(index, slot); };
        mixerWindow->removeInsert = [this](int index, int slot) { removeInsert(index, slot); };
        mixerWindow->addInsert = [this](int index, int kind) { addInsert(index, kind); };
        mixerWindow->getTrackSendGain = [this](int index, int bus) { return getTrackSendGainDb(index, bus); };
        mixerWindow->setTrackSendGain = [this](int index, int bus, double value) { setTrackSendGainDb(index, bus, value); };
        mixerWindow->hasFxReturn = [this](int bus) { return getFxReturnTrack(bus) != nullptr; };
        mixerWindow->onOpenFxReturn = [this](int bus) { openFxReturnEditor(bus); };
        mixerWindow->getTrackMeter = [this](int index) -> te::LevelMeterPlugin*
        {
            const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
            return juce::isPositiveAndBelow(index, tracks.size()) ? tracks.getUnchecked(index)->getLevelMeterPlugin() : nullptr;
        };
        mixerWindow->getMasterMeterContext = [this]() -> te::EditPlaybackContext*
        {
            return edit != nullptr ? edit->getCurrentPlaybackContext() : nullptr;
        };
        mixerWindow->getMasterGain = [this] { return edit != nullptr && edit->getMasterVolumePlugin() != nullptr
                                                     ? getMasterGainDb() : 0.0; };
        mixerWindow->setMasterGain = [this](double value) { setMasterGainDb(value); };
        mixerWindow->getMasterPan = [this] { return edit != nullptr && edit->getMasterVolumePlugin() != nullptr
                                                   ? edit->getMasterVolumePlugin()->getPan() : 0.0; };
        mixerWindow->setMasterPan = [this](double value) { setMasterPan(value); };
        mixerWindow->getMasterMuted = [this] { return isMasterMuted(); };
        mixerWindow->setMasterMuted = [this](bool value) { setMasterMuted(value); };

        equaliserWindow = std::make_unique<EqualiserWindow>();
        equaliserWindow->getTrackName = [this](int index)
        {
            const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
            if (edit != nullptr && index == tracks.size()) return juce::String("Master");
            return juce::isPositiveAndBelow(index, tracks.size()) ? tracks.getUnchecked(index)->getName() : juce::String("Generator");
        };
        equaliserWindow->getParameter = [this](int index, int parameter) { return getTrackEqParameter(index, parameter); };
        equaliserWindow->getResponse = [this](int index, float frequency) { return getTrackEqResponse(index, frequency); };
        equaliserWindow->setParameter = [this](int index, int parameter, float value) { setTrackEqParameter(index, parameter, value); };
        equaliserWindow->setBandValues = [this](int index, int band, float frequency, float gain)
        {
            setTrackEqBand(index, band, frequency, gain);
        };

        for (int i = 0; i < 16; ++i)
        {
            steps[static_cast<size_t>(i)] = std::make_unique<juce::TextButton>(juce::String(i + 1));
            auto* step = steps[static_cast<size_t>(i)].get();
            step->setClickingTogglesState(true);
            step->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
            step->setEnabled(false);
            step->onClick = [this, i] { updatePatternStep(i, steps[static_cast<size_t>(i)]->getToggleState()); };
            addAndMakeVisible(*step);
            step->setVisible(false);
        }

        arrangementTitle.setText("Playlist window: click a timeline cell to place the selected pattern", juce::dontSendNotification);
        arrangementTitle.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(arrangementTitle);

        mixerTitle.setText("Mixer - Instrument 1", juce::dontSendNotification);
        mixerTitle.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(mixerTitle);
        levelLabel.setText("Level", juce::dontSendNotification);
        panLabel.setText("Pan", juce::dontSendNotification);
        levelLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 199, 210));
        panLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 199, 210));
        addAndMakeVisible(levelLabel);
        addAndMakeVisible(panLabel);
        levelSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        levelSlider.setRange(-60.0, 6.0, 0.1);
        levelSlider.setValue(0.0, juce::dontSendNotification);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
        levelSlider.setTextValueSuffix(" dB");
        levelSlider.onValueChange = [this]
        {
            if (instrumentTrack == nullptr) return;
            if (auto* volumePan = instrumentTrack->getVolumePlugin())
            {
                volumePan->setVolumeDb(static_cast<float>(levelSlider.getValue()));
                isDirty = true;
                status.setText("Track level changed. Save to keep the changes.", juce::dontSendNotification);
            }
        };
        panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0, juce::dontSendNotification);
        panSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
        panSlider.onValueChange = [this]
        {
            if (instrumentTrack == nullptr) return;
            if (auto* volumePan = instrumentTrack->getVolumePlugin())
            {
                volumePan->setPan(static_cast<float>(panSlider.getValue()));
                isDirty = true;
                status.setText("Track pan changed. Save to keep the changes.", juce::dontSendNotification);
            }
        };
        levelSlider.setEnabled(false);
        panSlider.setEnabled(false);
        addAndMakeVisible(levelSlider);
        addAndMakeVisible(panSlider);

        meterStatus.setText("Track output: no recent audio activity", juce::dontSendNotification);
        meterStatus.setColour(juce::Label::textColourId, juce::Colour::fromRGB(170, 204, 222));
        addAndMakeVisible(meterStatus);

        buildLabel.setText("Build 24  -  v0.3.3", juce::dontSendNotification);
        buildLabel.setJustificationType(juce::Justification::centredRight);
        buildLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(154, 181, 196));
        addAndMakeVisible(buildLabel);
        setSize(1080, 680);
        refreshProjectView("Create a project to begin.");
        updateAudioStatus();
        startTimerHz(2);
    }

    ~ProjectView() override
    {
        stopTimer();
        if (mixerWindow != nullptr) mixerWindow->detachMeters();
        detachTrackMeter();
        if (edit != nullptr)
            edit->getTransport().stop(false, false);
    }

    void timerCallback() override
    {
        updateAudioStatus();
        updateMeterStatus();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(20, 29, 38));
        if (!generatorPanelBounds.isEmpty())
        {
            g.setColour(juce::Colour::fromRGB(24, 37, 48));
            g.fillRoundedRectangle(generatorPanelBounds.toFloat(), 8.0f);
            g.setColour(juce::Colour::fromRGB(55, 87, 105));
            g.drawRoundedRectangle(generatorPanelBounds.toFloat(), 8.0f, 1.0f);
        }
        g.setColour(juce::Colour::fromRGB(55, 87, 105));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(18.0f), 12.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(36);
        title.setBounds(area.removeFromTop(52));
        area.removeFromTop(12);
        auto row = area.removeFromTop(34);
        newProjectButton.setBounds(row.removeFromLeft(100)); row.removeFromLeft(6);
        openButton.setBounds(row.removeFromLeft(68)); row.removeFromLeft(6);
        saveButton.setBounds(row.removeFromLeft(60)); row.removeFromLeft(6);
        addTrackButton.setBounds(row.removeFromLeft(112)); row.removeFromLeft(6);
        trackSelector.setBounds(row.removeFromLeft(130)); row.removeFromLeft(6);
        patternButton.setBounds(row.removeFromLeft(100)); row.removeFromLeft(6);
        midiExportButton.setBounds(row.removeFromLeft(90)); row.removeFromLeft(6);
        audioSettingsButton.setBounds(row.removeFromLeft(104));

        area.removeFromTop(6);
        auto transportRow = area.removeFromTop(34);
        tempoLabel.setBounds(transportRow.removeFromLeft(52));
        tempoSlider.setBounds(transportRow.removeFromLeft(154)); transportRow.removeFromLeft(12);
        stopButton.setBounds(transportRow.removeFromLeft(62)); transportRow.removeFromLeft(6);
        playButton.setBounds(transportRow.removeFromLeft(68)); transportRow.removeFromLeft(14);
        playlistButton.setBounds(transportRow.removeFromLeft(76)); transportRow.removeFromLeft(6);
        mixerButton.setBounds(transportRow.removeFromLeft(68)); transportRow.removeFromLeft(6);
        masterButton.setBounds(transportRow.removeFromLeft(74));

        area.removeFromTop(12);
        status.setBounds(area.removeFromTop(26));
        projectDetails.setBounds(area.removeFromTop(24));
        musicalDetails.setBounds(area.removeFromTop(24));
        audioDetails.setBounds(area.removeFromTop(24));
        generatorPanelBounds = {};
        if (instrumentTrack != nullptr)
        {
            auto panel = area;
            area.removeFromTop(8);
            generatorTitle.setBounds(area.removeFromTop(24));
            area.removeFromTop(3);
            patternTitle.setBounds(area.removeFromTop(22));
            area.removeFromTop(3);
            auto patternControls = area.removeFromTop(64);
            const auto bankX = patternControls.getX();
            for (int i = 0; i < 8; ++i)
            {
                const auto x = bankX + i * 42;
                patternBankButtons[static_cast<size_t>(i)]->setBounds(x, patternControls.getY(), 38, 28);
                patternSlotButtons[static_cast<size_t>(i)]->setBounds(x, patternControls.getY() + 32, 38, 28);
            }
            patternControls.removeFromLeft(8 * 42 + 14);
            pitchSelector.setBounds(patternControls.removeFromLeft(142).withHeight(30).withY(patternControls.getY() + 17));
            patternControls.removeFromLeft(8);
            pianoRollButton.setBounds(patternControls.removeFromLeft(38).withHeight(30).withY(patternControls.getY() + 17));
            area.removeFromTop(8);
            auto grid = area.removeFromTop(54);
            const auto gap = 5;
            const auto stepWidth = (grid.getWidth() - gap * 15) / 16;
            for (int i = 0; i < 16; ++i)
                steps[static_cast<size_t>(i)]->setBounds(grid.getX() + i * (stepWidth + gap), grid.getY(), stepWidth, grid.getHeight());
            generatorPanelBounds = panel.withBottom(grid.getBottom() + 8);
        }
        area.removeFromTop(12);
        arrangementTitle.setBounds(area.removeFromTop(24));
        area.removeFromTop(4);
        mixerTitle.setBounds(area.removeFromTop(24));
        area.removeFromTop(4);
        auto mixer = area.removeFromTop(30);
        levelLabel.setBounds(mixer.removeFromLeft(48));
        levelSlider.setBounds(mixer.removeFromLeft(370));
        mixer.removeFromLeft(24);
        panLabel.setBounds(mixer.removeFromLeft(34));
        panSlider.setBounds(mixer.removeFromLeft(250));
        area.removeFromTop(8);
        meterStatus.setBounds(area.removeFromTop(24));
        buildLabel.setBounds(getLocalBounds().reduced(36).removeFromBottom(22));
    }

private:
    static void configureButton(juce::TextButton& button, const juce::String& text, std::function<void()> action)
    {
        button.setButtonText(text);
        button.onClick = std::move(action);
    }

    static juce::StringArray makePitchNames()
    {
        juce::StringArray names;
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            names.add(pitchName(pitch));
        return names;
    }

    static juce::String pitchName(int midiNote)
    {
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(noteNames[midiNote % 12]) + juce::String(midiNote / 12 - 1);
    }

    static int pitchToItemId(int midiNote) { return midiNote - minimumPitch + 1; }
    int selectedPitch() const { return minimumPitch + pitchSelector.getSelectedId() - 1; }

    void loadSelectedPitchSteps()
    {
        const auto pitch = selectedPitch();
        for (int i = 0; i < 16; ++i)
            steps[static_cast<size_t>(i)]->setToggleState(noteGrid[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(i)],
                                                          juce::dontSendNotification);
        const auto patternLabel = juce::String::charToString(static_cast<juce::juce_wchar>('A' + selectedPatternIndex() / 8))
                                  + juce::String(selectedPatternIndex() % 8 + 1);
        const auto generatorName = instrumentTrack != nullptr ? instrumentTrack->getName() : juce::String("Generator");
        generatorTitle.setText("Pattern Generator  |  " + generatorName, juce::dontSendNotification);
        patternTitle.setText(generatorName + "  |  Pattern " + patternLabel + " - " + pitchName(pitch), juce::dontSendNotification);
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(patternLabel);
    }

    void showPianoRoll()
    {
        if (pianoRollWindow == nullptr || !hasPattern) return;
        pianoRollWindow->refresh(patternTitle.getText().upToFirstOccurrenceOf(" - ", false, false));
        pianoRollWindow->setVisible(true);
        pianoRollWindow->toFront(true);
    }

    void togglePianoRollNote(int pitch, int step)
    {
        if (!hasPattern || pitch < minimumPitch || pitch > maximumPitch || !juce::isPositiveAndBelow(step, 16)) return;
        const auto row = static_cast<size_t>(pitch - minimumPitch);
        const auto column = static_cast<size_t>(step);
        const bool enabled = !noteGrid[row][column];
        noteGrid[row][column] = enabled;
        project.setPatternNote(selectedProjectTrackId(), selectedPatternIndex(), pitch, step, enabled);
        if (pitch == selectedPitch())
            steps[column]->setToggleState(enabled, juce::dontSendNotification);
        isDirty = true;
        status.setText("Piano roll note edited. Save to keep the change.", juce::dontSendNotification);
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(patternTitle.getText().upToFirstOccurrenceOf(" - ", false, false));
        rebuildPatternClip();
    }

    void createProject()
    {
        if (hasProject && isDirty)
        {
            showError("Unsaved changes", "Save the current project before creating another.");
            return;
        }
        // Release references into the current Edit before replacing it. Tracktion
        // owns the tracks, and releasing an AudioTrack after its Edit is gone is unsafe.
        resetPatternButtons();
        project = ProjectDocument::createNew();
        edit = te::loadEditFromState(engine, te::createEmptyEdit(engine));
        hasProject = edit != nullptr;
        isDirty = true;
        refreshProjectView(hasProject ? "New project created. Add an instrument and a pattern." : "Tracktion could not create an edit.");
    }

    void openProject()
    {
        if (hasProject && isDirty)
        {
            showError("Unsaved changes", "Save the current project before opening another.");
            return;
        }
        fileChooser = std::make_unique<juce::FileChooser>("Open an Orbit project", juce::File{}, "*.orbit");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();
                if (file == juce::File{}) return;
                juce::String error;
                auto loaded = ProjectDocument::loadFromFile(file, error);
                if (!loaded)
                {
                    showError("Could not open project", error);
                    return;
                }
                auto engineState = loaded->getTracktionEditState();
                if (!engineState.isValid())
                {
                    showError("Older project format", "This project has no saved audio-engine state yet. Create a new project for the instrument workflow.");
                    return;
                }
                auto restoredEdit = te::loadEditFromState(engine, engineState);
                if (restoredEdit == nullptr)
                {
                    showError("Could not open audio session", "Tracktion Engine could not restore this project's audio state.");
                    return;
                }
                resetPatternButtons();
                edit = std::move(restoredEdit);
                project = std::move(*loaded);
                hasProject = true;
                isDirty = false;
                migratePlaylistState();
                ensureExistingFxReturns();
                findFirstInstrumentTrack();
                refreshTrackSelector();
                readPatternSteps();
                refreshMixerControls();
                refreshProjectView("Opened project and restored its Tracktion session.");
            });
    }

    void saveProject()
    {
        if (!hasProject || edit == nullptr) return;
        const auto currentFile = project.getFile();
        if (currentFile.existsAsFile()) { saveToFile(currentFile); return; }
        fileChooser = std::make_unique<juce::FileChooser>("Save Orbit project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Untitled Project.orbit"), "*.orbit");
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file == juce::File{}) return;
                if (!file.hasFileExtension("orbit")) file = file.withFileExtension("orbit");
                saveToFile(file);
            });
    }

    void exportMidi()
    {
        if (!hasPattern || patternClip == nullptr || edit == nullptr)
        {
            showError("No MIDI pattern", "Create or open a project with a MIDI pattern before exporting.");
            return;
        }

        const auto suggestedName = project.getName().isNotEmpty() ? project.getName() + ".mid" : "Orbit Pattern.mid";
        fileChooser = std::make_unique<juce::FileChooser>("Export MIDI pattern",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(suggestedName), "*.mid");
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file == juce::File{}) return;
                if (!file.hasFileExtension("mid")) file = file.withFileExtension("mid");

                auto notes = patternClip->getSequence().exportToPlaybackMidiSequence(
                    *patternClip, te::MidiList::TimeBase::beatsRaw, false);
                if (notes.getNumEvents() == 0)
                {
                    showError("Nothing to export", "The pattern contains no MIDI notes.");
                    return;
                }

                for (int i = 0; i < notes.getNumEvents(); ++i)
                {
                    auto* event = notes.getEventPointer(i);
                    event->message.setTimeStamp(event->message.getTimeStamp() * te::Edit::ticksPerQuarterNote);
                }
                notes.updateMatchedPairs();

                juce::MidiMessageSequence tempo;
                auto tempoEvent = juce::MidiMessage::tempoMetaEvent(juce::roundToInt(60000000.0 / project.getInitialTempoBpm()));
                tempoEvent.setTimeStamp(0.0);
                tempo.addEvent(tempoEvent);
                const auto meter = project.getInitialTimeSignature();
                auto meterEvent = juce::MidiMessage::timeSignatureMetaEvent(meter.numerator, meter.denominator);
                meterEvent.setTimeStamp(0.0);
                tempo.addEvent(meterEvent);

                juce::MidiFile midiFile;
                midiFile.addTrack(tempo);
                midiFile.addTrack(notes);
                midiFile.setTicksPerQuarterNote(te::Edit::ticksPerQuarterNote);
                juce::FileOutputStream output(file);
                if (!output.openedOk() || !midiFile.writeTo(output))
                {
                    showError("MIDI export failed", "Orbit could not write the MIDI file.");
                    return;
                }
                output.flush();
                status.setText("Exported MIDI pattern: " + file.getFileName(), juce::dontSendNotification);
            });
    }

    void saveToFile(const juce::File& file)
    {
        project.setName(file.getFileNameWithoutExtension());
        for (auto* track : getMixerTracks()) syncTrackMixerState(track);
        syncMasterMixerState();
        edit->flushState();
        project.setTracktionEditState(edit->state);
        juce::String error;
        if (!project.saveToFile(file, error)) { showError("Could not save project", error); return; }
        isDirty = false;
        refreshProjectView("Saved project with instrument and MIDI state: " + file.getFileName());
    }

    void addInstrumentTrack()
    {
        if (!hasProject || edit == nullptr) return;
        const auto trackName = "Instrument " + juce::String(project.getTrackCount() + 1);
        project.addInstrumentTrack();
        const auto existingTracks = getGeneratorTracks();
        te::AudioTrack* track = existingTracks.size() == 1 && project.getTrackCount() == 1
                                    ? existingTracks[0]
                                    : nullptr;
        if (track == nullptr)
        {
            auto insertedTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr);
            track = insertedTrack.get();
        }
        if (track == nullptr) { showError("Could not add track", "Tracktion Engine could not create an instrument track."); return; }
        track->setName(trackName);
        if (auto synth = edit->getPluginCache().createNewPlugin(te::FourOscPlugin::xmlTypeName, {}))
            track->pluginList.insertPlugin(*synth, 0, nullptr);
        instrumentTrack = track;
        refreshTrackSelector();
        readPatternSteps();
        refreshMixerControls();
        isDirty = true;
        refreshProjectView("Added a Tracktion instrument track with the built-in 4OSC synth.");
    }

    void addPattern()
    {
        if (!hasProject || edit == nullptr) return;
        if (instrumentTrack == nullptr) findFirstInstrumentTrack();
        if (instrumentTrack == nullptr) { showError("Add an instrument first", "Create an instrument track before adding a MIDI pattern."); return; }
        const auto patternIndex = selectedPatternIndex();
        const auto trackId = selectedProjectTrackId();
        project.setPatternCreated(trackId, patternIndex, true);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 16; ++step)
                project.setPatternNote(trackId, patternIndex, pitch, step, false);
        for (auto& pitchSteps : noteGrid) pitchSteps.fill(false);
        pitchSelector.setSelectedId(pitchToItemId(60), juce::dontSendNotification);
        pitchSelector.setEnabled(true);
        pianoRollButton.setEnabled(true);
        for (int i = 0; i < 16; ++i)
        {
            const bool enabled = i == 0 || i == 4 || i == 8 || i == 12;
            noteGrid[static_cast<size_t>(60 - minimumPitch)][static_cast<size_t>(i)] = enabled;
            project.setPatternNote(trackId, patternIndex, 60, i, enabled);
        }
        project.setPatternCreated(trackId, patternIndex, true);
        hasPattern = true;
        for (auto& step : steps) step->setEnabled(true);
        loadSelectedPitchSteps();
        rebuildPatternClip();
        isDirty = true;
        refreshProjectView("Pattern created. Open Playlist and click a timeline cell to place its labeled clip.");
    }

    void updatePatternStep(int index, bool enabled)
    {
        if (!hasPattern || index < 0 || index >= 16) return;
        noteGrid[static_cast<size_t>(selectedPitch() - minimumPitch)][static_cast<size_t>(index)] = enabled;
        project.setPatternNote(selectedProjectTrackId(), selectedPatternIndex(), selectedPitch(), index, enabled);
        rebuildPatternClip();
        isDirty = true;
        status.setText("Pattern edited. Save to keep the changes.", juce::dontSendNotification);
    }

    void rebuildPatternClip()
    {
        refreshEnginePlaylist();
    }

    void refreshEnginePlaylist()
    {
        if (edit == nullptr) return;
        patternClip = nullptr;
        const auto engineTracks = getGeneratorTracks();
        for (int trackIndex = 0; trackIndex < engineTracks.size() && trackIndex < project.getTrackCount(); ++trackIndex)
        {
            auto* track = engineTracks.getUnchecked(trackIndex);
            const auto trackId = project.getTrackId(trackIndex);
            std::vector<te::MidiClip*> oldClips;
            for (int item = 0; item < track->getNumTrackItems(); ++item)
                if (auto* clip = dynamic_cast<te::MidiClip*>(track->getTrackItem(item)))
                    oldClips.push_back(clip);
            for (auto* clip : oldClips)
                track->deleteRegionOfClip(clip, clip->getEditTimeRange(), nullptr);

            for (int bar = 0; bar < PlaylistGrid::barCount; ++bar)
            {
                const auto patternId = project.getPlaylistPatternId(trackId, bar);
                const auto patternIndex = project.getPatternIndexForId(trackId, patternId);
                if (patternIndex < 0 || !project.isPatternCreated(trackId, patternIndex)) continue;
                const auto name = project.getPatternName(trackId, patternIndex);
                const auto start = bar * 2.0;
                auto inserted = track->insertMIDIClip(name, { tracktion::TimePosition::fromSeconds(start),
                                                              tracktion::TimePosition::fromSeconds(start + 2.0) }, nullptr);
                auto* clip = inserted.get();
                if (clip == nullptr) continue;
                clip->setLoopRangeBeats({ tracktion::BeatPosition::fromBeats(0.0), tracktion::BeatPosition::fromBeats(4.0) });
                for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                    for (int step = 0; step < 16; ++step)
                        if (project.getPatternNote(trackId, patternIndex, pitch, step))
                            clip->getSequence().addNote(pitch, tracktion::BeatPosition::fromBeats(step * 0.25),
                                                        tracktion::BeatDuration::fromBeats(0.22), 100, 0, nullptr);
                if (track == instrumentTrack && patternIndex == selectedPatternIndex())
                    patternClip = clip;
            }
        }
        if (playlistWindow != nullptr) playlistWindow->refresh();
    }

    void showPlaylist()
    {
        if (playlistWindow == nullptr) return;
        playlistWindow->refresh();
        playlistWindow->setVisible(true);
        playlistWindow->toFront(true);
    }

    void stopPlayback()
    {
        if (edit != nullptr) edit->getTransport().stop(false, false);
        playButton.setButtonText("Play");
        status.setText("Stopped.", juce::dontSendNotification);
    }

    void showMixer(bool focusMaster = false)
    {
        if (mixerWindow == nullptr) return;
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        mixerWindow->refresh(tracks.indexOf(instrumentTrack));
        mixerWindow->setVisible(true);
        mixerWindow->toFront(true);
        if (focusMaster) mixerWindow->focusMaster();
    }

    juce::Array<te::AudioTrack*> getGeneratorTracks() const
    {
        juce::Array<te::AudioTrack*> result;
        if (edit == nullptr) return result;
        for (auto* track : te::getAudioTracks(*edit))
            if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1)) < 0)
                result.add(track);
        return result;
    }

    juce::Array<te::AudioTrack*> getMixerTracks() const
    {
        auto result = getGeneratorTracks();
        for (int bus = 0; bus < 4; ++bus)
            if (auto* returnTrack = getFxReturnTrack(bus))
                result.add(returnTrack);
        return result;
    }

    te::AudioTrack* getFxReturnTrack(int bus) const
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 4)) return nullptr;
        for (auto* track : te::getAudioTracks(*edit))
            if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1)) == bus)
                return track;
        return nullptr;
    }

    te::AudioTrack* ensureFxReturnTrack(int bus)
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 4)) return nullptr;
        if (auto* existing = getFxReturnTrack(bus)) return existing;

        const auto busName = "FX " + juce::String(bus + 1);
        edit->setAuxBusName(bus, busName);

        // Remove legacy returns from the master chain before migrating to an
        // isolated audio return track, so return effects never process the dry mix.
        std::vector<juce::ValueTree> legacyReturns;
        for (auto* plugin : edit->getMasterPluginList())
            if (auto* auxReturn = dynamic_cast<te::AuxReturnPlugin*>(plugin))
                if (static_cast<int>(auxReturn->busNumber) == bus)
                    legacyReturns.push_back(auxReturn->state);
        for (auto& state : legacyReturns)
            if (state.getParent().isValid())
                state.getParent().removeChild(state, &edit->getUndoManager());

        auto returnTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr, true);
        if (returnTrack == nullptr) return nullptr;
        returnTrack->setName(busName + " Return");
        returnTrack->state.setProperty(juce::Identifier("orbitFxReturnBus"), bus, &edit->getUndoManager());
        returnTrack->state.setProperty(juce::Identifier("orbitMixerChannelId"), juce::Uuid().toString(),
                                       &edit->getUndoManager());

        auto returnPlugin = edit->getPluginCache().createNewPlugin(te::AuxReturnPlugin::xmlTypeName, {});
        auto* auxReturn = dynamic_cast<te::AuxReturnPlugin*>(returnPlugin.get());
        if (auxReturn == nullptr) return returnTrack.get();
        auxReturn->busNumber = bus;
        returnTrack->pluginList.insertPlugin(*auxReturn, 0, nullptr);

        const std::array<juce::String, 4> effectTypes {
            te::ReverbPlugin::xmlTypeName, te::DelayPlugin::xmlTypeName,
            te::ChorusPlugin::xmlTypeName, te::PhaserPlugin::xmlTypeName
        };
        const auto effectType = effectTypes[static_cast<size_t>(bus)];
        auto effect = edit->getPluginCache().createNewPlugin(effectType, {});
        if (effect != nullptr)
            returnTrack->pluginList.insertPlugin(*effect, 1, nullptr);
        return returnTrack.get();
    }

    void ensureExistingFxReturns()
    {
        if (edit == nullptr) return;
        const auto generatorTracks = getGeneratorTracks();
        for (int bus = 0; bus < 4; ++bus)
        {
            if (auto* existingReturn = getFxReturnTrack(bus))
            {
                syncTrackMixerState(existingReturn);
                continue;
            }
            const bool hasActiveSend = std::any_of(generatorTracks.begin(), generatorTracks.end(), [bus](te::AudioTrack* track)
            {
                auto* send = track->getAuxSendPlugin(bus);
                return send != nullptr && send->isEnabled() && send->getGainDb() > -59.9f;
            });
            if (hasActiveSend && ensureFxReturnTrack(bus) != nullptr)
                isDirty = true;
        }
    }

    te::Plugin::Ptr getFxReturnEffect(int bus) const
    {
        auto* track = getFxReturnTrack(bus);
        if (track == nullptr) return {};
        for (auto* plugin : track->pluginList)
            if (dynamic_cast<te::AuxReturnPlugin*>(plugin) == nullptr
                && dynamic_cast<te::VolumeAndPanPlugin*>(plugin) == nullptr
                && dynamic_cast<te::LevelMeterPlugin*>(plugin) == nullptr)
                return plugin;
        return {};
    }

    void openFxReturnEditor(int bus)
    {
        if (mixerWindow == nullptr) return;
        auto effect = getFxReturnEffect(bus);
        if (effect == nullptr)
        {
            status.setText("Raise the FX " + juce::String(bus + 1) + " send to create its return effect.", juce::dontSendNotification);
            return;
        }
        mixerWindow->showEffectEditor(effect, [this, bus]
        {
            isDirty = true;
            status.setText("FX " + juce::String(bus + 1) + " return effect changed. Save to keep the change.", juce::dontSendNotification);
        });
    }

    double getTrackGainDb(int index) const
    {
        if (edit == nullptr) return 0.0;
        const auto tracks = getMixerTracks();
        if (!juce::isPositiveAndBelow(index, tracks.size())) return 0.0;
        const auto* plugin = tracks.getUnchecked(index)->getVolumePlugin();
        return plugin != nullptr ? plugin->getVolumeDb() : 0.0;
    }

    double getTrackPan(int index) const
    {
        if (edit == nullptr) return 0.0;
        const auto tracks = getMixerTracks();
        if (!juce::isPositiveAndBelow(index, tracks.size())) return 0.0;
        const auto* plugin = tracks.getUnchecked(index)->getVolumePlugin();
        return plugin != nullptr ? plugin->pan.get() : 0.0;
    }

    bool getTrackMute(int index) const
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        return juce::isPositiveAndBelow(index, tracks.size()) && tracks.getUnchecked(index)->isMuted(false);
    }

    bool getTrackSolo(int index) const
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        return juce::isPositiveAndBelow(index, tracks.size()) && tracks.getUnchecked(index)->isSolo(false);
    }

    void syncTrackMixerState(te::AudioTrack* track)
    {
        if (track == nullptr || edit == nullptr) return;
        const auto generators = getGeneratorTracks();
        const auto projectIndex = generators.indexOf(track);
        auto* volume = track->getVolumePlugin();
        if (!juce::isPositiveAndBelow(projectIndex, project.getTrackCount()))
        {
            const auto bus = static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1));
            if (!juce::isPositiveAndBelow(bus, 4)) return;

            auto channelId = track->state.getProperty(juce::Identifier("orbitMixerChannelId")).toString();
            if (channelId.isEmpty())
            {
                channelId = juce::Uuid().toString();
                track->state.setProperty(juce::Identifier("orbitMixerChannelId"), channelId, &edit->getUndoManager());
                isDirty = true;
            }

            if (project.setFxReturnMixerState(bus, channelId, track->getName(),
                                              volume != nullptr ? volume->getVolumeDb() : 0.0,
                                              volume != nullptr ? volume->pan.get() : 0.0,
                                              track->isMuted(false), track->isSolo(false)))
                isDirty = true;
            return;
        }

        project.setTrackMixerState(projectIndex,
                                   volume != nullptr ? volume->getVolumeDb() : 0.0,
                                   volume != nullptr ? volume->pan.get() : 0.0,
                                   track->isMuted(false), track->isSolo(false));
        for (int bus = 0; bus < 4; ++bus)
        {
            auto* send = track->getAuxSendPlugin(bus);
            project.setTrackSendGain(projectIndex, bus, send != nullptr ? send->getGainDb() : -60.0);
        }
    }

    void syncMasterMixerState()
    {
        if (edit == nullptr || edit->getMasterVolumePlugin() == nullptr) return;
        project.setMasterMixerState(getMasterGainDb(), edit->getMasterVolumePlugin()->getPan(), isMasterMuted());
    }

    void setTrackGainDb(int index, double value)
    {
        if (edit == nullptr) return;
        const auto tracks = getMixerTracks();
        if (!juce::isPositiveAndBelow(index, tracks.size())) return;
        if (auto* plugin = tracks.getUnchecked(index)->getVolumePlugin())
        {
            plugin->setVolumeDb(static_cast<float>(value));
            syncTrackMixerState(tracks.getUnchecked(index));
            isDirty = true;
            status.setText("Mixer level changed. Save to keep the change.", juce::dontSendNotification);
        }
    }

    void setTrackPan(int index, double value)
    {
        if (edit == nullptr) return;
        const auto tracks = getMixerTracks();
        if (!juce::isPositiveAndBelow(index, tracks.size())) return;
        if (auto* plugin = tracks.getUnchecked(index)->getVolumePlugin())
        {
            plugin->setPan(static_cast<float>(value));
            syncTrackMixerState(tracks.getUnchecked(index));
            isDirty = true;
            status.setText("Mixer pan changed. Save to keep the change.", juce::dontSendNotification);
        }
    }

    void setTrackMute(int index, bool value)
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        if (!juce::isPositiveAndBelow(index, tracks.size())) return;
        tracks.getUnchecked(index)->setMute(value);
        syncTrackMixerState(tracks.getUnchecked(index));
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(tracks.indexOf(instrumentTrack));
        status.setText(value ? "Track muted. Save to keep the change." : "Track unmuted. Save to keep the change.",
                       juce::dontSendNotification);
    }

    void setTrackSolo(int index, bool value)
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        if (!juce::isPositiveAndBelow(index, tracks.size())) return;
        tracks.getUnchecked(index)->setSolo(value);
        syncTrackMixerState(tracks.getUnchecked(index));
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(tracks.indexOf(instrumentTrack));
        status.setText(value ? "Generator solo enabled. Save to keep the change." : "Generator solo disabled. Save to keep the change.",
                       juce::dontSendNotification);
    }

    te::EqualiserPlugin* getTrackEqualiser(int index) const
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        if (edit != nullptr && index == tracks.size())
            for (auto* plugin : edit->getMasterPluginList())
                if (auto* eq = dynamic_cast<te::EqualiserPlugin*>(plugin)) return eq;
        return juce::isPositiveAndBelow(index, tracks.size()) ? tracks.getUnchecked(index)->getEqualiserPlugin() : nullptr;
    }

    te::EqualiserPlugin* ensureTrackEqualiser(int index)
    {
        if (edit == nullptr) return nullptr;
        auto tracks = getMixerTracks();
        if (index == tracks.size())
        {
            for (auto* plugin : edit->getMasterPluginList())
                if (auto* eq = dynamic_cast<te::EqualiserPlugin*>(plugin)) return eq;
            auto plugin = edit->getPluginCache().createNewPlugin(te::EqualiserPlugin::xmlTypeName, {});
            if (plugin == nullptr) return nullptr;
            auto& masterPlugins = edit->getMasterPluginList();
            const auto* volume = edit->getMasterVolumePlugin().get();
            auto insertIndex = volume != nullptr ? masterPlugins.indexOf(volume) : masterPlugins.size();
            if (insertIndex < 0) insertIndex = masterPlugins.size();
            masterPlugins.insertPlugin(*plugin, insertIndex, nullptr);
            isDirty = true;
            return dynamic_cast<te::EqualiserPlugin*>(plugin.get());
        }
        if (!juce::isPositiveAndBelow(index, tracks.size())) return nullptr;
        auto* track = tracks.getUnchecked(index);
        if (auto* existing = track->getEqualiserPlugin()) return existing;
        auto plugin = edit->getPluginCache().createNewPlugin(te::EqualiserPlugin::xmlTypeName, {});
        if (plugin == nullptr) return nullptr;
        const auto* volume = track->getVolumePlugin();
        const auto insertIndex = volume != nullptr ? track->pluginList.indexOf(volume) : track->pluginList.size();
        track->pluginList.insertPlugin(*plugin, insertIndex, nullptr);
        isDirty = true;
        return track->getEqualiserPlugin();
    }

    float getTrackEqParameter(int index, int parameter) const
    {
        if (!juce::isPositiveAndBelow(parameter, 12)) return 0.0f;
        auto* eq = getTrackEqualiser(index);
        const auto band = parameter / 3;
        const auto kind = parameter % 3;
        if (eq == nullptr)
        {
            static constexpr std::array<float, 4> defaultFrequencies { 80.0f, 3000.0f, 5000.0f, 17000.0f };
            if (kind == 0) return defaultFrequencies[static_cast<size_t>(band)];
            if (kind == 2) return 0.5f;
            return 0.0f;
        }
        if (band == 0) return kind == 0 ? eq->loFreqValue.get() : kind == 1 ? eq->loGainValue.get() : eq->loQValue.get();
        if (band == 1) return kind == 0 ? eq->midFreqValue1.get() : kind == 1 ? eq->midGainValue1.get() : eq->midQValue1.get();
        if (band == 2) return kind == 0 ? eq->midFreqValue2.get() : kind == 1 ? eq->midGainValue2.get() : eq->midQValue2.get();
        return kind == 0 ? eq->hiFreqValue.get() : kind == 1 ? eq->hiGainValue.get() : eq->hiQValue.get();
    }

    float getTrackEqResponse(int index, float frequency) const
    {
        auto* eq = getTrackEqualiser(index);
        return eq != nullptr ? eq->getDBGainAtFrequency(frequency) : 0.0f;
    }

    void setTrackEqEnabled(int index, bool enabled)
    {
        auto* eq = enabled ? ensureTrackEqualiser(index) : getTrackEqualiser(index);
        if (eq == nullptr) return;
        if (eq->isEnabled() != enabled)
        {
            eq->setEnabled(enabled);
            isDirty = true;
        }
        if (mixerWindow != nullptr)
            mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
        status.setText(enabled ? "Equaliser enabled. Save to keep the change."
                               : "Equaliser bypassed. Save to keep the change.",
                       juce::dontSendNotification);
    }

    void setTrackEqParameter(int index, int parameter, float value)
    {
        auto* eq = ensureTrackEqualiser(index);
        if (eq == nullptr || !juce::isPositiveAndBelow(parameter, 12)) return;
        const auto band = parameter / 3;
        const auto kind = parameter % 3;
        if (band == 0) { if (kind == 0) eq->setLowFreq(value); else if (kind == 1) eq->setLowGain(value); else eq->setLowQ(value); }
        else if (band == 1) { if (kind == 0) eq->setMidFreq1(value); else if (kind == 1) eq->setMidGain1(value); else eq->setMidQ1(value); }
        else if (band == 2) { if (kind == 0) eq->setMidFreq2(value); else if (kind == 1) eq->setMidGain2(value); else eq->setMidQ2(value); }
        else { if (kind == 0) eq->setHighFreq(value); else if (kind == 1) eq->setHighGain(value); else eq->setHighQ(value); }
        isDirty = true;
        if (mixerWindow != nullptr)
        {
            mixerWindow->refreshEqControls();
            mixerWindow->repaintEqGraphs();
        }
        status.setText("Equaliser changed. Save to keep the change.", juce::dontSendNotification);
    }

    void setTrackEqBand(int index, int band, float frequency, float gain)
    {
        auto* eq = ensureTrackEqualiser(index);
        if (eq == nullptr || !juce::isPositiveAndBelow(band, 4)) return;
        const auto safeFrequency = juce::jlimit(20.0f, 20000.0f, frequency);
        const auto safeGain = juce::jlimit(-20.0f, 20.0f, gain);
        if (band == 0) { eq->setLowFreq(safeFrequency); eq->setLowGain(safeGain); }
        else if (band == 1) { eq->setMidFreq1(safeFrequency); eq->setMidGain1(safeGain); }
        else if (band == 2) { eq->setMidFreq2(safeFrequency); eq->setMidGain2(safeGain); }
        else { eq->setHighFreq(safeFrequency); eq->setHighGain(safeGain); }
        isDirty = true;
        if (mixerWindow != nullptr)
        {
            mixerWindow->refreshEqControls();
            mixerWindow->repaintEqGraphs();
        }
        if (equaliserWindow != nullptr) equaliserWindow->refreshParameterValues(index);
        status.setText("Equaliser band changed. Save to keep the change.", juce::dontSendNotification);
    }

    void openTrackEqualiser(int index)
    {
        if (ensureTrackEqualiser(index) == nullptr || equaliserWindow == nullptr) return;
        if (mixerWindow != nullptr) mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
        equaliserWindow->openForTrack(index);
    }

    juce::Array<te::Plugin*> getInsertPlugins(int index) const
    {
        juce::Array<te::Plugin*> result;
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        if (edit == nullptr || (index != tracks.size() && !juce::isPositiveAndBelow(index, tracks.size()))) return result;
        const auto& pluginList = index == tracks.size() ? edit->getMasterPluginList() : tracks.getUnchecked(index)->pluginList;
        for (auto* plugin : pluginList)
            if (dynamic_cast<te::FourOscPlugin*>(plugin) == nullptr
                && dynamic_cast<te::VolumeAndPanPlugin*>(plugin) == nullptr
                && dynamic_cast<te::LevelMeterPlugin*>(plugin) == nullptr
                && dynamic_cast<te::EqualiserPlugin*>(plugin) == nullptr
                && dynamic_cast<te::SamplerPlugin*>(plugin) == nullptr
                && dynamic_cast<te::AuxReturnPlugin*>(plugin) == nullptr
                && dynamic_cast<te::AuxSendPlugin*>(plugin) == nullptr)
                result.add(plugin);
        return result;
    }

    juce::StringArray getInsertNames(int index) const
    {
        juce::StringArray names;
        for (auto* plugin : getInsertPlugins(index)) names.add(plugin->getName());
        return names;
    }

    bool getInsertEnabled(int index, int slot) const
    {
        const auto plugins = getInsertPlugins(index);
        return juce::isPositiveAndBelow(slot, plugins.size()) && plugins.getUnchecked(slot)->isEnabled();
    }

    void toggleInsert(int index, int slot)
    {
        const auto plugins = getInsertPlugins(index);
        if (!juce::isPositiveAndBelow(slot, plugins.size())) return;
        auto* plugin = plugins.getUnchecked(slot);
        plugin->setEnabled(!plugin->isEnabled());
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
        status.setText(plugin->getName() + (plugin->isEnabled() ? " enabled." : " bypassed."), juce::dontSendNotification);
    }

    void openInsertEditor(int index, int slot)
    {
        if (mixerWindow == nullptr || edit == nullptr) return;
        const auto plugins = getInsertPlugins(index);
        if (!juce::isPositiveAndBelow(slot, plugins.size())) return;
        te::Plugin::Ptr pluginRef = plugins.getUnchecked(slot);
        mixerWindow->showEffectEditor(pluginRef, [this]
        {
            isDirty = true;
            status.setText("Effect parameter changed. Save to keep the change.", juce::dontSendNotification);
        });
    }

    void removeInsert(int index, int slot)
    {
        const auto plugins = getInsertPlugins(index);
        if (!juce::isPositiveAndBelow(slot, plugins.size()) || edit == nullptr) return;
        const auto insertName = plugins.getUnchecked(slot)->getName();
        const auto state = plugins.getUnchecked(slot)->state;
        if (state.getParent().isValid())
            state.getParent().removeChild(state, &edit->getUndoManager());
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
        status.setText("Removed " + insertName + ". Save to keep the change.", juce::dontSendNotification);
    }

    void addInsert(int index, int kind)
    {
        if (edit == nullptr) return;
        auto tracks = getMixerTracks();
        const bool isMaster = index == tracks.size();
        if (!isMaster && !juce::isPositiveAndBelow(index, tracks.size())) return;
        if (getInsertPlugins(index).size() >= 4)
        {
            status.setText("This mixer strip has four insert slots. Remove one before adding another.", juce::dontSendNotification);
            return;
        }
        const char* pluginType = nullptr;
        if (kind == 1) pluginType = te::CompressorPlugin::xmlTypeName;
        else if (kind == 2) pluginType = te::ReverbPlugin::xmlTypeName;
        else if (kind == 3) pluginType = te::DelayPlugin::xmlTypeName;
        else if (kind == 4) pluginType = te::ChorusPlugin::xmlTypeName;
        else if (kind == 5) pluginType = te::PhaserPlugin::xmlTypeName;
        else if (kind == 6) pluginType = te::LowPassPlugin::xmlTypeName;
        if (pluginType == nullptr) return;

        auto* track = isMaster ? nullptr : tracks.getUnchecked(index);
        auto plugin = edit->getPluginCache().createNewPlugin(pluginType, {});
        if (plugin == nullptr) return;
        auto& pluginList = isMaster ? edit->getMasterPluginList() : track->pluginList;
        const auto* volume = isMaster ? edit->getMasterVolumePlugin().get() : track->getVolumePlugin();
        auto insertIndex = volume != nullptr ? pluginList.indexOf(volume) : pluginList.size();
        if (insertIndex < 0) insertIndex = pluginList.size();
        pluginList.insertPlugin(*plugin, insertIndex, nullptr);
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(tracks.indexOf(instrumentTrack));
        const auto insertCount = getInsertPlugins(index).size();
        status.setText("Added " + plugin->getName() + " to " + (isMaster ? juce::String("Master") : track->getName())
                           + " (" + juce::String(insertCount) + " insert" + (insertCount == 1 ? ")" : "s)")
                           + ". Save to keep the change.",
                       juce::dontSendNotification);
    }

    double getTrackSendGainDb(int index, int bus) const
    {
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        if (!juce::isPositiveAndBelow(index, tracks.size()) || !juce::isPositiveAndBelow(bus, 4)) return -60.0;
        if (auto* send = tracks.getUnchecked(index)->getAuxSendPlugin(bus))
            return send->getGainDb();
        return -60.0;
    }

    void setTrackSendGainDb(int index, int bus, double value)
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 4)) return;
        auto tracks = getGeneratorTracks();
        if (!juce::isPositiveAndBelow(index, tracks.size())) return;
        auto* track = tracks.getUnchecked(index);
        auto* send = track->getAuxSendPlugin(bus);
        const bool fxReturnWasMissing = getFxReturnTrack(bus) == nullptr;
        if (send != nullptr && fxReturnWasMissing && ensureFxReturnTrack(bus) == nullptr)
            return;

        if (send == nullptr && value > -59.9)
        {
            const auto busName = "FX " + juce::String(bus + 1);
            if (ensureFxReturnTrack(bus) == nullptr) return;

            auto sendPlugin = edit->getPluginCache().createNewPlugin(te::AuxSendPlugin::xmlTypeName, {});
            if (auto* auxSend = dynamic_cast<te::AuxSendPlugin*>(sendPlugin.get()))
            {
                auxSend->busNumber = bus;
                auxSend->setGainDb(static_cast<float>(value));
                const auto* meter = track->getLevelMeterPlugin();
                auto insertIndex = meter != nullptr ? track->pluginList.indexOf(meter) : track->pluginList.size();
                if (insertIndex < 0) insertIndex = track->pluginList.size();
                track->pluginList.insertPlugin(*auxSend, insertIndex, nullptr);
                send = auxSend;
            }
        }
        else if (send != nullptr)
        {
            send->setGainDb(static_cast<float>(value));
        }

        if (send != nullptr)
        {
            syncTrackMixerState(track);
            isDirty = true;
            if (fxReturnWasMissing && mixerWindow != nullptr)
                mixerWindow->refresh(getGeneratorTracks().indexOf(instrumentTrack));
            static const std::array<const char*, 4> fxNames { "reverb", "delay", "chorus", "phaser" };
            status.setText("FX " + juce::String(bus + 1) + " send routed to its shared "
                           + fxNames[static_cast<size_t>(bus)] + " return. Save to keep the change.", juce::dontSendNotification);
        }
    }

    static const juce::Identifier& masterMuteProperty()
    {
        static const juce::Identifier property("orbitMasterMuted");
        return property;
    }

    static const juce::Identifier& masterPreMuteGainProperty()
    {
        static const juce::Identifier property("orbitMasterPreMuteGainDb");
        return property;
    }

    bool isMasterMuted() const
    {
        return edit != nullptr && static_cast<bool>(edit->state.getProperty(masterMuteProperty(), false));
    }

    double getMasterGainDb() const
    {
        if (edit == nullptr || edit->getMasterVolumePlugin() == nullptr) return 0.0;
        if (isMasterMuted())
            return static_cast<double>(edit->state.getProperty(masterPreMuteGainProperty(), 0.0));
        return edit->getMasterVolumePlugin()->getVolumeDb();
    }

    void setMasterMuted(bool shouldMute)
    {
        if (edit == nullptr || edit->getMasterVolumePlugin() == nullptr || isMasterMuted() == shouldMute) return;
        auto* volume = edit->getMasterVolumePlugin().get();
        if (shouldMute)
        {
            edit->state.setProperty(masterPreMuteGainProperty(), volume->getVolumeDb(), &edit->getUndoManager());
            volume->setVolumeDb(-100.0f);
        }
        else
        {
            const auto previousGain = static_cast<float>(edit->state.getProperty(masterPreMuteGainProperty(), 0.0));
            volume->setVolumeDb(previousGain);
        }
        edit->state.setProperty(masterMuteProperty(), shouldMute, &edit->getUndoManager());
        syncMasterMixerState();
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(getMixerTracks().size());
        status.setText(shouldMute ? "Master output muted. Save to keep the change." : "Master output unmuted. Save to keep the change.",
                       juce::dontSendNotification);
    }

    void setMasterPan(double value)
    {
        if (edit == nullptr || edit->getMasterVolumePlugin() == nullptr) return;
        edit->getMasterVolumePlugin()->setPan(static_cast<float>(value));
        syncMasterMixerState();
        isDirty = true;
        status.setText("Master pan changed. Save to keep the change.", juce::dontSendNotification);
    }

    void setMasterGainDb(double value)
    {
        if (edit == nullptr || edit->getMasterVolumePlugin() == nullptr) return;
        if (isMasterMuted())
        {
            edit->state.setProperty(masterPreMuteGainProperty(), value, &edit->getUndoManager());
            edit->getMasterVolumePlugin()->setVolumeDb(-100.0f);
        }
        else
        {
            edit->getMasterVolumePlugin()->setVolumeDb(static_cast<float>(value));
        }
        syncMasterMixerState();
        isDirty = true;
        status.setText("Master level changed. Save to keep the change.", juce::dontSendNotification);
    }

    void selectTrackByIndex(int index)
    {
        if (edit == nullptr) return;
        trackSelector.setSelectedId(index + 1, juce::dontSendNotification);
        selectInstrumentTrack();
    }

    juce::String getPlaylistClipLabel(int trackIndex, int bar) const
    {
        const auto trackId = project.getTrackId(trackIndex);
        const auto patternId = project.getPlaylistPatternId(trackId, bar);
        const auto patternIndex = project.getPatternIndexForId(trackId, patternId);
        return patternIndex >= 0 ? project.getPatternName(trackId, patternIndex) : juce::String{};
    }

    void togglePlaylistClip(int trackIndex, int bar)
    {
        if (!hasProject || edit == nullptr || trackIndex < 0 || trackIndex >= project.getTrackCount()) return;
        const auto trackId = project.getTrackId(trackIndex);
        const auto patternIndex = selectedPatternIndex();
        if (!project.isPatternCreated(trackId, patternIndex))
        {
            showError("Empty pattern slot", "Select an instrument, create this pattern, then place it in the Playlist.");
            return;
        }
        const auto patternId = project.getPatternId(trackId, patternIndex);
        const auto current = project.getPlaylistPatternId(trackId, bar);
        project.setPlaylistClip(trackId, patternId, bar, 1, current != patternId);
        rebuildPatternClip();
        isDirty = true;
        refreshProjectView(current == patternId ? "Removed the pattern clip." : "Placed " + project.getPatternName(trackId, patternIndex) + " in the Playlist.");
    }

    void movePlaylistClip(int trackIndex, int sourceBar, int destinationBar)
    {
        if (!hasProject || edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(sourceBar, PlaylistGrid::barCount)
            || !juce::isPositiveAndBelow(destinationBar, PlaylistGrid::barCount)
            || sourceBar == destinationBar)
            return;

        const auto trackId = project.getTrackId(trackIndex);
        const auto patternId = project.getPlaylistPatternId(trackId, sourceBar);
        if (patternId.isEmpty()) return;

        project.setPlaylistClip(trackId, {}, sourceBar, 1, false);
        project.setPlaylistClip(trackId, patternId, destinationBar, 1, true);
        rebuildPatternClip();
        isDirty = true;
        const auto destinationPattern = project.getPatternIndexForId(trackId, patternId);
        const juce::String name = destinationPattern >= 0 ? project.getPatternName(trackId, destinationPattern) : juce::String("pattern");
        refreshProjectView("Moved " + name + " to Playlist bar " + juce::String(destinationBar + 1) + ".");
    }

    void togglePlayback()
    {
        if (edit == nullptr) return;
        auto* activeDevice = engine.getDeviceManager().deviceManager.getCurrentAudioDevice();
        if (activeDevice == nullptr)
            status.setText("No active audio output. Choose a device in Audio Settings.", juce::dontSendNotification);
        auto& transport = edit->getTransport();
        if (transport.isPlaying())
        {
            transport.stop(false, false);
            playButton.setButtonText("Play");
            return;
        }
        bool hasPlaylistContent = false;
        for (int trackIndex = 0; trackIndex < project.getTrackCount() && !hasPlaylistContent; ++trackIndex)
            for (int bar = 0; bar < PlaylistGrid::barCount; ++bar)
                hasPlaylistContent |= project.getPlaylistPatternId(project.getTrackId(trackIndex), bar).isNotEmpty();
        if (!hasPlaylistContent)
        {
            status.setText("Place a pattern clip in the Playlist before starting playback.", juce::dontSendNotification);
            return;
        }
        transport.setLoopRange({ tracktion::TimePosition::fromSeconds(0.0),
                                 tracktion::TimePosition::fromSeconds(PlaylistGrid::barCount * 2.0) });
        transport.looping = true;
        transport.playFromStart(false);
        playButton.setButtonText("Stop");
        if (activeDevice != nullptr)
            status.setText("Playing through " + activeDevice->getName(), juce::dontSendNotification);
    }

    void showAudioSettings()
    {
        if (audioSettingsWindow == nullptr)
            audioSettingsWindow = std::make_unique<AudioSettingsWindow>(engine.getDeviceManager().deviceManager);
        audioSettingsWindow->setVisible(true);
        audioSettingsWindow->toFront(true);
    }

    void updateAudioStatus()
    {
        auto* device = engine.getDeviceManager().deviceManager.getCurrentAudioDevice();
        if (device == nullptr)
        {
            audioDetails.setText("Audio: No active output device", juce::dontSendNotification);
            return;
        }

        const auto outputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
        audioDetails.setText("Audio: " + device->getName() + "  ·  "
                                 + juce::String(device->getCurrentSampleRate(), 0) + " Hz  ·  "
                                 + juce::String(outputChannels) + " output channels",
                             juce::dontSendNotification);
    }

    void migratePlaylistState()
    {
        if (edit == nullptr) return;
        const auto tracks = getGeneratorTracks();
        for (int trackIndex = 0; trackIndex < tracks.size() && trackIndex < project.getTrackCount(); ++trackIndex)
        {
            auto* track = tracks.getUnchecked(trackIndex);
            const auto trackId = project.getTrackId(trackIndex);
            std::vector<te::MidiClip*> clips;
            for (int item = 0; item < track->getNumTrackItems(); ++item)
                if (auto* clip = dynamic_cast<te::MidiClip*>(track->getTrackItem(item)))
                    clips.push_back(clip);
            if (clips.empty()) continue;

            auto* source = clips.front();
            int matchedPattern = -1;
            for (int candidate = 0; candidate < ProjectDocument::patternSlotCount && matchedPattern < 0; ++candidate)
            {
                if (!project.isPatternCreated(trackId, candidate)) continue;
                bool matches = true;
                for (int pitch = minimumPitch; pitch <= maximumPitch && matches; ++pitch)
                    for (int step = 0; step < 16; ++step)
                    {
                        bool clipHasNote = false;
                        for (int noteIndex = 0; noteIndex < source->getSequence().getNumNotes(); ++noteIndex)
                            if (auto* note = source->getSequence().getNote(noteIndex))
                                clipHasNote |= note->getNoteNumber() == pitch
                                    && juce::jlimit(0, 15, static_cast<int>(std::round(note->getStartBeat().inBeats() * 4.0))) == step;
                        if (clipHasNote != project.getPatternNote(trackId, candidate, pitch, step))
                        {
                            matches = false;
                            break;
                        }
                    }
                if (matches) matchedPattern = candidate;
            }

            if (matchedPattern < 0)
            {
                matchedPattern = 0;
                for (int noteIndex = 0; noteIndex < source->getSequence().getNumNotes(); ++noteIndex)
                    if (auto* note = source->getSequence().getNote(noteIndex))
                    {
                        const auto step = juce::jlimit(0, 15, static_cast<int>(std::round(note->getStartBeat().inBeats() * 4.0)));
                        if (note->getNoteNumber() >= minimumPitch && note->getNoteNumber() <= maximumPitch)
                            project.setPatternNote(trackId, matchedPattern, note->getNoteNumber(), step, true);
                    }
                project.setPatternCreated(trackId, matchedPattern, true);
            }

            const auto patternId = project.getPatternId(trackId, matchedPattern);
            for (auto* clip : clips)
            {
                const auto bar = juce::roundToInt(clip->getEditTimeRange().getStart().inSeconds() / 2.0);
                if (juce::isPositiveAndBelow(bar, 16) && project.getPlaylistPatternId(trackId, bar).isEmpty())
                    project.setPlaylistClip(trackId, patternId, bar, 1, true);
            }
        }
        refreshEnginePlaylist();
    }

    void findFirstInstrumentTrack()
    {
        instrumentTrack = nullptr;
        patternClip = nullptr;
        if (edit == nullptr) return;
        for (auto* track : getGeneratorTracks())
        {
            if (instrumentTrack == nullptr) instrumentTrack = track;
            for (int i = 0; i < track->getNumTrackItems(); ++i)
                if (auto* clip = dynamic_cast<te::MidiClip*>(track->getTrackItem(i)))
                {
                    if (patternClip == nullptr)
                    {
                        instrumentTrack = track;
                        patternClip = clip;
                    }
                }
        }
    }

    void readPatternSteps()
    {
        for (auto& pitchSteps : noteGrid) pitchSteps.fill(false);
        const auto trackId = selectedProjectTrackId();
        hasPattern = project.isPatternCreated(trackId, selectedPatternIndex());
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 16; ++step)
                noteGrid[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)] =
                    project.getPatternNote(trackId, selectedPatternIndex(), pitch, step);
        pitchSelector.setEnabled(hasPattern);
        pitchSelector.setVisible(instrumentTrack != nullptr);
        pianoRollButton.setEnabled(hasPattern);
        pianoRollButton.setVisible(instrumentTrack != nullptr);
        generatorTitle.setVisible(instrumentTrack != nullptr);
        patternTitle.setVisible(instrumentTrack != nullptr);
        for (auto& step : steps)
        {
            step->setEnabled(hasPattern);
            step->setVisible(instrumentTrack != nullptr);
        }
        for (auto& button : patternBankButtons)
        {
            button->setEnabled(instrumentTrack != nullptr);
            button->setVisible(instrumentTrack != nullptr);
        }
        for (auto& button : patternSlotButtons)
        {
            button->setEnabled(instrumentTrack != nullptr);
            button->setVisible(instrumentTrack != nullptr);
        }
        resized();
        loadSelectedPitchSteps();
    }

    int selectedPatternIndex() const
    {
        return selectedBankIndex * ProjectDocument::patternsPerBank + selectedSlotIndex;
    }

    juce::String selectedProjectTrackId() const
    {
        if (instrumentTrack == nullptr || edit == nullptr)
            return {};
        const auto tracks = getGeneratorTracks();
        const auto index = tracks.indexOf(instrumentTrack);
        return project.getTrackId(index);
    }

    void selectPatternBank(int bankIndex)
    {
        selectedBankIndex = juce::jlimit(0, 7, bankIndex);
        for (int i = 0; i < 8; ++i)
            patternBankButtons[static_cast<size_t>(i)]->setToggleState(i == selectedBankIndex, juce::dontSendNotification);
        loadPatternSelection();
    }

    void selectPatternSlot(int slotIndex)
    {
        selectedSlotIndex = juce::jlimit(0, 7, slotIndex);
        for (int i = 0; i < 8; ++i)
            patternSlotButtons[static_cast<size_t>(i)]->setToggleState(i == selectedSlotIndex, juce::dontSendNotification);
        loadPatternSelection();
    }

    void loadPatternSelection()
    {
        if (instrumentTrack == nullptr || edit == nullptr) return;
        readPatternSteps();
        status.setText(hasPattern ? "Selected pattern. Playlist clips retain their own pattern links."
                                  : "Empty slot. Create this pattern, then place its labeled clip in the Playlist.",
                       juce::dontSendNotification);
    }

    void resetPatternButtons()
    {
        hasPattern = false;
        if (mixerWindow != nullptr) mixerWindow->detachMeters();
        detachTrackMeter();
        instrumentTrack = nullptr;
        patternClip = nullptr;
        refreshingTrackSelector = true;
        trackSelector.clear(juce::dontSendNotification);
        trackSelector.setEnabled(false);
        refreshingTrackSelector = false;
        levelSlider.setEnabled(false);
        panSlider.setEnabled(false);
        for (auto& pitchSteps : noteGrid) pitchSteps.fill(false);
        pitchSelector.setEnabled(false);
        pitchSelector.setVisible(false);
        pianoRollButton.setEnabled(false);
        pianoRollButton.setVisible(false);
        generatorTitle.setVisible(false);
        patternTitle.setVisible(false);
        for (auto& button : patternBankButtons) { button->setEnabled(false); button->setVisible(false); }
        for (auto& button : patternSlotButtons) { button->setEnabled(false); button->setVisible(false); }
        for (auto& step : steps)
        {
            step->setEnabled(false);
            step->setVisible(false);
            step->setToggleState(false, juce::dontSendNotification);
        }
        selectedBankIndex = 0;
        selectedSlotIndex = 0;
        resized();
        for (int i = 0; i < 8; ++i)
        {
            patternBankButtons[static_cast<size_t>(i)]->setToggleState(i == 0, juce::dontSendNotification);
            patternSlotButtons[static_cast<size_t>(i)]->setToggleState(i == 0, juce::dontSendNotification);
        }
        if (playlistWindow != nullptr) playlistWindow->refresh();
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh("A1");
    }

    void refreshMixerControls()
    {
        const auto* volumePan = instrumentTrack != nullptr ? instrumentTrack->getVolumePlugin() : nullptr;
        const bool available = volumePan != nullptr;
        levelSlider.setEnabled(available);
        panSlider.setEnabled(available);
        attachTrackMeter();
        if (!available) return;
        levelSlider.setValue(volumePan->getVolumeDb(), juce::dontSendNotification);
        panSlider.setValue(volumePan->pan.get(), juce::dontSendNotification);
        mixerTitle.setText("Mixer - " + instrumentTrack->getName(), juce::dontSendNotification);
    }

    void refreshTrackSelector()
    {
        refreshingTrackSelector = true;
        trackSelector.clear(juce::dontSendNotification);
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        for (int i = 0; i < tracks.size(); ++i)
            trackSelector.addItem(tracks.getUnchecked(i)->getName(), i + 1);
        const auto selected = tracks.indexOf(instrumentTrack);
        trackSelector.setSelectedId(selected >= 0 ? selected + 1 : 0, juce::dontSendNotification);
        trackSelector.setEnabled(!tracks.isEmpty());
        for (auto& button : patternBankButtons) button->setEnabled(instrumentTrack != nullptr);
        for (auto& button : patternSlotButtons) button->setEnabled(instrumentTrack != nullptr);
        refreshingTrackSelector = false;
        if (playlistWindow != nullptr) playlistWindow->refresh();
    }

    void selectInstrumentTrack()
    {
        if (refreshingTrackSelector || edit == nullptr) return;
        const auto tracks = getGeneratorTracks();
        const auto index = trackSelector.getSelectedId() - 1;
        if (index < 0 || index >= tracks.size()) return;
        instrumentTrack = tracks.getUnchecked(index);
        patternClip = nullptr;
        for (int i = 0; i < instrumentTrack->getNumTrackItems(); ++i)
            if (auto* clip = dynamic_cast<te::MidiClip*>(instrumentTrack->getTrackItem(i)))
            {
                patternClip = clip;
                break;
            }
        readPatternSteps();
        refreshMixerControls();
        if (playlistWindow != nullptr) playlistWindow->refresh();
        status.setText("Selected " + instrumentTrack->getName() + ". Pattern bank A-H x 1-8 is ready.", juce::dontSendNotification);
    }

    void detachTrackMeter()
    {
        if (trackMeter != nullptr)
            trackMeter->measurer.removeClient(trackMeterClient);
        trackMeter = nullptr;
        trackMeterClient.reset();
    }

    void attachTrackMeter()
    {
        auto* nextMeter = instrumentTrack != nullptr ? instrumentTrack->getLevelMeterPlugin() : nullptr;
        if (nextMeter == trackMeter) return;
        detachTrackMeter();
        trackMeter = nextMeter;
        if (trackMeter != nullptr)
            trackMeter->measurer.addClient(trackMeterClient);
    }

    void updateMeterStatus()
    {
        if (trackMeter == nullptr)
        {
            meterStatus.setText("Track output: no instrument meter", juce::dontSendNotification);
            return;
        }

        const auto leftDb = trackMeterClient.getAndClearAudioLevel(0).dB;
        const auto rightDb = trackMeterClient.getAndClearAudioLevel(1).dB;
        const auto peakDb = juce::jmax(leftDb, rightDb);
        if (peakDb > -90.0f)
            meterStatus.setText("Track output: audio signal detected · peak " + juce::String(peakDb, 1) + " dBFS",
                                juce::dontSendNotification);
        else
            meterStatus.setText("Track output: no recent audio activity", juce::dontSendNotification);
    }

    void refreshProjectView(const juce::String& message)
    {
        status.setText(message, juce::dontSendNotification);
        const bool ready = hasProject && edit != nullptr;
        saveButton.setEnabled(ready);
        midiExportButton.setEnabled(ready && hasPattern && patternClip != nullptr);
        addTrackButton.setEnabled(ready);
        patternButton.setEnabled(ready);
        playlistButton.setEnabled(ready);
        mixerButton.setEnabled(ready);
        masterButton.setEnabled(ready);
        tempoLabel.setEnabled(ready);
        tempoSlider.setEnabled(ready);
        stopButton.setEnabled(ready);
        if (ready)
        {
            tempoSlider.setValue(project.getInitialTempoBpm(), juce::dontSendNotification);
            if (mixerWindow != nullptr)
            {
                const auto tracks = getGeneratorTracks();
                mixerWindow->refresh(tracks.indexOf(instrumentTrack));
            }
        }
        playButton.setEnabled(ready && instrumentTrack != nullptr);
        if (!ready)
        {
            projectDetails.setText({}, juce::dontSendNotification);
            musicalDetails.setText({}, juce::dontSendNotification);
            return;
        }
        const auto meter = project.getInitialTimeSignature();
        projectDetails.setText("Project: " + project.getName() + "   ·   ID: " + project.getProjectId()
                                   + (isDirty ? "   ·   Unsaved" : ""), juce::dontSendNotification);
        musicalDetails.setText("Tracks: " + juce::String(project.getTrackCount()) + "   ·   "
                                   + juce::String(project.getInitialTempoBpm(), 1) + " BPM   ·   "
                                   + juce::String(meter.numerator) + "/" + juce::String(meter.denominator)
                                   + "   ·   Built-in 4OSC   ·   64 pattern slots per instrument (A1-H8)",
                               juce::dontSendNotification);
        resized();
    }

    void showError(const juce::String& titleText, const juce::String& message)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, titleText, message);
    }

    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    te::AudioTrack* instrumentTrack = nullptr;
    te::MidiClip* patternClip = nullptr;
    te::LevelMeterPlugin* trackMeter = nullptr;
    te::LevelMeasurer::Client trackMeterClient;
    ProjectDocument project;
    bool hasProject = false;
    bool isDirty = false;
    bool hasPattern = false;
    bool refreshingTrackSelector = false;
    static constexpr int minimumPitch = 48; // C3
    static constexpr int maximumPitch = 84; // C6
    std::array<std::array<bool, 16>, maximumPitch - minimumPitch + 1> noteGrid{};
    std::array<std::unique_ptr<juce::TextButton>, 16> steps;
    std::array<std::unique_ptr<juce::TextButton>, 8> patternBankButtons, patternSlotButtons;
    int selectedBankIndex = 0;
    int selectedSlotIndex = 0;
    juce::ComboBox pitchSelector;
    juce::TextButton pianoRollButton;
    juce::ComboBox trackSelector;
    juce::Slider levelSlider, panSlider;
    juce::Slider tempoSlider;
    juce::Label title, status, projectDetails, musicalDetails, audioDetails, generatorTitle, patternTitle,
                arrangementTitle, mixerTitle, levelLabel, panLabel, meterStatus, buildLabel, tempoLabel;
    juce::TextButton newProjectButton, openButton, saveButton, midiExportButton, audioSettingsButton, addTrackButton,
                     patternButton, playlistButton, playButton, stopButton, mixerButton, masterButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<AudioSettingsWindow> audioSettingsWindow;
    std::unique_ptr<PlaylistWindow> playlistWindow;
    std::unique_ptr<MixerWindow> mixerWindow;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<EqualiserWindow> equaliserWindow;
    juce::Rectangle<int> generatorPanelBounds;
};

class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow(juce::JUCEApplication& application)
        : DocumentWindow("Orbit", juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::allButtons), app(application)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new ProjectView(), true);
        setResizable(true, true);
        centreWithSize(1080, 680);
        setVisible(true);
    }
    void closeButtonPressed() override { app.systemRequestedQuit(); }
private:
    juce::JUCEApplication& app;
};

class OrbitApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Orbit"; }
    const juce::String getApplicationVersion() override { return "0.3.3"; }
    void initialise(const juce::String&) override { mainWindow = std::make_unique<MainWindow>(*this); }
    void shutdown() override { mainWindow.reset(); }
private:
    std::unique_ptr<MainWindow> mainWindow;
};
}

START_JUCE_APPLICATION(OrbitApplication)
