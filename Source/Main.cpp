#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

#if JUCE_MAC || JUCE_LINUX
#include <csignal>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#endif

#include "Project/ProjectDocument.h"

namespace te = tracktion::engine;

namespace
{
#if JUCE_MAC || JUCE_LINUX
char crashReportPath[PATH_MAX] = {};
char crashReportHeader[512] = "Orbit crashed while running.";
char currentVst3ScanTarget[PATH_MAX] = "(no VST3 plugin scan was active)";

void writeSignalSafe(int fd, const char* text, size_t length)
{
    while (length > 0)
    {
        const auto written = ::write(fd, text, length);
        if (written <= 0) return;
        text += written;
        length -= static_cast<size_t>(written);
    }
}

void orbitFatalSignalHandler(int signalNumber)
{
    if (crashReportPath[0] != '\0')
    {
        const int fd = ::open(crashReportPath, O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd >= 0)
        {
            size_t headerLength = 0;
            while (headerLength < sizeof(crashReportHeader) && crashReportHeader[headerLength] != '\0')
                ++headerLength;
            writeSignalSafe(fd, crashReportHeader, headerLength);
            writeSignalSafe(fd, "\nFatal signal: ", 15);
            const char* signalName = "UNKNOWN";
            if (signalNumber == SIGABRT) signalName = "SIGABRT";
            else if (signalNumber == SIGBUS) signalName = "SIGBUS";
            else if (signalNumber == SIGFPE) signalName = "SIGFPE";
            else if (signalNumber == SIGILL) signalName = "SIGILL";
            else if (signalNumber == SIGSEGV) signalName = "SIGSEGV";
            else if (signalNumber == SIGTRAP) signalName = "SIGTRAP";
            size_t signalNameLength = 0;
            while (signalName[signalNameLength] != '\0') ++signalNameLength;
            writeSignalSafe(fd, signalName, signalNameLength);
            writeSignalSafe(fd, " (", 2);
            char digits[16];
            size_t count = 0;
            unsigned int number = static_cast<unsigned int>(signalNumber);
            do { digits[count++] = static_cast<char>('0' + number % 10); number /= 10; } while (number > 0);
            while (count > 0) writeSignalSafe(fd, &digits[--count], 1);
            writeSignalSafe(fd, ")", 1);
            writeSignalSafe(fd, "\nPlugin being scanned: ", 24);
            size_t targetLength = 0;
            while (targetLength < sizeof(currentVst3ScanTarget) && currentVst3ScanTarget[targetLength] != '\0')
                ++targetLength;
            writeSignalSafe(fd, currentVst3ScanTarget, targetLength);
            static constexpr char footer[] = "\n\nThe operating system may also have created a detailed crash report in its DiagnosticReports folder.\n";
            writeSignalSafe(fd, footer, sizeof(footer) - 1);
            ::fsync(fd);
            ::close(fd);
        }
    }

    ::signal(signalNumber, SIG_DFL);
    ::raise(signalNumber);
    ::_exit(128 + signalNumber);
}

void installOrbitCrashReportHandler()
{
    const auto desktop = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    const auto stamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
    const auto path = desktop.getChildFile("Orbit-Crash-Report-" + stamp + ".log").getFullPathName();
    std::snprintf(crashReportPath, sizeof(crashReportPath), "%s", path.toRawUTF8());
    std::snprintf(crashReportHeader, sizeof(crashReportHeader),
                  "Orbit Build %s crashed.\nTime: %s\n", ProjectInfo::versionString, stamp.toRawUTF8());

    for (const int signalNumber : { SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP })
        std::signal(signalNumber, orbitFatalSignalHandler);
}

void setCurrentVst3ScanTarget(const juce::String& path)
{
    const auto utf8 = path.toRawUTF8();
    std::snprintf(currentVst3ScanTarget, sizeof(currentVst3ScanTarget), "%s", utf8);
}
#else
void installOrbitCrashReportHandler() {}
void setCurrentVst3ScanTarget(const juce::String&) {}
#endif

class ToolbarButton final : public juce::TextButton
{
public:
    enum class Symbol { none, play, stop, rewind, previous, next, record, playlist, mixer, master, settings, save, open };

    explicit ToolbarButton(Symbol buttonSymbol = Symbol::none) : symbol(buttonSymbol)
    {
        setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(24, 31, 37));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(37, 112, 143));
        setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(222, 232, 237));
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void setTextVisible(bool shouldShow) { showText = shouldShow; repaint(); }

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        auto fill = findColour(getToggleState() ? juce::TextButton::buttonOnColourId
                                                : juce::TextButton::buttonColourId);
        if (down) fill = fill.brighter(0.14f);
        else if (highlighted) fill = fill.brighter(0.08f);
        if (!isEnabled()) fill = fill.withAlpha(0.55f);
        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour::fromRGB(92, 122, 137));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        const auto iconColour = isEnabled() ? juce::Colour::fromRGB(185, 224, 238)
                                            : juce::Colour::fromRGB(125, 145, 153);
        const bool hasIcon = symbol != Symbol::none;
        const float iconSize = hasIcon ? (showText ? 14.0f : juce::jmin(18.0f, static_cast<float>(getHeight()) * 0.62f)) : 0.0f;
        const float iconX = !hasIcon ? bounds.getX() + 5.0f
                          : showText ? bounds.getX() + 7.0f
                                     : bounds.getCentreX() - iconSize * 0.5f;
        const float iconY = bounds.getCentreY() - iconSize * 0.5f;
        g.setColour(symbol == Symbol::record ? juce::Colour::fromRGB(225, 74, 67) : iconColour);
        const auto triangle = [this, &g, iconX, iconY, iconSize](bool pointsRight, bool doubleArrow)
        {
            juce::Path path;
            if (doubleArrow)
            {
                const float offset = iconSize * 0.48f;
                path.addTriangle(iconX + (pointsRight ? 1.0f : iconSize * 0.52f), iconY + 1.0f,
                                 iconX + (pointsRight ? iconSize * 0.48f : iconSize - 1.0f), iconY + iconSize * 0.5f,
                                 iconX + (pointsRight ? 1.0f : iconSize * 0.52f), iconY + iconSize - 1.0f);
                if (pointsRight)
                    path.addTriangle(iconX + offset, iconY + 1.0f, iconX + iconSize - 1.0f, iconY + iconSize * 0.5f,
                                     iconX + offset, iconY + iconSize - 1.0f);
                else
                    path.addTriangle(iconX + 1.0f, iconY + iconSize * 0.5f, iconX + iconSize - offset, iconY + 1.0f,
                                     iconX + iconSize - offset, iconY + iconSize - 1.0f);
            }
            else if (pointsRight)
                path.addTriangle(iconX + 1.0f, iconY + 1.0f, iconX + iconSize - 1.0f, iconY + iconSize * 0.5f,
                                 iconX + 1.0f, iconY + iconSize - 1.0f);
            else
                path.addTriangle(iconX + iconSize - 1.0f, iconY + 1.0f, iconX + 1.0f, iconY + iconSize * 0.5f,
                                 iconX + iconSize - 1.0f, iconY + iconSize - 1.0f);
            g.fillPath(path);
        };

        switch (symbol)
        {
            case Symbol::play: triangle(true, false); break;
            case Symbol::previous:
                g.fillRect(iconX + iconSize - 2.0f, iconY + 1.0f, 2.0f, iconSize - 2.0f);
                triangle(false, false);
                break;
            case Symbol::next:
                g.fillRect(iconX, iconY + 1.0f, 2.0f, iconSize - 2.0f);
                triangle(true, false);
                break;
            case Symbol::rewind:
                g.fillRect(iconX, iconY + 1.0f, 2.0f, iconSize - 2.0f);
                triangle(false, false);
                break;
            case Symbol::stop:
                g.fillRoundedRectangle(iconX + 2.0f, iconY + 2.0f, iconSize - 4.0f, iconSize - 4.0f, 1.5f);
                break;
            case Symbol::record:
                g.fillEllipse(iconX + 2.0f, iconY + 2.0f, iconSize - 4.0f, iconSize - 4.0f);
                break;
            case Symbol::playlist:
                for (int row = 0; row < 3; ++row)
                {
                    const float y = iconY + 2.0f + row * 4.2f;
                    g.fillRoundedRectangle(iconX, y, 4.0f, 3.0f, 0.7f);
                    g.fillRect(iconX + 6.0f, y + 0.5f, iconSize - 7.0f, 1.2f);
                }
                break;
            case Symbol::mixer:
                for (int col = 0; col < 3; ++col)
                {
                    const float x = iconX + 3.0f + col * 4.7f;
                    g.drawLine(x, iconY + 1.0f, x, iconY + iconSize - 1.0f, 1.2f);
                    g.fillRoundedRectangle(x - 2.0f, iconY + (col == 1 ? 4.0f : 9.0f), 4.0f, 3.0f, 0.8f);
                }
                break;
            case Symbol::master:
                for (int col = 0; col < 4; ++col)
                {
                    const float h = 4.0f + static_cast<float>((col * 7) % 10);
                    g.fillRoundedRectangle(iconX + 1.0f + col * 3.3f, iconY + iconSize - h,
                                           2.2f, h, 0.7f);
                }
                break;
            case Symbol::settings:
                g.drawEllipse(iconX + 3.5f, iconY + 3.5f, iconSize - 7.0f, iconSize - 7.0f, 2.0f);
                g.fillEllipse(iconX + iconSize * 0.42f, iconY + iconSize * 0.42f, iconSize * 0.16f, iconSize * 0.16f);
                for (int spoke = 0; spoke < 8; ++spoke)
                {
                    const auto angle = juce::MathConstants<float>::twoPi * spoke / 8.0f;
                    const auto center = juce::Point<float>(iconX + iconSize * 0.5f, iconY + iconSize * 0.5f);
                    g.drawLine(center.x + std::cos(angle) * 5.5f, center.y + std::sin(angle) * 5.5f,
                               center.x + std::cos(angle) * 7.5f, center.y + std::sin(angle) * 7.5f, 2.0f);
                }
                break;
            case Symbol::save:
                g.drawRoundedRectangle(iconX + 1.0f, iconY + 1.0f, iconSize - 2.0f, iconSize - 2.0f, 1.5f, 1.4f);
                g.fillRect(iconX + 4.0f, iconY + 2.0f, iconSize - 8.0f, 4.0f);
                g.drawRect(iconX + 4.0f, iconY + 8.0f, iconSize - 8.0f, iconSize - 10.0f, 1.2f);
                break;
            case Symbol::open:
                g.drawLine(iconX + 1.0f, iconY + iconSize * 0.35f, iconX + 5.0f, iconY + iconSize * 0.35f, 1.8f);
                g.drawLine(iconX + 5.0f, iconY + iconSize * 0.35f, iconX + 7.0f, iconY + iconSize * 0.5f, 1.8f);
                g.drawRoundedRectangle(iconX + 1.0f, iconY + iconSize * 0.42f, iconSize - 2.0f,
                                       iconSize * 0.45f, 1.2f, 1.5f);
                break;
            case Symbol::none: break;
        }

        if (showText)
        {
            g.setColour(findColour(getToggleState() ? juce::TextButton::textColourOnId
                                                    : juce::TextButton::textColourOffId));
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            const auto textX = hasIcon ? static_cast<int>(iconX + iconSize + 6.0f) : 3;
            const auto textWidth = hasIcon ? getWidth() - textX - 4 : getWidth() - 6;
            g.drawFittedText(getButtonText(), juce::Rectangle<int>(textX, 0, textWidth, getHeight()),
                juce::Justification::centredLeft, 1);
        }
    }

private:
    Symbol symbol;
    bool showText = true;
};

class ModeSelectorButton final : public juce::TextButton
{
public:
    ModeSelectorButton()
    {
        setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(24, 31, 37));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(37, 112, 143));
        setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(170, 190, 199));
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        auto fill = findColour(getToggleState() ? juce::TextButton::buttonOnColourId
                                                : juce::TextButton::buttonColourId);
        if (down) fill = fill.brighter(0.12f);
        else if (highlighted) fill = fill.brighter(0.07f);
        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colour::fromRGB(92, 122, 137));
        g.drawRoundedRectangle(bounds, 2.0f, 0.8f);
        g.setColour(findColour(getToggleState() ? juce::TextButton::textColourOnId
                                                : juce::TextButton::textColourOffId));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }
};

class PianoRollButton final : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        juce::TextButton::paintButton(g, highlighted, down);
        const auto x = 7.0f;
        const auto y = static_cast<float>(getHeight()) * 0.5f - 7.0f;
        const auto white = isEnabled() ? juce::Colour::fromRGB(190, 224, 238)
                                       : juce::Colour::fromRGB(125, 145, 153);
        g.setColour(white);
        g.fillRoundedRectangle(x, y, 20.0f, 14.0f, 1.5f);
        g.setColour(juce::Colour::fromRGB(74, 109, 125));
        g.drawRoundedRectangle(x, y, 20.0f, 14.0f, 1.5f, 1.0f);
        for (int key = 1; key < 7; ++key)
            g.drawVerticalLine(static_cast<int>(x + static_cast<float>(key) * 20.0f / 7.0f), y + 7.0f, y + 13.0f);
        g.setColour(juce::Colour::fromRGB(24, 39, 49));
        for (const auto key : { 1, 2, 4, 5, 6 })
        {
            const auto keyX = x + static_cast<float>(key) * 20.0f / 7.0f - 1.2f;
            g.fillRoundedRectangle(keyX, y, 2.5f, 7.5f, 0.8f);
        }
    }
};

class OutputMeterDisplay final : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour(juce::Colour::fromRGB(18, 25, 30));
        g.fillRoundedRectangle(area, 4.0f);
        g.setColour(juce::Colour::fromRGB(81, 112, 126));
        g.drawRoundedRectangle(area, 4.0f, 1.0f);
        g.setColour(juce::Colour::fromRGB(177, 200, 210));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText("OUT", getLocalBounds().removeFromLeft(27), juce::Justification::centred);
        auto meters = getLocalBounds().withTrimmedLeft(28).reduced(2, 5);
        const int gap = 3;
        const int channelWidth = (meters.getWidth() - gap) / 2;
        for (int channel = 0; channel < 2; ++channel)
        {
            auto x = meters.getX() + channel * (channelWidth + gap);
            g.setColour(juce::Colour::fromRGB(8, 15, 19));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(meters.getY()),
                                   static_cast<float>(channelWidth), static_cast<float>(meters.getHeight()), 1.5f);
            constexpr int segments = 14;
            constexpr int segmentGap = 2;
            const auto segmentHeight = juce::jmax(1, (meters.getHeight() - (segments - 1) * segmentGap) / segments);
            for (int segment = 0; segment < segments; ++segment)
            {
                const auto y = meters.getBottom() - segmentHeight - segment * (segmentHeight + segmentGap);
                const auto colour = segment >= 12 ? juce::Colour::fromRGB(184, 76, 62)
                                  : segment >= 10 ? juce::Colour::fromRGB(203, 164, 66)
                                                  : juce::Colour::fromRGB(48, 139, 158);
                g.setColour(colour.withAlpha(0.48f));
                g.fillRoundedRectangle(static_cast<float>(x + 2), static_cast<float>(y),
                                       static_cast<float>(channelWidth - 4), static_cast<float>(segmentHeight), 0.8f);
            }
        }
    }
};

class AudioSettingsWindow final : public juce::DocumentWindow
{
public:
    explicit AudioSettingsWindow(juce::AudioDeviceManager& manager)
        : DocumentWindow("Orbit Audio & MIDI Settings", juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new juce::AudioDeviceSelectorComponent(manager, 0, 0, 1, 2, true, true, true, true), true);
        setResizable(true, true);
        centreWithSize(520, 460);
    }

    void closeButtonPressed() override { setVisible(false); }
};

class SearchPathEditor final : public juce::Component, private juce::ListBoxModel
{
public:
    SearchPathEditor(juce::StringArray initialPaths,
                     std::function<void(juce::StringArray)> save,
                     std::function<void()> cancel,
                     bool showVstStartupOption = false,
                     bool scanVst3AtStartupInitially = false,
                     std::function<void(bool)> saveVstStartupOption = {})
        : paths(std::move(initialPaths)), onSave(std::move(save)), onCancel(std::move(cancel)),
          hasVstStartupOption(showVstStartupOption), onSaveVstStartupOption(std::move(saveVstStartupOption))
    {
        title.setText(showVstStartupOption
                          ? "Standard VST3 locations are included automatically. Add extra folders below."
                          : "Folders Orbit searches",
                      juce::dontSendNotification);
        title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(208, 224, 233));
        addAndMakeVisible(title);
        folderList.setModel(this);
        folderList.setRowHeight(30);
        folderList.setMultipleSelectionEnabled(true);
        addAndMakeVisible(folderList);

        scanVst3AtStartup.setButtonText("Scan VST3 plugins at startup");
        scanVst3AtStartup.setToggleState(scanVst3AtStartupInitially, juce::dontSendNotification);
        scanVst3AtStartup.setVisible(hasVstStartupOption);
        if (hasVstStartupOption) addAndMakeVisible(scanVst3AtStartup);

        addFolder.setButtonText("Add Folder");
        addFolder.onClick = [this] { chooseFolder(); };
        addAndMakeVisible(addFolder);
        removeFolder.setButtonText("Remove");
        removeFolder.onClick = [this] { removeSelectedFolders(); };
        addAndMakeVisible(removeFolder);
        saveButton.setButtonText("Save");
        saveButton.onClick = [this]
        {
            if (onSave) onSave(paths);
            if (hasVstStartupOption && onSaveVstStartupOption)
                onSaveVstStartupOption(scanVst3AtStartup.getToggleState());
            if (onCancel) onCancel();
        };
        addAndMakeVisible(saveButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this] { if (onCancel) onCancel(); };
        addAndMakeVisible(cancelButton);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        title.setBounds(area.removeFromTop(24));
        area.removeFromTop(6);
        auto footer = area.removeFromBottom(34);
        addFolder.setBounds(footer.removeFromLeft(112));
        footer.removeFromLeft(6);
        removeFolder.setBounds(footer.removeFromLeft(82));
        auto right = footer.removeFromRight(176);
        saveButton.setBounds(right.removeFromRight(92));
        right.removeFromRight(6);
        cancelButton.setBounds(right.removeFromRight(78));
        if (hasVstStartupOption)
        {
            area.removeFromBottom(6);
            scanVst3AtStartup.setBounds(area.removeFromBottom(24));
        }
        folderList.setBounds(area);
    }

private:
    int getNumRows() override { return paths.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (!juce::isPositiveAndBelow(row, paths.size())) return;
        g.fillAll(selected ? juce::Colour::fromRGB(54, 109, 137)
                           : (row % 2 == 0 ? juce::Colour::fromRGB(31, 45, 55)
                                           : juce::Colour::fromRGB(37, 52, 62)));
        g.setColour(juce::Colour::fromRGB(220, 231, 237));
        g.setFont(13.0f);
        g.drawText(paths[row], 10, 0, width - 20, height, juce::Justification::centredLeft, true);
    }

    void chooseFolder()
    {
        addFolder.setEnabled(false);
        chooser = std::make_unique<juce::FileChooser>("Add a search folder",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*");
        juce::Component::SafePointer<SearchPathEditor> safeThis(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectDirectories,
            [safeThis](const juce::FileChooser& fileChooser)
            {
                if (safeThis == nullptr) return;
                const auto folder = fileChooser.getResult();
                if (folder.isDirectory())
                {
                    const auto fullPath = folder.getFullPathName();
                    if (!safeThis->paths.contains(fullPath)) safeThis->paths.add(fullPath);
                    safeThis->folderList.updateContent();
                    safeThis->folderList.repaint();
                }
                safeThis->addFolder.setEnabled(true);
            });
    }

    void removeSelectedFolders()
    {
        auto selected = folderList.getSelectedRows();
        for (int i = selected.size(); --i >= 0;)
            if (juce::isPositiveAndBelow(selected[i], paths.size())) paths.remove(selected[i]);
        folderList.updateContent();
        folderList.repaint();
    }

    juce::StringArray paths;
    std::function<void(juce::StringArray)> onSave;
    std::function<void()> onCancel;
    juce::Label title;
    juce::ListBox folderList;
    juce::ToggleButton scanVst3AtStartup;
    juce::TextButton addFolder, removeFolder, saveButton, cancelButton;
    std::unique_ptr<juce::FileChooser> chooser;
    bool hasVstStartupOption = false;
    std::function<void(bool)> onSaveVstStartupOption;
};

class SearchPathsWindow final : public juce::DocumentWindow
{
public:
    SearchPathsWindow(const juce::String& windowTitle, juce::StringArray paths,
                      std::function<void(juce::StringArray)> onSave,
                      bool showVstStartupOption = false,
                      bool scanVst3AtStartupInitially = false,
                      std::function<void(bool)> saveVstStartupOption = {})
        : DocumentWindow(windowTitle, juce::Colour::fromRGB(20, 29, 38), juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new SearchPathEditor(std::move(paths), std::move(onSave), [this] { setVisible(false); },
                                             showVstStartupOption, scanVst3AtStartupInitially,
                                             std::move(saveVstStartupOption)), true);
        setResizable(true, true);
        centreWithSize(620, 390);
    }

    void closeButtonPressed() override { setVisible(false); }
};

class Vst3InstrumentBrowser final : public juce::DocumentWindow,
                                    private juce::ListBoxModel,
                                    private juce::Timer
{
public:
    Vst3InstrumentBrowser(juce::AudioPluginFormatManager& formats, juce::KnownPluginList& knownPlugins,
                          std::function<juce::StringArray()> getSearchPaths,
                          std::function<void()> openPathSettings,
                          std::function<void(const juce::PluginDescription&)> onInsert)
        : DocumentWindow("Insert VST3 Instrument", juce::Colour::fromRGB(20, 29, 38),
                         juce::DocumentWindow::closeButton),
          pluginFormats(formats), pluginList(knownPlugins), readPaths(std::move(getSearchPaths)),
          showPathSettings(std::move(openPathSettings)), insertPlugin(std::move(onInsert))
    {
        setUsingNativeTitleBar(true);
        list.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromRGB(25, 38, 48));
        list.setModel(this);
        list.setRowHeight(34);
        list.setMultipleSelectionEnabled(false);

        scanButton.onClick = [this] { beginScan(); };
        pathsButton.onClick = [this] { if (showPathSettings) showPathSettings(); };
        insertButton.onClick = [this]
        {
            const int selected = list.getSelectedRow();
            if (!juce::isPositiveAndBelow(selected, instruments.size())) return;
            if (insertPlugin) insertPlugin(instruments.getReference(selected));
            setVisible(false);
        };
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(&list);
        addAndMakeVisible(&scanButton);
        addAndMakeVisible(&pathsButton);
        addAndMakeVisible(&insertButton);
        addAndMakeVisible(&closeButton);
        addAndMakeVisible(&information);
        setResizable(true, true);
        setSize(640, 430);
        refreshInstrumentList();
    }

    ~Vst3InstrumentBrowser() override { stopTimer(); }
    void closeButtonPressed() override { setVisible(false); }
    bool needsInitialScan() const { return !hasStartedScan; }
    void scanNow() { beginScan(); }

private:
    int getNumRows() override { return instruments.size(); }
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (!juce::isPositiveAndBelow(row, instruments.size())) return;
        g.fillAll(selected ? juce::Colour::fromRGB(38, 111, 143) : juce::Colour::fromRGB(25, 38, 48));
        const auto& description = instruments.getReference(row);
        g.setColour(juce::Colour::fromRGB(229, 237, 241));
        g.setFont(14.0f);
        g.drawText(description.name, 10, 2, width - 20, height / 2, juce::Justification::centredLeft);
        g.setColour(juce::Colour::fromRGB(151, 178, 190));
        g.setFont(11.0f);
        g.drawText(description.manufacturerName + "  ·  VST3", 10, height / 2, width - 20,
                   height / 2 - 2, juce::Justification::centredLeft);
    }

    void resized() override
    {
        DocumentWindow::resized();
        auto bounds = getLocalBounds().reduced(12);
        auto buttons = bounds.removeFromBottom(34);
        information.setBounds(bounds.removeFromTop(48));
        list.setBounds(bounds);
        scanButton.setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        pathsButton.setBounds(buttons.removeFromLeft(132));
        buttons.removeFromLeft(8);
        insertButton.setBounds(buttons.removeFromRight(104));
        buttons.removeFromRight(8);
        closeButton.setBounds(buttons.removeFromRight(88));
    }

    void beginScan()
    {
        stopTimer();
        scanner.reset();
        hasStartedScan = true;
        auto* format = findVst3Format();
        if (format == nullptr)
        {
            information.setText("VST3 hosting is unavailable in this build.", juce::dontSendNotification);
            return;
        }

        auto searchPath = format->getDefaultLocationsToSearch();
        const auto configuredPaths = readPaths ? readPaths() : juce::StringArray{};
        for (const auto& path : configuredPaths)
        {
            juce::File folder(path);
            if (folder.isDirectory())
                searchPath.add(folder);
        }
        if (searchPath.getNumPaths() == 0)
        {
            information.setText("Add one or more VST3 folders, then scan again.", juce::dontSendNotification);
            if (showPathSettings) showPathSettings();
            return;
        }

        const auto stamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
        scanLogFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                          .getChildFile("Orbit-VST3-Scan-" + stamp + ".log");
        scanLog = std::make_unique<juce::FileOutputStream>(scanLogFile);
        if (scanLog != nullptr && scanLog->openedOk())
        {
            scanLog->writeText("Orbit VST3 scan started: " + juce::Time::getCurrentTime().toString(true, true)
                               + "\nSearch folders (standard plus custom):\n", false, false, nullptr);
            for (int i = 0; i < searchPath.getNumPaths(); ++i)
                scanLog->writeText("  " + searchPath[i].getFullPathName() + "\n", false, false, nullptr);
            const auto blacklistedFiles = pluginList.getBlacklistedFiles();
            scanLog->writeText("\nPreviously blacklisted after a scan crash: "
                               + juce::String(blacklistedFiles.size()) + "\n", false, false, nullptr);
            for (const auto& blacklisted : blacklistedFiles)
                scanLog->writeText("  " + blacklisted + "\n", false, false, nullptr);
            scanLog->flush();
        }

        auto deadMansPedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("Orbit").getChildFile("vst3-scan-dead-mans-pedal.txt");
        deadMansPedal.getParentDirectory().createDirectory();
        scanner = std::make_unique<juce::PluginDirectoryScanner>(pluginList, *format, searchPath, true,
                                                                 deadMansPedal, false);
        information.setText("Scanning VST3 instruments... " + juce::String(searchPath.getNumPaths())
                            + " search folder(s). Scan log: Desktop/" + scanLogFile.getFileName(),
                            juce::dontSendNotification);
        scanTotalCandidates = format->searchPathsForPlugins(searchPath, true, false).size();
        scanCompletedCandidates = 0;
        if (scanLog != nullptr && scanLog->openedOk())
        {
            scanLog->writeText("\nPlugin bundles found to check: " + juce::String(scanTotalCandidates) + "\n",
                               false, false, nullptr);
            scanLog->flush();
        }
        scanButton.setEnabled(false);
        startTimerHz(15);
    }

    void timerCallback() override
    {
        if (scanner == nullptr) { stopTimer(); return; }
        const auto currentPlugin = scanner->getNextPluginFileThatWillBeScanned();
        setCurrentVst3ScanTarget(currentPlugin);
        if (currentPlugin.isNotEmpty())
        {
            ++scanCompletedCandidates;
            information.setText("Scanning " + juce::File(currentPlugin).getFileName()
                                    + "  (" + juce::String(scanCompletedCandidates) + "/"
                                    + juce::String(scanTotalCandidates) + ")",
                                juce::dontSendNotification);
            if (scanLog != nullptr && scanLog->openedOk())
            {
                scanLog->writeText("Checking: " + currentPlugin + "\n", false, false, nullptr);
                scanLog->flush();
            }
        }
        juce::String currentFile;
        const bool moreToScan = scanner->scanNextFile(true, currentFile);
        if (moreToScan)
        {
            const auto progress = juce::roundToInt(scanner->getProgress() * 100.0f);
            if (currentPlugin.isNotEmpty() && scanLog != nullptr && scanLog->openedOk())
            {
                scanLog->writeText("Checked: " + currentPlugin + "\n", false, false, nullptr);
                scanLog->flush();
            }
            information.setText("Scanning " + juce::File(currentPlugin).getFileName()
                                    + "  (" + juce::String(progress) + "% complete)",
                                juce::dontSendNotification);
            return;
        }
        if (currentPlugin.isNotEmpty() && scanLog != nullptr && scanLog->openedOk())
            scanLog->writeText("Checked: " + currentPlugin + "\n", false, false, nullptr);
        const auto failedFiles = scanner->getFailedFiles();
        if (scanLog != nullptr && scanLog->openedOk())
        {
            scanLog->writeText("\nFailed plugin bundles: " + juce::String(failedFiles.size()) + "\n",
                               false, false, nullptr);
            for (const auto& failed : failedFiles)
                scanLog->writeText("  " + failed + "\n", false, false, nullptr);
            scanLog->writeText("\nVST3 instruments available after scan:\n", false, false, nullptr);
            for (const auto& instrument : instruments)
                scanLog->writeText("  " + instrument.name + " — " + instrument.manufacturerName + "\n",
                                   false, false, nullptr);
        }
        scanner.reset();
        stopTimer();
        scanButton.setEnabled(true);
        refreshInstrumentList();
        int discoveredVst3Types = 0;
        for (const auto& description : pluginList.getTypes())
            if (description.pluginFormatName == "VST3") ++discoveredVst3Types;
        const auto summary = "\nScan complete. VST3 plugins discovered: " + juce::String(discoveredVst3Types)
            + "; instruments available: " + juce::String(instruments.size())
            + "; bundles that failed: " + juce::String(failedFiles.size()) + "\nLog: " + scanLogFile.getFullPathName() + "\n";
        if (scanLog != nullptr && scanLog->openedOk())
        {
            scanLog->writeText(summary, false, false, nullptr);
            scanLog->flush();
            scanLog.reset();
        }
        information.setText("Done. " + juce::String(instruments.size()) + " VST3 instrument(s) ready; "
                            + juce::String(failedFiles.size()) + " bundle(s) failed. Plugin names are in the Desktop scan log.",
                            juce::dontSendNotification);
        setCurrentVst3ScanTarget("(no VST3 plugin scan was active)");
    }

    juce::AudioPluginFormat* findVst3Format() const
    {
        for (int i = 0; i < pluginFormats.getNumFormats(); ++i)
            if (auto* format = pluginFormats.getFormat(i); format != nullptr && format->getName() == "VST3")
                return format;
        return nullptr;
    }

    void refreshInstrumentList()
    {
        instruments.clear();
        for (const auto& description : pluginList.getTypes())
            if (description.pluginFormatName == "VST3" && description.isInstrument)
                instruments.add(description);
        std::sort(instruments.begin(), instruments.end(), [](const auto& a, const auto& b)
        {
            return a.name.compareIgnoreCase(b.name) < 0;
        });
        list.updateContent();
        insertButton.setEnabled(!instruments.isEmpty());
    }

    juce::AudioPluginFormatManager& pluginFormats;
    juce::KnownPluginList& pluginList;
    std::function<juce::StringArray()> readPaths;
    std::function<void()> showPathSettings;
    std::function<void(const juce::PluginDescription&)> insertPlugin;
    juce::ListBox list{"VST3 instruments", this};
    juce::Label information;
    juce::TextButton scanButton{"Scan"}, pathsButton{"VST Paths..."}, insertButton{"Insert"}, closeButton{"Close"};
    juce::Array<juce::PluginDescription> instruments;
    std::unique_ptr<juce::PluginDirectoryScanner> scanner;
    juce::File scanLogFile;
    std::unique_ptr<juce::FileOutputStream> scanLog;
    int scanTotalCandidates = 0;
    int scanCompletedCandidates = 0;
    bool hasStartedScan = false;
};

class PlaylistGrid final : public juce::Component
{
public:
    static constexpr int barCount = 512;
    PlaylistGrid()
    {
        zoomOut.setButtonText("H-");
        zoomOut.setTooltip("Zoom out in the Playlist.");
        zoomOut.onClick = [this] { setHorizontalZoom(horizontalZoom - 0.1f); };
        addAndMakeVisible(zoomOut);
        zoomIn.setButtonText("+");
        zoomIn.setTooltip("Zoom in in the Playlist.");
        zoomIn.onClick = [this] { setHorizontalZoom(horizontalZoom + 0.1f); };
        addAndMakeVisible(zoomIn);
        verticalZoomOut.setButtonText("V-");
        verticalZoomOut.setTooltip("Reduce Playlist track height.");
        verticalZoomOut.onClick = [this] { setVerticalZoom(verticalZoom - 0.1f); };
        addAndMakeVisible(verticalZoomOut);
        verticalZoomIn.setButtonText("V+");
        verticalZoomIn.setTooltip("Increase Playlist track height.");
        verticalZoomIn.onClick = [this] { setVerticalZoom(verticalZoom + 0.1f); };
        addAndMakeVisible(verticalZoomIn);
        pencilTool.setButtonText("Draw");
        pencilTool.setTooltip("Draw tool: click or drag to place the selected pattern, replacing a clip when needed.");
        pencilTool.setClickingTogglesState(false);
        pencilTool.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
        addAndMakeVisible(pencilTool);
        selectTool.setButtonText("Move");
        selectTool.setTooltip("Selection tool: drag clips to move them.");
        selectTool.setClickingTogglesState(false);
        addAndMakeVisible(selectTool);
        pencilTool.setToggleState(true, juce::dontSendNotification);
        pencilTool.onClick = [this] { pencilTool.setToggleState(true, juce::dontSendNotification); selectTool.setToggleState(false, juce::dontSendNotification); if (onToolChanged) onToolChanged(true); };
        selectTool.onClick = [this] { pencilTool.setToggleState(false, juce::dontSendNotification); selectTool.setToggleState(true, juce::dontSendNotification); if (onToolChanged) onToolChanged(false); };
        // The controls are hosted in the fixed Playlist toolbar so they stay
        // reachable while the timeline itself is scrolled.
        for (auto* control : { &zoomOut, &zoomIn, &verticalZoomOut, &verticalZoomIn, &pencilTool, &selectTool })
            control->setVisible(false);
    }

    // Leave enough room for readable generator names and M/S/R controls before
    // the first Playlist bar begins.
    int labelWidth() const { return 250; }
    int headerHeight() const { return 34; }
    int rowHeight() const { return juce::roundToInt(46.0f * verticalZoom); }
    int barWidth() const { return juce::jmax(4, juce::roundToInt(92.0f * horizontalZoom)); }
    int contentWidth() const { return labelWidth() + barCount * barWidth(); }
    int contentHeight() const
    {
        const auto rows = juce::jmax(1, trackCount ? trackCount() : 0);
        return juce::jmax(headerHeight() + rows * rowHeight(), headerHeight() + visibleArea.getHeight());
    }
    float currentHorizontalZoom() const { return horizontalZoom; }
    float currentVerticalZoom() const { return verticalZoom; }
    void setPlayheadPosition(double seconds, double bpm, bool active)
    {
        playheadVisible = active;
        playheadX = labelWidth() + (seconds / (240.0 / juce::jmax(20.0, bpm))) * barWidth();
        repaint();
    }

    void setLoopRange(int startBar, int lengthBars, bool enabled)
    {
        loopStartBar = juce::jmax(1, startBar);
        loopLengthBars = juce::jmax(1, lengthBars);
        loopRangeEnabled = enabled;
        repaint();
    }

    void setHorizontalZoom(float nextZoom)
    {
        horizontalZoom = juce::jlimit(0.035f, 1.55f, nextZoom);
        refreshLayout();
    }

    std::function<void(int, int)> setLoopRangeFromRuler;

    void setVerticalZoom(float nextZoom)
    {
        verticalZoom = juce::jlimit(0.65f, 1.5f, nextZoom);
        refreshLayout();
    }

    void setDrawMode(bool shouldDraw)
    {
        pencilTool.setToggleState(shouldDraw, juce::dontSendNotification);
        selectTool.setToggleState(!shouldDraw, juce::dontSendNotification);
        if (onToolChanged) onToolChanged(shouldDraw);
    }

    void refreshLayout()
    {
        if (onZoomChanged) onZoomChanged();
    }
    void setViewportArea(juce::Rectangle<int> area)
    {
        visibleArea = area;
        setSize(contentWidth(), contentHeight());
        refreshCellButtons();
    }
    bool pencilActive() const { return pencilTool.getToggleState(); }
    bool isAutomationMode() const { return automationMode; }
    void setAutomationMode(bool enabled) { automationMode = enabled; repaint(); refreshCellButtons(); }
    bool isSnapToGrid() const { return snapToGrid; }
    void setSnapToGrid(bool enabled) { snapToGrid = enabled; }
    bool isLightTheme() const { return lightTheme; }
    void setLightTheme(bool enabled)
    {
        lightTheme = enabled;
        repaint();
    }
    std::function<void(bool)> onAutomationModeChanged;
    std::function<void(bool)> onThemeChanged;

    std::function<void(bool)> onToolChanged;
    std::function<void()> onPaintFinished;
    std::function<bool()> isPencilToolActive;
    std::function<void(int, bool)> setTrackMute;
    std::function<bool(int)> getTrackMute;
    std::function<void(int, bool)> setTrackSolo;
    std::function<bool(int)> getTrackSolo;
    std::function<void(int, bool)> setTrackRecordArm;
    std::function<bool(int)> getTrackRecordArm;

    std::function<void()> onZoomChanged;
    std::function<int()> trackCount;
    std::function<juce::String(int)> trackName;
    std::function<juce::String(int, int)> clipName;
    std::function<std::vector<juce::Point<int>>(int, int)> clipNotes;
    std::function<void(int, int)> cellClicked;
    std::function<void(int, int)> cellPainted;
    std::function<void(int, int, int, int)> clipDragged;
    std::function<void(int, int, juce::Component*)> showPatternMenu;
    std::function<void(int)> openGeneratorForTrack;
    std::function<void(int, int)> cellDeleted;
    std::function<void(int, int, int)> scrollPattern;
    std::function<void(int, juce::Component*)> trackContextMenu;
    std::function<bool(int)> isAudioTrack;
    std::function<int(int, int)> audioClipStartBar;
    std::function<int(int, int)> audioClipLengthBars;
    std::function<void(int, int, juce::Component*)> audioContextMenu;
    std::function<juce::Colour(int)> getTrackColour;
    std::function<float(int, int)> getSongAutomationValue;
    std::function<void(int, int, float)> setSongAutomationPoint;

private:
    float horizontalZoom = 1.0f;
    float verticalZoom = 1.0f;
    juce::TextButton zoomOut, zoomIn, verticalZoomOut, verticalZoomIn, pencilTool, selectTool;
    void resized() override
    {
        const auto h = headerHeight();
        pencilTool.setBounds(5, 4, 40, juce::jmax(20, h - 8));
        selectTool.setBounds(47, 4, 40, juce::jmax(20, h - 8));
        zoomOut.setButtonText("H-");
        zoomIn.setButtonText("H+");
        zoomOut.setBounds(labelWidth() - 112, 4, 25, juce::jmax(20, h - 8));
        zoomIn.setBounds(labelWidth() - 86, 4, 25, juce::jmax(20, h - 8));
        zoomOut.setButtonText("H-");
        verticalZoomOut.setButtonText("V-");
        verticalZoomOut.setBounds(labelWidth() - 59, 4, 29, juce::jmax(20, h - 8));
        verticalZoomIn.setBounds(labelWidth() - 29, 4, 25, juce::jmax(20, h - 8));
    }

public:

    class PlaylistCellButton final : public juce::TextButton
    {
    public:
        std::function<void()> onActivate;
        std::function<void(int, int)> onDragToCell;
        std::function<void(int, int, juce::Component*)> onPatternMenu;
        std::function<void(int)> onOpenGenerator;
        std::function<void(int, int)> onDeleteClip;
        std::function<void(int, int, int)> onPatternScroll;
        std::function<void(int, int, juce::Component*)> onAudioMenu;
        std::function<bool()> pencilMode;
        std::function<void(int, int)> paintCell;
        std::function<void()> paintFinished;
        std::function<bool()> sourceHasClip;
        std::function<bool()> automationMode;
        std::function<float(int, int)> getAutomationValue;
        std::function<void(int, int, float)> setAutomationValue;
        bool audioTrack = false;
        int barWidthForDrag = 92;
        int rowHeightForDrag = 46;

        PlaylistCellButton()
        {
            setClickingTogglesState(false);
            setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
            setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
            onClick = [this]
            {
                if (automationMode && automationMode()) return;
                if (!dragGesture && pencilMode && pencilMode())
                {
                    if (sourceHasClip && sourceHasClip())
                    {
                        if (onActivate) onActivate();
                    }
                    else if (paintCell) paintCell(trackIndex, barIndex);
                }
                else if (!dragGesture && onActivate) onActivate();
            };
        }

        void configure(int track, int bar, const juce::String& trackLabel,
                       const juce::String& currentClip,
                       std::function<void(int, int)> activate,
                       std::function<void(int, int)> paint,
                       std::function<void()> finishPaint,
                       std::function<bool()> pencil,
                       std::function<void(int, int, int, int)> drag,
                       int scaledBarWidth, int scaledRowHeight,
                       std::function<bool()> automationModeFn,
                       std::function<float(int, int)> getAutomation,
                       std::function<void(int, int, float)> setAutomation)
        {
            barIndex = bar;
            trackIndex = track;
            barWidthForDrag = scaledBarWidth;
            rowHeightForDrag = scaledRowHeight;
            dragGesture = false;
            onActivate = [activateCallback = std::move(activate), track, bar]
            {
                if (activateCallback) activateCallback(track, bar);
            };
            onDragToCell = [dragCallback = std::move(drag), track, bar](int destinationTrack, int destinationBar)
            {
                if (dragCallback) dragCallback(track, bar, destinationTrack, destinationBar);
            };
            paintCell = std::move(paint);
            paintFinished = std::move(finishPaint);
            pencilMode = std::move(pencil);
            automationMode = std::move(automationModeFn);
            getAutomationValue = std::move(getAutomation);
            setAutomationValue = std::move(setAutomation);
            sourceHasClip = [currentClip] { return currentClip.isNotEmpty(); };
            const auto barLabel = "Bar " + juce::String(bar + 1);
            setButtonText(currentClip.isNotEmpty() ? currentClip : barLabel);
            setTitle(trackLabel + ", " + barLabel);
            setDescription(currentClip.isNotEmpty()
                ? "Contains pattern " + currentClip + ". Drag it to move the clip."
                : "Empty playlist cell. Activate to place the selected pattern here.");
            setTooltip(trackLabel + ", " + barLabel + (currentClip.isNotEmpty()
                ? " — " + currentClip : " — empty; activate to place the selected pattern"));
        }

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (!event.mods.isRightButtonDown() && automationMode && automationMode())
            {
                automationGesture = true;
                writeAutomationPoint(event);
                return;
            }
            if (event.mods.isRightButtonDown() || event.mods.isPopupMenu())
            {
                if (audioTrack && onAudioMenu)
                {
                    onAudioMenu(trackIndex, barIndex, this);
                    rightClickHandled = true;
                    return;
                }
                if (sourceHasClip && sourceHasClip() && onDeleteClip)
                    onDeleteClip(trackIndex, barIndex);
                rightEraseGesture = true;
                rightClickHandled = true;
                lastErasedTrack = trackIndex;
                lastErasedBar = barIndex;
                return;
            }
            dragGesture = false;
            dragSourceHasClip = sourceHasClip && sourceHasClip();
            juce::TextButton::mouseDown(event);
        }

        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (automationGesture)
            {
                writeAutomationPoint(event);
                return;
            }
            if (rightEraseGesture)
            {
                if (event.getDistanceFromDragStart() >= 8 && onDeleteClip)
                {
                    const auto pointInGrid = event.getEventRelativeTo(getParentComponent()).position;
                    const auto* grid = dynamic_cast<const PlaylistGrid*>(getParentComponent());
                    const int destinationTrack = grid != nullptr
                        ? static_cast<int>((pointInGrid.y - grid->headerHeight()) / rowHeightForDrag) : -1;
                    const int destinationBar = grid != nullptr
                        ? static_cast<int>((pointInGrid.x - grid->labelWidth()) / barWidthForDrag) : -1;
                    if (destinationTrack >= 0 && destinationBar >= 0
                        && (destinationTrack != lastErasedTrack || destinationBar != lastErasedBar))
                    {
                        onDeleteClip(destinationTrack, destinationBar);
                        lastErasedTrack = destinationTrack;
                        lastErasedBar = destinationBar;
                    }
                }
                return;
            }
            if (event.getDistanceFromDragStart() >= 8)
            {
                if (pencilMode && pencilMode())
                {
                    dragGesture = true;
                    if (paintCell) paintCell(trackIndex, barIndex);
                    const auto pointInGrid = event.getEventRelativeTo(getParentComponent()).position;
                    const auto* grid = dynamic_cast<const PlaylistGrid*>(getParentComponent());
                    const int destinationTrack = grid != nullptr
                        ? static_cast<int>((pointInGrid.y - grid->headerHeight()) / rowHeightForDrag) : -1;
                    const int destinationBar = grid != nullptr
                        ? static_cast<int>((pointInGrid.x - grid->labelWidth()) / barWidthForDrag) : -1;
                    if (destinationTrack >= 0 && destinationBar >= 0 && paintCell)
                        paintCell(destinationTrack, destinationBar);
                }
                else if (dragSourceHasClip) dragGesture = true;
            }
            juce::TextButton::mouseDrag(event);
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            if (automationGesture)
            {
                automationGesture = false;
                if (paintFinished) paintFinished();
                return;
            }
            if (rightClickHandled)
            {
                rightClickHandled = false;
                rightEraseGesture = false;
                lastErasedTrack = lastErasedBar = -1;
                return;
            }
            const auto pointInGrid = event.getEventRelativeTo(getParentComponent()).position;
            const auto* grid = dynamic_cast<const PlaylistGrid*>(getParentComponent());
            const int destinationTrack = grid != nullptr
                ? static_cast<int>((pointInGrid.y - grid->headerHeight()) / rowHeightForDrag) : -1;
            const int destinationBar = grid != nullptr
                ? static_cast<int>((pointInGrid.x - grid->labelWidth()) / barWidthForDrag) : -1;
            const bool shouldMove = dragGesture && dragSourceHasClip && !(pencilMode && pencilMode())
                && destinationTrack >= 0 && destinationBar >= 0;
            juce::TextButton::mouseUp(event);
            if (shouldMove && onDragToCell)
                onDragToCell(destinationTrack, destinationBar);
            if (pencilMode && pencilMode() && paintFinished) paintFinished();
            dragGesture = false;
            dragSourceHasClip = false;
        }

        void mouseDoubleClick(const juce::MouseEvent& event) override
        {
            if (audioTrack && onAudioMenu)
            {
                onAudioMenu(trackIndex, barIndex, this);
                return;
            }
            if (sourceHasClip && sourceHasClip() && onOpenGenerator)
                onOpenGenerator(trackIndex);
            juce::TextButton::mouseDoubleClick(event);
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            if (sourceHasClip && sourceHasClip() && onPatternScroll && std::abs(wheel.deltaY) > 0.001f)
                onPatternScroll(trackIndex, barIndex, wheel.deltaY > 0.0f ? -1 : 1);
            else
                juce::TextButton::mouseWheelMove(event, wheel);
        }

    private:
        void writeAutomationPoint(const juce::MouseEvent& event)
        {
            if (!setAutomationValue) return;
            const auto point = event.getEventRelativeTo(getParentComponent()).position.toInt();
            auto* grid = dynamic_cast<PlaylistGrid*>(getParentComponent());
            if (grid == nullptr) return;
            const auto row = (point.y - grid->headerHeight()) / rowHeightForDrag;
            const auto bar = (point.x - grid->labelWidth()) / barWidthForDrag;
            if (row < 0 || bar < 0) return;
            const auto cellX = (point.x - grid->labelWidth()) % barWidthForDrag;
            const auto cellY = (point.y - grid->headerHeight()) % rowHeightForDrag;
            const auto substep = juce::jlimit(0, 15, cellX * 16 / barWidthForDrag);
            const auto value = 1.0f - juce::jlimit(0.0f, 1.0f,
                static_cast<float>(cellY) / static_cast<float>(juce::jmax(1, rowHeightForDrag)));
            setAutomationValue(row, bar * 16 + substep, value);
            grid->repaint();
        }

        int barIndex = -1;
        int trackIndex = -1;
        bool dragGesture = false;
        bool automationGesture = false;
        bool dragSourceHasClip = false;
        bool rightClickHandled = false;
        bool rightEraseGesture = false;
        int lastErasedTrack = -1;
        int lastErasedBar = -1;
    };

    class TrackControls final : public juce::Component
    {
    public:
        juce::TextButton mute { "M" }, solo { "S" }, record { "R" };
        TrackControls()
        {
            for (auto* button : { &mute, &solo, &record })
            {
                button->setClickingTogglesState(true);
                button->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 51, 62));
                button->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(164, 72, 66));
                button->setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(197, 214, 224));
                button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
                addAndMakeVisible(*button);
            }
        }
        void resized() override
        {
            auto area = getLocalBounds().reduced(1, 9);
            mute.setBounds(area.removeFromLeft(25)); area.removeFromLeft(2);
            solo.setBounds(area.removeFromLeft(25)); area.removeFromLeft(2);
            record.setBounds(area.removeFromLeft(25));
        }
    };

    void paint(juce::Graphics& g) override
    {
        const auto labelWidthPx = labelWidth();
        const auto headerHeightPx = headerHeight();
        const auto rowHeightPx = rowHeight();
        const auto barWidthPx = barWidth();
        g.fillAll(lightTheme ? juce::Colour::fromRGB(177, 187, 202)
                             : juce::Colour::fromRGB(20, 29, 38));
        g.setColour(lightTheme ? juce::Colour::fromRGB(42, 53, 63)
                               : juce::Colour::fromRGB(29, 43, 55));
        g.fillRect(0, 0, getWidth(), headerHeightPx);
        if (loopRangeEnabled)
        {
            const auto rangeX = labelWidthPx + (loopStartBar - 1) * barWidthPx;
            const auto rangeWidth = loopLengthBars * barWidthPx;
            g.setColour(juce::Colour::fromRGB(54, 151, 190).withAlpha(0.22f));
            g.fillRect(rangeX, 0, rangeWidth, headerHeightPx);
            g.setColour(juce::Colour::fromRGB(80, 184, 220));
            g.drawRect(rangeX, 1, rangeWidth, headerHeightPx - 2, 2);
        }
        g.setColour(lightTheme ? juce::Colour::fromRGB(220, 229, 236)
                               : juce::Colour::fromRGB(178, 198, 211));
        g.setFont(14.0f);
        g.drawText("GENERATOR", 8, 0, labelWidthPx - 112, headerHeightPx, juce::Justification::centredLeft);
        // Keep a number on every visible bar. The ruler's stronger guide is
        // controlled separately by highlightEveryBars below.
        const auto rulerLabelInterval = 1;
        const auto clipBounds = g.getClipBounds();
        const auto firstVisibleBar = juce::jlimit(0, barCount,
            (clipBounds.getX() - labelWidthPx) / juce::jmax(1, barWidthPx));
        const auto lastVisibleBar = juce::jlimit(firstVisibleBar, barCount,
            (clipBounds.getRight() - labelWidthPx + barWidthPx - 1) / juce::jmax(1, barWidthPx));
        for (int bar = firstVisibleBar; bar < lastVisibleBar; ++bar)
        {
            if (bar % rulerLabelInterval == 0)
            {
                const bool majorBar = highlightEveryBars > 1 && bar % highlightEveryBars == 0;
                g.setColour(majorBar ? juce::Colour::fromRGB(239, 246, 250)
                                     : juce::Colour::fromRGB(196, 215, 226));
                const auto label = juce::String(bar + 1);
                g.setFont(barWidthPx < 18 ? 8.0f : majorBar ? 11.0f : 10.0f);
                g.drawText(label, labelWidthPx + bar * barWidthPx, 0, barWidthPx, headerHeightPx,
                           juce::Justification::centred);
            }
        }

        const auto count = trackCount ? trackCount() : 0;
        const auto displayedRowCount = juce::jmax(count,
            (clipBounds.getBottom() - headerHeightPx + rowHeightPx - 1) / juce::jmax(1, rowHeightPx));
        const auto firstVisibleRow = juce::jlimit(0, count,
            (clipBounds.getY() - headerHeightPx) / juce::jmax(1, rowHeightPx));
        const auto lastVisibleRow = juce::jlimit(firstVisibleRow, count,
            (clipBounds.getBottom() - headerHeightPx + rowHeightPx - 1) / juce::jmax(1, rowHeightPx));
        for (int row = firstVisibleRow; row < lastVisibleRow; ++row)
        {
            const auto y = headerHeightPx + row * rowHeightPx;
            const auto trackTint = getTrackColour ? getTrackColour(row) : juce::Colour{};
            const auto rowBase = lightTheme
                ? (row % 2 == 0 ? juce::Colour::fromRGB(169, 181, 196) : juce::Colour::fromRGB(183, 193, 207))
                : (row % 2 == 0 ? juce::Colour::fromRGB(25, 38, 49) : juce::Colour::fromRGB(22, 33, 43));
            g.setColour(trackTint.isTransparent() ? rowBase : rowBase.overlaidWith(trackTint.withAlpha(0.25f)));
            g.fillRect(0, y, getWidth(), rowHeightPx);
            g.setColour(lightTheme ? juce::Colour::fromRGB(45, 57, 69)
                                   : juce::Colour::fromRGB(178, 198, 211));
            g.drawFittedText(juce::String(row + 1) + "   " + (trackName ? trackName(row) : juce::String{}),
                             8, y, labelWidthPx - 108, rowHeightPx,
                             juce::Justification::centredLeft, 1, 0.72f);
            for (int bar = firstVisibleBar; bar < lastVisibleBar; ++bar)
            {
                const auto x = labelWidthPx + bar * barWidthPx;
                const bool majorBar = highlightEveryBars > 1 && bar % highlightEveryBars == 0;
                g.setColour(majorBar
                    ? (lightTheme ? juce::Colour::fromRGB(247, 249, 251) : juce::Colour::fromRGB(111, 151, 169))
                    : (lightTheme ? juce::Colour::fromRGB(125, 139, 155) : juce::Colour::fromRGB(68, 88, 101)));
                g.drawRect(x, y, barWidthPx, rowHeightPx, 1);
                const auto label = clipName ? clipName(row, bar) : juce::String{};
                const bool audioRow = isAudioTrack && isAudioTrack(row);
                const int audioStart = audioRow && audioClipStartBar ? audioClipStartBar(row, bar) : bar;
                if (label.isNotEmpty() && !audioRow)
                {
                    const int clipBars = audioRow && audioClipLengthBars
                        ? juce::jmax(1, audioClipLengthBars(row, bar)) : 1;
                    const int clipX = labelWidthPx + audioStart * barWidthPx;
                    const int clipWidth = juce::jmin(contentWidth() - clipX, clipBars * barWidthPx);
                    const auto clipFill = trackTint.isTransparent()
                        ? juce::Colour::fromRGB(39, 125, 165)
                        : trackTint.interpolatedWith(juce::Colour::fromRGB(39, 125, 165), 0.35f);
                    g.setColour(clipFill.withAlpha(0.72f));
                    g.fillRoundedRectangle(static_cast<float>(clipX + 3), static_cast<float>(y + 4),
                                           static_cast<float>(juce::jmax(1, clipWidth - 6)), static_cast<float>(rowHeightPx - 8), 5.0f);
                    if (audioRow)
                    {
                        const auto midY = y + rowHeightPx / 2;
                        g.setColour(juce::Colour::fromRGB(178, 229, 244).withAlpha(0.85f));
                        juce::Path waveform;
                        waveform.startNewSubPath(static_cast<float>(clipX + 8), static_cast<float>(midY));
                        const int samples = juce::jlimit(8, 80, clipWidth / 4);
                        for (int sample = 1; sample <= samples; ++sample)
                        {
                            const auto px = clipX + 8 + sample * juce::jmax(1, clipWidth - 16) / samples;
                            const auto phase = static_cast<float>(sample * 2.41 + row * 3.17);
                            const auto envelope = 0.22f + 0.78f * std::sin(juce::MathConstants<float>::pi
                                * static_cast<float>(sample) / static_cast<float>(samples));
                            const auto amplitude = envelope * (0.18f + 0.32f * std::abs(std::sin(phase)));
                            waveform.lineTo(static_cast<float>(px), static_cast<float>(midY)
                                + static_cast<float>(rowHeightPx) * amplitude * std::sin(phase * 2.7f));
                        }
                        g.strokePath(waveform, juce::PathStrokeType(1.2f));
                    }
                    // Draw the MIDI preview after the translucent clip body so
                    // note marks remain visible, like Orion's Playlist blocks.
                    if (!audioRow && showPatternNotes && clipNotes)
                    {
                        const auto notes = clipNotes(row, bar);
                        const auto noteWidth = juce::jmax(2.0f, static_cast<float>(barWidthPx) / 64.0f * 2.4f);
                        g.setColour(juce::Colour::fromRGB(15, 30, 40).withAlpha(0.88f));
                        for (const auto note : notes)
                        {
                            const auto noteX = x + 5 + note.x * (barWidthPx - 12) / 64;
                            const auto noteY = y + 7 + (36 - note.y) * (rowHeightPx - 17) / 36;
                            g.fillRect(static_cast<float>(noteX), static_cast<float>(noteY), noteWidth,
                                       juce::jmax(1.5f, static_cast<float>(rowHeightPx) / 18.0f));
                        }
                    }
                    if (showPatternLabels || audioRow)
                    {
                        g.setColour(juce::Colours::white);
                        g.drawText(label, clipX + 5, y + rowHeightPx - 15, juce::jmax(8, clipWidth - 10), 11,
                                   juce::Justification::centredLeft);
                    }
                }
            }
            if (isAudioTrack && isAudioTrack(row) && audioClipStartBar)
                for (int bar = firstVisibleBar; bar < lastVisibleBar; ++bar)
                {
                    const auto startBar = audioClipStartBar(row, bar);
                    const bool clippedAtLeftEdge = bar == firstVisibleBar && startBar >= 0 && startBar < firstVisibleBar;
                    if (startBar != bar && !clippedAtLeftEdge) continue;
                    const auto label = clipName ? clipName(row, bar) : juce::String{};
                    if (label.isEmpty()) continue;
                    const auto clipBars = audioClipLengthBars ? juce::jmax(1, audioClipLengthBars(row, bar)) : 1;
                    const auto clipOriginX = labelWidthPx + startBar * barWidthPx;
                    const auto clipEndX = juce::jmin(contentWidth(), clipOriginX + clipBars * barWidthPx);
                    const auto clipX = juce::jmax(labelWidthPx + firstVisibleBar * barWidthPx, clipOriginX);
                    const auto clipWidth = clipEndX - clipX;
                    const auto clipFill = trackTint.isTransparent()
                        ? juce::Colour::fromRGB(39, 125, 165)
                        : trackTint.interpolatedWith(juce::Colour::fromRGB(39, 125, 165), 0.35f);
                    g.setColour(clipFill.withAlpha(0.82f));
                    g.fillRoundedRectangle(static_cast<float>(clipX + 3), static_cast<float>(y + 4),
                        static_cast<float>(juce::jmax(1, clipWidth - 6)), static_cast<float>(rowHeightPx - 8), 5.0f);
                    const auto midY = y + rowHeightPx / 2;
                    g.setColour(juce::Colour::fromRGB(178, 229, 244).withAlpha(0.85f));
                    juce::Path waveform;
                    waveform.startNewSubPath(static_cast<float>(clipX + 8), static_cast<float>(midY));
                    const int samples = juce::jlimit(8, 80, clipWidth / 4);
                    for (int sample = 1; sample <= samples; ++sample)
                    {
                        const auto px = clipX + 8 + sample * juce::jmax(1, clipWidth - 16) / samples;
                        const auto phase = static_cast<float>(sample * 2.41 + row * 3.17);
                        const auto envelope = 0.22f + 0.78f * std::sin(juce::MathConstants<float>::pi
                            * static_cast<float>(sample) / static_cast<float>(samples));
                        const auto amplitude = envelope * (0.18f + 0.32f * std::abs(std::sin(phase)));
                        waveform.lineTo(static_cast<float>(px), static_cast<float>(midY)
                            + static_cast<float>(rowHeightPx) * amplitude * std::sin(phase * 2.7f));
                    }
                    g.strokePath(waveform, juce::PathStrokeType(1.2f));
                    g.setColour(juce::Colours::white);
                    if (!clippedAtLeftEdge)
                        g.drawText(label, clipX + 5, y + rowHeightPx - 15, juce::jmax(8, clipWidth - 10), 11,
                                   juce::Justification::centredLeft);
                }
        }
        // Keep the timeline grid painted below the last generator row so an
        // empty project still reads as a Playlist, not an unpainted canvas.
        for (int row = juce::jmax(count, firstVisibleRow); row < displayedRowCount; ++row)
        {
            const auto y = headerHeightPx + row * rowHeightPx;
            const auto rowBase = lightTheme
                ? (row % 2 == 0 ? juce::Colour::fromRGB(169, 181, 196) : juce::Colour::fromRGB(183, 193, 207))
                : (row % 2 == 0 ? juce::Colour::fromRGB(25, 38, 49) : juce::Colour::fromRGB(22, 33, 43));
            g.setColour(rowBase);
            g.fillRect(0, y, getWidth(), rowHeightPx);
            for (int bar = firstVisibleBar; bar < lastVisibleBar; ++bar)
            {
                const auto x = labelWidthPx + bar * barWidthPx;
                const bool majorBar = highlightEveryBars > 1 && bar % highlightEveryBars == 0;
                g.setColour(majorBar
                    ? (lightTheme ? juce::Colour::fromRGB(247, 249, 251) : juce::Colour::fromRGB(111, 151, 169))
                    : (lightTheme ? juce::Colour::fromRGB(125, 139, 155) : juce::Colour::fromRGB(68, 88, 101)));
                g.drawRect(x, y, barWidthPx, rowHeightPx, 1);
            }
        }
        // Extend the configured ruler interval through the track rows after
        // clips are drawn so the major guides remain easy to see.
        if (highlightEveryBars > 1)
        {
            const auto firstGuide = ((firstVisibleBar + highlightEveryBars - 1) / highlightEveryBars)
                                    * highlightEveryBars;
            for (int bar = firstGuide; bar < lastVisibleBar; bar += highlightEveryBars)
            {
                const auto x = labelWidthPx + bar * barWidthPx;
            g.setColour(juce::Colour::fromRGB(177, 207, 222).withAlpha(0.62f));
                g.drawVerticalLine(x, headerHeightPx, headerHeightPx + displayedRowCount * rowHeightPx);
                g.setColour(lightTheme ? juce::Colour::fromRGB(255, 255, 255)
                                       : juce::Colour::fromRGB(205, 225, 236));
                g.drawVerticalLine(x, 0.0f, static_cast<float>(headerHeightPx));
            }
        }
        if (playheadVisible)
        {
            g.setColour(juce::Colour::fromRGB(232, 72, 68));
            g.fillRect(juce::roundToInt(playheadX) - 1, headerHeightPx, 2,
                       juce::jmax(0, displayedRowCount * rowHeightPx));
        }
        if (automationMode && getSongAutomationValue)
        {
            for (int row = firstVisibleRow; row < lastVisibleRow; ++row)
            {
                juce::Path curve;
                const auto y = headerHeightPx + row * rowHeightPx;
                const auto firstStep = juce::jlimit(0, barCount * 16,
                    (clipBounds.getX() - labelWidthPx) * 16 / juce::jmax(1, barWidthPx));
                const auto lastStep = juce::jlimit(firstStep, barCount * 16,
                    ((clipBounds.getRight() - labelWidthPx) * 16 + barWidthPx - 1) / juce::jmax(1, barWidthPx));
                for (int step = firstStep; step < lastStep; ++step)
                {
                    const auto bar = step / 16;
                    const auto substep = step % 16;
                    const auto x = static_cast<float>(labelWidthPx + bar * barWidthPx
                        + substep * barWidthPx / 16 + barWidthPx / 32);
                    const auto value = juce::jlimit(0.0f, 1.0f, getSongAutomationValue(row, step));
                    const auto pointY = static_cast<float>(y + rowHeightPx - 5
                        - value * juce::jmax(1, rowHeightPx - 16));
                    if (step == firstStep) curve.startNewSubPath(x, pointY); else curve.lineTo(x, pointY);
                }
                g.setColour(juce::Colour::fromRGB(73, 207, 237).withAlpha(0.85f));
                g.strokePath(curve, juce::PathStrokeType(2.0f));
            }
        }
        g.setColour(juce::Colour::fromRGB(65, 94, 112));
        g.drawRect(getLocalBounds(), 1);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isLeftButtonDown() && event.y < headerHeight() && event.x >= labelWidth())
        {
            loopDragAnchorBar = barAtX(event.x);
            draggingLoopRange = true;
            updateLoopFromRuler(event.x);
            return;
        }
        const auto row = (event.y - headerHeight()) / rowHeight();
        const auto bar = (event.x - labelWidth()) / barWidth();
        if ((event.mods.isPopupMenu() || event.mods.isRightButtonDown())
            && event.x < labelWidth() && event.y >= headerHeight()
            && juce::isPositiveAndBelow(row, trackCount ? trackCount() : 0))
        {
            if (trackContextMenu) trackContextMenu(row, this);
            return;
        }
        if (event.x < labelWidth() || event.y < headerHeight() || !trackCount
            || !juce::isPositiveAndBelow(row, trackCount()) || !juce::isPositiveAndBelow(bar, barCount))
            return;
        dragSourceRow = row;
        dragSourceBar = bar;
        dragSourceHasClip = clipName && clipName(row, bar).isNotEmpty();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        updateLoopFromRuler(event.x);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (draggingLoopRange)
        {
            updateLoopFromRuler(event.x);
            draggingLoopRange = false;
            return;
        }
        if (dragSourceRow < 0 || dragSourceBar < 0) return;
        const auto destinationRow = (event.y - headerHeight()) / rowHeight();
        const auto destinationBar = (event.x - labelWidth()) / barWidth();
        const bool moved = dragSourceHasClip && event.getDistanceFromDragStart() >= 8
            && juce::isPositiveAndBelow(destinationRow, trackCount ? trackCount() : 0)
            && juce::isPositiveAndBelow(destinationBar, barCount);
        if (moved)
        {
            if (clipDragged && (destinationRow != dragSourceRow || destinationBar != dragSourceBar))
                clipDragged(dragSourceRow, dragSourceBar, destinationRow, destinationBar);
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
        while (static_cast<int>(trackControls.size()) < rows)
        {
            auto controls = std::make_unique<TrackControls>();
            addAndMakeVisible(*controls);
            trackControls.push_back(std::move(controls));
        }
        while (static_cast<int>(trackControls.size()) > rows)
            trackControls.pop_back();
        const auto firstRow = juce::jlimit(0, rows,
            (visibleArea.getY() - headerHeight()) / juce::jmax(1, rowHeight()) - 1);
        const auto lastRow = juce::jlimit(firstRow, rows,
            (visibleArea.getBottom() - headerHeight() + rowHeight() - 1) / juce::jmax(1, rowHeight()) + 1);
        const auto firstBar = juce::jlimit(0, barCount,
            (visibleArea.getX() - labelWidth()) / juce::jmax(1, barWidth()) - 1);
        const auto lastBar = juce::jlimit(firstBar, barCount,
            (visibleArea.getRight() - labelWidth() + barWidth() - 1) / juce::jmax(1, barWidth()) + 1);

        std::set<size_t> visibleKeys;
        for (int row = firstRow; row < lastRow; ++row)
            for (int bar = firstBar; bar < lastBar; ++bar)
                visibleKeys.insert(static_cast<size_t>(row * barCount + bar));
        for (auto it = cellButtons.begin(); it != cellButtons.end();)
            if (!visibleKeys.contains(it->first)) it = cellButtons.erase(it); else ++it;

        for (int row = 0; row < rows; ++row)
        {
            auto& controls = *trackControls[static_cast<size_t>(row)];
            controls.setBounds(labelWidth() - 100, headerHeight() + row * rowHeight(), 78, rowHeight());
            controls.mute.setToggleState(getTrackMute && getTrackMute(row), juce::dontSendNotification);
            controls.solo.setToggleState(getTrackSolo && getTrackSolo(row), juce::dontSendNotification);
            controls.record.setToggleState(getTrackRecordArm && getTrackRecordArm(row), juce::dontSendNotification);
            auto* controlPtr = &controls;
            controls.mute.onClick = [this, row, controlPtr] { if (setTrackMute) setTrackMute(row, controlPtr->mute.getToggleState()); };
            controls.solo.onClick = [this, row, controlPtr] { if (setTrackSolo) setTrackSolo(row, controlPtr->solo.getToggleState()); };
            controls.record.onClick = [this, row, controlPtr] { if (setTrackRecordArm) setTrackRecordArm(row, controlPtr->record.getToggleState()); };
            controls.record.setTooltip("Arm this generator for MIDI recording.");
            controls.mute.setTooltip("Mute this generator.");
            controls.solo.setTooltip("Solo this generator.");
        }

        for (int row = firstRow; row < lastRow; ++row)
            for (int bar = firstBar; bar < lastBar; ++bar)
            {
                const auto index = static_cast<size_t>(row * barCount + bar);
                auto found = cellButtons.find(index);
                if (found == cellButtons.end())
                {
                    auto button = std::make_unique<PlaylistCellButton>();
                    addAndMakeVisible(*button);
                    found = cellButtons.emplace(index, std::move(button)).first;
                }
                auto& button = *found->second;
                const auto currentClip = clipName ? clipName(row, bar) : juce::String{};
                button.configure(row, bar, trackName ? trackName(row) : juce::String("Generator ") + juce::String(row + 1),
                                 currentClip, cellClicked, cellPainted, onPaintFinished, isPencilToolActive,
                                 clipDragged, barWidth(), rowHeight(),
                                 [this] { return automationMode; }, getSongAutomationValue, setSongAutomationPoint);
                button.onPatternMenu = showPatternMenu;
                button.onOpenGenerator = openGeneratorForTrack;
                button.onDeleteClip = cellDeleted;
                button.onPatternScroll = scrollPattern;
                button.audioTrack = isAudioTrack && isAudioTrack(row);
                button.onAudioMenu = audioContextMenu;
                button.setBounds(labelWidth() + bar * barWidth(), headerHeight() + row * rowHeight(), barWidth(), rowHeight());
            }
    }

    void showSettings()
    {
        juce::AlertWindow dialog("Playlist Settings", "Set snapping, pattern previews, and Playlist automation.",
                                 juce::MessageBoxIconType::NoIcon);
        juce::ToggleButton snapToggle("Snap to grid");
        juce::ToggleButton followToggle("Follow play cursor");
        juce::ToggleButton notesToggle("Show notes in patterns");
        juce::ToggleButton velocitiesToggle("Show note velocities");
        juce::ToggleButton navigationToggle("Show navigation bar");
        juce::ToggleButton automationToggle("Song automation mode");
        juce::ToggleButton lightThemeToggle("Light Playlist grid");
        std::array<juce::ToggleButton*, 7> toggles {
            &snapToggle, &followToggle, &notesToggle, &velocitiesToggle, &navigationToggle,
            &automationToggle, &lightThemeToggle
        };
        const std::array<bool, 7> toggleValues {
            snapToGrid, followPlayCursor, showPatternNotes, showPatternVelocities, showNavigationBar,
            automationMode, lightTheme
        };
        for (size_t i = 0; i < toggles.size(); ++i)
        {
            toggles[i]->setBounds(0, 0, 300, 26);
            toggles[i]->setToggleState(toggleValues[i], juce::dontSendNotification);
            dialog.addCustomComponent(toggles[i]);
        }
        const std::array<int, 4> highlightChoices { 1, 2, 4, 8 };
        const std::array<int, 5> snapChoices { 1, 2, 4, 8, 16 };
        int selectedHighlight = highlightEveryBars;
        int selectedSnap = snapDivision;
        juce::TextButton highlightButton, snapDivisionButton;
        const auto updateHighlightText = [&]
        {
            highlightButton.setButtonText("Highlight ruler: every " + juce::String(selectedHighlight) + " bars  ›");
        };
        const auto updateSnapText = [&]
        {
            selectedSnap = juce::jlimit(1, static_cast<int>(snapChoices.size()), selectedSnap);
            snapDivisionButton.setButtonText("Snap interval: " + juce::String(snapChoices[static_cast<size_t>(selectedSnap - 1)]) + " steps  ›");
        };
        highlightButton.setBounds(0, 0, 300, 28);
        snapDivisionButton.setBounds(0, 0, 300, 28);
        updateHighlightText();
        updateSnapText();
        highlightButton.onClick = [&]
        {
            auto found = std::find(highlightChoices.begin(), highlightChoices.end(), selectedHighlight);
            const auto index = found == highlightChoices.end() ? 0 : static_cast<int>(std::distance(highlightChoices.begin(), found));
            selectedHighlight = highlightChoices[static_cast<size_t>((index + 1) % static_cast<int>(highlightChoices.size()))];
            updateHighlightText();
        };
        snapDivisionButton.onClick = [&]
        {
            selectedSnap = selectedSnap % static_cast<int>(snapChoices.size()) + 1;
            updateSnapText();
        };
        dialog.addCustomComponent(&highlightButton);
        dialog.addCustomComponent(&snapDivisionButton);
        dialog.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;
        const auto previousAutomation = automationMode;
        snapToGrid = snapToggle.getToggleState();
        followPlayCursor = followToggle.getToggleState();
        showPatternNotes = notesToggle.getToggleState();
        showPatternVelocities = velocitiesToggle.getToggleState();
        showNavigationBar = navigationToggle.getToggleState();
        automationMode = automationToggle.getToggleState();
        lightTheme = lightThemeToggle.getToggleState();
        if (previousAutomation != automationMode && onAutomationModeChanged)
            onAutomationModeChanged(automationMode);
        if (onThemeChanged) onThemeChanged(lightTheme);
        highlightEveryBars = selectedHighlight;
        snapDivision = selectedSnap;
        repaint();
    }

private:
    int barAtX(int x) const
    {
        return juce::jlimit(1, barCount, ((x - labelWidth()) / barWidth()) + 1);
    }

    void updateLoopFromRuler(int x)
    {
        if (!draggingLoopRange || !setLoopRangeFromRuler) return;
        const auto endBar = barAtX(x);
        const auto first = juce::jmin(loopDragAnchorBar, endBar);
        const auto last = juce::jmax(loopDragAnchorBar, endBar);
        setLoopRangeFromRuler(first, last - first + 1);
    }

    int dragSourceRow = -1;
    int dragSourceBar = -1;
    bool dragSourceHasClip = false;
    int highlightEveryBars = 4;
    bool lightTheme = false;
    bool showPatternLabels = true;
    bool automationMode = false;
    bool snapToGrid = true;
    bool followPlayCursor = false;
    bool showPatternNotes = true;
    bool showPatternVelocities = false;
    bool showNavigationBar = true;
    int snapDivision = 5;
    bool playheadVisible = false;
    double playheadX = 0.0;
    int loopStartBar = 1;
    int loopLengthBars = 4;
    bool loopRangeEnabled = false;
    bool draggingLoopRange = false;
    int loopDragAnchorBar = 1;
    juce::Rectangle<int> visibleArea { 0, 0, 1800, 800 };
    std::map<size_t, std::unique_ptr<PlaylistCellButton>> cellButtons;
    std::vector<std::unique_ptr<TrackControls>> trackControls;
};

class PlaylistIconButton final : public juce::Button
{
public:
    enum class Kind { draw, select, minus, plus, snap, settings };
    explicit PlaylistIconButton(Kind buttonKind) : juce::Button("Playlist control"), kind(buttonKind)
    {
        setClickingTogglesState(kind == Kind::draw || kind == Kind::select || kind == Kind::snap);
    }
    void setKind(Kind buttonKind) { kind = buttonKind; repaint(); }
    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        const auto active = getToggleState();
        g.setColour(active ? juce::Colour::fromRGB(36, 132, 174)
                           : juce::Colour::fromRGB(35, 51, 62));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour::fromRGB(112, 151, 170));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        g.setColour(juce::Colour::fromRGB(220, 237, 245).withAlpha(down ? 0.65f : highlighted ? 1.0f : 0.9f));
        const auto cx = bounds.getCentreX(), cy = bounds.getCentreY();
        const auto stroke = juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path icon;
        switch (kind)
        {
            case Kind::draw:
                icon.startNewSubPath(cx - 6.0f, cy + 5.0f); icon.lineTo(cx + 4.5f, cy - 5.5f);
                icon.lineTo(cx + 6.0f, cy - 4.0f); icon.lineTo(cx - 4.5f, cy + 6.5f);
                g.strokePath(icon, stroke); g.fillEllipse(cx + 3.5f, cy - 7.0f, 4.0f, 4.0f);
                break;
            case Kind::select:
                icon.startNewSubPath(cx - 4.5f, cy - 6.0f); icon.lineTo(cx + 4.5f, cy);
                icon.lineTo(cx, cy + 1.0f); icon.lineTo(cx - 1.0f, cy + 5.0f); icon.closeSubPath();
                g.fillPath(icon); break;
            case Kind::minus:
                g.drawLine(cx - 5.5f, cy, cx + 5.5f, cy, 2.0f); break;
            case Kind::plus:
                g.drawLine(cx - 5.0f, cy, cx + 5.0f, cy, 2.0f);
                g.drawLine(cx, cy - 5.0f, cx, cy + 5.0f, 2.0f); break;
            case Kind::snap:
                for (int i = -1; i <= 1; ++i)
                {
                    g.drawLine(cx - 5.0f, cy + i * 4.0f, cx + 5.0f, cy + i * 4.0f, 0.8f);
                    g.drawLine(cx + i * 4.0f, cy - 5.0f, cx + i * 4.0f, cy + 5.0f, 0.8f);
                }
                break;
            case Kind::settings:
                icon.startNewSubPath(cx - 5.0f, cy + 5.0f); icon.lineTo(cx + 2.0f, cy - 2.0f);
                icon.lineTo(cx + 5.0f, cy - 5.0f); icon.lineTo(cx + 7.0f, cy - 3.0f);
                icon.lineTo(cx + 4.0f, cy); icon.lineTo(cx - 3.0f, cy + 7.0f);
                g.strokePath(icon, stroke); g.drawEllipse(cx - 6.5f, cy + 3.5f, 3.5f, 3.5f, 1.3f);
                break;
        }
    }
private:
    Kind kind;
};

class PlaylistWindowIconButton final : public juce::Button
{
public:
    enum class Kind { minimize, restore, maximize, close };
    explicit PlaylistWindowIconButton(Kind buttonKind) : juce::Button("Playlist window control"), kind(buttonKind) {}
    void setKind(Kind buttonKind) { kind = buttonKind; repaint(); }
    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour((down ? juce::Colour::fromRGB(48, 113, 143)
                          : highlighted ? juce::Colour::fromRGB(48, 78, 95)
                                        : juce::Colour::fromRGB(35, 51, 62)));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(juce::Colour::fromRGB(112, 151, 170));
        g.drawRoundedRectangle(bounds, 3.0f, 0.8f);
        g.setColour(juce::Colour::fromRGB(220, 237, 245));
        const auto cx = bounds.getCentreX(), cy = bounds.getCentreY();
        if (kind == Kind::minimize || kind == Kind::restore)
        {
            g.drawLine(cx - 5.0f, cy, cx + 5.0f, cy, 1.6f);
            if (kind == Kind::restore)
                g.drawLine(cx, cy - 5.0f, cx, cy + 5.0f, 1.6f);
        }
        else if (kind == Kind::maximize)
        {
            g.drawRect(juce::Rectangle<float>(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f), 1.4f);
        }
        else
        {
            g.drawLine(cx - 4.0f, cy - 4.0f, cx + 4.0f, cy + 4.0f, 1.6f);
            g.drawLine(cx + 4.0f, cy - 4.0f, cx - 4.0f, cy + 4.0f, 1.6f);
        }
    }
private:
    Kind kind;
};


class PlaylistSurface final : public juce::Component
{
public:
    explicit PlaylistSurface(PlaylistGrid* playlistGrid) : grid(playlistGrid)
    {
        setOpaque(true);
        viewport.setGrid(grid);
        viewport.setViewedComponent(grid, true);
        addAndMakeVisible(viewport);
        configure(drawButton, PlaylistIconButton::Kind::draw, "Draw pattern", [this]
        {
            grid->setDrawMode(true);
            drawButton.setToggleState(true, juce::dontSendNotification);
            moveButton.setToggleState(false, juce::dontSendNotification);
        });
        configure(moveButton, PlaylistIconButton::Kind::select, "Select and move clips", [this]
        {
            grid->setDrawMode(false);
            drawButton.setToggleState(false, juce::dontSendNotification);
            moveButton.setToggleState(true, juce::dontSendNotification);
        });
        drawButton.setToggleState(true, juce::dontSendNotification);
        configure(horizontalOut, PlaylistIconButton::Kind::minus, "Zoom out horizontally", [this] { grid->setHorizontalZoom(horizontalZoom() - 0.1f); });
        configure(horizontalIn, PlaylistIconButton::Kind::plus, "Zoom in horizontally", [this] { grid->setHorizontalZoom(horizontalZoom() + 0.1f); });
        configure(verticalOut, PlaylistIconButton::Kind::minus, "Reduce track height", [this] { grid->setVerticalZoom(verticalZoom() - 0.1f); });
        configure(verticalIn, PlaylistIconButton::Kind::plus, "Increase track height", [this] { grid->setVerticalZoom(verticalZoom() + 0.1f); });
        configure(snapButton, PlaylistIconButton::Kind::snap, "Snap to grid", [this]
        {
            grid->setSnapToGrid(snapButton.getToggleState());
        });
        configure(settingsButton, PlaylistIconButton::Kind::settings, "Playlist settings", [this]
        {
            grid->showSettings();
            snapButton.setToggleState(grid->isSnapToGrid(), juce::dontSendNotification);
        });
        grid->onAutomationModeChanged = [this](bool enabled)
        {
            if (onAutomationModeChanged) onAutomationModeChanged(enabled);
        };
        snapButton.setClickingTogglesState(true);
        snapButton.setToggleState(true, juce::dontSendNotification);
        grid->setSnapToGrid(true);
        for (auto* button : { &drawButton, &moveButton, &horizontalOut, &horizontalIn, &snapButton, &settingsButton })
            addAndMakeVisible(*button);
        addAndMakeVisible(verticalOut);
        addAndMakeVisible(verticalIn);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto footer = area.removeFromBottom(30).reduced(4, 2);
        settingsButton.setBounds(footer.removeFromRight(26)); footer.removeFromRight(2);
        snapButton.setBounds(footer.removeFromRight(26)); footer.removeFromRight(2);
        horizontalIn.setBounds(footer.removeFromRight(26)); footer.removeFromRight(2);
        horizontalOut.setBounds(footer.removeFromRight(26)); footer.removeFromRight(2);
        moveButton.setBounds(footer.removeFromRight(26)); footer.removeFromRight(2);
        drawButton.setBounds(footer.removeFromRight(26));
        // Place track-height zoom beside the lower-right toolbar, matching
        // the paired vertical controls in the Orion Playlist.
        verticalOut.setBounds(getWidth() - 27, juce::jmax(0, getHeight() - 30 - 48), 23, 22);
        verticalIn.setBounds(getWidth() - 27, juce::jmax(0, getHeight() - 30 - 24), 23, 22);
        area.removeFromRight(30);
        viewport.setBounds(area);
    }
    void paint(juce::Graphics& g) override
    {
        // The Playlist is a workspace view; do not let the floating generator
        // panel beneath it show through unused rows or the bottom toolbar.
        g.fillAll(lightTheme ? juce::Colour::fromRGB(177, 187, 202)
                             : juce::Colour::fromRGB(20, 29, 38));
    }
    void setAutomationMode(bool enabled)
    {
        grid->setAutomationMode(enabled);
    }
    void setLightTheme(bool enabled)
    {
        lightTheme = enabled;
        viewport.setLightTheme(enabled);
        repaint();
    }

private:
    static void configure(PlaylistIconButton& button, PlaylistIconButton::Kind kind,
                          const juce::String& tooltip, std::function<void()> action)
    {
        button.setKind(kind);
        button.setTooltip(tooltip);
        button.setDescription(tooltip);
        button.setClickingTogglesState(false);
        button.onClick = std::move(action);
    }

    float horizontalZoom() const { return grid->currentHorizontalZoom(); }
    float verticalZoom() const { return grid->currentVerticalZoom(); }
    class PlaylistViewport final : public juce::Viewport
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.fillAll(lightTheme ? juce::Colour::fromRGB(177, 187, 202)
                                 : juce::Colour::fromRGB(20, 29, 38));
        }
        void setLightTheme(bool enabled) { lightTheme = enabled; repaint(); }
        void setGrid(PlaylistGrid* playlistGrid) { grid = playlistGrid; }
        void visibleAreaChanged(const juce::Rectangle<int>& area) override
        {
            if (grid != nullptr) grid->setViewportArea(area);
        }
    private:
        PlaylistGrid* grid = nullptr;
        bool lightTheme = false;
    };
    PlaylistGrid* grid = nullptr;
    PlaylistViewport viewport;
    bool lightTheme = false;
    PlaylistIconButton drawButton { PlaylistIconButton::Kind::draw }, moveButton { PlaylistIconButton::Kind::select },
        horizontalOut { PlaylistIconButton::Kind::minus }, horizontalIn { PlaylistIconButton::Kind::plus },
        verticalOut { PlaylistIconButton::Kind::minus }, verticalIn { PlaylistIconButton::Kind::plus },
        snapButton { PlaylistIconButton::Kind::snap }, settingsButton { PlaylistIconButton::Kind::settings };
public:
    std::function<void(bool)> onAutomationModeChanged;
};


class PlaylistWindow final : public juce::Component
{
public:
    class ResizeGrip final : public juce::Component
    {
    public:
        enum class Corner { topLeft, topRight, bottomLeft, bottomRight };
        explicit ResizeGrip(Corner gripCorner) : corner(gripCorner) {}
        std::function<void()> onStarted;
        std::function<void(juce::Point<int>)> onDragged;
        std::function<void()> onEnded;
        void mouseDown(const juce::MouseEvent&) override { if (onStarted) onStarted(); }
        void mouseDrag(const juce::MouseEvent& e) override { if (onDragged) onDragged(e.getOffsetFromDragStart()); }
        void mouseUp(const juce::MouseEvent&) override { if (onEnded) onEnded(); }
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour::fromRGB(112, 179, 201));
            const bool left = corner == Corner::topLeft || corner == Corner::bottomLeft;
            const bool top = corner == Corner::topLeft || corner == Corner::topRight;
            const float x0 = left ? 0.0f : static_cast<float>(getWidth());
            const float y0 = top ? 0.0f : static_cast<float>(getHeight());
            const float sx = left ? 1.0f : -1.0f;
            const float sy = top ? 1.0f : -1.0f;
            for (int inset = 5; inset <= 13; inset += 4)
                g.drawLine(x0 + sx * static_cast<float>(inset), y0,
                           x0, y0 + sy * static_cast<float>(inset), 1.5f);
        }
    private:
        Corner corner;
    };

    PlaylistWindow(std::function<int()> getTrackCount,
                   std::function<juce::String(int)> getTrackName,
                   std::function<juce::String(int, int)> getClipName,
                   std::function<void(int, int)> onCellClicked,
                   std::function<void(int, int)> onCellPainted,
                   std::function<void()> onPaintFinished,
                   std::function<bool(int)> getMute,
                   std::function<void(int, bool)> setMute,
                   std::function<bool(int)> getSolo,
                   std::function<void(int, bool)> setSolo,
                   std::function<bool(int)> getRecordArm,
                   std::function<void(int, bool)> setRecordArm,
                   std::function<void(int, int, int, int)> onClipDragged,
                   std::function<void(int, int, juce::Component*)> onPatternMenu,
                   std::function<void(int)> onOpenGenerator,
                   std::function<void(int, int)> onCellDeleted,
                   std::function<void(int, int, int)> onPatternScroll)
    {
        setOpaque(true);
        title.setText("Playlist", juce::dontSendNotification);
        title.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(222, 235, 242));
        title.setJustificationType(juce::Justification::centred);
        title.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(title);
        for (auto* button : { &minimizeButton, &maximizeButton, &closeButton })
        {
            button->setTooltip(button == &closeButton ? "Close Playlist"
                               : button == &maximizeButton ? "Maximize or restore Playlist" : "Minimize Playlist");
            addAndMakeVisible(*button);
        }
        minimizeButton.onClick = [this] { setMinimized(!minimized); };
        maximizeButton.onClick = [this] { if (onMaximizeToggled) onMaximizeToggled(); };
        closeButton.onClick = [this] { if (onClose) onClose(); else setVisible(false); };
        for (auto corner : { ResizeGrip::Corner::topLeft, ResizeGrip::Corner::topRight,
                             ResizeGrip::Corner::bottomLeft, ResizeGrip::Corner::bottomRight })
        {
            auto grip = std::make_unique<ResizeGrip>(corner);
            grip->setTitle("Resize Playlist");
            const auto cursor = (corner == ResizeGrip::Corner::topLeft || corner == ResizeGrip::Corner::bottomRight)
                ? juce::MouseCursor::TopLeftCornerResizeCursor : juce::MouseCursor::TopRightCornerResizeCursor;
            grip->setMouseCursor(cursor);
            grip->onStarted = [this] { resizeStartBounds = getBounds(); };
            grip->onDragged = [this, corner](juce::Point<int> delta)
            {
                if (!onResizedByDrag) return;
                auto resized = resizeStartBounds;
                const bool left = corner == ResizeGrip::Corner::topLeft || corner == ResizeGrip::Corner::bottomLeft;
                const bool top = corner == ResizeGrip::Corner::topLeft || corner == ResizeGrip::Corner::topRight;
                if (left)
                {
                    const auto newLeft = juce::jmin(resizeStartBounds.getRight() - 440, resizeStartBounds.getX() + delta.x);
                    resized.setLeft(newLeft);
                }
                else resized.setWidth(juce::jmax(440, resizeStartBounds.getWidth() + delta.x));
                if (top)
                {
                    const auto newTop = juce::jmin(resizeStartBounds.getBottom() - 250, resizeStartBounds.getY() + delta.y);
                    resized.setTop(newTop);
                }
                else resized.setHeight(juce::jmax(250, resizeStartBounds.getHeight() + delta.y));
                onResizedByDrag(resized);
            };
            grip->onEnded = [] {};
            addAndMakeVisible(*grip);
            resizeGrips.push_back(std::move(grip));
        }
        grid = new PlaylistGrid();
        grid->trackCount = std::move(getTrackCount);
        grid->trackName = std::move(getTrackName);
        grid->clipName = std::move(getClipName);
        grid->cellClicked = std::move(onCellClicked);
        grid->cellPainted = std::move(onCellPainted);
        grid->onPaintFinished = std::move(onPaintFinished);
        grid->clipDragged = std::move(onClipDragged);
        grid->showPatternMenu = std::move(onPatternMenu);
        grid->openGeneratorForTrack = std::move(onOpenGenerator);
        grid->cellDeleted = std::move(onCellDeleted);
        grid->scrollPattern = std::move(onPatternScroll);
        grid->getTrackMute = std::move(getMute);
        grid->setTrackMute = std::move(setMute);
        grid->getTrackSolo = std::move(getSolo);
        grid->setTrackSolo = std::move(setSolo);
        grid->getTrackRecordArm = std::move(getRecordArm);
        grid->setTrackRecordArm = std::move(setRecordArm);
        grid->isPencilToolActive = [this] { return grid != nullptr && grid->pencilActive(); };
        grid->onToolChanged = [](bool) {};
        grid->onZoomChanged = [this] { refresh(); };
        surface = std::make_unique<PlaylistSurface>(grid);
        addAndMakeVisible(*surface);
        setVisible(false);
    }

    std::function<void(juce::Point<int>)> onDragged;
    std::function<void(juce::Rectangle<int>)> onResizedByDrag;
    std::function<void(bool)> onMinimizeChanged;
    std::function<void()> onMaximizeToggled, onClose;
    bool isMinimized() const { return minimized; }
    void setMinimized(bool shouldMinimize)
    {
        if (minimized == shouldMinimize) return;
        minimized = shouldMinimize;
        minimizeButton.setKind(minimized ? PlaylistWindowIconButton::Kind::restore
                                         : PlaylistWindowIconButton::Kind::minimize);
        minimizeButton.setTooltip(minimized ? "Restore Playlist" : "Minimize Playlist");
        if (onMinimizeChanged) onMinimizeChanged(minimized);
        resized();
    }
    void setMaximized(bool shouldMaximize)
    {
        maximized = shouldMaximize;
    }
    bool isMaximized() const { return maximized; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.eventComponent == this && e.y < titleBarHeight)
        {
            dragOrigin = getPosition();
            if (onDragged) onDragged(dragOrigin);
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.eventComponent == this && e.getMouseDownY() < titleBarHeight && onDragged)
            onDragged(dragOrigin + e.getOffsetFromDragStart());
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (e.eventComponent == this && e.y < titleBarHeight && onMaximizeToggled)
            onMaximizeToggled();
    }

    void resized() override
    {
        auto header = getLocalBounds().removeFromTop(titleBarHeight);
        closeButton.setBounds(getWidth() - 48, 2, 22, titleBarHeight - 4);
        maximizeButton.setBounds(getWidth() - 72, 2, 22, titleBarHeight - 4);
        minimizeButton.setBounds(getWidth() - 96, 2, 22, titleBarHeight - 4);
        header.removeFromRight(100);
        title.setBounds(header);
        surface->setVisible(!minimized);
        for (auto& grip : resizeGrips) grip->setVisible(!minimized && !maximized);
        if (minimized)
        {
            surface->setBounds(0, titleBarHeight, 0, 0);
            return;
        }
        auto content = getLocalBounds().withTrimmedTop(titleBarHeight);
        surface->setBounds(content);
        const int gripSize = 16;
        resizeGrips[0]->setBounds(0, 0, gripSize, gripSize);
        resizeGrips[1]->setBounds(getWidth() - gripSize, 0, gripSize, gripSize);
        resizeGrips[2]->setBounds(0, getHeight() - gripSize, gripSize, gripSize);
        resizeGrips[3]->setBounds(getWidth() - gripSize, getHeight() - gripSize, gripSize, gripSize);
        // PlaylistSurface owns the full client area and would otherwise sit
        // above the corner handles, making the lower resize corners inert.
        for (auto& grip : resizeGrips) grip->toFront(false);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(20, 29, 38));
        g.setColour(juce::Colour::fromRGB(32, 42, 52));
        g.fillRect(0, 0, getWidth(), titleBarHeight);
        g.setColour(juce::Colour::fromRGB(151, 194, 216));
        g.drawRect(getLocalBounds().toFloat().reduced(0.5f), 1.0f);
    }

    void refresh()
    {
        if (grid == nullptr) return;
        grid->setSize(grid->contentWidth(), grid->contentHeight());
        grid->refreshCellButtons();
        grid->repaint();
    }
    void repaintGrid()
    {
        if (grid != nullptr) grid->repaint();
    }

    void setPlayheadPosition(double seconds, double bpm, bool active)
    {
        if (grid != nullptr) grid->setPlayheadPosition(seconds, bpm, active);
    }
    void setLoopRange(int startBar, int lengthBars, bool enabled)
    {
        if (grid != nullptr) grid->setLoopRange(startBar, lengthBars, enabled);
    }
    void setLoopRangeFromRuler(std::function<void(int, int)> callback)
    {
        if (grid != nullptr) grid->setLoopRangeFromRuler = std::move(callback);
    }
    void showSettings() { if (grid != nullptr) grid->showSettings(); }
    void setClipPreview(std::function<std::vector<juce::Point<int>>(int, int)> provider)
    {
        if (grid != nullptr) grid->clipNotes = std::move(provider);
    }
    void setSongAutomationCallbacks(std::function<float(int, int)> getValue,
                                    std::function<void(int, int, float)> setPoint)
    {
        if (grid != nullptr)
        {
            grid->getSongAutomationValue = std::move(getValue);
            grid->setSongAutomationPoint = std::move(setPoint);
        }
    }
    void setTrackContextMenuHandler(std::function<void(int, juce::Component*)> callback)
    {
        if (grid != nullptr) grid->trackContextMenu = std::move(callback);
    }
    void setAudioClipHandlers(std::function<bool(int)> isAudio,
                              std::function<int(int, int)> clipStart,
                              std::function<int(int, int)> clipLength,
                              std::function<void(int, int, juce::Component*)> contextMenu)
    {
        if (grid != nullptr)
        {
            grid->isAudioTrack = std::move(isAudio);
            grid->audioClipStartBar = std::move(clipStart);
            grid->audioClipLengthBars = std::move(clipLength);
            grid->audioContextMenu = std::move(contextMenu);
        }
    }
    void setTrackColourProvider(std::function<juce::Colour(int)> provider)
    {
        if (grid != nullptr) grid->getTrackColour = std::move(provider);
    }
    void setAutomationModeChanged(std::function<void(bool)> callback)
    {
        if (surface != nullptr) surface->onAutomationModeChanged = std::move(callback);
    }

    void setLightTheme(bool enabled)
    {
        if (grid != nullptr) grid->setLightTheme(enabled);
        if (surface != nullptr) surface->setLightTheme(enabled);
    }

    void setThemeChangedHandler(std::function<void(bool)> callback)
    {
        if (grid != nullptr) grid->onThemeChanged = std::move(callback);
    }
    void setAutomationMode(bool enabled)
    {
        if (surface != nullptr) surface->setAutomationMode(enabled);
    }

private:
    static constexpr int titleBarHeight = 28;
    juce::Label title;
    PlaylistWindowIconButton minimizeButton { PlaylistWindowIconButton::Kind::minimize },
                           maximizeButton { PlaylistWindowIconButton::Kind::maximize },
                           closeButton { PlaylistWindowIconButton::Kind::close };
    std::vector<std::unique_ptr<ResizeGrip>> resizeGrips;
    juce::Point<int> dragOrigin;
    juce::Rectangle<int> resizeStartBounds;
    bool minimized = false, maximized = false;
    PlaylistGrid* grid = nullptr;
    std::unique_ptr<PlaylistSurface> surface;
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

class MixerWindow final : public juce::Component
{
public:
    class ResizeGrip final : public juce::Component
    {
    public:
        std::function<void()> onStarted, onEnded;
        std::function<void(juce::Point<int>)> onDragged;
        void mouseDown(const juce::MouseEvent&) override { if (onStarted) onStarted(); }
        void mouseDrag(const juce::MouseEvent& event) override { if (onDragged) onDragged(event.getOffsetFromDragStart()); }
        void mouseUp(const juce::MouseEvent&) override { if (onEnded) onEnded(); }
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour::fromRGB(112, 179, 201));
            const auto centreX = static_cast<float>(getWidth()) * 0.5f;
            const auto centreY = static_cast<float>(getHeight()) * 0.5f;
            for (int offset = -4; offset <= 4; offset += 4)
                g.drawLine(centreX - 5.0f, centreY + static_cast<float>(offset),
                           centreX + 5.0f, centreY + static_cast<float>(offset), 1.5f);
        }
    };

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
    std::function<void()> onFocus;
    std::function<void()> onMaximizeToggled;
    std::function<void()> onResizeStarted, onResizeEnded;
    std::function<void(juce::Point<int>)> onResizedByDrag;
    std::function<juce::StringArray(int)> getInsertNames;
    std::function<bool(int, int)> getInsertEnabled;
    std::function<void(int, int)> toggleInsert;
    std::function<void(int, int)> editInsert;
    std::function<void(int, int)> removeInsert;
    std::function<void(int, int)> addInsert;
    std::function<double(int, int)> getTrackSendGain;
    std::function<void(int, int, double)> setTrackSendGain;
    std::function<int(int)> getTrackOutputBus;
    std::function<void(int, int)> setTrackOutputBus;
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
    {
        content.setOpaque(true);
        minimizeButton.setKind(PlaylistWindowIconButton::Kind::minimize);
        maximizeButton.setKind(PlaylistWindowIconButton::Kind::maximize);
        closeButton.setKind(PlaylistWindowIconButton::Kind::close);
        minimizeButton.setTooltip("Minimize mixer");
        maximizeButton.setTooltip("Maximize or restore mixer");
        closeButton.setTooltip("Close mixer");
        for (auto* button : { &minimizeButton, &maximizeButton, &closeButton })
        {
            button->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 48, 58));
            button->setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(220, 235, 243));
            addAndMakeVisible(button);
        }
        focusListener.onMouseDown = [this] { if (onFocus) onFocus(); };
        minimizeButton.onClick = [this]
        {
            minimized = !minimized;
            updateMinimizeButton();
            if (onMinimizeChanged) onMinimizeChanged(minimized);
            resized();
        };
        maximizeButton.onClick = [this] { if (onMaximizeToggled) onMaximizeToggled(); };
        closeButton.onClick = [this] { if (onClose) onClose(); };
        addAndMakeVisible(content);
        content.addAndMakeVisible(viewport);
        resizeGrip.setTitle("Resize mixer");
        resizeGrip.setMouseCursor(juce::MouseCursor::TopEdgeResizeCursor);
        resizeGrip.onStarted = [this] { if (onResizeStarted) onResizeStarted(); };
        resizeGrip.onDragged = [this](juce::Point<int> delta) { if (onResizedByDrag) onResizedByDrag(delta); };
        resizeGrip.onEnded = [this] { if (onResizeEnded) onResizeEnded(); };
        addAndMakeVisible(resizeGrip);
        // Keep channel strips flush with the fitted panel width. Vertical
        // navigation remains available from the wheel without reserving a
        // permanent scrollbar gutter; horizontal scrolling appears only when
        // all strips cannot fit in the available workspace.
        viewport.setScrollBarsShown(false, true, true, false);
        viewport.setViewedComponent(&strips, false);
        strips.setOpaque(false);
        strips.addAndMakeVisible(masterStrip);
        masterStrip.addMouseListener(&focusListener, true);
        strips.setSize(640, 1);
        effectWindow = std::make_unique<PluginParameterWindow>();
    }

    std::function<void(bool)> onMinimizeChanged;
    std::function<void()> onClose;
    std::function<void(juce::Point<int>)> onDragged;
    std::function<void()> onDragEnded;
    bool isMinimized() const { return minimized; }
    bool isMaximized() const { return maximized; }
    void setMaximized(bool shouldMaximize)
    {
        maximized = shouldMaximize;
        maximizeButton.setKind(PlaylistWindowIconButton::Kind::maximize);
        maximizeButton.setTooltip(maximized ? "Restore mixer size" : "Maximize mixer");
    }
    void setMasterSectionMode(bool enabled) { masterSectionMode = enabled; }
    bool isMasterSectionMode() const { return masterSectionMode; }
    int getVisibleTrackCount() const { return visibleTrackIndices.size(); }
    int getPreferredWidth() const
    {
        constexpr int mixerStripStep = 132;
        const auto columns = getVisibleTrackCount() + (masterSectionMode ? 1 : 0);
        return juce::jmax(152, columns * mixerStripStep + 20);
    }
    void setMinimized(bool shouldMinimize)
    {
        minimized = shouldMinimize;
        updateMinimizeButton();
        resized();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (onFocus) onFocus();
        if (event.eventComponent == this && event.y < titleBarHeight) dragOrigin = getPosition();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (event.eventComponent == this && event.getMouseDownY() < titleBarHeight && onDragged)
            onDragged(dragOrigin + event.getOffsetFromDragStart());
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (event.eventComponent == this && event.getMouseDownY() < titleBarHeight && onDragEnded) onDragEnded();
    }

    void refresh(int selectedTrack)
    {
        const auto count = getTrackCount ? getTrackCount() : 0;
        visibleTrackIndices.clear();
        for (int i = 0; i < count; ++i)
            if (isGeneratorTrack && isGeneratorTrack(i) != masterSectionMode)
                visibleTrackIndices.add(i);
        const auto visibleCount = visibleTrackIndices.size();
        while (channelStrips.size() < visibleCount)
        {
            auto* strip = new ChannelStrip();
            strip->addMouseListener(&focusListener, true);
            channelStrips.add(strip);
        }
        while (channelStrips.size() > visibleCount) channelStrips.removeLast();
        masterStrip.setVisible(masterSectionMode);
        for (int stripIndex = 0; stripIndex < visibleCount; ++stripIndex)
        {
            const auto i = visibleTrackIndices.getUnchecked(stripIndex);
            auto* strip = channelStrips[stripIndex];
            if (strip->getParentComponent() != &strips) strips.addAndMakeVisible(strip);
            // The selected-track control and footer already identify a
            // channel. The mixer strip itself starts with the send section.
            strip->setFooterText(getTrackName ? getTrackName(i) : "Channel " + juce::String(i + 1));
            strip->setSelected(i == selectedTrack);
            strip->setMeterPlugin(getTrackMeter ? getTrackMeter(i) : nullptr);
            strip->setGainValue(getTrackGain ? getTrackGain(i) : 0.0);
            strip->setPanValue(getTrackPan ? getTrackPan(i) : 0.0);
            for (int bus = 0; bus < 8; ++bus)
                strip->setSendValue(bus, getTrackSendGain ? getTrackSendGain(i, bus) : -60.0);
            strip->setOutputBus(getTrackOutputBus ? getTrackOutputBus(i) : -1);
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
                if (setTrackEqEnabled) setTrackEqEnabled(i, true);
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
            // Output routing is useful for every mixer channel, including
            // ordinary audio tracks and FX returns, so keep the Master/Bus
            // selector bank available across all strips.
            strip->setOutputRoutesVisible(true);
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
            strip->onOutputBus = [this, i](int bus)
            {
                if (setTrackOutputBus) setTrackOutputBus(i, bus);
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
                if (setTrackEqEnabled) setTrackEqEnabled(count, true);
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
        content.setBounds(getLocalBounds().reduced(1).withTrimmedTop(titleBarHeight - 1));
        minimizeButton.setBounds(getWidth() - 83, 2, 24, titleBarHeight - 4);
        maximizeButton.setBounds(getWidth() - 56, 2, 24, titleBarHeight - 4);
        closeButton.setBounds(getWidth() - 29, 2, 24, titleBarHeight - 4);
        viewport.setBounds(content.getLocalBounds());
        viewport.setVisible(!minimized);
        resizeGrip.setVisible(!minimized && !maximized);
        resizeGrip.setBounds(getWidth() - 18, getHeight() - 18, 18, 18);
        layoutStrips();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour::fromRGB(25, 46, 60));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colour::fromRGB(32, 42, 52));
        g.fillRect(0, 0, getWidth(), titleBarHeight);
        g.setColour(juce::Colour::fromRGB(222, 235, 242));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(masterSectionMode ? "Master Section" : "Mixing Desk", 9, 0, juce::jmax(0, getWidth() - 98), titleBarHeight,
                   juce::Justification::centred);
        g.setColour(juce::Colour::fromRGB(151, 194, 216));
        g.drawRect(getLocalBounds().toFloat().reduced(0.5f), 1.0f);
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
    class FocusListener final : public juce::MouseListener
    {
    public:
        std::function<void()> onMouseDown;
        void mouseDown(const juce::MouseEvent&) override
        {
            if (onMouseDown) onMouseDown();
        }
    } focusListener;
    static constexpr int titleBarHeight = 26;
    PlaylistWindowIconButton minimizeButton { PlaylistWindowIconButton::Kind::minimize },
                            maximizeButton { PlaylistWindowIconButton::Kind::maximize },
                            closeButton { PlaylistWindowIconButton::Kind::close };
    ResizeGrip resizeGrip;
    juce::Point<int> dragOrigin;
    bool minimized = false, maximized = false;
    bool masterSectionMode = false;
    juce::Array<int> visibleTrackIndices;

    void updateMinimizeButton()
    {
        minimizeButton.setButtonText(minimized ? "+" : "−");
        minimizeButton.setTooltip(minimized ? "Restore mixer" : "Minimize mixer");
    }

    void layoutStrips()
    {
        // Use a consistent narrow strip width so the console stays compact.
        // The viewport adds horizontal scrolling when the active bank grows.
        const int count = channelStrips.size();
        const int columns = count + (masterSectionMode ? 1 : 0);
        constexpr int columnStep = 132;
        constexpr int columnWidth = 126;
        // Keep each strip inside the visible mixer viewport. A taller fixed
        // minimum pushed the output-routing buttons below the window edge.
        const int stripHeight = juce::jmax(1, viewport.getHeight());

        for (int i = 0; i < count; ++i)
            channelStrips[i]->setBounds(i * columnStep, 0, columnWidth, stripHeight);
        if (masterSectionMode) masterStrip.setBounds(count * columnStep, 0, columnWidth, stripHeight);
        strips.setSize(juce::jmax(1, columns) * columnStep, stripHeight);
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
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            const auto switchWidth = juce::jmin(20.0f, bounds.getWidth());
            const auto switchHeight = juce::jmin(11.0f, bounds.getHeight());
            const auto pill = juce::Rectangle<float>(bounds.getRight() - switchWidth,
                bounds.getCentreY() - switchHeight * 0.5f, switchWidth, switchHeight);
            const auto active = isEnabled ? juce::Colour::fromRGB(48, 157, 183)
                                          : juce::Colour::fromRGB(37, 55, 65);
            g.setColour(down ? active.brighter(0.12f) : active);
            g.fillRoundedRectangle(pill, switchHeight * 0.5f);
            g.setColour(juce::Colour::fromRGB(146, 185, 200));
            g.drawRoundedRectangle(pill, switchHeight * 0.5f, 0.8f);
            const auto knobDiameter = switchHeight - 3.0f;
            const auto knobX = isEnabled ? pill.getRight() - knobDiameter - 1.5f : pill.getX() + 1.5f;
            g.setColour(isEnabled ? juce::Colour::fromRGB(225, 250, 255)
                                  : juce::Colour::fromRGB(157, 179, 188));
            g.fillEllipse(knobX, pill.getCentreY() - knobDiameter * 0.5f,
                          knobDiameter, knobDiameter);
            if (highlighted)
            {
                g.setColour(juce::Colours::white.withAlpha(0.14f));
                g.drawRoundedRectangle(pill.expanded(1.0f), switchHeight * 0.5f, 0.8f);
            }
        }

    private:
        bool isEnabled = false;
    };

    class ChannelStrip final : public juce::Component, private juce::Timer
    {
    public:
        class FaderSlider final : public juce::Slider
        {
        public:
            void paint(juce::Graphics& g) override
            {
                const auto bounds = getLocalBounds().toFloat().reduced(2.0f, 2.0f);
                if (bounds.isEmpty()) return;
                const auto centreX = bounds.getCentreX();
                const auto railWidth = juce::jmin(9.0f, juce::jmax(6.0f, bounds.getWidth() * 0.17f));
                auto rail = juce::Rectangle<float>(centreX - railWidth * 0.5f, bounds.getY() + 7.0f,
                                                    railWidth, juce::jmax(0.0f, bounds.getHeight() - 14.0f));
                g.setColour(juce::Colour::fromRGB(23, 43, 55));
                g.fillRoundedRectangle(rail, railWidth * 0.5f);
                g.setColour(juce::Colour::fromRGB(103, 157, 177).withAlpha(0.7f));
                g.drawRoundedRectangle(rail, railWidth * 0.5f, 0.8f);

                for (const double db : { -60.0, -36.0, -24.0, -18.0, -12.0, -6.0, 0.0 })
                {
                    const auto p = static_cast<float>(valueToProportionOfLength(db));
                    const auto y = rail.getBottom() - p * rail.getHeight();
                    g.setColour(juce::Colour::fromRGB(167, 199, 211).withAlpha(0.65f));
                    g.drawHorizontalLine(juce::roundToInt(y), centreX - 9.0f, centreX + 9.0f);
                }

                const auto p = static_cast<float>(valueToProportionOfLength(getValue()));
                const auto thumbY = rail.getBottom() - p * rail.getHeight();
                const auto thumbWidth = juce::jmin(38.0f, juce::jmax(0.0f, bounds.getWidth() - 6.0f));
                const auto thumb = juce::Rectangle<float>(centreX - thumbWidth * 0.5f, thumbY - 7.0f,
                                                          thumbWidth, 14.0f);
                juce::ColourGradient face(juce::Colour::fromRGB(231, 245, 250), thumb.getX(), thumb.getY(),
                                          juce::Colour::fromRGB(112, 163, 183), thumb.getRight(), thumb.getBottom(), false);
                g.setGradientFill(face);
                g.fillRoundedRectangle(thumb, 3.0f);
                g.setColour(juce::Colour::fromRGB(23, 48, 61));
                g.drawRoundedRectangle(thumb, 3.0f, 1.0f);
                g.setColour(juce::Colour::fromRGB(248, 253, 255).withAlpha(0.85f));
                g.drawHorizontalLine(juce::roundToInt(thumb.getCentreY()), thumb.getX() + 6.0f, thumb.getRight() - 6.0f);
            }
        };

        class EqKnob final : public juce::Slider
        {
        public:
            void paint(juce::Graphics& g) override
            {
                auto bounds = getLocalBounds().toFloat().reduced(2.0f);
                const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
                bounds = bounds.withSizeKeepingCentre(diameter, diameter);
                const auto centre = bounds.getCentre();
                const auto radius = diameter * 0.5f;
                constexpr float start = juce::MathConstants<float>::pi * 0.75f;
                constexpr float sweep = juce::MathConstants<float>::pi * 1.5f;

                g.setColour(juce::Colour::fromRGB(21, 39, 51).withAlpha(0.55f));
                g.fillEllipse(bounds.translated(0.0f, 1.3f));
                juce::ColourGradient bezel(juce::Colour::fromRGB(155, 190, 204),
                                           centre.x, bounds.getY(),
                                           juce::Colour::fromRGB(48, 76, 92),
                                           centre.x, bounds.getBottom(), false);
                g.setGradientFill(bezel);
                g.fillEllipse(bounds);

                const auto face = bounds.reduced(radius * 0.15f);
                juce::ColourGradient faceGradient(juce::Colour::fromRGB(71, 119, 143),
                                                  face.getX(), face.getY(),
                                                  juce::Colour::fromRGB(29, 54, 69),
                                                  face.getRight(), face.getBottom(), true);
                g.setGradientFill(faceGradient);
                g.fillEllipse(face);
                g.setColour(juce::Colour::fromRGB(18, 38, 50).withAlpha(0.75f));
                g.drawEllipse(face, 0.8f);

                const auto arcRadius = radius * 0.77f;
                juce::Path track;
                track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                    start, start + sweep, true);
                g.setColour(juce::Colour::fromRGB(23, 44, 57));
                g.strokePath(track, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

                const auto proportion = static_cast<float>(valueToProportionOfLength(getValue()));
                if (proportion > 0.001f)
                {
                    juce::Path activeArc;
                    activeArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                            start, start + sweep * proportion, true);
                    g.setColour(juce::Colour::fromRGB(78, 207, 235));
                    g.strokePath(activeArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                                  juce::PathStrokeType::rounded));
                }

                const auto angle = start + sweep * proportion;
                const auto inner = juce::Point<float>(centre.x + std::cos(angle) * radius * 0.22f,
                                                       centre.y + std::sin(angle) * radius * 0.22f);
                const auto outer = juce::Point<float>(centre.x + std::cos(angle) * radius * 0.63f,
                                                       centre.y + std::sin(angle) * radius * 0.63f);
                g.setColour(juce::Colour::fromRGB(211, 243, 252));
                g.drawLine(inner.x, inner.y, outer.x, outer.y, 2.0f);
                g.fillEllipse(outer.x - 1.6f, outer.y - 1.6f, 3.2f, 3.2f);
                g.setColour(juce::Colour::fromRGB(239, 250, 255).withAlpha(0.45f));
                juce::Path highlight;
                highlight.addCentredArc(centre.x, centre.y, radius - 1.0f, radius - 1.0f, 0.0f,
                                        juce::MathConstants<float>::pi * 1.08f,
                                        juce::MathConstants<float>::pi * 1.92f, true);
                g.strokePath(highlight, juce::PathStrokeType(0.9f));
            }
        };

        class SendKnob final : public juce::Slider
        {
        public:
            void paint(juce::Graphics& g) override
            {
                const auto bounds = getLocalBounds().toFloat().reduced(3.0f);
                const auto centre = bounds.getCentre();
                const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.43f;
                constexpr float startAngle = juce::MathConstants<float>::pi * 0.75f;
                constexpr float endAngle = juce::MathConstants<float>::pi * 2.25f;
                juce::Path arc;
                arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                                  startAngle, endAngle, true);
                g.setColour(juce::Colour::fromRGB(29, 53, 67));
                g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

                const auto proportion = static_cast<float>(valueToProportionOfLength(getValue()));
                if (proportion > 0.0f)
                {
                    juce::Path activeArc;
                    activeArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                                            startAngle, startAngle + (endAngle - startAngle) * proportion, true);
                    g.setColour(juce::Colour::fromRGB(62, 184, 215));
                    g.strokePath(activeArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                                  juce::PathStrokeType::rounded));
                }

                const auto angle = startAngle + (endAngle - startAngle) * proportion;
                const auto indicator = juce::Point<float>(centre.x + std::cos(angle) * radius,
                                                          centre.y + std::sin(angle) * radius);
                g.setColour(juce::Colour::fromRGB(23, 42, 53));
                g.fillEllipse(centre.x - radius * 0.64f, centre.y - radius * 0.64f,
                              radius * 1.28f, radius * 1.28f);
                g.setColour(juce::Colour::fromRGB(196, 229, 241));
                g.fillEllipse(indicator.x - 3.3f, indicator.y - 3.3f, 6.6f, 6.6f);
            }
        };

        class InsertSlotButton final : public juce::TextButton
        {
        public:
            void setInsertActive(bool active) { insertActive = active; repaint(); }
        protected:
            void paintButton(juce::Graphics& g, bool highlighted, bool down) override
            {
                juce::TextButton::paintButton(g, highlighted, down);
                const auto centreY = static_cast<float>(getHeight()) * 0.5f;
                const auto colour = insertActive ? juce::Colour::fromRGB(68, 205, 227)
                                                 : juce::Colour::fromRGB(23, 35, 43);
                g.setColour(colour);
                g.fillEllipse(5.0f, centreY - 3.3f, 6.6f, 6.6f);
                g.setColour(insertActive ? juce::Colour::fromRGB(185, 243, 250).withAlpha(0.75f)
                                         : juce::Colour::fromRGB(103, 131, 144));
                g.drawEllipse(5.0f, centreY - 3.3f, 6.6f, 6.6f, 0.8f);
            }
        private:
            bool insertActive = false;
        };

        ChannelStrip()
        {
            startTimerHz(20);
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
            insertsTitle.setBorderSize(juce::BorderSize<int>(0, 0, 0, 14));
            eqButton.setButtonText({});
            eqButton.setTitle("Toggle equaliser");
            eqButton.setClickingTogglesState(true);
            eqButton.setTooltip("Enable or bypass the Tracktion equaliser. Click the graph to open its full controls.");
            eqButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(53, 103, 130));
            eqButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(43, 118, 145));
            eqButton.onClick = [this] { if (onToggleEq) onToggleEq(eqButton.getToggleState()); };
            // Compact power switch is placed beside the HI band title, so EQ
            // can be bypassed without restoring a separate control row.
            eqButton.setVisible(false);
            static const std::array<const char*, 4> bandNames { "HI", "HM", "LMid", "Low" };
            static const std::array<const char*, 3> parameterNames { "Gain", "Frequency", "Q" };
            static const std::array<const char*, 3> compactParameterNames { "dB", "Freq", "Q" };
            for (int band = 0; band < 4; ++band)
            {
                auto& bandTitle = eqBandTitles[static_cast<size_t>(band)];
                bandTitle.setText(bandNames[static_cast<size_t>(band)], juce::dontSendNotification);
                bandTitle.setFont(juce::FontOptions(9.0f, juce::Font::bold));
                bandTitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 240, 248));
                bandTitle.setJustificationType(juce::Justification::centredLeft);
                bandTitle.setTooltip(band == 0 ? "High shelf" : band == 1 ? "High mid" : band == 2 ? "Low mid" : "Low shelf");
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
                    const bool parameterUsed = parameter < 2 || band == 1;
                    control.setVisible(parameterUsed);
                    control.onValueChange = [this, band, parameter]
                    {
                        if (!eqEnabled && onToggleEq)
                        {
                            eqEnabled = true;
                            onToggleEq(true);
                        }
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
                    controlLabel.setVisible(parameterUsed);
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
                insert.onClick = [this, slot]
                {
                    if (insertSlotHasEffect[static_cast<size_t>(slot)])
                    {
                        if (onEditInsert) onEditInsert(slot);
                    }
                    else if (onAddInsert)
                        onAddInsert();
                };
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
            for (int bus = 0; bus < 8; ++bus)
            {
                auto& label = sendNames[static_cast<size_t>(bus)];
                auto& send = sends[static_cast<size_t>(bus)];
                addAndMakeVisible(label);
                label.setText(juce::String(bus + 1), juce::dontSendNotification);
                label.setJustificationType(juce::Justification::centredRight);
                label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
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
            for (int bus = 0; bus < 8; ++bus)
            {
                auto& route = outputBusButtons[static_cast<size_t>(bus)];
                addAndMakeVisible(route);
                route.setButtonText(juce::String(bus + 1));
                route.setClickingTogglesState(false);
                route.setTooltip("Route this channel to Bus " + juce::String(bus + 1));
                route.onClick = [this, bus] { if (onOutputBus) onOutputBus(bus); };
                route.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 58, 72));
                route.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
                route.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(208, 231, 243));
                route.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            }
            addAndMakeVisible(outputMasterButton);
            outputMasterButton.setClickingTogglesState(false);
            outputMasterButton.setTooltip("Route this channel to the Master output");
            outputMasterButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 58, 72));
            outputMasterButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
            outputMasterButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(208, 231, 243));
            outputMasterButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            outputMasterButton.onClick = [this] { if (onOutputBus) onOutputBus(-1); };
            addAndMakeVisible(gain);
            gain.setSliderStyle(juce::Slider::LinearVertical);
            gain.setRange(-60.0, 6.0, 0.1);
            gain.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            gain.setTextValueSuffix(" dB");
            gain.setPopupDisplayEnabled(true, true, this);
            gain.onValueChange = [this] { if (onGain) onGain(gain.getValue()); };
            for (auto* slider : std::array<juce::Slider*, 2> { &gain, &pan })
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
        std::function<void(int)> onOutputBus;
        std::function<void(bool)> onMute, onSolo, onToggleEq;
        std::function<void()> onOpenEq;
        std::function<void(int, float)> onEqParameter;
        std::function<float(int)> getEqParameter;
        std::function<void(int)> onEditInsert;
        std::function<void(int)> onToggleInsert, onRemoveInsert;
        std::function<void()> onAddInsert;
        FaderSlider gain;
        juce::Label footer;
        juce::Label insertsTitle;
        std::array<juce::Label, 4> eqBandTitles;
        std::array<std::array<EqKnob, 3>, 4> eqControls;
        std::array<juce::Rectangle<int>, 4> eqBandBounds;
        std::array<juce::Label, 8> sendNames;
        std::array<SendKnob, 8> sends;
        std::array<juce::TextButton, 8> outputBusButtons;
        juce::TextButton outputMasterButton { "Master" };
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
            if (juce::isPositiveAndBelow(bus, 8))
                sends[static_cast<size_t>(bus)].setValue(value, juce::dontSendNotification);
        }
        void setOutputBus(int bus)
        {
            for (int i = 0; i < 8; ++i)
                outputBusButtons[static_cast<size_t>(i)].setToggleState(i == bus, juce::dontSendNotification);
            outputMasterButton.setToggleState(bus < 0, juce::dontSendNotification);
        }
        void setMuteState(bool value) { mute.setToggleState(value, juce::dontSendNotification); }
        void setSoloState(bool value) { solo.setToggleState(value, juce::dontSendNotification); }
        void setEqEnabled(bool value)
        {
            eqEnabled = value;
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
            for (int bus = 0; bus < 8; ++bus)
            {
                sendNames[static_cast<size_t>(bus)].setVisible(shouldShow && bus < 4);
                sends[static_cast<size_t>(bus)].setVisible(shouldShow && bus < 4);
            }
            outputMasterButton.setVisible(shouldShow && !masterEqView);
            for (auto& route : outputBusButtons) route.setVisible(shouldShow && !masterEqView);
            eqButton.setVisible(false);
            eqGraph.setVisible(shouldShow);
            for (int band = 0; band < 4; ++band)
            {
                eqBandTitles[static_cast<size_t>(band)].setVisible(shouldShow);
                for (int parameter = 0; parameter < 3; ++parameter)
                {
                    const bool parameterUsed = parameter < 2 || band == 1;
                    eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(shouldShow && parameterUsed);
                    eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setVisible(shouldShow && parameterUsed);
                }
            }
            insertsTitle.setVisible(shouldShow);
            addInsertButton.setVisible(false);
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
            for (int bus = 0; bus < 8; ++bus)
            {
                sendNames[static_cast<size_t>(bus)].setVisible(false);
                sends[static_cast<size_t>(bus)].setVisible(false);
                outputBusButtons[static_cast<size_t>(bus)].setVisible(false);
            }
            outputMasterButton.setVisible(false);
            mute.setVisible(shouldShow);
            mute.setEnabled(shouldShow);
            solo.setVisible(shouldShow);
            solo.setEnabled(false);
            solo.setTooltip("The master output cannot be soloed");
            eqButton.setVisible(false);
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
            addInsertButton.setVisible(false);
            resized();
        }
        void setSendsVisible(bool shouldShow)
        {
            for (int bus = 0; bus < 8; ++bus)
            {
                sendNames[static_cast<size_t>(bus)].setVisible(shouldShow && bus < 4);
                sends[static_cast<size_t>(bus)].setVisible(shouldShow && bus < 4);
            }
        }
        void setOutputRoutesVisible(bool shouldShow)
        {
            outputMasterButton.setVisible(shouldShow && !masterEqView);
            for (auto& route : outputBusButtons) route.setVisible(shouldShow && !masterEqView);
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
            leftPeakHoldTicks = rightPeakHoldTicks = 0;
            clipHeld = false;
            meterVisible = false;
        }
        void setInsertRow(int slot, const juce::String& insertName, bool enabled)
        {
            if (!juce::isPositiveAndBelow(slot, 4)) return;
            auto& insert = insertButtons[static_cast<size_t>(slot)];
            auto& edit = editInsertButtons[static_cast<size_t>(slot)];
            auto& bypass = bypassInsertButtons[static_cast<size_t>(slot)];
            auto& remove = removeInsertButtons[static_cast<size_t>(slot)];
            const bool visible = slot < 3;
            const bool populated = visible && insertName.isNotEmpty();
            insertSlotHasEffect[static_cast<size_t>(slot)] = populated;
            insert.setVisible(visible);
            remove.setVisible(populated);
            // The effect-name button already opens its editor. Hiding the
            // duplicate E control gives the name enough room to read at the
            // reference mixer’s narrow channel width.
            edit.setVisible(false);
            bypass.setVisible(populated);
            insert.setButtonText(populated ? "    " + insertName : juce::String{});
            insert.setInsertActive(populated && enabled);
            insert.setColour(juce::TextButton::buttonColourId,
                             !populated ? juce::Colour::fromRGB(48, 78, 95)
                                : enabled ? juce::Colour::fromRGB(47, 91, 117) : juce::Colour::fromRGB(55, 69, 78));
            insert.setTooltip(populated ? "Open " + insertName + " controls" : "Add an effect to this insert slot");
            bypass.setButtonText("");
            bypass.setTooltip(enabled ? "Bypass this effect" : "Enable this effect");
            bypass.setActive(enabled);
            bypass.setColour(juce::TextButton::buttonColourId,
                             enabled ? juce::Colour::fromRGB(44, 80, 101) : juce::Colour::fromRGB(55, 69, 78));
        }
        void setSelected(bool selected) { isSelected = selected; repaint(); }

        void mouseMove(const juce::MouseEvent& event) override
        {
            setMouseCursor(meterVisible && meterBounds.contains(event.getPosition())
                               ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        }

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (!meterVisible || !meterBounds.contains(event.getPosition()))
                return;

            leftPeakHoldDb = rightPeakHoldDb = -100.0f;
            leftPeakHoldTicks = rightPeakHoldTicks = 0;
            clipHeld = false;
            repaint(meterBounds.expanded(2));
        }

        void paint(juce::Graphics& g) override
        {
            auto box = getLocalBounds().toFloat().reduced(1.0f);
            const auto base = isSelected ? juce::Colour::fromRGB(91, 143, 171) : juce::Colour::fromRGB(71, 117, 146);
            g.setGradientFill(juce::ColourGradient(base.brighter(0.12f), 0.0f, 0.0f,
                                                   base.darker(0.16f), 0.0f, getHeight(), false));
            g.fillRoundedRectangle(box, 5.0f);
            g.setColour(juce::Colour::fromRGB(161, 197, 216));
            g.drawRoundedRectangle(box, 5.0f, 1.0f);
            if (!masterEqView)
            {
                for (size_t band = 0; band < eqBandBounds.size(); ++band)
                {
                    const auto panel = eqBandBounds[band].toFloat().reduced(1.0f, 0.5f);
                    if (panel.isEmpty()) continue;
                    const auto panelBase = band % 2 == 0 ? juce::Colour::fromRGB(58, 102, 126)
                                                        : juce::Colour::fromRGB(53, 96, 120);
                    g.setColour(panelBase.withAlpha(0.42f));
                    g.fillRoundedRectangle(panel, 3.0f);
                    g.setColour(juce::Colour::fromRGB(168, 207, 222).withAlpha(0.28f));
                    g.drawRoundedRectangle(panel, 3.0f, 0.7f);
                    g.setColour(juce::Colour::fromRGB(92, 210, 233).withAlpha(0.78f));
                    g.fillRoundedRectangle(panel.getX() + 2.0f, panel.getY() + 2.0f, 2.0f,
                                           juce::jmax(2.0f, panel.getHeight() - 4.0f), 1.0f);
                }
            }
            if (insertsTitle.isVisible())
            {
                const auto centreY = insertsTitle.getBounds().toFloat().getCentreY();
                g.setColour(juce::Colour::fromRGB(234, 62, 52));
                g.fillEllipse(static_cast<float>(insertsTitle.getX() - 12), centreY - 3.0f, 6.0f, 6.0f);
                g.setColour(juce::Colour::fromRGB(255, 183, 166).withAlpha(0.75f));
                g.drawEllipse(static_cast<float>(insertsTitle.getX() - 12), centreY - 3.0f, 6.0f, 6.0f, 0.8f);
            }
            paintMeter(g);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(6, 6);
            footer.setBounds(area.removeFromBottom(20));
            area.removeFromBottom(4);
            if (sends[0].isVisible())
            {
                // Four auxiliary send knobs arranged like the Orion reference.
                auto sendArea = area.removeFromTop(64);
                for (int bus = 0; bus < 4; ++bus)
                {
                    const auto row = bus / 2;
                    const auto column = bus % 2;
                    const auto cellX = area.getX() + column * (area.getWidth() / 2);
                    auto cell = juce::Rectangle<int>(cellX, sendArea.getY() + row * 32,
                                                     area.getWidth() / 2, 32).reduced(1, 0);
                    const auto knobCell = cell;
                    auto sendNameArea = cell.removeFromTop(9).removeFromRight(18);
                    sendNames[static_cast<size_t>(bus)].setBounds(sendNameArea);
                    const auto knobSize = juce::jmin(31, knobCell.getWidth() - 2, knobCell.getHeight() - 1);
                    sends[static_cast<size_t>(bus)].setBounds(knobCell.withSizeKeepingCentre(knobSize, knobSize));
                }
                area.removeFromTop(2);
            }
            if (eqGraph.isVisible())
            {
                insertsTitle.setBounds(area.removeFromTop(16).withTrimmedLeft(16));
                for (int slot = 0; slot < 4; ++slot)
                {
                    if (slot >= 3)
                    {
                        insertButtons[static_cast<size_t>(slot)].setBounds({});
                        editInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        bypassInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        removeInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        continue;
                    }
                    auto row = area.removeFromTop(23);
                    if (insertSlotHasEffect[static_cast<size_t>(slot)])
                    {
                        auto removeArea = row.removeFromRight(17);
                        auto bypassArea = row.removeFromRight(17);
                        row.removeFromRight(2);
                        insertButtons[static_cast<size_t>(slot)].setBounds(row);
                        bypassInsertButtons[static_cast<size_t>(slot)].setBounds(bypassArea);
                        removeInsertButtons[static_cast<size_t>(slot)].setBounds(removeArea);
                    }
                    else
                    {
                        insertButtons[static_cast<size_t>(slot)].setBounds(row);
                        bypassInsertButtons[static_cast<size_t>(slot)].setBounds({});
                        removeInsertButtons[static_cast<size_t>(slot)].setBounds({});
                    }
                    area.removeFromTop(1);
                }
                addInsertButton.setBounds({});
                area.removeFromTop(2);
                if (masterEqView)
                {
                    // The reference Master strip gives its EQ response graph
                    // a dedicated display area instead of repeating the four
                    // compact channel EQ control rows.
                    const auto graphHeight = juce::jmin(230, juce::jmax(130, area.getHeight() / 3));
                    for (auto& bandBounds : eqBandBounds) bandBounds = {};
                    eqGraph.setBounds(area.removeFromTop(graphHeight));
                    area.removeFromTop(3);
                }
                else
                {
                    const int eqGraphHeight = 64;
                    const int eqTitleHeight = 17;
                    const int eqLabelHeight = 11;
                    const int fixedEqHeight = eqGraphHeight + 2 + 2
                        + 4 * (eqTitleHeight + 10 + eqLabelHeight);
                    // Keep the fader and meter compact so the four EQ bands
                    // have room for controls that can be adjusted precisely.
                    const int compactVolumeHeight = 80;
                    const int lowerControlsHeight = 38 + 3 + 52 + compactVolumeHeight;
                    const int controlsHeight = juce::jlimit(28, 56,
                        (area.getHeight() - fixedEqHeight - lowerControlsHeight) / 4);
                    eqGraph.setBounds(area.removeFromTop(eqGraphHeight));
                    area.removeFromTop(2);
                    for (int band = 0; band < 4; ++band)
                    {
                        auto bandArea = area.removeFromTop(eqTitleHeight + controlsHeight + eqLabelHeight).reduced(1, 0);
                        eqBandBounds[static_cast<size_t>(band)] = bandArea;
                        auto titleRow = bandArea.removeFromTop(eqTitleHeight).reduced(3, 0);
                        if (band == 0)
                        {
                            eqButton.setBounds(titleRow.removeFromRight(22));
                            titleRow.removeFromRight(3);
                        }
                        eqBandTitles[static_cast<size_t>(band)].setBounds(titleRow);
                        auto controls = bandArea.removeFromTop(controlsHeight);
                        const int parameterCount = band == 1 ? 3 : 2;
                        const std::array<float, 3> twoKnobPositions { 0.28f, 0.72f, 0.0f };
                        const std::array<float, 3> threeKnobPositions { 0.16f, 0.50f, 0.84f };
                        for (int parameter = 0; parameter < parameterCount; ++parameter)
                        {
                            const auto xPosition = parameterCount == 3
                                ? threeKnobPositions[static_cast<size_t>(parameter)]
                                : twoKnobPositions[static_cast<size_t>(parameter)];
                            const int knobSize = juce::jmin(parameterCount == 3 ? 31 : 35,
                                                            controls.getHeight() - 2);
                            const int x = controls.getX() + juce::roundToInt(
                                xPosition * static_cast<float>(controls.getWidth())) - knobSize / 2;
                            eqControls[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(
                                juce::Rectangle<int>(x, controls.getCentreY() - knobSize / 2, knobSize, knobSize)
                                    .constrainedWithin(controls));
                        }
                        auto labels = bandArea.removeFromTop(eqLabelHeight);
                        for (int parameter = 0; parameter < parameterCount; ++parameter)
                        {
                            const auto xPosition = parameterCount == 3
                                ? threeKnobPositions[static_cast<size_t>(parameter)]
                                : twoKnobPositions[static_cast<size_t>(parameter)];
                            const int labelWidth = parameterCount == 3 ? 36 : 44;
                            const int x = controls.getX() + juce::roundToInt(
                                xPosition * static_cast<float>(controls.getWidth())) - labelWidth / 2;
                            eqControlLabels[static_cast<size_t>(band)][static_cast<size_t>(parameter)].setBounds(
                                juce::Rectangle<int>(x, labels.getY(), labelWidth, labels.getHeight())
                                    .constrainedWithin(labels));
                        }
                    }
                }
                area.removeFromTop(2);
            }
            if (mute.isVisible())
            {
                auto modePanRow = area.removeFromTop(38);
                auto modeButtons = modePanRow.removeFromLeft(48);
                mute.setBounds(modeButtons.removeFromTop(18).reduced(1, 0));
                solo.setBounds(modeButtons.removeFromTop(18).reduced(1, 0));
                modePanRow.removeFromLeft(2);
                auto panArea = modePanRow;
                auto panLabelRow = panArea.removeFromTop(10);
                panLeft.setBounds(panLabelRow.removeFromLeft(14));
                panRight.setBounds(panLabelRow.removeFromRight(14));
                panTitle.setBounds(panLabelRow);
                const int panWidth = juce::jmin(38, panArea.getWidth());
                pan.setBounds(panArea.withSizeKeepingCentre(panWidth, panArea.getHeight()));
                area.removeFromTop(3);
            }
            if (!masterEqView)
            {
                outputMasterButton.setVisible(true);
                for (auto& route : outputBusButtons) route.setVisible(true);
                auto routeArea = area.removeFromBottom(52);
                outputMasterButton.setBounds(routeArea.removeFromTop(16));
                routeArea.removeFromTop(1);
                for (int rowIndex = 0; rowIndex < 2; ++rowIndex)
                {
                    auto row = routeArea.removeFromTop(17);
                    for (int column = 0; column < 4; ++column)
                    {
                        const int bus = rowIndex * 4 + column;
                        auto cell = row.removeFromLeft(row.getWidth() / (4 - column));
                        outputBusButtons[static_cast<size_t>(bus)].setBounds(cell.reduced(1, 0));
                    }
                }
            }
            auto meterArea = area.removeFromRight(38);
            area.removeFromRight(1);
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
                clipHeld = true;
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
            if (clipHeld)
            {
                g.setColour(juce::Colour::fromRGB(235, 75, 61));
                g.fillRect(meterBounds.getX() + scaleWidth, meterBounds.getY() + 2,
                           meterBounds.getWidth() - scaleWidth - 2, 3);
            }
            g.setColour(juce::Colour::fromRGB(147, 185, 202));
            g.drawRoundedRectangle(meterBounds.toFloat(), 2.0f, 1.0f);
        }

        juce::Slider pan;
        juce::TextButton mute, solo;
        EqHeaderButton eqButton;
        EqResponseGraph eqGraph;
        std::array<InsertSlotButton, 4> insertButtons;
        std::array<juce::TextButton, 4> editInsertButtons;
        std::array<SmallActionButton, 4> bypassInsertButtons;
        std::array<SmallActionButton, 4> removeInsertButtons;
        juce::TextButton addInsertButton;
        std::array<bool, 4> insertSlotHasEffect{};
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
        int leftPeakHoldTicks = 0, rightPeakHoldTicks = 0;
        bool clipHeld = false;
        bool meterVisible = false, isMasterMeter = false;
        bool masterEqView = false, eqEnabled = false;
        bool isSelected = false;
    };

    class MixerPanel final : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override { g.fillAll(juce::Colour::fromRGB(68, 112, 139)); }
    };

    MixerPanel content;
    juce::Viewport viewport;
    juce::Component strips;
    juce::OwnedArray<ChannelStrip> channelStrips;
    ChannelStrip masterStrip;
    std::unique_ptr<PluginParameterWindow> effectWindow;
};

class PianoRollGrid final : public juce::Component
{
public:
    // Keep the maximized editor's time ruler broad like Orion's piano roll.
    // A pattern can still be shorter; the red boundary shows its current end.
    static constexpr int baseVisibleStepCount = 48;
    std::function<int()> getActiveStepCount;
    std::function<bool(int, int)> noteIsOn;
    std::function<bool()> areAllNotesSelected;
    std::function<int(int, int)> getNoteLength;
    std::function<void(int, int, bool)> noteClicked;
    std::function<void(int, int, int)> noteLengthChanged;
    std::function<void()> noteGestureStarted;
    std::function<void()> noteGestureEnded;
    std::function<void(int)> paintLengthUpdated;
    std::function<void(int, bool)> keyboardNoteChanged;
    std::function<float(int)> getAutomationValue;
    std::function<void(int, float)> setAutomationValue;
    std::function<void(int)> removeAutomationValue;
    std::function<void(int, int, float)> moveAutomationValue;
    std::function<void()> automationGestureStarted, automationGestureEnded;
    std::function<int(int)> getVelocityAtStep;
    std::function<void(int, int)> setVelocityAtStep;
    std::function<void()> velocityGestureStarted, velocityGestureEnded;
    std::function<int()> getMaximumLaneHeight;
    std::function<void()> laneHeightChanged;
    std::function<void()> pasteNotes;

    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto character = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
        if (character == 'v' && (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()))
        {
            if (pasteNotes) pasteNotes();
            return true;
        }
        return false;
    }

    void setPlayheadStep(double step, bool active)
    {
        playheadStep = step;
        playheadVisible = active;
        repaint();
    }

    int activeSteps() const { return juce::jlimit(1, 64, getActiveStepCount ? getActiveStepCount() : 16); }
    int visibleSteps() const { return juce::jmax(baseVisibleStepCount, activeSteps()); }
    int labelWidth() const { return 58; }
    int headerHeight() const { return 30; }
    static constexpr int defaultAutomationLaneHeight() { return 88; }
    int velocityLaneHeight() const { return laneHeightPx; }
    int rowHeight() const { return juce::jmax(6, juce::roundToInt(baseRowHeight * verticalZoom)); }
    int stepWidth() const { return juce::jmax(14, juce::roundToInt(baseStepWidth * horizontalZoom)); }
    void setViewMetrics(int width, int height)
    {
        baseStepWidth = juce::jmax(22.0f, static_cast<float>(width - labelWidth()) / visibleSteps());
        const auto laneHeight = (velocityLaneVisible || automationLaneVisible) ? laneHeightPx : 0;
        baseRowHeight = juce::jmax(8.0f, static_cast<float>(height - headerHeight() - 8 - laneHeight) / 37.0f);
    }
    void setZoom(float horizontal, float vertical)
    {
        horizontalZoom = juce::jlimit(0.5f, 3.0f, horizontal);
        verticalZoom = juce::jlimit(0.6f, 2.2f, vertical);
        repaint();
    }
    float getHorizontalZoom() const { return horizontalZoom; }
    float getVerticalZoom() const { return verticalZoom; }
    void setVelocityLaneVisible(bool visible) { velocityLaneVisible = visible; repaint(); }
    void setAutomationLaneVisible(bool visible) { automationLaneVisible = visible; repaint(); }
    void setAutomationLaneHeight(int height) { laneHeightPx = juce::jlimit(64, 260, height); repaint(); }
    int getAutomationLaneHeight() const { return laneHeightPx; }
    void setAutomationTool(int tool) { automationTool = juce::jlimit(0, 3, tool); }
    int getAutomationTool() const { return automationTool; }
    void setAutomationBars(bool enabled) { automationBars = enabled; repaint(); }
    bool isAutomationBars() const { return automationBars; }
    void setVelocityEditEnabled(bool enabled) { velocityEditEnabled = enabled; }
    bool isVelocityLaneVisible() const { return velocityLaneVisible; }
    bool isAutomationLaneVisible() const { return automationLaneVisible; }
    void setKeyboardOptions(bool coloured, bool labels)
    {
        colouredKeys = coloured;
        showNoteNames = labels;
        repaint();
    }
    void setGridOptions(int stepsPerBar, int emphasis)
    {
        subdivisionsPerBar = juce::jlimit(4, 64, stepsPerBar);
        highlightEveryN = juce::jlimit(1, subdivisionsPerBar, emphasis);
        repaint();
    }
    int getSubdivisionsPerBar() const { return subdivisionsPerBar; }
    int getHighlightEveryN() const { return highlightEveryN; }
    bool hasColouredKeys() const { return colouredKeys; }
    bool hasNoteNames() const { return showNoteNames; }

    void paint(juce::Graphics& g) override
    {
        const auto velocityHeight = laneHeightPx;
        const auto labelWidthPx = labelWidth();
        const auto headerHeightPx = headerHeight();
        const auto rowHeightPx = rowHeight();
        const auto stepWidthPx = stepWidth();
        g.fillAll(juce::Colour::fromRGB(20, 29, 38));
        g.setColour(juce::Colour::fromRGB(29, 43, 55));
        g.fillRect(0, 0, getWidth(), headerHeightPx);
        g.setFont(12.0f);
        const auto activeStepCount = activeSteps();
        const auto visibleStepCount = visibleSteps();
        for (int step = 0; step < visibleStepCount; ++step)
        {
            g.setColour(step < activeStepCount ? juce::Colour::fromRGB(178, 198, 211)
                                                : juce::Colour::fromRGB(90, 111, 124));
            g.drawText(juce::String(step + 1), labelWidthPx + step * stepWidthPx, 0, stepWidthPx, headerHeightPx,
                       juce::Justification::centred);
        }

        for (int pitch = 84; pitch >= 48; --pitch)
        {
            const auto row = 84 - pitch;
            const auto y = headerHeightPx + row * rowHeightPx;
            const bool blackKey = ((pitch % 12) == 1 || (pitch % 12) == 3 || (pitch % 12) == 6
                                   || (pitch % 12) == 8 || (pitch % 12) == 10);
            const auto keyWidth = blackKey ? (labelWidthPx * 3) / 5 : labelWidthPx - 2;
            g.setColour(colouredKeys
                ? (blackKey ? juce::Colour::fromRGB(24, 34, 43) : juce::Colour::fromRGB(224, 232, 237))
                : juce::Colour::fromRGB(196, 207, 215));
            g.fillRect(0, y, keyWidth, rowHeightPx);
            g.setColour(blackKey ? juce::Colour::fromRGB(13, 21, 28) : juce::Colour::fromRGB(106, 126, 139));
            g.drawRect(0, y, keyWidth, rowHeightPx, 1);
            g.setColour(blackKey ? juce::Colour::fromRGB(178, 198, 211) : juce::Colour::fromRGB(28, 43, 53));
            if (showNoteNames)
                g.drawText(pitchLabel(pitch), 4, y, labelWidthPx - 8, rowHeightPx, juce::Justification::centredRight);

            for (int step = 0; step < visibleStepCount; ++step)
            {
                const auto x = labelWidthPx + step * stepWidthPx;
                const bool inPattern = step < activeStepCount;
                const bool active = inPattern && noteIsOn && noteIsOn(pitch, step);
                g.setColour(inPattern
                    ? (blackKey ? juce::Colour::fromRGB(24, 34, 43) : juce::Colour::fromRGB(32, 46, 58))
                    : (blackKey ? juce::Colour::fromRGB(18, 27, 35) : juce::Colour::fromRGB(23, 34, 42)));
                g.fillRect(x + 1, y + 1, stepWidthPx - 2, rowHeightPx - 2);
                if (active)
                {
                    const auto noteLength = juce::jlimit(1, activeStepCount - step,
                        getNoteLength ? getNoteLength(pitch, step) : 1);
                    const auto noteWidth = juce::jmax(4, noteLength * stepWidthPx - 5);
                    g.setColour(juce::Colour::fromRGB(48, 158, 199));
                    g.fillRoundedRectangle(static_cast<float>(x + 2), static_cast<float>(y + 3),
                                           static_cast<float>(noteWidth), static_cast<float>(rowHeightPx - 6), 3.0f);
                    g.setColour(juce::Colour::fromRGB(156, 232, 250));
                    g.drawRoundedRectangle(static_cast<float>(x + 2), static_cast<float>(y + 3),
                                           static_cast<float>(noteWidth), static_cast<float>(rowHeightPx - 6), 3.0f, 1.0f);
                    if (areAllNotesSelected && areAllNotesSelected())
                    {
                        g.setColour(juce::Colour::fromRGB(255, 220, 125));
                        g.drawRoundedRectangle(static_cast<float>(x + 1), static_cast<float>(y + 2),
                                               static_cast<float>(noteWidth + 2), static_cast<float>(rowHeightPx - 4), 3.0f, 2.0f);
                    }
                }
                g.setColour(!inPattern ? juce::Colour::fromRGB(40, 55, 66)
                    : step % subdivisionsPerBar == 0 ? juce::Colour::fromRGB(100, 137, 155)
                    : step % highlightEveryN == 0 ? juce::Colour::fromRGB(82, 113, 130)
                                                   : juce::Colour::fromRGB(53, 75, 90));
                g.drawRect(x, y, stepWidthPx, rowHeightPx, 1);
            }
        }

        const auto velocityTop = headerHeightPx + 37 * rowHeightPx + 8;
        if (velocityLaneVisible || automationLaneVisible)
        {
            g.setColour(juce::Colour::fromRGB(29, 43, 55));
            g.fillRect(0, velocityTop, getWidth(), velocityHeight);
            g.setColour(juce::Colour::fromRGB(103, 139, 157));
            g.drawHorizontalLine(velocityTop, 0.0f, static_cast<float>(getWidth()));
            g.setColour(resizingLane ? juce::Colour::fromRGB(183, 232, 246)
                                     : juce::Colour::fromRGB(112, 179, 201));
            g.fillRoundedRectangle(getWidth() * 0.5f - 24.0f,
                                   static_cast<float>(velocityTop - 3), 48.0f, 6.0f, 3.0f);
            g.setColour(juce::Colour::fromRGB(178, 198, 211));
            g.setFont(11.0f);
            g.drawText(automationLaneVisible ? "Automation" : "Velocity", 4, velocityTop,
                       labelWidthPx - 8, 20, juce::Justification::centredLeft);
            if (automationLaneVisible)
            {
                juce::Path curve;
                bool started = false;
                for (int step = 0; step < activeStepCount; ++step)
                {
                    const auto value = juce::jlimit(0.0f, 1.0f, getAutomationValue ? getAutomationValue(step) : 0.5f);
                    const auto x = static_cast<float>(labelWidthPx + step * stepWidthPx + stepWidthPx / 2);
                    const auto y = static_cast<float>(velocityTop + 26 + (1.0f - value) * (velocityHeight - 38));
                    if (!started) { curve.startNewSubPath(x, y); started = true; }
                    else curve.lineTo(x, y);
                    const auto colour = automationColour(value);
                    if (automationBars)
                    {
                        const auto barWidth = juce::jmax(5.0f, static_cast<float>(stepWidthPx) * 0.52f);
                        g.setColour(colour);
                        g.fillRoundedRectangle(x - barWidth * 0.5f,
                            static_cast<float>(velocityTop + velocityHeight - 8) - value * (velocityHeight - 38),
                            barWidth, value * (velocityHeight - 38), 2.0f);
                    }
                    else
                    {
                        g.setColour(colour);
                        g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
                    }
                }
                if (!automationBars)
                {
                    for (int step = 1; step < activeStepCount; ++step)
                    {
                        const auto a = juce::jlimit(0.0f, 1.0f, getAutomationValue ? getAutomationValue(step - 1) : 0.5f);
                        const auto b = juce::jlimit(0.0f, 1.0f, getAutomationValue ? getAutomationValue(step) : 0.5f);
                        const auto x1 = static_cast<float>(labelWidthPx + (step - 1) * stepWidthPx + stepWidthPx / 2);
                        const auto x2 = static_cast<float>(labelWidthPx + step * stepWidthPx + stepWidthPx / 2);
                        const auto y1 = static_cast<float>(velocityTop + 26 + (1.0f - a) * (velocityHeight - 38));
                        const auto y2 = static_cast<float>(velocityTop + 26 + (1.0f - b) * (velocityHeight - 38));
                        g.setColour(automationColour((a + b) * 0.5f));
                        g.drawLine(x1, y1, x2, y2, 2.5f);
                    }
                }
                for (int step = 0; step < visibleStepCount; ++step)
                    g.setColour(step < activeStepCount ? juce::Colour::fromRGB(53, 75, 90)
                                                       : juce::Colour::fromRGB(38, 52, 62)),
                    g.drawVerticalLine(labelWidthPx + step * stepWidthPx,
                                       static_cast<float>(velocityTop + 22),
                                       static_cast<float>(velocityTop + velocityHeight));
            }
            else
            for (int step = 0; step < visibleStepCount; ++step)
            {
                const auto x = labelWidthPx + step * stepWidthPx;
                const bool inPattern = step < activeStepCount;
                g.setColour(inPattern ? juce::Colour::fromRGB(53, 75, 90) : juce::Colour::fromRGB(38, 52, 62));
                g.drawVerticalLine(x, static_cast<float>(velocityTop + 22), static_cast<float>(velocityTop + velocityHeight));
                const auto velocity = inPattern && getVelocityAtStep ? getVelocityAtStep(step) : 0;
                if (velocity > 0)
                {
                    g.setColour(juce::Colour::fromRGB(48, 158, 199));
                    const auto barHeight = juce::jmax(2, juce::roundToInt((velocity / 127.0f) * (velocityHeight - 38)));
                    const auto barX = x + stepWidthPx / 2 - juce::jmin(5, stepWidthPx / 3);
                    g.fillRoundedRectangle(static_cast<float>(barX),
                        static_cast<float>(velocityTop + velocityHeight - 8 - barHeight),
                        static_cast<float>(juce::jmin(10, stepWidthPx - 2)), static_cast<float>(barHeight), 2.0f);
                    if (velocityEditEnabled && !automationLaneVisible)
                    {
                        g.setColour(juce::Colour::fromRGB(176, 235, 250));
                        g.fillEllipse(static_cast<float>(x + stepWidthPx / 2 - 3),
                            static_cast<float>(velocityTop + velocityHeight - 8 - barHeight - 3), 6.0f, 6.0f);
                    }
                }
            }
        }
        const auto boundaryX = labelWidthPx + activeStepCount * stepWidthPx;
        g.setColour(juce::Colour::fromRGB(220, 73, 69));
        const auto gridBottom = (velocityLaneVisible || automationLaneVisible) ? velocityTop + velocityHeight : velocityTop;
        g.fillRect(boundaryX - 1, headerHeightPx, 2, gridBottom - headerHeightPx);
        if (playheadVisible)
        {
            const auto x = labelWidthPx + juce::roundToInt(playheadStep * stepWidthPx);
            g.fillRect(x - 1, headerHeightPx, 2, gridBottom - headerHeightPx);
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        grabKeyboardFocus();
        const auto velocityTop = headerHeight() + 37 * rowHeight() + 8;
        if ((velocityLaneVisible || automationLaneVisible)
            && std::abs(event.y - velocityTop) <= 8)
        {
            resizingLane = true;
            resizeLaneStartY = event.y;
            resizeLaneStartHeight = laneHeightPx;
            return;
        }
        if (automationLaneVisible && event.y >= velocityTop && event.x >= labelWidth()
            && (setAutomationValue || removeAutomationValue))
        {
            automationDragActive = true;
            if (automationGestureStarted) automationGestureStarted();
            lastAutomationStep = juce::jlimit(0, activeSteps() - 1, (event.x - labelWidth()) / stepWidth());
            lastAutomationValue = automationValueFromMouse(event);
            if (automationTool == 2)
            {
                if (removeAutomationValue) removeAutomationValue(lastAutomationStep);
            }
            else
            {
                setAutomationPointFromMouse(event);
            }
            repaint();
            return;
        }
        if (velocityLaneVisible && !automationLaneVisible && velocityEditEnabled
            && event.y >= velocityTop && event.x >= labelWidth() && setVelocityAtStep)
        {
            velocityGestureActive = true;
            if (velocityGestureStarted) velocityGestureStarted();
            setVelocityPointFromMouse(event);
            return;
        }
        const auto pitch = 84 - (event.y - headerHeight()) / rowHeight();
        const auto step = (event.x - labelWidth()) / stepWidth();
        if (event.y < headerHeight() || pitch < 48 || pitch > 84)
            return;
        // The piano keys are also a live audition keyboard. Keep the grid's
        // note-entry gesture separate so clicking a key never paints a note.
        if (event.x < labelWidth())
        {
            if (!event.mods.isRightButtonDown() && keyboardNoteChanged)
            {
                keyboardPitch = pitch;
                keyboardNoteChanged(pitch, true);
            }
            return;
        }
        if (!juce::isPositiveAndBelow(step, activeSteps())) return;
        const auto noteStart = findNoteStart(pitch, step);
        if (event.mods.isRightButtonDown())
        {
            if (noteStart >= 0 && noteClicked) noteClicked(pitch, noteStart, true);
            return;
        }
        dragPitch = pitch;
        dragStartStep = noteStart >= 0 ? noteStart : step;
        dragWasExisting = noteStart >= 0;
        dragPressStep = step;
        dragInitialLength = dragWasExisting && getNoteLength
            ? juce::jlimit(1, activeSteps() - dragStartStep, getNoteLength(pitch, dragStartStep)) : 1;
        dragResizingFromEdge = false;
        if (dragWasExisting)
        {
            const auto noteEndX = labelWidth() + (dragStartStep + dragInitialLength) * stepWidth() - 3;
            dragResizingFromEdge = noteEndX - event.x <= juce::jmax(7, stepWidth() / 4);
        }
        dragChanged = false;
        noteGestureActive = true;
        if (noteGestureStarted) noteGestureStarted();
        if (!dragWasExisting && noteClicked)
        {
            noteClicked(pitch, step, false);
            if (paintLength > 1 && noteLengthChanged)
                noteLengthChanged(pitch, step, juce::jmin(paintLength, activeSteps() - step));
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (resizingLane)
        {
            const auto maxHeight = getMaximumLaneHeight ? getMaximumLaneHeight() : 260;
            const auto nextHeight = juce::jlimit(64, juce::jmax(64, maxHeight),
                resizeLaneStartHeight + resizeLaneStartY - event.getPosition().y);
            if (nextHeight != laneHeightPx)
            {
                laneHeightPx = nextHeight;
                if (laneHeightChanged) laneHeightChanged();
                repaint();
            }
            return;
        }
        if (automationDragActive)
        {
            const auto step = juce::jlimit(0, activeSteps() - 1, (event.x - labelWidth()) / stepWidth());
            const auto value = automationValueFromMouse(event);
            if (automationTool == 2)
            {
                if (removeAutomationValue) removeAutomationValue(step);
            }
            else if (automationTool == 3 && moveAutomationValue)
            {
                moveAutomationValue(lastAutomationStep, step, value);
                lastAutomationStep = step;
            }
            else if (automationTool == 1 && setAutomationValue)
            {
                const auto start = lastAutomationStep;
                const auto end = step;
                const auto distance = juce::jmax(1, std::abs(end - start));
                for (int index = 0; index <= distance; ++index)
                {
                    const auto fraction = static_cast<float>(index) / static_cast<float>(distance);
                    const auto pointStep = start <= end ? start + index : start - index;
                    setAutomationValue(pointStep, juce::jmap(fraction, lastAutomationValue, value));
                }
                lastAutomationStep = step;
                lastAutomationValue = value;
            }
            else setAutomationPointFromMouse(event);
            repaint();
            return;
        }
        if (velocityGestureActive)
        {
            setVelocityPointFromMouse(event);
            return;
        }
        if (dragPitch < 0 || !noteLengthChanged) return;
        if (event.getDistanceFromDragStart() < 5) return;
        const auto pointerStep = juce::jlimit(0, activeSteps() - 1,
            (event.x - labelWidth()) / stepWidth());
        const auto length = dragWasExisting
            ? (dragResizingFromEdge
                ? juce::jlimit(1, activeSteps() - dragStartStep, pointerStep - dragStartStep + 1)
                : juce::jlimit(1, activeSteps() - dragStartStep,
                               dragInitialLength + pointerStep - dragPressStep))
            : juce::jlimit(1, activeSteps() - dragStartStep, pointerStep - dragStartStep + 1);
        if (dragWasExisting || length > 1)
        {
            noteLengthChanged(dragPitch, dragStartStep, length);
            if (!dragWasExisting)
            {
                paintLength = length;
                if (paintLengthUpdated) paintLengthUpdated(paintLength);
            }
            dragChanged = true;
            repaint();
        }
    }

    void setPaintLength(int length) { paintLength = juce::jlimit(1, 64, length); }
    int getPaintLength() const { return paintLength; }

    void mouseUp(const juce::MouseEvent&) override
    {
        resizingLane = false;
        if (automationDragActive && automationGestureEnded) automationGestureEnded();
        automationDragActive = false;
        if (velocityGestureActive && velocityGestureEnded) velocityGestureEnded();
        velocityGestureActive = false;
        if (keyboardPitch >= 0 && keyboardNoteChanged)
            keyboardNoteChanged(keyboardPitch, false);
        keyboardPitch = -1;
        if (noteGestureActive && noteGestureEnded) noteGestureEnded();
        noteGestureActive = false;
        dragPitch = -1;
        dragStartStep = -1;
        dragWasExisting = false;
        dragResizingFromEdge = false;
        dragChanged = false;
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        const auto velocityTop = headerHeight() + 37 * rowHeight() + 8;
        if ((velocityLaneVisible || automationLaneVisible)
            && std::abs(event.y - velocityTop) <= 8)
        {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            return;
        }
        const auto pitch = 84 - (event.y - headerHeight()) / rowHeight();
        const auto step = (event.x - labelWidth()) / stepWidth();
        if (event.x >= labelWidth() && event.y >= headerHeight()
            && pitch >= 48 && pitch <= 84 && juce::isPositiveAndBelow(step, activeSteps()))
        {
            const auto start = findNoteStart(pitch, step);
            if (start >= 0)
            {
                const auto length = juce::jlimit(1, activeSteps() - start,
                    getNoteLength ? getNoteLength(pitch, start) : 1);
                const auto noteEndX = labelWidth() + (start + length) * stepWidth() - 3;
                if (noteEndX - event.x <= juce::jmax(7, stepWidth() / 4))
                {
                    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                    return;
                }
            }
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

private:
    void setAutomationPointFromMouse(const juce::MouseEvent& event)
    {
        if (!setAutomationValue) return;
        const auto step = juce::jlimit(0, activeSteps() - 1, (event.x - labelWidth()) / stepWidth());
        setAutomationValue(step, automationValueFromMouse(event));
        repaint();
    }

    float automationValueFromMouse(const juce::MouseEvent& event) const
    {
        const auto laneTop = headerHeight() + 37 * rowHeight() + 8;
        return 1.0f - juce::jlimit(0.0f, 1.0f,
            static_cast<float>(event.y - laneTop - 26)
                / static_cast<float>(juce::jmax(1, laneHeightPx - 38)));
    }

    static juce::Colour automationColour(float value)
    {
        const auto v = juce::jlimit(0.0f, 1.0f, value);
        const auto low = juce::Colour::fromRGB(190, 62, 74);
        const auto middle = juce::Colour::fromRGB(161, 91, 180);
        const auto high = juce::Colour::fromRGB(57, 174, 218);
        return v < 0.5f ? low.interpolatedWith(middle, v * 2.0f)
                        : middle.interpolatedWith(high, (v - 0.5f) * 2.0f);
    }

    void setVelocityPointFromMouse(const juce::MouseEvent& event)
    {
        if (!setVelocityAtStep) return;
        const auto step = juce::jlimit(0, activeSteps() - 1, (event.x - labelWidth()) / stepWidth());
        const auto laneTop = headerHeight() + 37 * rowHeight() + 8;
        const auto velocity = juce::jlimit(1, 127, juce::roundToInt((1.0f
            - juce::jlimit(0.0f, 1.0f, static_cast<float>(event.y - laneTop - 22)
                / static_cast<float>(juce::jmax(1, velocityLaneHeight() - 30)))) * 126.0f) + 1);
        setVelocityAtStep(step, velocity);
        repaint();
    }

    int findNoteStart(int pitch, int step) const
    {
        if (!noteIsOn) return -1;
        for (int start = step; start >= 0; --start)
        {
            if (!noteIsOn(pitch, start)) continue;
            const auto length = juce::jlimit(1, activeSteps() - start,
                getNoteLength ? getNoteLength(pitch, start) : 1);
            if (step < start + length) return start;
        }
        return -1;
    }

    int dragPitch = -1;
    int dragStartStep = -1;
    int dragPressStep = -1;
    int dragInitialLength = 1;
    int keyboardPitch = -1;
    bool dragWasExisting = false;
    bool dragResizingFromEdge = false;
    bool dragChanged = false;
    bool noteGestureActive = false;
    int paintLength = 1;
    double playheadStep = 0.0;
    bool playheadVisible = false;
    int subdivisionsPerBar = 16;
    int highlightEveryN = 4;
    int laneHeightPx = defaultAutomationLaneHeight();
    float baseStepWidth = 22.0f, baseRowHeight = 16.0f;
    float horizontalZoom = 1.0f, verticalZoom = 1.0f;
    int automationTool = 0;
    int lastAutomationStep = -1;
    float lastAutomationValue = 0.5f;
    bool automationBars = false;
    bool velocityLaneVisible = false;
    bool automationLaneVisible = false;
    bool automationDragActive = false;
    bool resizingLane = false;
    int resizeLaneStartY = 0, resizeLaneStartHeight = defaultAutomationLaneHeight();
    bool velocityEditEnabled = false;
    bool velocityGestureActive = false;
    bool colouredKeys = true;
    bool showNoteNames = true;
    static juce::String pitchLabel(int pitch)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(names[pitch % 12]) + juce::String(pitch / 12 - 1);
    }
};

class PianoRollIconButton final : public juce::Button
{
public:
    enum class Icon { Velocity, VelocityEdit, Keyboard, NoteNames, Line, Erase, Move, Bars, Settings };
    PianoRollIconButton(const juce::String& name, Icon iconToDraw)
        : juce::Button(name), icon(iconToDraw)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const auto background = getToggleState() ? juce::Colour::fromRGB(35, 130, 165)
            : juce::Colour::fromRGB(30, 48, 61);
        g.setColour(down ? background.brighter(0.18f) : highlighted ? background.brighter(0.10f) : background);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(getToggleState() ? juce::Colour::fromRGB(188, 242, 255)
                                     : juce::Colour::fromRGB(148, 182, 198));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        auto iconArea = bounds.reduced(5.0f, 4.0f);
        if (icon == Icon::Velocity)
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto x = iconArea.getX() + 2.0f + static_cast<float>(i) * (iconArea.getWidth() - 4.0f) / 3.0f;
                const auto height = 4.0f + static_cast<float>((i * 7) % 10);
                g.fillRoundedRectangle(x, iconArea.getBottom() - height, 3.0f, height, 1.0f);
            }
        }
        else if (icon == Icon::VelocityEdit)
        {
            juce::Path pencil;
            pencil.startNewSubPath(iconArea.getX() + 3.0f, iconArea.getBottom() - 3.0f);
            pencil.lineTo(iconArea.getRight() - 3.0f, iconArea.getY() + 3.0f);
            g.setColour(getToggleState() ? juce::Colour::fromRGB(188, 242, 255)
                                         : juce::Colour::fromRGB(148, 182, 198));
            g.strokePath(pencil, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
            g.drawLine(iconArea.getX() + 2.0f, iconArea.getBottom() - 2.0f,
                       iconArea.getX() + 6.0f, iconArea.getBottom() - 2.0f, 1.5f);
        }
        else if (icon == Icon::Line)
        {
            g.drawLine(iconArea.getX() + 2.0f, iconArea.getBottom() - 2.0f,
                       iconArea.getRight() - 2.0f, iconArea.getY() + 2.0f, 2.0f);
        }
        else if (icon == Icon::Erase)
        {
            g.drawLine(iconArea.getX() + 2.0f, iconArea.getY() + 2.0f,
                       iconArea.getRight() - 2.0f, iconArea.getBottom() - 2.0f, 2.0f);
            g.drawLine(iconArea.getRight() - 2.0f, iconArea.getY() + 2.0f,
                       iconArea.getX() + 2.0f, iconArea.getBottom() - 2.0f, 2.0f);
        }
        else if (icon == Icon::Move)
        {
            const auto cx = iconArea.getCentreX(), cy = iconArea.getCentreY();
            g.drawLine(cx, iconArea.getY() + 1.0f, cx, iconArea.getBottom() - 1.0f, 1.5f);
            g.drawLine(iconArea.getX() + 1.0f, cy, iconArea.getRight() - 1.0f, cy, 1.5f);
            g.drawLine(cx - 2.5f, iconArea.getY() + 4.0f, cx, iconArea.getY() + 1.0f, 1.5f);
            g.drawLine(cx + 2.5f, iconArea.getY() + 4.0f, cx, iconArea.getY() + 1.0f, 1.5f);
        }
        else if (icon == Icon::Bars)
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto x = iconArea.getX() + 2.0f + i * (iconArea.getWidth() - 4.0f) / 3.0f;
                const auto height = 4.0f + static_cast<float>((i * 7) % 10);
                g.fillRoundedRectangle(x, iconArea.getBottom() - height, 3.0f, height, 1.0f);
            }
        }
        else if (icon == Icon::Settings)
        {
            const auto centre = iconArea.getCentre();
            for (int tooth = 0; tooth < 8; ++tooth)
            {
                const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(tooth) / 8.0f;
                const auto inner = centre.translated(std::cos(angle) * 4.5f, std::sin(angle) * 4.5f);
                const auto outer = centre.translated(std::cos(angle) * 7.0f, std::sin(angle) * 7.0f);
                g.drawLine(inner.x, inner.y, outer.x, outer.y, 2.0f);
            }
            g.drawEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f, 2.0f);
            g.fillEllipse(centre.x - 1.8f, centre.y - 1.8f, 3.6f, 3.6f);
        }
        else if (icon == Icon::Keyboard)
        {
            auto keys = iconArea.reduced(0.0f, 2.0f);
            g.fillRoundedRectangle(keys, 1.5f);
            g.setColour(juce::Colour::fromRGB(33, 51, 64));
            for (int i = 1; i < 5; ++i)
                g.drawVerticalLine(juce::roundToInt(keys.getX() + keys.getWidth() * i / 5.0f), keys.getY(), keys.getBottom());
            for (int i : { 1, 2, 4 })
            {
                const auto x = keys.getX() + keys.getWidth() * i / 5.0f - 1.5f;
                g.fillRect(x, keys.getY(), 3.0f, keys.getHeight() * 0.56f);
            }
        }
        else
        {
            g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("Aa", bounds.toNearestInt(), juce::Justification::centred);
        }
    }

private:
    Icon icon;
};

class PianoRollWindow final : public juce::Component
{
public:
    std::function<void()> onClose;
    std::function<void(juce::Point<int>, bool)> onManipulationStarted;
    std::function<void(juce::Rectangle<int>)> onBoundsChanged;
    PianoRollWindow(std::function<int()> activeStepCount,
                    std::function<bool(int, int)> isNoteOn,
                    std::function<bool()> areAllNotesSelected,
                    std::function<int(int, int)> getNoteLength,
                    std::function<void(int, int, bool)> onNoteClicked,
                    std::function<void(int, int, int)> onNoteLengthChanged,
                    std::function<void()> onNoteGestureStarted,
                    std::function<void()> onNoteGestureEnded,
                    std::function<void(int, bool)> onKeyboardNoteChanged)
    {
        setOpaque(true);
        addAndMakeVisible(title);
        title.setText("PIANO ROLL", juce::dontSendNotification);
        title.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(218, 237, 246));
        // Let the editor receive title-bar pointer events directly. A label
        // that intercepts the gesture prevents the parent drag handlers from
        // seeing the same mouse-down/drag sequence consistently.
        title.setInterceptsMouseClicks(true, false);
        title.addMouseListener(this, false);
        addAndMakeVisible(closeButton);
        closeButton.setButtonText("X");
        closeButton.onClick = [this] { if (onClose) onClose(); else setVisible(false); };
        addAndMakeVisible(zoomOutButton);
        zoomOutButton.setButtonText("−");
        zoomOutButton.setTooltip("Zoom out vertically in the piano roll");
        zoomOutButton.onClick = [this]
        {
            if (grid == nullptr) return;
            grid->setZoom(grid->getHorizontalZoom(), grid->getVerticalZoom() - 0.15f);
            resizeGridToViewport();
        };
        addAndMakeVisible(zoomInButton);
        zoomInButton.setButtonText("+");
        zoomInButton.setTooltip("Zoom in vertically in the piano roll");
        zoomInButton.onClick = [this]
        {
            if (grid == nullptr) return;
            grid->setZoom(grid->getHorizontalZoom(), grid->getVerticalZoom() + 0.15f);
            resizeGridToViewport();
        };
        addAndMakeVisible(horizontalZoomOutButton);
        horizontalZoomOutButton.setButtonText("H−");
        horizontalZoomOutButton.setTooltip("Zoom out horizontally in the piano roll");
        horizontalZoomOutButton.onClick = [this]
        {
            if (grid != nullptr) { grid->setZoom(grid->getHorizontalZoom() - 0.15f, grid->getVerticalZoom()); resizeGridToViewport(); }
        };
        addAndMakeVisible(horizontalZoomInButton);
        horizontalZoomInButton.setButtonText("H+");
        horizontalZoomInButton.setTooltip("Zoom in horizontally in the piano roll");
        horizontalZoomInButton.onClick = [this]
        {
            if (grid != nullptr) { grid->setZoom(grid->getHorizontalZoom() + 0.15f, grid->getVerticalZoom()); resizeGridToViewport(); }
        };
        addAndMakeVisible(automationToggleButton);
        automationToggleButton.setClickingTogglesState(true);
        automationToggleButton.setTooltip("Show or hide the Automation lane and its parameter controls");
        automationToggleButton.onClick = [this]
        {
            const bool automation = automationToggleButton.getToggleState();
            if (automation)
            {
                velocityButton.setToggleState(false, juce::dontSendNotification);
                velocityEditButton.setToggleState(false, juce::dontSendNotification);
                velocityEditButton.setVisible(false);
                if (grid != nullptr)
                {
                    grid->setVelocityEditEnabled(false);
                    grid->setVelocityLaneVisible(false);
                }
            }
            automationTargetSelector.setVisible(automation);
            if (grid != nullptr)
            {
                grid->setAutomationLaneVisible(automation);
                resized();
                resizeGridToViewport(true);
            }
        };
        addAndMakeVisible(automationTargetSelector);
        automationTargetSelector.setVisible(false);
        automationTargetSelector.setTooltip("Automated plugin parameter");
        automationTargetSelector.onChange = [this]
        {
            if (onAutomationTargetChanged && automationTargetSelector.getSelectedId() > 0)
            {
                const auto index = automationTargetSelector.getSelectedId() - 1;
                if (juce::isPositiveAndBelow(index, automationTargetIds.size()))
                    onAutomationTargetChanged(automationTargetIds[index]);
            }
            if (grid != nullptr) grid->repaint();
        };
        addAndMakeVisible(settingsButton);
        settingsButton.setTooltip("Piano roll settings");
        settingsButton.setClickingTogglesState(false);
        settingsButton.onClick = [this] { showSettings(); };
        for (auto* button : { &automationDrawButton, &automationLineButton, &automationEraseButton, &automationMoveButton })
        {
            addAndMakeVisible(*button);
            button->setVisible(false);
        }
        automationDrawButton.setTooltip("Freehand automation drawing");
        automationLineButton.setTooltip("Draw straight automation lines");
        automationEraseButton.setTooltip("Delete automation points");
        automationMoveButton.setTooltip("Move automation points");
        for (int i = 0; i < 4; ++i)
        {
            auto* button = automationToolButton(i);
            button->onClick = [this, i]
            {
                for (int other = 0; other < 4; ++other)
                    automationToolButton(other)->setToggleState(other == i, juce::dontSendNotification);
                if (grid != nullptr) grid->setAutomationTool(i);
            };
        }
        automationDrawButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(automationBarsButton);
        automationBarsButton.setVisible(false);
        automationBarsButton.setTooltip("Show automation as stepped bars");
        automationBarsButton.onClick = [this]
        {
            if (grid != nullptr) grid->setAutomationBars(automationBarsButton.getToggleState());
        };
        for (auto* button : { &laneSmallerButton, &laneLargerButton }) addAndMakeVisible(*button);
        laneSmallerButton.setVisible(false);
        laneLargerButton.setVisible(false);
        laneSmallerButton.setButtonText("−");
        laneLargerButton.setButtonText("+");
        laneSmallerButton.setTooltip("Make the automation lane shorter");
        laneLargerButton.setTooltip("Make the automation lane taller");
        laneSmallerButton.onClick = [this]
        {
            if (grid != nullptr) { grid->setAutomationLaneHeight(grid->getAutomationLaneHeight() - 16); resizeGridToViewport(); }
        };
        laneLargerButton.onClick = [this]
        {
            if (grid != nullptr) { grid->setAutomationLaneHeight(grid->getAutomationLaneHeight() + 16); resizeGridToViewport(); }
        };
        addAndMakeVisible(velocityButton);
        velocityButton.setTooltip("Show or hide the velocity lane");
        velocityButton.setToggleState(false, juce::dontSendNotification);
        velocityButton.onClick = [this]
        {
            if (grid != nullptr)
            {
                const bool showVelocity = velocityButton.getToggleState();
                if (showVelocity)
                {
                    automationToggleButton.setToggleState(false, juce::dontSendNotification);
                    automationTargetSelector.setVisible(false);
                    grid->setAutomationLaneVisible(false);
                }
                velocityEditButton.setToggleState(false, juce::dontSendNotification);
                grid->setVelocityEditEnabled(false);
                grid->setVelocityLaneVisible(showVelocity);
                velocityEditButton.setVisible(showVelocity);
                resized();
                resizeGridToViewport(true);
            }
        };
        addAndMakeVisible(velocityEditButton);
        velocityEditButton.setVisible(false);
        velocityEditButton.setTooltip("Enable velocity editing in the lower lane; click or drag to set note velocity");
        velocityEditButton.onClick = [this]
        {
            if (grid != nullptr) grid->setVelocityEditEnabled(velocityEditButton.getToggleState());
        };
        addAndMakeVisible(keyboardButton);
        keyboardButton.setTooltip("Colour the piano keyboard keys");
        keyboardButton.setToggleState(true, juce::dontSendNotification);
        keyboardButton.onClick = [this]
        {
            if (grid != nullptr) grid->setKeyboardOptions(keyboardButton.getToggleState(), grid->hasNoteNames());
        };
        addAndMakeVisible(noteNamesButton);
        noteNamesButton.setTooltip("Show pitch names beside the keyboard");
        noteNamesButton.setToggleState(true, juce::dontSendNotification);
        noteNamesButton.onClick = [this]
        {
            if (grid != nullptr) grid->setKeyboardOptions(grid->hasColouredKeys(), noteNamesButton.getToggleState());
        };
        grid = new PianoRollGrid();
        grid->setWantsKeyboardFocus(true);
        grid->getActiveStepCount = std::move(activeStepCount);
        grid->noteIsOn = std::move(isNoteOn);
        grid->areAllNotesSelected = std::move(areAllNotesSelected);
        grid->getNoteLength = std::move(getNoteLength);
        grid->noteClicked = std::move(onNoteClicked);
        grid->noteLengthChanged = std::move(onNoteLengthChanged);
        grid->noteGestureStarted = std::move(onNoteGestureStarted);
        grid->noteGestureEnded = std::move(onNoteGestureEnded);
        grid->keyboardNoteChanged = std::move(onKeyboardNoteChanged);
        grid->getAutomationValue = [this](int step) { return automationValue ? automationValue(step) : 0.5f; };
        grid->setAutomationValue = [this](int step, float value) { if (automationPointChanged) automationPointChanged(step, value); };
        grid->removeAutomationValue = [this](int step) { if (automationPointRemoved) automationPointRemoved(step); };
        grid->moveAutomationValue = [this](int from, int to, float value)
        {
            if (automationPointMoved) automationPointMoved(from, to, value);
        };
        grid->automationGestureStarted = [this] { if (onAutomationGestureStarted) onAutomationGestureStarted(); };
        grid->automationGestureEnded = [this] { if (onAutomationGestureEnded) onAutomationGestureEnded(); };
        grid->getVelocityAtStep = [this](int step)
        {
            int total = 0, count = 0;
            for (int pitch = 48; pitch <= 84; ++pitch)
                if (grid->noteIsOn && grid->noteIsOn(pitch, step))
                {
                    total += getVelocityAtStep ? getVelocityAtStep(step, pitch) : 100;
                    ++count;
                }
            return count > 0 ? total / count : 0;
        };
        grid->setVelocityAtStep = [this](int step, int velocity)
        {
            if (setVelocityAtStep) setVelocityAtStep(step, velocity);
        };
        grid->velocityGestureStarted = [this] { if (onVelocityGestureStarted) onVelocityGestureStarted(); };
        grid->velocityGestureEnded = [this] { if (onVelocityGestureEnded) onVelocityGestureEnded(); };
        grid->getMaximumLaneHeight = [this]
        {
            constexpr int noteGridMinimumHeight = 30 + 37 * 8 + 8;
            return juce::jmax(64, viewport.getHeight() - noteGridMinimumHeight);
        };
        grid->laneHeightChanged = [this] { resizeGridToViewport(); };
        grid->setSize(58 + grid->visibleSteps() * 58,
                      30 + 37 * 22 + 8 + grid->getAutomationLaneHeight());
        viewport.setViewedComponent(grid, true);
        addAndMakeVisible(viewport);
        resizeGrip.setMouseCursor(juce::MouseCursor::BottomRightCornerResizeCursor);
        resizeGrip.setTitle("Resize piano roll");
        resizeGrip.onDragStarted = [this](juce::Point<int> point)
        {
            if (onManipulationStarted) onManipulationStarted(point, true);
            resizing = true;
            resizeStartBounds = getBounds();
        };
        resizeGrip.onDragged = [this](juce::Point<int> delta)
        {
            if (!resizing) return;
            auto bounds = resizeStartBounds;
            bounds.setSize(juce::jmax(460, bounds.getWidth() + delta.x),
                           juce::jmax(260, bounds.getHeight() + delta.y));
            if (auto* parent = getParentComponent())
            {
                const auto workspace = parent->getLocalBounds().withTrimmedTop(82).withTrimmedBottom(48).reduced(14, 10);
                bounds.setSize(juce::jmin(bounds.getWidth(), workspace.getWidth()),
                               juce::jmin(bounds.getHeight(), workspace.getHeight()));
                bounds.setPosition(juce::jlimit(workspace.getX(), juce::jmax(workspace.getX(), workspace.getRight() - bounds.getWidth()), bounds.getX()),
                                   juce::jlimit(workspace.getY(), juce::jmax(workspace.getY(), workspace.getBottom() - bounds.getHeight()), bounds.getY()));
            }
            setBounds(bounds);
            if (onBoundsChanged) onBoundsChanged(getBounds());
        };
        resizeGrip.onDragEnded = [this] { resizing = false; };
        addAndMakeVisible(resizeGrip);
        setVisible(false);
    }

    void refresh(const juce::String& patternName)
    {
        title.setText("PIANO ROLL  |  " + patternName, juce::dontSendNotification);
        if (grid == nullptr) return;
        resized();
        grid->repaint();
    }

    void setPasteHandler(std::function<void()> handler)
    {
        if (grid != nullptr) grid->pasteNotes = std::move(handler);
    }

    void focusGrid()
    {
        if (grid != nullptr) grid->grabKeyboardFocus();
    }

    std::function<float(int)> automationValue;
    std::function<void(int, float)> automationPointChanged;
    std::function<void(int)> automationPointRemoved;
    std::function<void(int, int, float)> automationPointMoved;
    std::function<void()> onAutomationGestureStarted, onAutomationGestureEnded;
    std::function<int(int, int)> getVelocityAtStep;
    std::function<void(int, int)> setVelocityAtStep;
    std::function<void()> onVelocityGestureStarted, onVelocityGestureEnded;
    std::function<void(const juce::String&)> onAutomationTargetChanged;
    void setAutomationTargets(const juce::StringArray& labels, const juce::StringArray& ids, int selectedId = 1)
    {
        automationTargetSelector.clear(juce::dontSendNotification);
        automationTargetIds = ids;
        for (int i = 0; i < labels.size(); ++i) automationTargetSelector.addItem(labels[i], i + 1);
        if (labels.isEmpty()) automationTargetSelector.addItem("No automatable parameters", 1);
        automationTargetSelector.setSelectedId(juce::jlimit(1, juce::jmax(1, labels.size()), selectedId),
                                                juce::dontSendNotification);
        if (onAutomationTargetChanged && !ids.isEmpty()) onAutomationTargetChanged(ids[0]);
    }

    void setPlayheadStep(double step, bool active)
    {
        if (grid != nullptr) grid->setPlayheadStep(step, active);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6);
        auto header = area.removeFromTop(28);
        title.setBounds(header);
        closeButton.setBounds(header.removeFromRight(28).reduced(1));
        auto gripBounds = area.removeFromBottom(18).removeFromRight(18);
        resizeGrip.setBounds(gripBounds);
        auto footer = area.removeFromBottom(27);
        // Keep time zoom controls with the other lower-right tools. The
        // previous lower-left placement separated them from the lane tools.
        horizontalZoomOutButton.setBounds(footer.removeFromRight(32).reduced(1));
        horizontalZoomInButton.setBounds(footer.removeFromRight(32).reduced(1));
        footer.removeFromRight(2);
        keyboardButton.setBounds({});
        noteNamesButton.setBounds({});
        keyboardButton.setVisible(false);
        noteNamesButton.setVisible(false);
        settingsButton.setBounds(footer.removeFromRight(28).reduced(1));
        automationToggleButton.setBounds(footer.removeFromRight(86).reduced(1));
        velocityButton.setBounds(footer.removeFromRight(28).reduced(1));
        if (automationToggleButton.getToggleState())
        {
            velocityEditButton.setBounds({});
            automationTargetSelector.setBounds(footer.removeFromRight(230).reduced(2, 2));
            automationDrawButton.setBounds(footer.removeFromRight(26).reduced(1));
            automationLineButton.setBounds(footer.removeFromRight(26).reduced(1));
            automationEraseButton.setBounds(footer.removeFromRight(26).reduced(1));
            automationMoveButton.setBounds(footer.removeFromRight(26).reduced(1));
            automationBarsButton.setBounds(footer.removeFromRight(26).reduced(1));
            laneLargerButton.setBounds(footer.removeFromRight(24).reduced(1));
            laneSmallerButton.setBounds(footer.removeFromRight(24).reduced(1));
            for (auto* button : std::array<juce::Component*, 7> { &automationDrawButton, &automationLineButton,
                    &automationEraseButton, &automationMoveButton, &automationBarsButton, &laneSmallerButton,
                    &laneLargerButton })
                button->setVisible(true);
        }
        else
        {
            for (auto* button : std::array<juce::Component*, 7> { &automationDrawButton, &automationLineButton,
                    &automationEraseButton, &automationMoveButton, &automationBarsButton, &laneSmallerButton,
                    &laneLargerButton })
            {
                button->setBounds({});
                button->setVisible(false);
            }
            automationTargetSelector.setBounds({});
            if (velocityEditButton.isVisible())
                velocityEditButton.setBounds(footer.removeFromRight(28).reduced(1));
            else
                velocityEditButton.setBounds({});
        }
        // Pitch-range zoom lives on a slim rail beside the piano roll, like
        // Orion's right-edge controls, rather than in the title bar.
        auto sideRail = area.removeFromRight(28);
        const auto sideX = sideRail.getX() + 1;
        const auto sideBottom = sideRail.getBottom();
        zoomOutButton.setBounds(sideX, juce::jmax(sideRail.getY(), sideBottom - 50), 25, 23);
        zoomInButton.setBounds(sideX, juce::jmax(sideRail.getY(), sideBottom - 26), 25, 23);
        viewport.setBounds(area);
        resizeGridToViewport();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        const auto point = event.getEventRelativeTo(this).getPosition();
        if (point.y < 28 && point.x < getWidth() - 34)
        {
            const auto parentPoint = event.getEventRelativeTo(getParentComponent()).position.toInt();
            if (onManipulationStarted) onManipulationStarted(parentPoint, false);
            dragging = true;
            dragStartBounds = getBounds();
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!dragging) return;
        auto bounds = dragStartBounds;
        bounds.setPosition(bounds.getPosition() + event.getOffsetFromDragStart());
        if (auto* parent = getParentComponent())
        {
            const auto workspace = parent->getLocalBounds().withTrimmedTop(82).withTrimmedBottom(48).reduced(14, 10);
            bounds.setPosition(juce::jlimit(workspace.getX(), juce::jmax(workspace.getX(), workspace.getRight() - bounds.getWidth()), bounds.getX()),
                               juce::jlimit(workspace.getY(), juce::jmax(workspace.getY(), workspace.getBottom() - bounds.getHeight()), bounds.getY()));
        }
        setBounds(bounds);
        if (onBoundsChanged) onBoundsChanged(getBounds());
    }

    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(22, 32, 42));
        g.setColour(juce::Colour::fromRGB(103, 139, 157));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);
    }

private:
    class ResizeGrip final : public juce::Component
    {
    public:
        std::function<void(juce::Point<int>)> onDragStarted;
        std::function<void(juce::Point<int>)> onDragged;
        std::function<void()> onDragEnded;
        void mouseDown(const juce::MouseEvent& event) override
        {
            auto* window = getParentComponent();
            auto* workspace = window != nullptr ? window->getParentComponent() : nullptr;
            if (onDragStarted && workspace != nullptr)
                onDragStarted(event.getEventRelativeTo(workspace).position.toInt());
        }
        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (onDragged) onDragged(event.getOffsetFromDragStart());
        }
        void mouseUp(const juce::MouseEvent&) override { if (onDragEnded) onDragEnded(); }
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour::fromRGB(112, 179, 201));
            for (int inset = 5; inset <= 13; inset += 4)
                g.drawLine(static_cast<float>(getWidth() - inset), static_cast<float>(getHeight()),
                           static_cast<float>(getWidth()), static_cast<float>(getHeight() - inset), 1.5f);
        }
    };

    void showSettings()
    {
        juce::PopupMenu menu;
        juce::PopupMenu subdivisions;
        for (const int count : { 8, 16, 24, 32, 48, 64 })
            subdivisions.addItem(100 + count, juce::String(count) + " subdivisions per bar",
                                 true, grid != nullptr && grid->getSubdivisionsPerBar() == count);
        menu.addSubMenu("Grid subdivisions per bar", subdivisions);
        juce::PopupMenu emphasis;
        for (const int count : { 1, 2, 4, 8, 16 })
            emphasis.addItem(200 + count, "Highlight every " + juce::String(count) + " steps",
                             true, grid != nullptr && grid->getHighlightEveryN() == count);
        menu.addSubMenu("Highlight grid every", emphasis);
        juce::Component::SafePointer<PianoRollWindow> safeThis(this);
        menu.showMenuAsync({}, [safeThis](int result)
        {
            if (safeThis == nullptr || safeThis->grid == nullptr) return;
            if (result >= 108 && result <= 164)
                safeThis->grid->setGridOptions(result - 100, juce::jmin(safeThis->grid->getHighlightEveryN(), result - 100));
            else if (result >= 201 && result <= 216)
                safeThis->grid->setGridOptions(safeThis->grid->getSubdivisionsPerBar(), result - 200);
        });
    }

    void resizeGridToViewport(bool resetVerticalScroll = false)
    {
        if (grid == nullptr) return;
        constexpr int labelWidth = 58;
        constexpr int headerHeight = 30;
        const auto columns = grid->visibleSteps();
        grid->setViewMetrics(viewport.getWidth(), viewport.getHeight());
        const auto contentHeight = headerHeight + 37 * grid->rowHeight() + 8
            + ((grid->isVelocityLaneVisible() || grid->isAutomationLaneVisible()) ? grid->getAutomationLaneHeight() : 0);
        grid->setSize(labelWidth + columns * grid->stepWidth(), juce::jmax(viewport.getHeight(), contentHeight));
        if (resetVerticalScroll)
            viewport.setViewPosition(viewport.getViewPositionX(), 0);
    }

    PianoRollGrid* grid = nullptr;
    juce::Viewport viewport;
    juce::Label title;
    juce::TextButton closeButton;
    PianoRollIconButton settingsButton { "Piano roll settings", PianoRollIconButton::Icon::Settings };
    juce::TextButton zoomOutButton, zoomInButton, horizontalZoomOutButton, horizontalZoomInButton;
    ResizeGrip resizeGrip;
    PianoRollIconButton velocityButton { "Velocity lane", PianoRollIconButton::Icon::Velocity };
    PianoRollIconButton velocityEditButton { "Edit note velocities", PianoRollIconButton::Icon::VelocityEdit };
    PianoRollIconButton automationDrawButton { "Freehand automation", PianoRollIconButton::Icon::VelocityEdit };
    PianoRollIconButton automationLineButton { "Line automation", PianoRollIconButton::Icon::Line };
    PianoRollIconButton automationEraseButton { "Erase automation", PianoRollIconButton::Icon::Erase };
    PianoRollIconButton automationMoveButton { "Move automation", PianoRollIconButton::Icon::Move };
    PianoRollIconButton automationBarsButton { "Automation bars", PianoRollIconButton::Icon::Bars };
    juce::TextButton laneSmallerButton, laneLargerButton;
    PianoRollIconButton keyboardButton { "Colour keyboard", PianoRollIconButton::Icon::Keyboard };
    PianoRollIconButton noteNamesButton { "Note names", PianoRollIconButton::Icon::NoteNames };
    juce::ComboBox automationTargetSelector;
    juce::TextButton automationToggleButton { "Automation" };
    juce::StringArray automationTargetIds;
    juce::Rectangle<int> dragStartBounds, resizeStartBounds;
    juce::Point<int> dragStartPoint, resizeStartPoint;
    bool dragging = false, resizing = false;

    PianoRollIconButton* automationToolButton(int index)
    {
        switch (index)
        {
            case 1: return &automationLineButton;
            case 2: return &automationEraseButton;
            case 3: return &automationMoveButton;
            default: return &automationDrawButton;
        }
    }
};

// A dedicated drag source makes the generator move gesture independent from
// JUCE's parent hit testing and any controls layered over the generator.
class GeneratorDragLabel final : public juce::Label
{
public:
    std::function<void(juce::Point<int>)> dragStarted;
    std::function<void(juce::Point<int>)> dragged;
    std::function<void()> dragEnded;

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (dragStarted)
            dragStarted(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (dragged)
            dragged(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragEnded) dragEnded();
    }
};

class GeneratorResizeHandle final : public juce::Component
{
public:
    std::function<void(juce::Point<int>)> resizeStarted;
    std::function<void(juce::Point<int>)> resizedByDrag;
    std::function<void()> resizeEnded;

    explicit GeneratorResizeHandle(juce::String handleTitle = "Resize generator")
    {
        setMouseCursor(juce::MouseCursor::BottomRightCornerResizeCursor);
        setTitle(handleTitle);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (resizeStarted)
            resizeStarted(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (resizedByDrag)
            resizedByDrag(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (resizeEnded) resizeEnded();
    }

    void paint(juce::Graphics&) override
    {
        // Keep the corner hit target invisible: the embedded plugin editor has
        // its own controls, and a second drawn resize grip looked like a second
        // nested window.
    }
};

class OrbitAuditionKeyboard final : public juce::Component
{
public:
    std::function<void(int, bool)> onNote;

    void paint(juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        g.fillAll(juce::Colour::fromRGB(24, 36, 45));
        if (area.isEmpty()) return;
        constexpr int whiteCount = 14;
        const auto whiteWidth = area.getWidth() / static_cast<float>(whiteCount);
        const auto whiteNoteForIndex = [](int index)
        {
            constexpr std::array<int, 7> semitones { 0, 2, 4, 5, 7, 9, 11 };
            return 60 + (index / 7) * 12 + semitones[static_cast<size_t>(index % 7)];
        };
        for (int i = 0; i < whiteCount; ++i)
        {
            auto key = juce::Rectangle<float>(i * whiteWidth, 0, whiteWidth, area.getHeight());
            g.setColour(activePitch == whiteNoteForIndex(i) ? juce::Colour::fromRGB(105, 193, 218)
                                                            : juce::Colour::fromRGB(226, 234, 237));
            g.fillRect(key.reduced(0.5f));
            g.setColour(juce::Colour::fromRGB(41, 61, 72));
            g.drawRect(key, 1.0f);
        }
        constexpr std::array<int, 10> blackAfterWhite { 0, 1, 3, 4, 5, 7, 8, 10, 11, 12 };
        const auto blackWidth = whiteWidth * 0.58f;
        const auto blackHeight = area.getHeight() * 0.62f;
        for (int i = 0; i < static_cast<int>(blackAfterWhite.size()); ++i)
        {
            const auto whiteIndex = blackAfterWhite[static_cast<size_t>(i)];
            const auto octave = whiteIndex / 7;
            const auto within = whiteIndex % 7;
            constexpr std::array<int, 7> blackSemitones { 1, 3, -1, 6, 8, 10, -1 };
            const auto pitch = 60 + octave * 12 + blackSemitones[static_cast<size_t>(within)];
            auto key = juce::Rectangle<float>((whiteIndex + 1) * whiteWidth - blackWidth * 0.5f, 0, blackWidth, blackHeight);
            g.setColour(activePitch == pitch ? juce::Colour::fromRGB(51, 158, 191) : juce::Colour::fromRGB(23, 34, 42));
            g.fillRoundedRectangle(key, 2.0f);
            g.setColour(juce::Colour::fromRGB(82, 112, 126));
            g.drawRoundedRectangle(key, 2.0f, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& event) override { updatePitch(event.position.x, true); }
    void mouseDrag(const juce::MouseEvent& event) override { updatePitch(event.position.x, true); }
    void mouseUp(const juce::MouseEvent&) override { releasePitch(); }

private:
    int pitchAt(float x) const
    {
        constexpr int whiteCount = 14;
        const auto whiteWidth = static_cast<float>(getWidth()) / static_cast<float>(whiteCount);
        if (whiteWidth <= 0.0f) return 60;
        const auto whiteIndex = juce::jlimit(0, whiteCount - 1, static_cast<int>(x / whiteWidth));
        constexpr std::array<int, 7> whiteSemitones { 0, 2, 4, 5, 7, 9, 11 };
        constexpr std::array<int, 7> blackSemitones { 1, 3, -1, 6, 8, 10, -1 };
        const auto within = whiteIndex % 7;
        const auto boundaryX = (whiteIndex + 1) * whiteWidth;
        const auto blackWidth = whiteWidth * 0.29f;
        if (blackSemitones[static_cast<size_t>(within)] >= 0 && x >= boundaryX - blackWidth && x < boundaryX + blackWidth)
            return 60 + (whiteIndex / 7) * 12 + blackSemitones[static_cast<size_t>(within)];
        if (whiteIndex > 0 && blackSemitones[static_cast<size_t>((whiteIndex - 1) % 7)] >= 0
            && x < whiteIndex * whiteWidth + blackWidth)
            return 60 + ((whiteIndex - 1) / 7) * 12 + blackSemitones[static_cast<size_t>((whiteIndex - 1) % 7)];
        return 60 + (whiteIndex / 7) * 12 + whiteSemitones[static_cast<size_t>(within)];
    }

    void updatePitch(float x, bool down)
    {
        if (!down) return;
        const auto nextPitch = pitchAt(x);
        if (nextPitch == activePitch) return;
        releasePitch();
        activePitch = nextPitch;
        if (onNote) onNote(activePitch, true);
        repaint();
    }

    void releasePitch()
    {
        if (activePitch < 0) return;
        if (onNote) onNote(activePitch, false);
        activePitch = -1;
        repaint();
    }

    int activePitch = -1;
};

class MixerMoveHandle final : public juce::Component
{
public:
    std::function<void(juce::Point<int>)> dragStarted;
    std::function<void(juce::Point<int>)> dragged;
    std::function<void()> dragEnded;

    MixerMoveHandle()
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        setTitle("Move mixer");
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (dragStarted) dragStarted(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (dragged) dragged(event.getEventRelativeTo(getParentComponent()).position.toInt());
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragEnded) dragEnded();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour::fromRGB(183, 218, 233));
        const auto centreX = getWidth() * 0.5f;
        const auto centreY = getHeight() * 0.5f;
        for (int row = -1; row <= 1; ++row)
            for (int column = -1; column <= 1; ++column)
                g.fillEllipse(centreX + column * 4.0f - 1.2f, centreY + row * 4.0f - 1.2f, 2.4f, 2.4f);
    }
};

class OrbitPluginEditorContent final : public te::Plugin::EditorComponent
{
public:
    explicit OrbitPluginEditorContent(te::ExternalPlugin& external)
        : editor(external.getAudioPluginInstance() != nullptr
                     ? std::unique_ptr<juce::AudioProcessorEditor>(external.getAudioPluginInstance()->createEditorIfNeeded())
                     : nullptr)
    {
        if (editor == nullptr && external.getAudioPluginInstance() != nullptr)
            editor = std::make_unique<juce::GenericAudioProcessorEditor>(*external.getAudioPluginInstance());
        if (editor != nullptr)
        {
            addAndMakeVisible(*editor);
            setSize(juce::jmax(220, editor->getWidth()), juce::jmax(120, editor->getHeight()));
        }
        else
        {
            setSize(320, 120);
        }
    }

    bool allowWindowResizing() override { return editor != nullptr && editor->isResizable(); }
    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override
    {
        return editor != nullptr ? editor->getConstrainer() : nullptr;
    }
    void resized() override
    {
        if (editor != nullptr) editor->setBounds(getLocalBounds());
    }

private:
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};

class OrbitPluginWindow final : public juce::DocumentWindow
{
public:
    explicit OrbitPluginWindow(te::Plugin::WindowState& state)
        : DocumentWindow(state.plugin.getName(), juce::Colour::fromRGB(20, 29, 38), closeButton),
          pluginState(state), plugin(state.plugin)
    {
        setUsingNativeTitleBar(true);
        if (auto* external = dynamic_cast<te::ExternalPlugin*>(&plugin))
            editor = std::make_unique<OrbitPluginEditorContent>(*external);
        if (editor != nullptr)
        {
            setContentNonOwned(editor.get(), true);
            setResizable(editor->allowWindowResizing(), false);
            if (editor->allowWindowResizing()) setConstrainer(editor->getBoundsConstrainer());
            centreWithSize(juce::jmax(260, editor->getWidth()), juce::jmax(150, editor->getHeight()));
        }
        else
        {
            auto* message = new juce::Label({}, "This plugin does not provide an editor.");
            message->setJustificationType(juce::Justification::centred);
            setContentOwned(message, true);
            centreWithSize(360, 120);
        }
        setVisible(true);
        toFront(false);
    }

    void closeButtonPressed() override { pluginState.closeWindowExplicitly(); }

private:
    te::Plugin::WindowState& pluginState;
    te::Plugin& plugin;
    std::unique_ptr<te::Plugin::EditorComponent> editor;
};

// A small, native sampler front-end for Orbit's first drum-rack iteration.
// Each pad maps to one MIDI note; its 16-step row edits the normal pattern
// data, so rack hits use the same playback, playlist, and save path as notes.
class DrumRackWindow final : public juce::DocumentWindow
{
public:
    DrumRackWindow(std::function<void(int)> loadSample,
                   std::function<void(int)> padSelected,
                   std::function<void(int, int, bool)> setStep)
        : DocumentWindow("Drum Rack", juce::Colour::fromRGB(25, 37, 46), closeButton),
          loadSampleCallback(std::move(loadSample)), padSelectedCallback(std::move(padSelected)),
          setStepCallback(std::move(setStep))
    {
        setUsingNativeTitleBar(true);
        content.setLoadSampleHandler([this] { if (loadSampleCallback) loadSampleCallback(selectedPad); });
        content.setPadSelectedHandler([this](int pad)
        {
            selectedPad = pad;
            refreshPadSelection();
            if (padSelectedCallback) padSelectedCallback(pad);
        });
        content.setStepHandler([this](int pad, int step, bool enabled)
        {
            if (setStepCallback) setStepCallback(pad, step, enabled);
        });
        setContentOwned(&content, false);
        setResizable(true, true);
        setResizeLimits(540, 440, 1100, 850);
        centreWithSize(760, 570);
        setVisible(true);
        toFront(false);
    }

    void closeButtonPressed() override { setVisible(false); }

    void setPadSampleName(int pad, const juce::String& name) { content.setPadSampleName(pad, name); }
    void setSteps(int pad, const std::array<bool, 16>& steps) { content.setSteps(pad, steps); }
    int getSelectedPad() const { return selectedPad; }

private:
    class Content final : public juce::Component
    {
    public:
        Content()
        {
            setOpaque(true);
            title.setText("DRUM RACK  ·  16 PADS", juce::dontSendNotification);
            title.setFont(juce::FontOptions(16.0f, juce::Font::bold));
            title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(221, 236, 244));
            addAndMakeVisible(title);
            configureButton(loadButton, "Load Sample…");
            addAndMakeVisible(loadButton);
            for (int pad = 0; pad < 16; ++pad)
            {
                auto button = std::make_unique<juce::TextButton>();
                configureButton(*button, "PAD " + juce::String(pad + 1));
                button->setClickingTogglesState(true);
                button->setRadioGroupId(0x4f52);
                button->setToggleState(pad == selectedPad, juce::dontSendNotification);
                button->onClick = [this, pad] { selectedPad = pad; if (padSelected) padSelected(pad); repaint(); };
                addAndMakeVisible(*button);
                pads.push_back(std::move(button));
                for (int step = 0; step < 16; ++step)
                {
                    auto stepButton = std::make_unique<juce::TextButton>();
                    configureButton(*stepButton, {});
                    stepButton->setClickingTogglesState(true);
                    stepButton->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(43, 150, 190));
                    stepButton->onClick = [this, pad, step, raw = stepButton.get()]
                    { if (stepChanged) stepChanged(pad, step, raw->getToggleState()); };
                    addAndMakeVisible(*stepButton);
                    stepButtons.push_back(std::move(stepButton));
                }
            }
            loadButton.onClick = [this] { if (loadSample) loadSample(); };
        }

        void setLoadSampleHandler(std::function<void()> fn) { loadSample = std::move(fn); }
        void setPadSelectedHandler(std::function<void(int)> fn) { padSelected = std::move(fn); }
        void setStepHandler(std::function<void(int, int, bool)> fn) { stepChanged = std::move(fn); }
        void setSteps(int pad, const std::array<bool, 16>& values)
        {
            if (!juce::isPositiveAndBelow(pad, 16)) return;
            for (int i = 0; i < 16; ++i)
                stepButtons[static_cast<size_t>(pad * 16 + i)]->setToggleState(values[static_cast<size_t>(i)], juce::dontSendNotification);
        }
        void setPadSampleName(int pad, const juce::String& name)
        {
            if (!juce::isPositiveAndBelow(pad, static_cast<int>(pads.size()))) return;
            pads[static_cast<size_t>(pad)]->setButtonText("PAD " + juce::String(pad + 1) + "\n" + name);
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour::fromRGB(34, 49, 60));
            g.setColour(juce::Colour::fromRGB(68, 101, 118));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 8.0f, 1.0f);
            g.setColour(juce::Colour::fromRGB(180, 205, 218));
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText("PAD / SAMPLE", padHeaderBounds, juce::Justification::centredLeft);
            for (int step = 0; step < 16; ++step)
                g.drawText(juce::String(step + 1), stepHeaderBounds.withX(stepHeaderBounds.getX() + step * stepCellWidth)
                               .withWidth(stepCellWidth), juce::Justification::centred);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(18);
            auto header = area.removeFromTop(38);
            title.setBounds(header.removeFromLeft(330));
            loadButton.setBounds(header.removeFromRight(150).reduced(0, 3));
            area.removeFromTop(6);
            const int padWidth = juce::jmin(190, area.getWidth() / 4);
            auto stepHeader = area.removeFromTop(22);
            padHeaderBounds = stepHeader.removeFromLeft(padWidth);
            stepHeaderBounds = stepHeader;
            stepCellWidth = stepHeaderBounds.getWidth() / 16;
            const int rowHeight = area.getHeight() / 16;
            for (int i = 0; i < 16; ++i)
            {
                const int y = area.getY() + i * rowHeight;
                pads[static_cast<size_t>(i)]->setBounds(area.getX(), y + 1, padWidth - 5, rowHeight - 2);
                for (int step = 0; step < 16; ++step)
                {
                    auto* button = stepButtons[static_cast<size_t>(i * 16 + step)].get();
                    button->setBounds(area.getX() + padWidth + step * stepCellWidth + 1, y + 2,
                                      stepCellWidth - 2, rowHeight - 4);
                }
            }
        }

        int selectedPad = 0;
    private:
        void configureButton(juce::TextButton& button, const juce::String& text)
        {
            button.setButtonText(text);
            button.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(43, 62, 74));
            button.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(39, 129, 164));
            button.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(216, 231, 238));
            button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        }
        juce::Label title;
        juce::TextButton loadButton;
        std::vector<std::unique_ptr<juce::TextButton>> pads, stepButtons;
        std::function<void()> loadSample;
        std::function<void(int)> padSelected;
        std::function<void(int, int, bool)> stepChanged;
        juce::Rectangle<int> padHeaderBounds, stepHeaderBounds;
        int stepCellWidth = 1;
    } content;

    void refreshPadSelection() { content.repaint(); }
    std::function<void(int)> loadSampleCallback;
    std::function<void(int)> padSelectedCallback;
    std::function<void(int, int, bool)> setStepCallback;
    int selectedPad = 0;
};

class OrbitUIBehaviour final : public te::UIBehaviour
{
public:
    std::unique_ptr<juce::Component> createPluginWindow(te::PluginWindowState& state) override
    {
        if (auto* pluginState = dynamic_cast<te::Plugin::WindowState*>(&state))
            return std::make_unique<OrbitPluginWindow>(*pluginState);
        return {};
    }
};

class ProjectView final : public juce::Component, private juce::Timer
{
public:
    enum class PlaybackMode { pattern, song };
    void mouseDown(const juce::MouseEvent&) override { bringGeneratorToFront(); }
    ProjectView() : engine("Orbit", std::make_unique<OrbitUIBehaviour>(), nullptr)
    {
        // Moving the overlapping playlist, generator, and piano roll panels
        // exposes this component repeatedly. Paint it as an opaque backing so
        // old child pixels cannot remain as scrambled strips during a drag.
        setOpaque(true);
        juce::PropertiesFile::Options settingsOptions;
        settingsOptions.applicationName = "Orbit";
        settingsOptions.filenameSuffix = "settings";
        settingsOptions.osxLibrarySubFolder = "Application Support";
        applicationProperties.setStorageParameters(settingsOptions);
        if (auto* settings = applicationProperties.getUserSettings())
            showHints = settings->getBoolValue("showHints", true);
        tooltipWindow = std::make_unique<juce::TooltipWindow>(this, showHints ? 650 : 2147483647);

        title.setText("Orbit", juce::dontSendNotification);
        title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
        title.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title);

        const std::array<std::pair<juce::TextButton*, juce::String>, 7> menuDefinitions {{
            { &fileMenuButton, "File" }, { &editMenuButton, "Edit" }, { &viewMenuButton, "View" },
            { &insertMenuButton, "Insert" }, { &optionsMenuButton, "Options" },
            { &windowMenuButton, "Window" }, { &helpMenuButton, "Help" }
        }};
        for (int i = 0; i < static_cast<int>(menuDefinitions.size()); ++i)
        {
            auto* button = menuDefinitions[static_cast<size_t>(i)].first;
            button->setButtonText(menuDefinitions[static_cast<size_t>(i)].second);
            button->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(42, 48, 54));
            button->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(57, 67, 75));
            button->setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(224, 231, 235));
            button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            button->onClick = [this, i, button] { showMenu(*button, i); };
            addAndMakeVisible(*button);
        }

        configureButton(newProjectButton, "New Project", [this] { createProject(); });
        configureButton(openButton, "Open", [this] { openProject(); });
        configureButton(saveButton, "Save", [this] { saveProject(); });
        configureButton(midiExportButton, "Export MIDI", [this] { exportMidi(); });
        configureButton(audioSettingsButton, "Audio Settings", [this] { showAudioSettings(); });
        configureButton(addTrackButton, "Add Instrument", [this] { addInstrumentTrack(); });
        configureButton(patternButton, "New Pattern", [this] { addPattern(); });
        configureButton(playlistButton, "Playlist", [this] { togglePlaylistView(); });
        configureButton(playButton, "Play", [this] { togglePlayback(); });
        configureButton(stopButton, "Stop", [this] { stopPlayback(); });
        configureButton(patternModeButton, "Pattern", [this] { setPlaybackMode(PlaybackMode::pattern); });
        configureButton(songModeButton, "Song", [this] { setPlaybackMode(PlaybackMode::song); });
        for (auto* modeButton : { &patternModeButton, &songModeButton })
        {
            modeButton->setClickingTogglesState(false);
        }
        patternModeButton.setTooltip("Pattern mode plays each generator's remembered pattern together.");
        songModeButton.setTooltip("Song mode plays clips arranged in the Playlist.");
        updatePlaybackModeButtons();
        configureButton(mixerButton, "Mixer", [this] { toggleMixerView(); });
        configureButton(masterButton, "Master", [this] { showMixer(true); });
        configureButton(recordButton, "Record", [] {});
        recordButton.setEnabled(false);
        recordButton.setTooltip("Audio recording is not implemented yet.");
        playButton.setTextVisible(false);
        stopButton.setTextVisible(false);
        recordButton.setTextVisible(false);
        rewindButton.setTextVisible(false);
        previousButton.setTextVisible(false);
        nextButton.setTextVisible(false);
        playlistButton.setButtonText("Playlist");
        mixerButton.setButtonText("Mixer");
        masterButton.setButtonText("Master");
        stopButton.setButtonText("Stop");
        stopButton.setTooltip("Stop playback.");
        playButton.setButtonText("Play");
        playButton.setTooltip("Start playback.");
        rewindButton.setTooltip("Return playback to the beginning.");
        configureButton(rewindButton, "Rewind", [this] { rewindPlayback(); });
        configureButton(previousButton, "Previous Bar", [this] { skipTransport(-1); });
        configureButton(nextButton, "Next Bar", [this] { skipTransport(1); });
        configureButton(generatorMuteButton, "M", [this]
        {
            const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
            const auto index = tracks.indexOf(instrumentTrack);
            if (juce::isPositiveAndBelow(index, tracks.size())) setTrackMute(index, generatorMuteButton.getToggleState());
        });
        configureButton(generatorSoloButton, "S", [this]
        {
            const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
            const auto index = tracks.indexOf(instrumentTrack);
            if (juce::isPositiveAndBelow(index, tracks.size())) setTrackSolo(index, generatorSoloButton.getToggleState());
        });
        generatorMuteButton.setClickingTogglesState(true);
        generatorSoloButton.setClickingTogglesState(true);
        generatorMuteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(136, 70, 66));
        generatorSoloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(151, 123, 53));
        generatorMuteButton.setTooltip("Mute this generator.");
        generatorSoloButton.setTooltip("Solo this generator.");
        configureButton(generatorSettingsButton, "Pattern Settings", [this]
        {
            juce::PopupMenu settings;
            settings.addItem(1, "Pattern settings will be added in a later iteration", false);
            settings.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&generatorSettingsButton), [](int) {});
        });
        generatorSettingsButton.setTextVisible(false);
        configureButton(openPresetButton, "Open Pattern Preset", [this]
        {
            status.setText("Pattern preset loading is not implemented yet.", juce::dontSendNotification);
        });
        configureButton(savePresetButton, "Save Pattern Preset", [this]
        {
            status.setText("Pattern preset saving is not implemented yet.", juce::dontSendNotification);
        });
        openPresetButton.setTextVisible(false);
        savePresetButton.setTextVisible(false);
        patternPresetSelector.addItem("Default", 1);
        patternPresetSelector.setSelectedId(1, juce::dontSendNotification);
        patternPresetSelector.setTooltip("Selected pattern preset.");
        patternLengthDisplay.setText("016", juce::dontSendNotification);
        patternLengthDisplay.setJustificationType(juce::Justification::centred);
        patternLengthDisplay.setTooltip("Current pattern length: 16 steps.");
        patternLengthDisplay.setColour(juce::Label::textColourId, juce::Colour::fromRGB(203, 221, 229));
        patternLengthDisplay.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(25, 36, 43));
        patternLengthDisplay.setColour(juce::Label::outlineColourId, juce::Colour::fromRGB(91, 122, 136));
        patternLengthDisplay.setBorderSize({1, 4, 1, 4});
        addAndMakeVisible(patternPresetSelector);
        addAndMakeVisible(patternLengthDisplay);
        configureButton(patternLengthDownButton, "-", [this] { adjustPatternLength(-1); });
        patternLengthDownButton.setTooltip("Shorten this pattern by one step.");
        addAndMakeVisible(patternLengthDownButton);
        configureButton(patternLengthUpButton, "+", [this] { adjustPatternLength(1); });
        patternLengthUpButton.setTooltip("Extend this pattern by one step.");
        addAndMakeVisible(patternLengthUpButton);
        addAndMakeVisible(outputMeter);
        masterVolumeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        masterVolumeKnob.setRange(-60.0, 6.0, 0.1);
        masterVolumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        masterVolumeKnob.setPopupDisplayEnabled(true, true, this);
        masterVolumeKnob.setTextValueSuffix(" dB");
        masterVolumeKnob.setTooltip("Master output volume.");
        masterVolumeKnob.onValueChange = [this] { setMasterGainDb(masterVolumeKnob.getValue()); };
        addAndMakeVisible(masterVolumeKnob);
        barReadoutLabel.setText("001", juce::dontSendNotification);
        barReadoutLabel.setJustificationType(juce::Justification::centred);
        barReadoutLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(203, 221, 229));
        barReadoutLabel.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(25, 36, 43));
        barReadoutLabel.setColour(juce::Label::outlineColourId, juce::Colour::fromRGB(91, 122, 136));
        barReadoutLabel.setBorderSize({1, 4, 1, 4});
        barReadoutLabel.setTooltip("Current song position in bars.");
        barLabel.setText("BAR", juce::dontSendNotification);
        barLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(176, 193, 203));
        barLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        addAndMakeVisible(barLabel);
        addAndMakeVisible(barReadoutLabel);
        configureButton(loopButton, "LOOP", [this] { setLoopEnabled(loopButton.getToggleState()); });
        loopButton.setClickingTogglesState(true);
        loopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
        loopButton.setTooltip("Toggle the transport loop range.");
        configureButton(metronomeButton, "METRO", [this] { setMetronomeEnabled(metronomeButton.getToggleState()); });
        metronomeButton.setClickingTogglesState(true);
        metronomeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(36, 132, 174));
        metronomeButton.setTooltip("Toggle Tracktion Engine's click track.");
        loopStartSlider.setSliderStyle(juce::Slider::IncDecButtons);
        loopStartSlider.setRange(1, PlaylistGrid::barCount, 1);
        loopStartSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 42, 22);
        loopStartSlider.setValue(1, juce::dontSendNotification);
        loopStartSlider.setTooltip("Loop start bar.");
        loopStartLabel.setText("START", juce::dontSendNotification);
        loopLengthLabel.setText("LENGTH", juce::dontSendNotification);
        timeSignatureLabel.setText("4/4", juce::dontSendNotification);
        for (auto* label : { &loopStartLabel, &loopLengthLabel, &timeSignatureLabel })
        {
            label->setColour(juce::Label::textColourId, juce::Colour::fromRGB(176, 193, 203));
            label->setFont(juce::FontOptions(10.0f, juce::Font::bold));
            addAndMakeVisible(*label);
        }
        loopStartSlider.onValueChange = [this] { updateLoopRange(); };
        loopLengthSlider.setSliderStyle(juce::Slider::IncDecButtons);
        loopLengthSlider.setRange(1, PlaylistGrid::barCount, 1);
        loopLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 38, 22);
        loopLengthSlider.setValue(4, juce::dontSendNotification);
        loopLengthSlider.setTooltip("Loop length in bars.");
        loopLengthSlider.onValueChange = [this] { updateLoopRange(); };
        const std::array<juce::Component*, 25> actionButtons {{
            &newProjectButton, &openButton, &saveButton, &midiExportButton, &audioSettingsButton, &addTrackButton,
            &patternButton, &playlistButton, &playButton, &stopButton, &patternModeButton, &songModeButton, &mixerButton, &masterButton,
            &recordButton, &rewindButton, &previousButton, &nextButton, &loopButton, &metronomeButton,
            &generatorMuteButton, &generatorSoloButton, &generatorSettingsButton, &openPresetButton, &savePresetButton
        }};
        for (auto* button : actionButtons) addAndMakeVisible(*button);
        addAndMakeVisible(loopStartSlider);
        addAndMakeVisible(loopLengthSlider);

        tempoLabel.setText("BPM", juce::dontSendNotification);
        tempoLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(183, 207, 221));
        addAndMakeVisible(tempoLabel);
        tempoSlider.setSliderStyle(juce::Slider::IncDecButtons);
        tempoSlider.setRange(20.0, 300.0, 1.0);
        tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 54, 24);
        tempoSlider.setValue(120.0, juce::dontSendNotification);
        tempoSlider.onValueChange = [this]
        {
            if (edit == nullptr || !hasProject) return;
            const auto bpm = tempoSlider.getValue();
            if (auto* tempo = edit->tempoSequence.getTempo(0)) tempo->setBpm(bpm);
            project.setInitialTempoBpm(bpm);
            updateLoopRange();
            isDirty = true;
            const auto meter = project.getInitialTimeSignature();
            musicalDetails.setText("Tracks: " + juce::String(project.getTrackCount()) + "   |   "
                                       + juce::String(bpm, 1) + " BPM   |   "
                                       + juce::String(meter.numerator) + "/" + juce::String(meter.denominator)
                                       + "   |   Built-in 4OSC   |   64 patterns per generator",
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

        generatorDragHandle.setText("Instrument 1 · Pattern A1", juce::dontSendNotification);
        generatorDragHandle.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        generatorDragHandle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(213, 229, 239));
        generatorDragHandle.setJustificationType(juce::Justification::centredLeft);
        generatorDragHandle.setBorderSize(juce::BorderSize<int>(0, 28, 0, 24));
        generatorDragHandle.setTitle("Move generator");
        // This dedicated title label owns the move gesture and forwards its
        // coordinates to ProjectView, avoiding parent hit-test ambiguity.
        generatorDragHandle.setInterceptsMouseClicks(true, false);
        generatorDragHandle.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        generatorDragHandle.setTooltip("Drag this header to move the generator. Resize from the panel's bottom-right corner.");
        generatorDragHandle.dragStarted = [this](juce::Point<int> point) { beginGeneratorDrag(point); };
        generatorDragHandle.dragged = [this](juce::Point<int> point) { moveGeneratorPanel(point); };
        generatorDragHandle.dragEnded = [this] { endGeneratorDrag(); };
        addAndMakeVisible(generatorDragHandle);
        generatorDragHandle.addMouseListener(this, true);

        generatorResizeHandle.resizeStarted = [this](juce::Point<int> point) { beginGeneratorResize(point); };
        generatorResizeHandle.resizedByDrag = [this](juce::Point<int> point) { resizeGeneratorPanel(point); };
        generatorResizeHandle.resizeEnded = [this] { endGeneratorResize(); };
        generatorResizeHandle.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(generatorResizeHandle);
        generatorResizeHandle.addMouseListener(this, true);

        generatorCollapseButton.setKind(PlaylistWindowIconButton::Kind::minimize);
        generatorCollapseButton.setTooltip("Collapse the generator to its pattern controls");
        generatorCollapseButton.onClick = [this] { toggleGeneratorCollapsed(); };
        addAndMakeVisible(generatorCollapseButton);
        generatorCollapseButton.setVisible(false);

        patternTitle.setText("Select an instrument generator to edit patterns", juce::dontSendNotification);
        patternTitle.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(patternTitle);
        patternTitle.setVisible(false);

        pitchSelector.addItemList(makePitchNames(), 1);
        pitchSelector.setSelectedId(pitchToItemId(60), juce::dontSendNotification);
        pitchSelector.setTooltip("Choose a note pitch to edit in the 16-step pattern.");
        pitchSelector.onChange = [this] { loadSelectedPitchSteps(); };
        pitchSelector.setEnabled(false);
        addAndMakeVisible(pitchSelector);
        pitchSelector.setVisible(false);

        pianoRollButton.setButtonText("PIANO ROLL");
        pianoRollButton.setTooltip("Open the selected pattern in the piano roll");
        pianoRollButton.setTitle("Open piano roll");
        pianoRollButton.onClick = [this] { showPianoRoll(); };
        pianoRollButton.setEnabled(false);
        addAndMakeVisible(pianoRollButton);
        pianoRollButton.setVisible(false);
        generatorMuteButton.setEnabled(false);
        generatorSoloButton.setEnabled(false);
        patternPresetSelector.setEnabled(false);
        patternPresetSelector.setVisible(false);
        patternLengthDisplay.setVisible(false);
        patternLengthDownButton.setVisible(false);
        patternLengthUpButton.setVisible(false);
        generatorMuteButton.setVisible(false);
        generatorSoloButton.setVisible(false);
        generatorSettingsButton.setVisible(false);
        openPresetButton.setVisible(false);
        savePresetButton.setVisible(false);

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
            [this](int row, int bar) { paintPlaylistClip(row, bar); },
            [this] { finishPlaylistPaintGesture(); },
            [this](int row) { return getTrackMute(row); },
            [this](int row, bool value) { setTrackMute(row, value); },
            [this](int row) { return getTrackSolo(row); },
            [this](int row, bool value) { setTrackSolo(row, value); },
            [this](int row) { return getTrackRecordArm(row); },
            [this](int row, bool value) { setTrackRecordArm(row, value); },
            [this](int row, int sourceBar, int destinationRow, int destinationBar)
            {
                movePlaylistClip(row, sourceBar, destinationRow, destinationBar);
            },
            [this](int row, int bar, juce::Component* target) { showPlaylistPatternMenu(row, bar, target); },
            [this](int row)
            {
                if (!juce::isPositiveAndBelow(row, project.getTrackCount())) return;
                trackSelector.setSelectedId(row + 1, juce::sendNotificationSync);
                if (trackSelector.getSelectedId() == row + 1)
                    selectInstrumentTrack();
                bringGeneratorToFront();
            },
            [this](int row, int bar) { deletePlaylistClip(row, bar); },
            [this](int row, int bar, int direction) { scrollPlaylistPattern(row, bar, direction); });
        playlistWindow->setAudioClipHandlers(
            [this](int row)
            {
                return juce::isPositiveAndBelow(row, project.getTrackCount())
                    && project.getTrackKind(row) == ProjectDocument::TrackKind::audio;
            },
            [this](int row, int bar)
            {
                int startBar = -1;
                findAudioClipAtBar(row, bar, &startBar, nullptr);
                return startBar;
            },
            [this](int row, int bar)
            {
                int lengthBars = 1;
                findAudioClipAtBar(row, bar, nullptr, &lengthBars);
                return lengthBars;
            },
            [this](int row, int bar, juce::Component* target)
            {
                showPlaylistAudioContextMenu(row, bar, target);
            });
        playlistWindow->setTrackColourProvider([this](int row) { return project.getTrackColour(row); });
        if (auto* settings = applicationProperties.getUserSettings())
            playlistWindow->setLightTheme(settings->getBoolValue("playlistLightTheme", false));
        playlistWindow->setThemeChangedHandler([this](bool enabled)
        {
            if (auto* settings = applicationProperties.getUserSettings())
            {
                settings->setValue("playlistLightTheme", enabled);
                applicationProperties.saveIfNeeded();
            }
        });
        playlistWindow->setLoopRangeFromRuler([this](int startBar, int lengthBars)
        {
            loopStartSlider.setValue(startBar, juce::dontSendNotification);
            loopLengthSlider.setValue(lengthBars, juce::dontSendNotification);
            if (!loopButton.getToggleState()) loopButton.setToggleState(true, juce::dontSendNotification);
            updateLoopRange();
        });
        playlistWindow->setTrackContextMenuHandler(
            [this](int row, juce::Component* target) { showPlaylistTrackContextMenu(row, target); });
        playlistWindow->setClipPreview([this](int row, int bar) { return getPlaylistPreviewNotes(row, bar); });
        playlistWindow->setSongAutomationCallbacks(
            [this](int trackIndex, int songStep)
            {
                if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())) return 0.5f;
                const auto trackId = project.getTrackId(trackIndex);
                for (int i = 0; i < project.getSongAutomationLaneCount(trackId); ++i)
                {
                    const auto lane = project.getSongAutomationLane(trackId, i);
                    if (lane.pluginName == automationPluginName && lane.parameterId == automationParameterId)
                        return evaluateAutomationLane(lane, songStep);
                }
                return 0.5f;
            },
            [this](int trackIndex, int songStep, float value)
            {
                if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
                    || automationPluginName.isEmpty() || automationParameterId.isEmpty()) return;
                project.setSongAutomationPoint(project.getTrackId(trackIndex), automationPluginName,
                    automationParameterId, automationParameterName, songStep, value);
                isDirty = true;
                updateEngineAutomation();
                status.setText("Song Playlist automation point updated. Save to keep the change.", juce::dontSendNotification);
            });
        playlistWindow->setAutomationModeChanged([this](bool enabled)
        {
            status.setText(enabled
                ? "Playlist automation mode: draw the selected plugin parameter across song time."
                : "Playlist pattern drawing mode restored.", juce::dontSendNotification);
        });
        playlistWindow->onDragged = [this](juce::Point<int> position) { movePlaylistPanel(position); };
        playlistWindow->onResizedByDrag = [this](juce::Rectangle<int> bounds) { resizePlaylistPanel(bounds); };
        playlistWindow->onMinimizeChanged = [this](bool minimized)
        {
            if (!playlistBoundsInitialised || playlistWindow == nullptr) return;
            if (minimized)
            {
                playlistRestoreBounds = playlistPanelBounds;
                playlistPanelBounds.setHeight(28);
            }
            else if (!playlistRestoreBounds.isEmpty())
                playlistPanelBounds = playlistRestoreBounds;
            resized();
        };
        playlistWindow->onMaximizeToggled = [this] { togglePlaylistMaximized(); };
        playlistWindow->onClose = [this]
        {
            if (playlistWindow != nullptr) playlistWindow->setVisible(false);
            if (auto* settings = applicationProperties.getUserSettings())
            {
                settings->setValue("playlistWindowOpen", false);
                settings->saveIfNeeded();
            }
        };
        addAndMakeVisible(*playlistWindow);
        // The Playlist remains available from the toolbar without obscuring
        // the blank workspace when Orbit first opens.
        playlistWindow->setVisible(false);
        auditionKeyboard.setTitle("Play the selected instrument directly. Click or drag across keys to audition notes.");
        auditionKeyboard.onNote = [this](int pitch, bool isDown) { auditionSelectedInstrument(pitch, isDown); };
        addAndMakeVisible(auditionKeyboard);
        auditionKeyboard.setVisible(false);

        pianoRollWindow = std::make_unique<PianoRollWindow>(
            [this] { return project.getPatternLengthSteps(selectedProjectTrackId(), selectedPatternIndex()); },
            [this](int pitch, int step)
            {
                if (pitch < minimumPitch || pitch > maximumPitch || step < 0 || step >= 64) return false;
                return noteGrid[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)];
            },
            [this] { return allNotesSelected; },
            [this](int pitch, int step)
            {
                return project.getPatternNoteLength(selectedProjectTrackId(), selectedPatternIndex(), pitch, step);
            },
            [this](int pitch, int step, bool erase) { togglePianoRollNote(pitch, step, erase); },
            [this](int pitch, int step, int length) { setPianoRollNoteLength(pitch, step, length); },
            [this] { pianoRollGestureChanged = false; pianoRollGestureActive = true; },
            [this]
            {
                const bool changed = pianoRollGestureChanged;
                pianoRollGestureActive = false;
                pianoRollGestureChanged = false;
                if (changed)
                {
                    if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
                    rebuildPatternClip();
                }
            },
            [this](int pitch, bool isDown)
            {
                if (instrumentTrack == nullptr) return;
                constexpr int auditionChannel = 1;
                if (isDown)
                {
                    ++midiAuditionNoteOns;
                    lastMidiActivity = "Piano keyboard pitch " + juce::String(pitch) + " on";
                    instrumentTrack->playGuideNote(pitch, te::MidiChannel(auditionChannel), 100,
                                                   true, true, false);
                }
                else
                {
                    lastMidiActivity = "Piano keyboard pitch " + juce::String(pitch) + " off";
                    instrumentTrack->turnOffGuideNotes(te::MidiChannel(auditionChannel));
                }
            });
        pianoRollWindow->setPasteHandler([this] { pastePatternNotes(); });
        pianoRollWindow->automationValue = [this](int step)
        {
            const auto trackId = selectedProjectTrackId();
            const auto count = project.getPatternAutomationLaneCount(trackId, selectedPatternIndex());
            for (int laneIndex = 0; laneIndex < count; ++laneIndex)
            {
                const auto lane = project.getPatternAutomationLane(trackId, selectedPatternIndex(), laneIndex);
                if (lane.pluginName == automationPluginName && lane.parameterId == automationParameterId)
                    return evaluateAutomationLane(lane, step);
            }
            return 0.5f;
        };
        pianoRollWindow->automationPointChanged = [this](int step, float value)
        {
            if (automationPluginName.isEmpty() || automationParameterId.isEmpty()) return;
            project.setPatternAutomationPoint(selectedProjectTrackId(), selectedPatternIndex(),
                automationPluginName, automationParameterId, automationParameterName, step, value);
            isDirty = true;
        };
        pianoRollWindow->automationPointRemoved = [this](int step)
        {
            if (automationPluginName.isEmpty() || automationParameterId.isEmpty()) return;
            project.removePatternAutomationPoint(selectedProjectTrackId(), selectedPatternIndex(),
                                                  automationPluginName, automationParameterId, step);
            isDirty = true;
        };
        pianoRollWindow->automationPointMoved = [this](int from, int to, float value)
        {
            if (automationPluginName.isEmpty() || automationParameterId.isEmpty()) return;
            if (from != to)
                project.removePatternAutomationPoint(selectedProjectTrackId(), selectedPatternIndex(),
                                                      automationPluginName, automationParameterId, from);
            project.setPatternAutomationPoint(selectedProjectTrackId(), selectedPatternIndex(),
                automationPluginName, automationParameterId, automationParameterName, to, value);
            isDirty = true;
        };
        pianoRollWindow->onAutomationGestureStarted = [this] { rememberUndo("Edit pattern automation"); };
        pianoRollWindow->onAutomationGestureEnded = [this]
        {
            updateEngineAutomation();
            status.setText("Pattern automation updated. Save to keep the change.", juce::dontSendNotification);
        };
        pianoRollWindow->getVelocityAtStep = [this](int step, int pitch)
        {
            return project.getPatternNoteVelocity(selectedProjectTrackId(), selectedPatternIndex(), pitch, step);
        };
        pianoRollWindow->setVelocityAtStep = [this](int step, int velocity)
        {
            const auto trackId = selectedProjectTrackId();
            const auto patternIndex = selectedPatternIndex();
            bool changed = false;
            for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                if (project.getPatternNote(trackId, patternIndex, pitch, step))
                {
                    project.setPatternNoteVelocity(trackId, patternIndex, pitch, step, velocity);
                    changed = true;
                }
            isDirty = isDirty || changed;
        };
        pianoRollWindow->onVelocityGestureStarted = [this] { rememberUndo("Edit note velocities"); };
        pianoRollWindow->onVelocityGestureEnded = [this]
        {
            rebuildPatternClip();
            if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
            status.setText("Note velocities updated. Save to keep the change.", juce::dontSendNotification);
        };
        pianoRollWindow->onAutomationTargetChanged = [this](const juce::String& target)
        {
            const auto parts = juce::StringArray::fromLines(target);
            automationPluginName = parts.size() > 0 ? parts[0] : juce::String{};
            automationParameterId = parts.size() > 1 ? parts[1] : juce::String{};
            automationParameterName = parts.size() > 2 ? parts[2] : juce::String{};
            if (pianoRollWindow != nullptr) pianoRollWindow->repaint();
        };
        refreshPianoRollAutomationTargets();
        pianoRollWindow->onClose = [this] { closePianoRollView(); };
        // A maximized piano roll is still the editor embedded in its
        // generator panel. Starting a resize or drag must not detach it.
        pianoRollWindow->onManipulationStarted = [](juce::Point<int>, bool) {};
        pianoRollWindow->onBoundsChanged = [this](juce::Rectangle<int> bounds)
        {
            if (pianoRollMaximized && pianoRollWindow != nullptr)
            {
                auto panelBounds = juce::Rectangle<int>(bounds.getX() - 12, bounds.getY() - 86,
                                                         bounds.getWidth() + 32, bounds.getHeight() + 106);
                const auto workspace = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(50).reduced(14, 10);
                const auto width = juce::jlimit(juce::jmin(492, workspace.getWidth()), workspace.getWidth(), panelBounds.getWidth());
                const auto height = juce::jlimit(juce::jmin(366, workspace.getHeight()), workspace.getHeight(), panelBounds.getHeight());
                panelBounds.setSize(width, height);
                panelBounds.setPosition(
                    juce::jlimit(workspace.getX(), juce::jmax(workspace.getX(), workspace.getRight() - width), panelBounds.getX()),
                    juce::jlimit(workspace.getY(), juce::jmax(workspace.getY(), workspace.getBottom() - height), panelBounds.getY()));
                generatorPanelBounds = panelBounds;
                pianoRollBounds = bounds;
                resized();
                repaint();
                return;
            }
            pianoRollBounds = bounds;
        };
        pianoRollWindow->addMouseListener(this, true);
        addAndMakeVisible(*pianoRollWindow);
        pianoRollWindow->setVisible(false);
        generatorDockViewport.setViewedComponent(&generatorDockStrip, false);
        generatorDockViewport.setScrollBarsShown(false, true);
        addAndMakeVisible(generatorDockViewport);
        generatorDockViewport.setVisible(false);

        mixerWindow = std::make_unique<MixerWindow>();
        addAndMakeVisible(*mixerWindow);
        mixerWindow->setVisible(false);
        mixerWindow->onFocus = [this]
        {
            if (mixerWindow != nullptr) mixerWindow->toFront(false);
            mixerMoveHandle.setVisible(false);
            mixerResizeHandle.setVisible(false);
        };
        mixerWindow->onDragged = [this](juce::Point<int> position)
        {
            if (!mixerBoundsInitialised || mixerWindow == nullptr) return;
            mixerPanelBounds.setPosition(constrainMixerPosition(position));
            mixerWindow->setBounds(mixerPanelBounds);
            draggingMixer = true;
            layoutMixerHandles();
        };
        mixerWindow->onDragEnded = [this] { saveMixerPanelBounds(); };
        mixerWindow->onMinimizeChanged = [this](bool minimized)
        {
            if (mixerWindow == nullptr) return;
            const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
            if (minimized)
            {
                mixerRestoreBounds = mixerPanelBounds;
                mixerPanelBounds.setHeight(26);
            }
            else if (!mixerRestoreBounds.isEmpty()) mixerPanelBounds = mixerRestoreBounds;
            else mixerPanelBounds.setHeight(juce::jmin(650, area.getHeight()));
            mixerWindow->setBounds(mixerPanelBounds);
            layoutMixerHandles();
            saveMixerPanelBounds();
        };
        mixerWindow->onMaximizeToggled = [this] { toggleMixerMaximized(); };
        mixerWindow->onResizeStarted = [this]
        {
            resizingMixer = true;
            mixerMaximized = false;
            if (mixerWindow != nullptr) mixerWindow->setMaximized(false);
            mixerResizeStartBounds = mixerPanelBounds;
        };
        mixerWindow->onResizedByDrag = [this](juce::Point<int> delta)
        {
            if (!resizingMixer || mixerWindow == nullptr) return;
            const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
            mixerPanelBounds = mixerResizeStartBounds;
            mixerPanelBounds.setSize(
                juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(), mixerWindow->getPreferredWidth()),
                juce::jlimit(juce::jmin(300, area.getHeight()), area.getHeight(), mixerResizeStartBounds.getHeight() + delta.y));
            mixerPanelBounds.setX(constrainMixerPosition(mixerResizeStartBounds.getPosition()).x);
            mixerPanelBounds.setY(juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - 26), mixerPanelBounds.getY()));
            mixerWindow->setBounds(mixerPanelBounds);
        };
        mixerWindow->onResizeEnded = [this] { endMixerResize(); };
        mixerWindow->onClose = [this]
        {
            if (mixerWindow != nullptr) mixerWindow->setVisible(false);
            if (auto* settings = applicationProperties.getUserSettings())
            {
                settings->setValue("mixerWindowOpen", false);
                settings->saveIfNeeded();
            }
            layoutMixerHandles();
            repaint();
        };
        for (auto* control : std::array<juce::Component*, 14> {
                 &pianoRollButton, &generatorMuteButton, &generatorSoloButton, &generatorSettingsButton,
                 &patternPresetSelector, &openPresetButton, &savePresetButton, &patternLengthDownButton,
                 &patternLengthDisplay, &patternLengthUpButton, &trackSelector,
                 &generatorCollapseButton, &generatorDragHandle, &generatorResizeHandle })
            control->addMouseListener(this, true);
        for (auto& button : patternBankButtons) button->addMouseListener(this, true);
        for (auto& button : patternSlotButtons) button->addMouseListener(this, true);
        mixerMoveHandle.dragStarted = [this](juce::Point<int> point)
        {
            if (!mixerBoundsInitialised || mixerWindow == nullptr) return;
            mixerDragOffset = point - mixerPanelBounds.getPosition();
            draggingMixer = true;
            mixerWindow->toFront(false);
            mixerMoveHandle.toFront(false);
            mixerResizeHandle.toFront(false);
        };
        mixerMoveHandle.dragged = [this](juce::Point<int> point) { moveMixerPanel(point); };
        mixerMoveHandle.dragEnded = [this] { endMixerDrag(); };
        mixerResizeHandle.resizeStarted = [this](juce::Point<int> point)
        {
            if (!mixerBoundsInitialised || mixerWindow == nullptr) return;
            resizingMixer = true;
            mixerResizeStart = point;
            mixerResizeStartBounds = mixerPanelBounds;
            mixerWindow->toFront(false);
            mixerMoveHandle.toFront(false);
            mixerResizeHandle.toFront(false);
        };
        mixerResizeHandle.resizedByDrag = [this](juce::Point<int> point) { resizeMixerPanel(point); };
        mixerResizeHandle.resizeEnded = [this] { endMixerResize(); };
        addAndMakeVisible(mixerMoveHandle);
        addAndMakeVisible(mixerResizeHandle);
        mixerMoveHandle.setVisible(false);
        mixerResizeHandle.setVisible(false);
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
        mixerWindow->getTrackOutputBus = [this](int index) { return getTrackOutputBus(index); };
        mixerWindow->setTrackOutputBus = [this](int index, int bus) { setTrackOutputBus(index, bus); };
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

        buildLabel.setText("Build 131  -  v0.3.110", juce::dontSendNotification);
        buildLabel.setJustificationType(juce::Justification::centredRight);
        buildLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(154, 181, 196));
        addAndMakeVisible(buildLabel);
        setSize(1220, 760);
        const std::array<juce::Component*, 18> hiddenComponents {{
            &title, &newProjectButton, &openButton, &saveButton, &midiExportButton, &audioSettingsButton,
            &addTrackButton, &patternButton, &arrangementTitle, &mixerTitle, &levelLabel,
            &panLabel, &levelSlider, &panSlider, &meterStatus, &projectDetails, &musicalDetails, &audioDetails
        }};
        for (auto* component : hiddenComponents)
            component->setVisible(false);
        createProject();
        // A newly opened DAW starts as a clean, untitled workspace. The empty
        // starter edit is not a user change and must not block File > Open.
        isDirty = false;
        updateAudioStatus();
        auto* settings = applicationProperties.getUserSettings();
        if (settings != nullptr)
        {
            // Window visibility is a session choice, not part of the startup
            // document. Always launch on the uncluttered workspace.
            settings->setValue("playlistWindowOpen", false);
            settings->setValue("mixerWindowOpen", false);
            settings->saveIfNeeded();
        }
        startTimerHz(juce::jlimit(2, 60, settings != nullptr ? settings->getIntValue("updateRateHz", 30) : 30));
        lastAutoSaveMs = juce::Time::getMillisecondCounter();
        if (settings != nullptr && settings->getBoolValue("scanVst3AtStartup", false))
        {
            juce::Component::SafePointer<ProjectView> safeThis(this);
            juce::MessageManager::callAsync([safeThis]
            {
                if (safeThis != nullptr) safeThis->startVst3Scan();
            });
        }
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
        if (edit != nullptr)
        {
            auto& transport = edit->getTransport();
            const bool playing = transport.isPlaying();
            const auto seconds = transport.getPosition().inSeconds();
            const auto signature = project.getInitialTimeSignature();
            const auto secondsPerBar = (60.0 / juce::jmax(20.0, project.getInitialTempoBpm()))
                * signature.numerator * (4.0 / juce::jmax(1, signature.denominator));
            const auto currentBar = juce::jmax(1, static_cast<int>(std::floor(juce::jmax(0.0, seconds) / secondsPerBar)) + 1);
            barReadoutLabel.setText(juce::String(currentBar).paddedLeft('0', 3), juce::dontSendNotification);
            if (playing && playbackMode == PlaybackMode::song)
                rememberSongPatternsAt(seconds);
            const auto bpm = project.getInitialTempoBpm();
            const auto patternSteps = juce::jmax(1, project.getPatternLengthSteps(
                selectedProjectTrackId(), selectedPatternIndex()));
            const auto step = std::fmod(seconds * (bpm / 60.0) * 4.0, static_cast<double>(patternSteps));
            if (pianoRollWindow != nullptr) pianoRollWindow->setPlayheadStep(step, playing);
            if (playlistWindow != nullptr) playlistWindow->setPlayheadPosition(seconds, bpm, playing);
            if (!playing && auditionPatternPlayback)
            {
                auditionPatternPlayback = false;
                refreshEnginePlaylist();
                playButton.setButtonText("Play");
            }
        }
        if (auto* settings = applicationProperties.getUserSettings())
        {
            const auto intervalMinutes = juce::jlimit(0, 240, settings->getIntValue("autoSaveMinutes", 0));
            const auto now = juce::Time::getMillisecondCounter();
            const auto intervalMs = static_cast<juce::uint32>(intervalMinutes * 60 * 1000);
            if (intervalMinutes > 0 && hasProject && edit != nullptr && isDirty && project.hasFile()
                && static_cast<juce::uint32>(now - lastAutoSaveMs) >= intervalMs)
            {
                lastAutoSaveMs = now;
                saveToFile(project.getFile());
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(72, 96, 108));
        if (!generatorPanelBounds.isEmpty())
        {
            const bool showingExpandedRoll = pianoRollMaximized && pianoRollWindow != nullptr && pianoRollWindow->isVisible();
            g.setColour(showingExpandedRoll ? juce::Colour::fromRGB(22, 32, 42)
                                             : juce::Colour::fromRGB(45, 65, 77));
            g.fillRect(generatorPanelBounds);
            g.setColour(showingExpandedRoll ? juce::Colour::fromRGB(103, 139, 157)
                                             : juce::Colour::fromRGB(106, 137, 151));
            g.drawRect(generatorPanelBounds, 1.0f);
            if (!showingExpandedRoll)
            {
                auto titleBar = generatorPanelBounds.withHeight(22).toFloat();
                g.setColour(juce::Colour::fromRGB(45, 65, 77));
                g.fillRoundedRectangle(titleBar, 7.0f);
                g.fillRect(titleBar.withY(titleBar.getBottom() - 7.0f));
                g.setColour(juce::Colour::fromRGB(157, 193, 208));
                for (int row = 0; row < 3; ++row)
                    for (int column = 0; column < 2; ++column)
                        g.fillEllipse(static_cast<float>(generatorPanelBounds.getX() + 9 + column * 5),
                                      static_cast<float>(generatorPanelBounds.getY() + 6 + row * 5), 2.0f, 2.0f);
            }
        }
        if (!generatorInstrumentBounds.isEmpty() && (pianoRollWindow == nullptr || !pianoRollWindow->isVisible()))
        {
            g.setColour(juce::Colour::fromRGB(45, 65, 77));
            g.fillRoundedRectangle(generatorInstrumentBounds.toFloat(), 5.0f);
            g.setColour(juce::Colour::fromRGB(96, 129, 145));
            g.drawRoundedRectangle(generatorInstrumentBounds.toFloat(), 5.0f, 1.0f);
            auto instrumentHeader = generatorInstrumentBounds.withHeight(30).reduced(10, 0);
            g.setColour(juce::Colour::fromRGB(182, 207, 220));
            g.setFont(13.0f);
            g.drawText("INSTRUMENT GUI", instrumentHeader.removeFromLeft(170), juce::Justification::centredLeft);
            g.setColour(juce::Colour::fromRGB(124, 166, 184));
            g.drawText(getInstrumentHeaderText(), instrumentHeader, juce::Justification::centredRight);
            g.setColour(juce::Colour::fromRGB(72, 99, 113));
            g.drawHorizontalLine(generatorInstrumentBounds.getY() + 32,
                                 static_cast<float>(generatorInstrumentBounds.getX() + 10),
                                 static_cast<float>(generatorInstrumentBounds.getRight() - 10));
        }
        g.setColour(juce::Colour::fromRGB(51, 60, 67));
        g.fillRect(0, 0, getWidth(), 28);
        g.setColour(juce::Colour::fromRGB(27, 32, 37));
        g.fillRect(0, 28, getWidth(), 54);
        g.setColour(juce::Colour::fromRGB(35, 54, 65));
        g.fillRect(0, getHeight() - 22, getWidth(), 22);
    }

    void beginGeneratorDrag(juce::Point<int> point)
    {
        if (generatorPanelBounds.isEmpty()) return;
        bringGeneratorToFront();
        generatorDragStartPosition = generatorPanelBounds.getPosition();
        generatorDragOffset = point - generatorPanelBounds.getPosition();
        draggingGenerator = true;
        resizingGenerator = false;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!draggingGenerator) return;
        const auto point = event.getEventRelativeTo(this).position.toInt();
        moveGeneratorPanel(point);
        resized();
        repaint();
    }

    void beginGeneratorResize(juce::Point<int> point)
    {
        if (generatorPanelBounds.isEmpty()) return;
        resizingGenerator = true;
        draggingGenerator = false;
        generatorDragStart = point;
        generatorResizeStart = generatorPanelBounds;
    }

    void resizeGeneratorPanel(juce::Point<int> point)
    {
        if (!resizingGenerator) return;
        const auto available = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto delta = point - generatorDragStart;
        const auto maxWidth = available.getWidth();
        const auto maxHeight = available.getHeight();
        generatorPanelBounds.setWidth(juce::jlimit(juce::jmin(800, maxWidth), maxWidth,
            generatorResizeStart.getWidth() + delta.x));
        const auto minimumHeight = generatorCollapsed ? 34 : 350;
        generatorPanelBounds.setHeight(juce::jlimit(juce::jmin(minimumHeight, maxHeight), maxHeight,
            generatorResizeStart.getHeight() + delta.y));
        resized();
        repaint();
    }

    void moveGeneratorPanel(juce::Point<int> point)
    {
        if (!draggingGenerator) return;
        auto available = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto x = juce::jlimit(available.getX(),
            juce::jmax(available.getX(), available.getRight() - generatorPanelBounds.getWidth()),
            point.x - generatorDragOffset.x);
        const auto y = juce::jlimit(available.getY(),
            juce::jmax(available.getY(), available.getBottom() - generatorPanelBounds.getHeight()),
            point.y - generatorDragOffset.y);
        generatorPanelBounds.setPosition(x, y);
        if (generatorCollapsed && !expandedGeneratorBounds.isEmpty())
            expandedGeneratorBounds.setPosition(x, y);
        resized();
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingGenerator = false;
        reportGeneratorMove();
    }

    void endGeneratorResize()
    {
        resizingGenerator = false;
        status.setText("Generator resized. The piano roll grid fits the updated panel.", juce::dontSendNotification);
    }

    void endGeneratorDrag()
    {
        draggingGenerator = false;
        reportGeneratorMove();
    }

    void reportGeneratorMove()
    {
        if (generatorPanelBounds.getPosition() != generatorDragStartPosition)
            status.setText("Generator moved. Its embedded piano roll and pattern controls stay linked.",
                           juce::dontSendNotification);
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        const auto point = event.getEventRelativeTo(this).position.toInt();
        auto resizeBounds = generatorPanelBounds;
        resizeBounds = resizeBounds.removeFromRight(18).removeFromBottom(18);
        if (!generatorPanelBounds.isEmpty() && resizeBounds.contains(point))
            setMouseCursor(juce::MouseCursor::BottomRightCornerResizeCursor);
        else
            setMouseCursor(!generatorPanelBounds.isEmpty()
                           && generatorPanelBounds.withHeight(22).contains(point)
                               ? juce::MouseCursor::DraggingHandCursor
                               : juce::MouseCursor::NormalCursor);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void resized() override
    {
        auto menuRow = getLocalBounds().removeFromTop(28).reduced(4, 2);
        fileMenuButton.setBounds(menuRow.removeFromLeft(48));
        editMenuButton.setBounds(menuRow.removeFromLeft(48));
        viewMenuButton.setBounds(menuRow.removeFromLeft(50));
        insertMenuButton.setBounds(menuRow.removeFromLeft(54));
        optionsMenuButton.setBounds(menuRow.removeFromLeft(68));
        windowMenuButton.setBounds(menuRow.removeFromLeft(62));
        helpMenuButton.setBounds(menuRow.removeFromLeft(50));

        auto transportRow = getLocalBounds().withTrimmedTop(28).removeFromTop(54).reduced(8, 3);
        auto meterBounds = transportRow.removeFromRight(154).reduced(0, 7);
        outputMeter.setBounds(meterBounds);
        transportRow.removeFromRight(5);
        const auto controlY = transportRow.getY() + 15;
        tempoLabel.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(24));
        tempoSlider.setBounds(transportRow.removeFromLeft(72).withY(controlY).withHeight(24));
        transportRow.removeFromLeft(7);
        auto modeBounds = transportRow.removeFromLeft(58).withY(controlY - 1).withHeight(32);
        patternModeButton.setBounds(modeBounds.removeFromTop(15));
        songModeButton.setBounds(modeBounds.removeFromTop(15));
        transportRow.removeFromLeft(7);
        playButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(3);
        stopButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(3);
        rewindButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(3);
        previousButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(3);
        nextButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(3);
        recordButton.setBounds(transportRow.removeFromLeft(34).withY(controlY).withHeight(30)); transportRow.removeFromLeft(9);
        loopButton.setBounds(transportRow.removeFromLeft(52).withY(controlY).withHeight(26)); transportRow.removeFromLeft(5);
        auto barBounds = transportRow.removeFromLeft(58);
        barLabel.setBounds(barBounds.withY(transportRow.getY()).withHeight(12));
        barReadoutLabel.setBounds(barBounds.withY(controlY).withHeight(24));
        transportRow.removeFromLeft(5);
        auto startBounds = transportRow.removeFromLeft(82);
        loopStartLabel.setBounds(startBounds.withY(transportRow.getY()).withHeight(12));
        loopStartSlider.setBounds(startBounds.withY(controlY).withHeight(24));
        transportRow.removeFromLeft(5);
        auto lengthBounds = transportRow.removeFromLeft(82);
        loopLengthLabel.setBounds(lengthBounds.withY(transportRow.getY()).withHeight(12));
        loopLengthSlider.setBounds(lengthBounds.withY(controlY).withHeight(24));
        transportRow.removeFromLeft(7);
        metronomeButton.setBounds(transportRow.removeFromLeft(64).withY(controlY).withHeight(26)); transportRow.removeFromLeft(5);
        timeSignatureLabel.setBounds(transportRow.removeFromLeft(38).withY(controlY).withHeight(24)); transportRow.removeFromLeft(7);
        playlistButton.setBounds(transportRow.removeFromLeft(88).withY(controlY).withHeight(30)); transportRow.removeFromLeft(4);
        mixerButton.setBounds(transportRow.removeFromLeft(82).withY(controlY).withHeight(30)); transportRow.removeFromLeft(4);
        masterButton.setBounds(transportRow.removeFromLeft(88).withY(controlY).withHeight(30));
        transportRow.removeFromLeft(4);
        masterVolumeKnob.setBounds(transportRow.removeFromLeft(50)
            .withY(transportRow.getY() + 2).withHeight(42));

        auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        if (playlistWindow != nullptr)
        {
            if (!playlistBoundsInitialised)
            {
                playlistPanelBounds = area.withHeight(juce::jmin(700, area.getHeight()));
                playlistBoundsInitialised = true;
            }
            if (playlistWindow->isMaximized()) playlistPanelBounds = area;
            const auto minWidth = juce::jmin(440, area.getWidth());
            const auto minHeight = juce::jmin(250, area.getHeight());
            const auto panelHeight = playlistWindow->isMinimized() ? 28
                : juce::jlimit(minHeight, area.getHeight(), playlistPanelBounds.getHeight());
            playlistPanelBounds.setSize(juce::jlimit(minWidth, area.getWidth(), playlistPanelBounds.getWidth()), panelHeight);
            playlistPanelBounds.setX(juce::jlimit(area.getX(), juce::jmax(area.getX(), area.getRight() - playlistPanelBounds.getWidth()),
                                                  playlistPanelBounds.getX()));
            playlistPanelBounds.setY(juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - 28),
                                                  playlistPanelBounds.getY()));
            playlistWindow->setBounds(playlistPanelBounds);
        }
        generatorDockViewport.setBounds(10, juce::jmax(82, getHeight() - 49), juce::jmax(0, getWidth() - 20), 25);
        const int dockButtonWidth = 150;
        generatorDockStrip.setSize(static_cast<int>(generatorDockButtons.size()) * dockButtonWidth, 25);
        for (int i = 0; i < static_cast<int>(generatorDockButtons.size()); ++i)
            generatorDockButtons[static_cast<size_t>(i)]->setBounds(i * dockButtonWidth, 0, dockButtonWidth - 3, 23);
        if (mixerWindow != nullptr && mixerBoundsInitialised)
        {
            const auto minWidth = juce::jmin(220, area.getWidth());
            const auto minHeight = juce::jmin(420, area.getHeight());
            const auto panelHeight = mixerWindow->isMinimized() ? 26 : mixerWindow->isMaximized()
                ? area.getHeight() : juce::jlimit(minHeight, area.getHeight(), mixerPanelBounds.getHeight());
            mixerPanelBounds.setSize(juce::jlimit(minWidth, area.getWidth(), mixerPanelBounds.getWidth()), panelHeight);
            if (mixerWindow->isMaximized() && !mixerWindow->isMinimized())
                mixerPanelBounds.setPosition(area.getPosition());
            else mixerPanelBounds.setPosition(constrainMixerPosition(mixerPanelBounds.getPosition()));
            mixerWindow->setBounds(mixerPanelBounds);
            layoutMixerHandles();
        }
        generatorInstrumentBounds = {};
        if (instrumentTrack != nullptr && !allGeneratorsMinimized)
        {
            if (generatorPanelBounds.isEmpty())
                generatorPanelBounds = { area.getX(), area.getY(), juce::jmin(area.getWidth(), 1040), juce::jmin(area.getHeight(), 560) };
            const auto panelWidth = juce::jlimit(juce::jmin(800, area.getWidth()), area.getWidth(), generatorPanelBounds.getWidth());
            const auto minimumPanelHeight = generatorCollapsed ? 34 : 350;
            const auto panelHeight = juce::jlimit(juce::jmin(minimumPanelHeight, area.getHeight()),
                                                  area.getHeight(), generatorPanelBounds.getHeight());
            generatorPanelBounds.setSize(panelWidth, panelHeight);
            generatorPanelBounds.setX(juce::jlimit(area.getX(), juce::jmax(area.getX(), area.getRight() - panelWidth), generatorPanelBounds.getX()));
            generatorPanelBounds.setY(juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - panelHeight), generatorPanelBounds.getY()));
            generatorResizeHandle.setBounds(generatorPanelBounds.getRight() - 18,
                generatorPanelBounds.getBottom() - 18, 18, 18);
            auto panel = generatorPanelBounds;
            auto generatorHeader = panel.removeFromTop(22);
            generatorCollapseButton.setBounds(generatorHeader.removeFromRight(24).reduced(1));
            trackSelector.setBounds({});
            generatorDragHandle.setBounds(generatorHeader);
            trackSelector.setVisible(false);
            patternTitle.setBounds(0, 0, 0, 0);
            auto patternControls = panel.removeFromTop(56).reduced(8, 0);
            constexpr int bankButtonWidth = 23;
            constexpr int bankButtonPitch = 25;
            constexpr int bankButtonHeight = 18;
            constexpr int bankRowGap = 4;
            const auto bankGroupHeight = bankButtonHeight * 2 + bankRowGap;
            const auto bankTop = patternControls.getY() + (patternControls.getHeight() - bankGroupHeight) / 2;
            generatorMuteButton.setBounds(patternControls.removeFromLeft(24).withY(bankTop + 11).withHeight(18));
            patternControls.removeFromLeft(3);
            generatorSoloButton.setBounds(patternControls.removeFromLeft(24).withY(bankTop + 11).withHeight(18));
            patternControls.removeFromLeft(5);
            const auto bankX = patternControls.getX();
            for (int i = 0; i < 8; ++i)
            {
                const auto x = bankX + i * bankButtonPitch;
                patternBankButtons[static_cast<size_t>(i)]->setBounds(x, bankTop, bankButtonWidth, bankButtonHeight);
                patternSlotButtons[static_cast<size_t>(i)]->setBounds(x, bankTop + bankButtonHeight + bankRowGap,
                                                                       bankButtonWidth, bankButtonHeight);
            }
            patternControls.removeFromLeft(8 * bankButtonPitch + 6);
            const auto controlY = patternControls.getY() + 13;
            pianoRollButton.setBounds(patternControls.removeFromLeft(136).withHeight(27).withY(controlY));
            patternControls.removeFromLeft(4);
            generatorSettingsButton.setBounds(patternControls.removeFromLeft(28).withHeight(27).withY(controlY));
            patternControls.removeFromLeft(4);
            patternPresetSelector.setBounds(patternControls.removeFromLeft(128).withHeight(27).withY(controlY));
            patternControls.removeFromLeft(3);
            openPresetButton.setBounds(patternControls.removeFromLeft(28).withHeight(27).withY(controlY));
            patternControls.removeFromLeft(2);
            savePresetButton.setBounds(patternControls.removeFromLeft(28).withHeight(27).withY(controlY));
            patternControls.removeFromLeft(5);
            patternLengthDownButton.setBounds(patternControls.removeFromLeft(20).withHeight(27).withY(controlY));
            patternLengthDisplay.setBounds(patternControls.removeFromLeft(42).withHeight(27).withY(controlY));
            patternLengthUpButton.setBounds(patternControls.removeFromLeft(20).withHeight(27).withY(controlY));
            for (auto* control : std::array<juce::Component*, 10> {
                     &pianoRollButton, &generatorMuteButton, &generatorSoloButton, &generatorSettingsButton,
                     &patternPresetSelector, &openPresetButton, &savePresetButton, &patternLengthDownButton,
                     &patternLengthDisplay, &patternLengthUpButton })
                control->setVisible(!generatorCollapsed && instrumentTrack != nullptr);
            for (auto& button : patternBankButtons) button->setVisible(!generatorCollapsed && instrumentTrack != nullptr);
            for (auto& button : patternSlotButtons) button->setVisible(!generatorCollapsed && instrumentTrack != nullptr);
            generatorInstrumentBounds = generatorCollapsed ? juce::Rectangle<int>{}
                : generatorPanelBounds.withTrimmedLeft(12).withTrimmedRight(12)
                    .withY(patternControls.getBottom() + 8).withBottom(generatorPanelBounds.getBottom() - 10);
            if (pianoRollWindow != nullptr)
            {
                auto rollBounds = generatorInstrumentBounds;
                if (!generatorCollapsed)
                {
                    rollBounds.setRight(juce::jmax(rollBounds.getX(), generatorPanelBounds.getRight() - 20));
                    rollBounds.setBottom(juce::jmax(rollBounds.getY(), generatorPanelBounds.getBottom() - 20));
                }
                if (pianoRollMaximized || pianoRollBounds.isEmpty())
                    pianoRollBounds = rollBounds;
                else
                {
                    const auto workspace = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(48).reduced(14, 10);
                    pianoRollBounds.setSize(juce::jlimit(juce::jmin(460, workspace.getWidth()), workspace.getWidth(), pianoRollBounds.getWidth()),
                                            juce::jlimit(juce::jmin(260, workspace.getHeight()), workspace.getHeight(), pianoRollBounds.getHeight()));
                    pianoRollBounds.setPosition(
                        juce::jlimit(workspace.getX(), juce::jmax(workspace.getX(), workspace.getRight() - pianoRollBounds.getWidth()), pianoRollBounds.getX()),
                        juce::jlimit(workspace.getY(), juce::jmax(workspace.getY(), workspace.getBottom() - pianoRollBounds.getHeight()), pianoRollBounds.getY()));
                    rollBounds = pianoRollBounds;
                }
                pianoRollWindow->setBounds(rollBounds);
                if (generatorCollapsed) pianoRollWindow->setVisible(false);
            }
            if (activeInstrumentEditor != nullptr)
            {
                auto editorBounds = generatorInstrumentBounds;
                if (!generatorCollapsed)
                {
                    editorBounds.setRight(juce::jmax(editorBounds.getX(), generatorPanelBounds.getRight() - 8));
                    editorBounds.setBottom(juce::jmax(editorBounds.getY(), generatorPanelBounds.getBottom() - 54));
                }
                activeInstrumentEditor->setBounds(editorBounds);
                activeInstrumentEditor->setVisible(!generatorCollapsed
                                                   && (pianoRollWindow == nullptr || !pianoRollWindow->isVisible()));
                const auto keyboardBounds = generatorPanelBounds.withTrimmedLeft(12).withTrimmedRight(12)
                    .withBottom(generatorPanelBounds.getBottom() - 8).removeFromBottom(38);
                auditionKeyboard.setBounds(keyboardBounds);
                auditionKeyboard.setVisible(!generatorCollapsed
                    && (pianoRollWindow == nullptr || !pianoRollWindow->isVisible()));
            }
            else auditionKeyboard.setVisible(false);
        }
        else
        {
            generatorPanelBounds = {};
            if (activeInstrumentEditor != nullptr)
                activeInstrumentEditor->setVisible(false);
            auditionKeyboard.setVisible(false);
            if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
            generatorDragHandle.setVisible(false);
            generatorResizeHandle.setVisible(false);
            generatorCollapseButton.setVisible(false);
            trackSelector.setVisible(false);
            for (auto& button : patternBankButtons) button->setVisible(false);
            for (auto& button : patternSlotButtons) button->setVisible(false);
            for (auto* control : std::array<juce::Component*, 10> {
                     &pianoRollButton, &generatorMuteButton, &generatorSoloButton, &generatorSettingsButton,
                     &patternPresetSelector, &openPresetButton, &savePresetButton, &patternLengthDownButton,
                     &patternLengthDisplay, &patternLengthUpButton })
                control->setVisible(false);
        }
        status.setBounds(10, getHeight() - 21, juce::jmax(0, getWidth() - 205), 18);
        buildLabel.setBounds(getWidth() - 180, getHeight() - 22, 170, 18);
        if (playlistWindow != nullptr && playlistWindow->isVisible() && !playlistWindow->isMinimized())
            playlistWindow->toFront(false);
        else if (generatorDockViewport.isVisible())
            generatorDockViewport.toFront(false);
    }

private:
    struct ProjectSnapshot
    {
        ProjectDocument document;
        juce::ValueTree editState;
        bool hadProject = false;
        bool dirty = false;
        juce::String selectedTrackId;
        int bank = 0;
        int slot = 0;
        int pitch = 60;
    };

    struct UndoEntry
    {
        ProjectSnapshot snapshot;
        juce::String label;
    };

    struct PatternClipboard
    {
        bool hasNotes = false;
        int lengthSteps = 16;
        std::array<std::array<bool, 64>, 37> notes{};
        std::array<std::array<int, 64>, 37> lengths{};
        std::array<std::array<int, 64>, 37> velocities{};
    };

    struct PatternData
    {
        bool created = false;
        int lengthSteps = 16;
        std::array<std::array<bool, 64>, 37> notes{};
        std::array<std::array<int, 64>, 37> lengths{};
        std::array<std::array<int, 64>, 37> velocities{};
    };

    void showMenu(juce::TextButton& source, int menuKind)
    {
        juce::PopupMenu menu;
        if (menuKind == 0) // File
        {
            menu.addItem(101, "New Project...");
            menu.addItem(102, "Open Project...");
            menu.addItem(103, "Save Project");
            menu.addSeparator();
            menu.addItem(106, "Project Settings...", hasProject);
            menu.addSeparator();
            menu.addItem(104, "Export MIDI...", hasProject && hasPattern && patternClip != nullptr);
            menu.addSeparator();
            menu.addItem(105, "Quit Orbit");
        }
        else if (menuKind == 1) // Edit
        {
            menu.addItem(201, undoHistory.empty() ? "Undo" : "Undo " + undoHistory.back().label,
                         !undoHistory.empty());
            menu.addItem(202, redoHistory.empty() ? "Redo" : "Redo " + redoHistory.back().label,
                         !redoHistory.empty());
            menu.addSeparator();
            menu.addItem(203, "Cut Pattern Notes", hasPattern);
            menu.addItem(204, "Copy Pattern Notes", hasPattern);
            menu.addItem(205, "Paste Pattern Notes", instrumentTrack != nullptr && patternClipboard.hasNotes);
            menu.addItem(213, "Select All Notes", hasPattern);
            menu.addSeparator();
            menu.addItem(206, "Randomize", instrumentTrack != nullptr);
            menu.addItem(207, "Randomize Options...", instrumentTrack != nullptr);
            menu.addItem(208, "Humanize...", hasPattern);
            menu.addItem(209, "Quantize...", hasPattern);
            menu.addItem(210, "Transpose...", hasPattern);
            menu.addItem(211, "Scale Velocities...", hasPattern);
            menu.addItem(212, "Length...", hasPattern);
            menu.addItem(214, "Apply Groove Template... (not available yet)", false);
            menu.addItem(215, "Apply Pluck Template... (not available yet)", false);
        }
        else if (menuKind == 2) // View
        {
            menu.addItem(301, "Playlist", true, playlistWindow != nullptr && playlistWindow->isVisible());
            menu.addItem(302, "Mixing Console", true, mixerWindow != nullptr && mixerWindow->isVisible());
            menu.addItem(303, "Master Section", true, mixerWindow != nullptr && mixerWindow->isVisible());
            menu.addItem(304, "Event Editor / Piano Roll", hasPattern,
                         pianoRollWindow != nullptr && pianoRollWindow->isVisible());
            menu.addSeparator();
            menu.addItem(305, "Audio Output Settings...");
            menu.addItem(308, "Tempo...", hasProject);
            menu.addItem(309, "Time Signature...", hasProject);
            menu.addItem(311, "Time (not available yet)", false);
            menu.addItem(310, "Modules...", hasProject);
            auto* topWindow = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent());
            menu.addItem(307, "Full Screen (F11)", topWindow != nullptr,
                         topWindow != nullptr && topWindow->isFullScreen());
            menu.addItem(306, "Show Status Bar", true, status.isVisible());
            menu.addItem(312, "Show Hints", true, showHints);
        }
        else if (menuKind == 3) // Insert
        {
            // Keep the Insert hierarchy close to Orion: built-in instruments
            // live under Synths, while third-party instruments have their own
            // separate VST Instruments section.
            menu.addItem(420, "Drum Rack", hasProject);
            menu.addItem(421, "Piano (not available yet)", false);
            menu.addItem(422, "Pro-9 (not available yet)", false);
            menu.addItem(423, "Sampler (not available yet)", false);
            juce::PopupMenu synths;
            synths.addItem(401, "Built-in 4OSC Synth");
            menu.addSubMenu("Synths", synths);
            menu.addSeparator();

            juce::PopupMenu templates;
            templates.addItem(410, "Instrument Template (not available yet)", false);
            templates.addItem(411, "Pattern Template (not available yet)", false);
            menu.addSubMenu("Templates", templates);

            juce::PopupMenu vstEffects;
            vstEffects.addItem(409, "VST3 effects are not available yet", false);
            menu.addSubMenu("VST Effects", vstEffects);

            juce::PopupMenu vst3Instruments;
            vst3Instruments.addItem(404, "Scan / Manage VST3 Instruments...", hasProject);
            vst3Instruments.addSeparator();
            addKnownVst3InstrumentsToMenu(vst3Instruments);
            menu.addSubMenu("VST Instruments", vst3Instruments);

            juce::PopupMenu directX;
            directX.addItem(424, "DirectX instruments are not available", false);
            menu.addSubMenu("DirectX", directX);
            menu.addSeparator();
            juce::PopupMenu fxDynamics;
            fxDynamics.addItem(425, "No dynamics effects available yet", false);
            menu.addSubMenu("FX Dynamics", fxDynamics);
            juce::PopupMenu fxEchoDelay;
            fxEchoDelay.addItem(426, "No echo/delay effects available yet", false);
            menu.addSubMenu("FX Echo/Delay", fxEchoDelay);
            juce::PopupMenu fxReverb;
            fxReverb.addItem(427, "No reverb effects available yet", false);
            menu.addSubMenu("FX Reverb", fxReverb);
            juce::PopupMenu fxFilter;
            fxFilter.addItem(408, "Built-in Equaliser", instrumentTrack != nullptr);
            menu.addSubMenu("FX Filter", fxFilter);
            juce::PopupMenu fxMisc;
            fxMisc.addItem(428, "No miscellaneous effects available yet", false);
            menu.addSubMenu("FX Misc", fxMisc);
            juce::PopupMenu fxHolder;
            fxHolder.addItem(429, "No FX holders available yet", false);
            menu.addSubMenu("FX Holder", fxHolder);
            menu.addSeparator();
            juce::PopupMenu audioTrack;
            audioTrack.addItem(406, "Add Audio Track", hasProject);
            audioTrack.addItem(430, "Import Audio File...", hasProject);
            menu.addSubMenu("Audio Track", audioTrack);
            menu.addItem(407, "MIDI Out (not available yet)", false);
        }
        else if (menuKind == 4) // Options
        {
            menu.addItem(501, "Loop", hasProject, loopButton.getToggleState());
            menu.addItem(502, "Metronome", hasProject, metronomeButton.getToggleState());
            menu.addSeparator();
            menu.addItem(503, "Audio Output Settings...");
            menu.addItem(516, "Audio / MIDI Diagnostics...");
            menu.addItem(504, "Audio Input Settings...");
            menu.addSeparator();
            menu.addItem(505, "MIDI Input Settings...");
            menu.addItem(506, "MIDI In Controllers... (not available)", false);
            menu.addSeparator();
            menu.addItem(507, "Sample Paths...");
            menu.addItem(508, "VST Paths...");
            menu.addItem(514, "Skins (not available yet)", false);
            menu.addItem(515, "Wallpapers (not available yet)", false);
            menu.addSeparator();
            menu.addItem(509, "Time Signature...", hasProject);
            menu.addItem(510, "Timebase... (coming in a later pass)", false);
            menu.addSeparator();
            menu.addItem(511, "Preferences...");
            menu.addItem(512, "Project Settings...", hasProject);
            menu.addItem(513, "Playlist Settings...");
        }
        else if (menuKind == 5) // Window
        {
            menu.addItem(601, "Playlist", true, playlistWindow != nullptr && playlistWindow->isVisible());
            menu.addItem(602, "Mixing Console", true, mixerWindow != nullptr && mixerWindow->isVisible());
            menu.addItem(603, "Master Section", true, mixerWindow != nullptr && mixerWindow->isVisible());
            menu.addItem(604, "Piano Roll", hasPattern,
                         pianoRollWindow != nullptr && pianoRollWindow->isVisible());
            menu.addSeparator();
            menu.addItem(609, "Compact Mixer", mixerWindow != nullptr);
            menu.addItem(610, "Expand Mixer", mixerWindow != nullptr);
            menu.addItem(611, "Minimize Mixer", mixerWindow != nullptr && mixerWindow->isVisible());
            menu.addItem(613, "Drum Rack", instrumentTrack != nullptr
                         && findSamplerPlugin(selectedProjectTrackId()) != nullptr);
            menu.addSeparator();
            menu.addItem(605, "Hide All Child Windows");
            menu.addItem(606, "Minimize All Windows to Pattern Controls");
            menu.addItem(607, "Cascade Open Windows");
            menu.addItem(608, generatorCollapsed ? "Restore Generator" : "Minimize Generator",
                         instrumentTrack != nullptr, generatorCollapsed);
            menu.addItem(612, allGeneratorsMinimized ? "Restore Generator Panels" : "Minimize All Generators",
                         !getGeneratorTracks().isEmpty(), allGeneratorsMinimized);
        }
        else
        {
            menu.addItem(701, "About Orbit");
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&source),
            [this, menuKind](int result) { handleMenuResult(menuKind, result); });
    }

    void handleMenuResult(int menuKind, int item)
    {
        if (item == 0) return;
        const int vst3Index = item - 1000;
        if (juce::isPositiveAndBelow(vst3Index, vst3MenuDescriptions.size()))
        {
            addVst3Instrument(vst3MenuDescriptions.getReference(vst3Index));
            return;
        }
        switch (item)
        {
            case 201: undoProjectAction(); break;
            case 202: redoProjectAction(); break;
            case 203: copyOrCutPatternNotes(true); break;
            case 204: copyOrCutPatternNotes(false); break;
            case 205: pastePatternNotes(); break;
            case 206: randomizeCurrentPattern(); break;
            case 207: showRandomizeOptions(); break;
            case 208: humanizeCurrentPattern(); break;
            case 209: showQuantizeDialog(); break;
            case 210: showTransposeDialog(); break;
            case 211: showScaleVelocitiesDialog(); break;
            case 212: showLengthDialog(); break;
            case 213: selectAllPatternNotes(); break;
            case 101: createProject(); break;
            case 102: openProject(); break;
            case 103: saveProject(); break;
            case 104: exportMidi(); break;
            case 106: case 512: showProjectSettings(); break;
            case 511: showPreferences(); break;
            case 513: showPlaylistSettings(); break;
            case 105: juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
            case 301: togglePlaylistView(); break;
            case 302: showMixer(); break;
            case 303: showMixer(true); break;
            case 304: showPianoRoll(); break;
            case 305: case 503: showAudioSettings(); break;
            case 516: showAudioDiagnostics(); break;
            case 308: showTempoDialog(); break;
            case 309: case 509: showTimeSignatureDialog(); break;
            case 310: showModules(); break;
            case 307:
                if (auto* window = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent()))
                    window->setFullScreen(!window->isFullScreen());
                break;
            case 504: case 505: showAudioSettings(); break;
            case 507: showSamplePathsDialog(); break;
            case 508: showVstPathsDialog(); break;
            case 306:
                status.setVisible(!status.isVisible());
                buildLabel.setVisible(status.isVisible());
                resized();
                break;
            case 312:
                showHints = !showHints;
                if (tooltipWindow != nullptr)
                {
                    tooltipWindow->hideTip();
                    tooltipWindow->setMillisecondsBeforeTipAppears(showHints ? 650 : 2147483647);
                }
                if (auto* settings = applicationProperties.getUserSettings())
                {
                    settings->setValue("showHints", showHints);
                    applicationProperties.saveIfNeeded();
                }
                break;
            case 401: insertBuiltInInstrument(); break;
            case 420: addDrumRackTrack(); break;
            case 406: addAudioTrack(); break;
            case 430: importAudioFile(); break;
            case 404: showVst3InstrumentBrowser(); break;
            case 405: addPattern(); break;
            case 408:
                if (instrumentTrack != nullptr)
                    openTrackEqualiser(getMixerTracks().indexOf(instrumentTrack));
                break;
            case 501: loopButton.setToggleState(!loopButton.getToggleState(), juce::sendNotification); break;
            case 502: metronomeButton.setToggleState(!metronomeButton.getToggleState(), juce::sendNotification); break;
            case 601: togglePlaylistView(); break;
            case 602: showMixer(); break;
            case 603: showMixer(true); break;
            case 604: showPianoRoll(); break;
            case 613:
                if (instrumentTrack != nullptr)
                    openDrumRackWindow(selectedProjectTrackId());
                break;
            case 605:
                if (playlistWindow != nullptr) playlistWindow->setVisible(false);
                if (mixerWindow != nullptr) mixerWindow->setVisible(false);
                layoutMixerHandles();
                if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
                if (equaliserWindow != nullptr) equaliserWindow->setVisible(false);
                if (audioSettingsWindow != nullptr) audioSettingsWindow->setVisible(false);
                break;
            case 606:
                if (mixerWindow != nullptr) mixerWindow->setVisible(false);
                layoutMixerHandles();
                for (auto* window : { static_cast<juce::DocumentWindow*>(equaliserWindow.get()),
                                      static_cast<juce::DocumentWindow*>(audioSettingsWindow.get()) })
                    if (window != nullptr && window->isVisible()) window->setMinimised(true);
                if (instrumentTrack != nullptr && !generatorCollapsed)
                    toggleGeneratorCollapsed();
                break;
            case 607: cascadeOpenWindows(); break;
            case 608: toggleGeneratorCollapsed(); break;
            case 612:
                allGeneratorsMinimized = !allGeneratorsMinimized;
                if (allGeneratorsMinimized)
                {
                    generatorCollapsed = false;
                    pianoRollMaximized = false;
                    if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
                    if (!pianoRollRestoreBounds.isEmpty()) generatorPanelBounds = pianoRollRestoreBounds;
                    pianoRollButton.setButtonText("PIANO ROLL");
                    status.setText("All generator panels minimized. Select one from the generator bar to restore it.",
                                   juce::dontSendNotification);
                }
                resized();
                break;
            case 609: setMixerPanelSize(false); break;
            case 610: setMixerPanelSize(true); break;
            case 611:
                if (mixerWindow != nullptr) mixerWindow->setVisible(false);
                layoutMixerHandles();
                break;
            case 701: juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "About Orbit",
                "Orbit is a standalone, pattern-first DAW built with JUCE and Tracktion Engine."); break;
            default: break;
        }
        juce::ignoreUnused(menuKind);
    }

    void showProjectSettings()
    {
        if (!hasProject) return;
        juce::AlertWindow dialog("Project Settings", "General information and project-level options.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("name", project.getName(), "Project name:");
        dialog.addTextEditor("description", project.getDescription(), "Description:");
        if (auto* description = dialog.getTextEditor("description"))
        {
            description->setMultiLine(true);
            description->setReturnKeyStartsNewLine(true);
        }
        dialog.addTextEditor("creatorUrl", project.getCreatorUrl(), "Creator webpage URL:");
        dialog.addTextBlock("Changes are stored in the Orbit project file. Instrument bundling and alternate tuning tables are not available yet.");
        dialog.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;

        const auto name = dialog.getTextEditorContents("name").trim();
        const auto description = dialog.getTextEditorContents("description").trim();
        const auto creatorUrl = dialog.getTextEditorContents("creatorUrl").trim();
        if (name == project.getName() && description == project.getDescription()
            && creatorUrl == project.getCreatorUrl()) return;
        rememberUndo("Change project settings");
        project.setName(name);
        project.setDescription(description);
        project.setCreatorUrl(creatorUrl);
        isDirty = true;
        refreshProjectView("Project settings updated. Save to keep the change.");
    }

    void showPreferences()
    {
        auto* settings = applicationProperties.getUserSettings();
        if (settings == nullptr) return;
        juce::AlertWindow dialog("Preferences", "Visual update and automatic save settings.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("updateRate", juce::String(settings->getIntValue("updateRateHz", 30)),
                             "Update rate (frames per second):");
        dialog.addTextEditor("vuDecay", juce::String(settings->getIntValue("vuDecay", 75)),
                             "VU decay (percent):");
        dialog.addTextEditor("autoSave", juce::String(settings->getIntValue("autoSaveMinutes", 0)),
                             "Auto-save every N minutes (0 disables):");
        dialog.addTextEditor("defaultProject", settings->getValue("defaultProjectPath"),
                             "Default project file:");
        dialog.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;

        const auto updateRate = juce::jlimit(2, 60, dialog.getTextEditorContents("updateRate").getIntValue());
        const auto vuDecay = juce::jlimit(1, 100, dialog.getTextEditorContents("vuDecay").getIntValue());
        const auto autoSaveMinutes = juce::jlimit(0, 240, dialog.getTextEditorContents("autoSave").getIntValue());
        settings->setValue("updateRateHz", updateRate);
        settings->setValue("vuDecay", vuDecay);
        settings->setValue("autoSaveMinutes", autoSaveMinutes);
        settings->setValue("defaultProjectPath", dialog.getTextEditorContents("defaultProject").trim());
        applicationProperties.saveIfNeeded();
        startTimerHz(updateRate);
        lastAutoSaveMs = juce::Time::getMillisecondCounter();
        status.setText("Preferences saved.", juce::dontSendNotification);
    }

    void showSamplePathsDialog()
    {
        showSearchPathsDialog("Sample Paths", "samplePaths", "Sample search folders saved.");
    }

    void showVstPathsDialog()
    {
        showSearchPathsDialog("VST Paths", "vstPaths",
            "VST3 search folders saved.", true);
    }

    juce::StringArray getVstSearchPaths()
    {
        juce::StringArray paths;
        if (auto* settings = applicationProperties.getUserSettings())
        {
            paths.addTokens(settings->getValue("vstPaths"), ";\n\r", "\"");
            paths.trim();
            paths.removeEmptyStrings();
        }
        return paths;
    }

    void showVst3InstrumentBrowser()
    {
        if (!hasProject) return;
        if (vst3InstrumentBrowser == nullptr)
        {
            auto& plugins = engine.getPluginManager();
            vst3InstrumentBrowser = std::make_unique<Vst3InstrumentBrowser>(
                plugins.pluginFormatManager, plugins.knownPluginList,
                [this] { return getVstSearchPaths(); },
                [this] { showVstPathsDialog(); },
                [this](const juce::PluginDescription& description) { addVst3Instrument(description); });
        }
        vst3InstrumentBrowser->setVisible(true);
        vst3InstrumentBrowser->toFront(true);
        if (vst3InstrumentBrowser->needsInitialScan()) vst3InstrumentBrowser->scanNow();
    }

    void startVst3Scan()
    {
        if (vst3InstrumentBrowser == nullptr)
        {
            auto& plugins = engine.getPluginManager();
            vst3InstrumentBrowser = std::make_unique<Vst3InstrumentBrowser>(
                plugins.pluginFormatManager, plugins.knownPluginList,
                [this] { return getVstSearchPaths(); },
                [this] { showVstPathsDialog(); },
                [this](const juce::PluginDescription& description) { addVst3Instrument(description); });
        }
        vst3InstrumentBrowser->setVisible(false);
        vst3InstrumentBrowser->scanNow();
    }

    juce::String getVst3InstrumentGroup(const juce::PluginDescription& description)
    {
        const auto pluginPath = juce::File(description.fileOrIdentifier)
                                   .getFullPathName().replaceCharacter('\\', '/');
        const auto customPaths = getVstSearchPaths();
        for (const auto& path : customPaths)
        {
            const juce::File root(path);
            const auto rootPath = root.getFullPathName().replaceCharacter('\\', '/').trimCharactersAtEnd("/");
            if (rootPath.isEmpty() || !pluginPath.startsWithIgnoreCase(rootPath + "/")) continue;
            const auto relativePath = pluginPath.substring(rootPath.length() + 1);
            const auto firstFolder = relativePath.upToFirstOccurrenceOf("/", false, false);
            if (firstFolder.endsWithIgnoreCase(".vst3")) return root.getFileName();
            return firstFolder.isNotEmpty() ? firstFolder : root.getFileName();
        }

        juce::Array<juce::File> roots;
        auto& formats = engine.getPluginManager().pluginFormatManager;
        for (int i = 0; i < formats.getNumFormats(); ++i)
            if (auto* format = formats.getFormat(i); format != nullptr && format->getName() == "VST3")
            {
                const auto defaults = format->getDefaultLocationsToSearch();
                for (int pathIndex = 0; pathIndex < defaults.getNumPaths(); ++pathIndex)
                    roots.add(defaults[pathIndex]);
            }

        for (const auto& root : roots)
        {
            auto rootPath = root.getFullPathName().replaceCharacter('\\', '/').trimCharactersAtEnd("/");
            if (rootPath.isEmpty() || !pluginPath.startsWithIgnoreCase(rootPath + "/")) continue;
            auto relativePath = pluginPath.substring(rootPath.length() + 1);
            const auto firstFolder = relativePath.upToFirstOccurrenceOf("/", false, false);
            if (firstFolder.endsWithIgnoreCase(".vst3")) return {};
            if (firstFolder.isNotEmpty()) return firstFolder;
            return {};
        }
        return juce::File(description.fileOrIdentifier).getParentDirectory().getFileName();
    }

    void addKnownVst3InstrumentsToMenu(juce::PopupMenu& menu)
    {
        vst3MenuDescriptions.clear();
        juce::Array<juce::PluginDescription> instruments;
        for (const auto& description : engine.getPluginManager().knownPluginList.getTypes())
            if (description.pluginFormatName == "VST3" && description.isInstrument)
                instruments.add(description);
        std::sort(instruments.begin(), instruments.end(), [](const auto& a, const auto& b)
        {
            return a.name.compareIgnoreCase(b.name) < 0;
        });
        if (instruments.isEmpty())
        {
            menu.addItem(405, "No VST3 instruments scanned yet", false);
            return;
        }

        std::map<juce::String, juce::PopupMenu> grouped;
        for (const auto& description : instruments)
        {
            const int id = 1000 + vst3MenuDescriptions.size();
            vst3MenuDescriptions.add(description);
            juce::PopupMenu::Item item(description.name);
            item.setID(id);
            const auto group = getVst3InstrumentGroup(description);
            if (group.isEmpty()) menu.addItem(std::move(item));
            else grouped[group].addItem(std::move(item));
        }
        for (auto& [group, groupMenu] : grouped)
            menu.addSubMenu(group, groupMenu);
    }

    juce::String getInstrumentHeaderText() const
    {
        if (instrumentTrack != nullptr)
            for (auto* plugin : instrumentTrack->pluginList)
                if (auto* external = dynamic_cast<te::ExternalPlugin*>(plugin))
                    return external->getName() + "  ·  VST3 INSTRUMENT";
        return "4OSC  ·  BUILT-IN SYNTH";
    }

    void addVst3Instrument(const juce::PluginDescription& description)
    {
        if (!hasProject || edit == nullptr)
        {
            showError("Project required", "Create or open a project before inserting a VST3 instrument.");
            return;
        }
        rememberUndo("Insert VST3 Instrument");

        // A new Tracktion edit contains one empty audio track. Reuse it for the
        // first Orbit instrument so engine-track indexes stay aligned with the
        // Orbit project track IDs. Inserting a second engine track here used
        // to make the first VST's piano-roll edits target an empty ID, leaving
        // it silent in both Pattern and Song playback.
        auto engineTracks = getGeneratorTracks();
        const bool reusingInitialTrack = project.getTrackCount() == 0 && engineTracks.size() == 1;
        te::AudioTrack* track = reusingInitialTrack
            ? engineTracks.getUnchecked(0) : nullptr;
        if (track == nullptr)
        {
            auto insertedTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr);
            track = insertedTrack.get();
        }
        if (track == nullptr)
        {
            showError("Could not add VST3 instrument", "Tracktion Engine could not create an audio track.");
            return;
        }

        auto plugin = edit->getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, description);
        if (plugin == nullptr)
        {
            if (!reusingInitialTrack) edit->deleteTrack(track);
            showError("Could not load VST3 instrument", "The selected plugin could not be instantiated. Check its installation and architecture.");
            return;
        }

        const auto trackName = description.name.isNotEmpty() ? description.name
                                                              : "VST3 Instrument " + juce::String(project.getTrackCount() + 1);
        track->setName(trackName);
        track->pluginList.insertPlugin(plugin, 0, nullptr);
        // Prepare the live MIDI path as soon as an instrument is added so
        // direct keyboard audition works before transport has been started.
        edit->restartPlayback();
        project.addInstrumentTrack();
        project.setTrackName(project.getTrackCount() - 1, trackName);
        instrumentTrack = track;
        patternClip = nullptr;
        updateActiveInstrumentEditor();
        refreshTrackSelector();
        readPatternSteps();
        refreshPianoRollAutomationTargets();
        refreshMixerControls();
        isDirty = true;
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh("A1");
        refreshProjectView("Added VST3 instrument " + description.name + ". Save to keep the change.");
    }

    void showSearchPathsDialog(const juce::String& title, const juce::String& settingsKey,
                               const juce::String& savedStatus, bool includeVstStartupOption = false)
    {
        auto* settings = applicationProperties.getUserSettings();
        if (settings == nullptr) return;
        juce::StringArray paths;
        paths.addTokens(settings->getValue(settingsKey), ";\n\r", "\"");
        paths.trim();
        paths.removeEmptyStrings();
        auto window = std::make_unique<SearchPathsWindow>(title, std::move(paths),
            [this, settingsKey, savedStatus](juce::StringArray updatedPaths)
            {
                if (auto* userSettings = applicationProperties.getUserSettings())
                {
                    userSettings->setValue(settingsKey, updatedPaths.joinIntoString(";"));
                    applicationProperties.saveIfNeeded();
                    status.setText(savedStatus, juce::dontSendNotification);
                }
            }, includeVstStartupOption,
            includeVstStartupOption && settings->getBoolValue("scanVst3AtStartup", false),
            [this](bool enabled)
            {
                if (auto* userSettings = applicationProperties.getUserSettings())
                {
                    userSettings->setValue("scanVst3AtStartup", enabled);
                    applicationProperties.saveIfNeeded();
                }
            });
        window->setVisible(true);
        window->toFront(true);
        if (settingsKey == "samplePaths") samplePathsWindow = std::move(window);
        else vstPathsWindow = std::move(window);
    }

    void showTempoDialog()
    {
        if (!hasProject || edit == nullptr) return;
        juce::AlertWindow dialog("Tempo", "Set the project's initial tempo.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("bpm", juce::String(tempoSlider.getValue(), 1), "Beats per minute:");
        dialog.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;

        const auto bpm = juce::jlimit(20.0, 300.0, dialog.getTextEditorContents("bpm").getDoubleValue());
        if (std::abs(bpm - tempoSlider.getValue()) < 0.0001) return;
        rememberUndo("Change tempo");
        tempoSlider.setValue(bpm, juce::sendNotificationSync);
    }

    void showTimeSignatureDialog()
    {
        if (!hasProject || edit == nullptr) return;
        const auto current = project.getInitialTimeSignature();
        juce::AlertWindow dialog("Time Signature", "Set the project's initial meter.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("numerator", juce::String(current.numerator), "Beats per bar:");
        dialog.addTextEditor("denominator", juce::String(current.denominator), "Beat unit (1, 2, 4, 8, 16, 32):");
        dialog.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;

        const auto numerator = juce::jlimit(1, 32, dialog.getTextEditorContents("numerator").getIntValue());
        auto denominator = dialog.getTextEditorContents("denominator").getIntValue();
        if (denominator != 1 && denominator != 2 && denominator != 4 && denominator != 8
            && denominator != 16 && denominator != 32)
            denominator = 4;
        if (current.numerator == numerator && current.denominator == denominator) return;

        rememberUndo("Change time signature");
        project.setInitialTimeSignature(numerator, denominator);
        if (auto* initialMeter = edit->tempoSequence.getTimeSig(0))
            initialMeter->setStringTimeSig(juce::String(numerator) + "/" + juce::String(denominator));
        updateLoopRange();
        const auto signatureText = juce::String(numerator) + "/" + juce::String(denominator);
        timeSignatureLabel.setText(signatureText, juce::dontSendNotification);
        musicalDetails.setText("Tracks: " + juce::String(project.getTrackCount()) + "   |   "
                                   + juce::String(tempoSlider.getValue(), 1) + " BPM   |   " + signatureText
                                   + "   |   Built-in 4OSC   |   64 patterns per generator",
                               juce::dontSendNotification);
        isDirty = true;
        status.setText("Time signature changed. Save to keep the change.", juce::dontSendNotification);
    }

    void showModules()
    {
        if (!hasProject || edit == nullptr) return;
        const auto tracks = getGeneratorTracks();
        if (tracks.isEmpty())
        {
            showError("Modules", "This project has no active instrument generators.");
            return;
        }

        juce::StringArray names;
        const auto selectedIndex = tracks.indexOf(instrumentTrack);
        for (int i = 0; i < tracks.size(); ++i)
            names.add(tracks.getUnchecked(i)->getName() + "  ·  Built-in 4OSC instrument");

        juce::AlertWindow dialog("Modules", "Select a generator to activate in the project workspace.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addComboBox("generator", names, "Generator:");
        if (auto* selector = dialog.getComboBoxComponent("generator"))
            selector->setSelectedItemIndex(juce::jmax(0, selectedIndex), juce::dontSendNotification);
        dialog.addButton("Activate", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        dialog.addButton("Close", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        if (dialog.runModalLoop() != 1) return;
        if (auto* selector = dialog.getComboBoxComponent("generator"))
            trackSelector.setSelectedId(selector->getSelectedItemIndex() + 1, juce::sendNotificationSync);
    }

    void showPlaylistSettings()
    {
        showPlaylist();
        if (playlistWindow != nullptr) playlistWindow->showSettings();
    }

    void selectAllPatternNotes()
    {
        if (instrumentTrack == nullptr || !hasPattern) return;
        const auto hasAnyNotes = std::any_of(noteGrid.begin(), noteGrid.end(), [](const auto& pitchSteps)
        {
            return std::any_of(pitchSteps.begin(), pitchSteps.end(), [](bool active) { return active; });
        });
        allNotesSelected = hasAnyNotes;
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
        status.setText(hasAnyNotes ? "Selected all notes in the current pattern."
                                   : "The current pattern has no notes to select.",
                       juce::dontSendNotification);
    }

    void copyOrCutPatternNotes(bool cut)
    {
        if (instrumentTrack == nullptr || edit == nullptr || !hasPattern) return;
        patternClipboard = {};
        patternClipboard.lengthSteps = selectedPatternLength();
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        bool foundNote = false;
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                const auto enabled = project.getPatternNote(trackId, patternIndex, pitch, step);
                patternClipboard.notes[row][column] = enabled;
                if (enabled)
                {
                    foundNote = true;
                    patternClipboard.lengths[row][column] = project.getPatternNoteLength(trackId, patternIndex, pitch, step);
                    patternClipboard.velocities[row][column] = project.getPatternNoteVelocity(trackId, patternIndex, pitch, step);
                }
            }
        patternClipboard.hasNotes = foundNote;
        if (!cut || !foundNote) return;

        rememberUndo("Cut pattern notes");
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
                if (patternClipboard.notes[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)])
                    project.setPatternNote(trackId, patternIndex, pitch, step, false);
        readPatternSteps();
        allNotesSelected = false;
        refreshPatternLengthDisplay();
        loadSelectedPitchSteps();
        isDirty = true;
        rebuildPatternClip();
        status.setText("Cut notes from Pattern " + juce::String::charToString(static_cast<juce::juce_wchar>('A' + patternIndex / 8))
                       + juce::String(patternIndex % 8 + 1) + ".", juce::dontSendNotification);
    }

    void pastePatternNotes()
    {
        if (instrumentTrack == nullptr || edit == nullptr || !patternClipboard.hasNotes) return;
        rememberUndo("Paste pattern notes");
        allNotesSelected = false;
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        project.setPatternCreated(trackId, patternIndex, true);
        project.setPatternLengthSteps(trackId, patternIndex, patternClipboard.lengthSteps);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                const auto enabled = patternClipboard.notes[row][column];
                project.setPatternNote(trackId, patternIndex, pitch, step, enabled);
                if (enabled)
                {
                    project.setPatternNoteLength(trackId, patternIndex, pitch, step,
                        patternClipboard.lengths[row][column]);
                    project.setPatternNoteVelocity(trackId, patternIndex, pitch, step,
                        patternClipboard.velocities[row][column]);
                }
            }
        readPatternSteps();
        refreshPatternLengthDisplay();
        loadSelectedPitchSteps();
        isDirty = true;
        rebuildPatternClip();
        status.setText("Pasted notes into the selected pattern.", juce::dontSendNotification);
    }

    PatternData capturePatternData(const juce::String& trackId, int patternIndex) const
    {
        PatternData data;
        data.created = project.isPatternCreated(trackId, patternIndex);
        data.lengthSteps = project.getPatternLengthSteps(trackId, patternIndex);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                data.notes[row][column] = project.getPatternNote(trackId, patternIndex, pitch, step);
                if (data.notes[row][column])
                {
                    data.lengths[row][column] = project.getPatternNoteLength(trackId, patternIndex, pitch, step);
                    data.velocities[row][column] = project.getPatternNoteVelocity(trackId, patternIndex, pitch, step);
                }
            }
        return data;
    }

    void writePatternData(const juce::String& trackId, int patternIndex, const PatternData& data)
    {
        project.setPatternCreated(trackId, patternIndex, data.created);
        project.setPatternLengthSteps(trackId, patternIndex, data.lengthSteps);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                const auto enabled = data.notes[row][column];
                project.setPatternNote(trackId, patternIndex, pitch, step, enabled);
                if (enabled)
                {
                    project.setPatternNoteLength(trackId, patternIndex, pitch, step,
                        juce::jlimit(1, 64 - step, data.lengths[row][column]));
                    project.setPatternNoteVelocity(trackId, patternIndex, pitch, step,
                        juce::jlimit(1, 127, data.velocities[row][column]));
                }
            }
    }

    void finishPatternEdit(const juce::String& message)
    {
        readPatternSteps();
        refreshPatternLengthDisplay();
        loadSelectedPitchSteps();
        isDirty = true;
        rebuildPatternClip();
        status.setText(message + " Save to keep the change.", juce::dontSendNotification);
    }

    void showRandomizeOptions()
    {
        juce::AlertWindow dialog("Randomize Options", "Set the note range and density used by Randomize.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("density", juce::String(randomDensityPercent), "Density (percent):");
        dialog.addTextEditor("low", juce::String(randomPitchLow), "Lowest pitch (MIDI note):");
        dialog.addTextEditor("high", juce::String(randomPitchHigh), "Highest pitch (MIDI note):");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        randomDensityPercent = juce::jlimit(1, 100, dialog.getTextEditorContents("density").getIntValue());
        randomPitchLow = juce::jlimit(minimumPitch, maximumPitch, dialog.getTextEditorContents("low").getIntValue());
        randomPitchHigh = juce::jlimit(randomPitchLow, maximumPitch, dialog.getTextEditorContents("high").getIntValue());
        status.setText("Randomize options saved.", juce::dontSendNotification);
    }

    void randomizeCurrentPattern()
    {
        if (instrumentTrack == nullptr || edit == nullptr) return;
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        auto data = capturePatternData(trackId, patternIndex);
        const auto length = data.lengthSteps;
        auto& random = juce::Random::getSystemRandom();
        for (int step = 0; step < length; ++step)
        {
            if (random.nextInt(100) >= randomDensityPercent) continue;
            const auto pitch = random.nextInt(juce::Range<int>(randomPitchLow, randomPitchHigh + 1));
            const auto row = static_cast<size_t>(pitch - minimumPitch);
            const auto column = static_cast<size_t>(step);
            data.notes[row][column] = true;
            data.lengths[row][column] = 1;
            data.velocities[row][column] = random.nextInt(juce::Range<int>(70, 121));
        }
        data.created = true;
        rememberUndo("Randomize pattern");
        writePatternData(trackId, patternIndex, data);
        finishPatternEdit("Pattern randomized.");
    }

    void humanizeCurrentPattern()
    {
        if (instrumentTrack == nullptr || edit == nullptr || !hasPattern) return;
        juce::AlertWindow dialog("Humanize", "Vary note start times by up to one step and velocities by the selected amount.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("timing", "1", "Timing variation (steps, 0–4):");
        dialog.addTextEditor("velocity", "8", "Velocity variation (0–32):");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        const auto timing = juce::jlimit(0, 4, dialog.getTextEditorContents("timing").getIntValue());
        const auto velocity = juce::jlimit(0, 32, dialog.getTextEditorContents("velocity").getIntValue());
        if (timing == 0 && velocity == 0) return;

        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        auto data = capturePatternData(trackId, patternIndex);
        PatternData result = data;
        for (auto& row : result.notes) row.fill(false);
        auto& random = juce::Random::getSystemRandom();
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < data.lengthSteps; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                if (!data.notes[row][column]) continue;
                const auto shift = timing == 0 ? 0 : random.nextInt(2 * timing + 1) - timing;
                const auto movedStep = juce::jlimit(0, data.lengthSteps - 1, step + shift);
                const auto movedColumn = static_cast<size_t>(movedStep);
                result.notes[row][movedColumn] = true;
                result.lengths[row][movedColumn] = data.lengths[row][column];
                result.velocities[row][movedColumn] = juce::jlimit(1, 127,
                    data.velocities[row][column] + (velocity == 0 ? 0 : random.nextInt(2 * velocity + 1) - velocity));
            }
        rememberUndo("Humanize pattern");
        writePatternData(trackId, patternIndex, result);
        finishPatternEdit("Pattern humanized.");
    }

    void showQuantizeDialog()
    {
        if (!hasPattern) return;
        juce::AlertWindow dialog("Quantize", "Snap note starts to a rhythmic grid.", juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("resolution", "16", "Resolution (notes per bar):");
        dialog.addTextEditor("durations", "0", "Also quantize durations? 1 = yes, 0 = no:");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        const auto resolution = juce::jlimit(1, 64, dialog.getTextEditorContents("resolution").getIntValue());
        const auto quantizeDurations = dialog.getTextEditorContents("durations").getIntValue() != 0;
        const auto grid = juce::jmax(1, 16 / resolution);
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        auto data = capturePatternData(trackId, patternIndex);
        PatternData result = data;
        for (auto& row : result.notes) row.fill(false);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < data.lengthSteps; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                if (!data.notes[row][column]) continue;
                const auto snapped = juce::jlimit(0, data.lengthSteps - 1, ((step + grid / 2) / grid) * grid);
                const auto outColumn = static_cast<size_t>(snapped);
                result.notes[row][outColumn] = true;
                result.lengths[row][outColumn] = quantizeDurations
                    ? juce::jmax(1, ((data.lengths[row][column] + grid / 2) / grid) * grid)
                    : data.lengths[row][column];
                result.velocities[row][outColumn] = data.velocities[row][column];
            }
        rememberUndo("Quantize pattern");
        writePatternData(trackId, patternIndex, result);
        finishPatternEdit("Pattern quantized.");
    }

    void showTransposeDialog()
    {
        if (!hasPattern) return;
        juce::AlertWindow dialog("Transpose", "Transpose this pattern or more patterns on the selected generator.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("semitones", "1", "Semitones:");
        dialog.addTextEditor("scope", "0", "Scope: 0 current pattern, 1 generator, 2 all generators:");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        const auto amount = juce::jlimit(-48, 48, dialog.getTextEditorContents("semitones").getIntValue());
        const auto scope = juce::jlimit(0, 2, dialog.getTextEditorContents("scope").getIntValue());
        if (amount == 0) return;
        const auto selectedTrack = selectedProjectTrackId();
        rememberUndo("Transpose pattern");
        for (int t = 0; t < project.getTrackCount(); ++t)
        {
            const auto trackId = project.getTrackId(t);
            if (scope != 2 && trackId != selectedTrack) continue;
            const auto first = scope == 0 ? selectedPatternIndex() : 0;
            const auto last = scope == 0 ? first + 1 : 64;
            for (int pattern = first; pattern < last; ++pattern)
            {
                auto data = capturePatternData(trackId, pattern);
                if (!data.created) continue;
                PatternData result = data;
                for (auto& row : result.notes) row.fill(false);
                for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                    for (int step = 0; step < 64; ++step)
                    {
                        const auto row = static_cast<size_t>(pitch - minimumPitch);
                        const auto column = static_cast<size_t>(step);
                        if (!data.notes[row][column]) continue;
                        const auto newPitch = pitch + amount;
                        if (newPitch < minimumPitch || newPitch > maximumPitch) continue;
                        const auto newRow = static_cast<size_t>(newPitch - minimumPitch);
                        result.notes[newRow][column] = true;
                        result.lengths[newRow][column] = data.lengths[row][column];
                        result.velocities[newRow][column] = data.velocities[row][column];
                    }
                writePatternData(trackId, pattern, result);
            }
        }
        finishPatternEdit("Transpose applied.");
    }

    void showScaleVelocitiesDialog()
    {
        if (!hasPattern) return;
        juce::AlertWindow dialog("Scale Velocities", "Scale note velocities across the selected pattern.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("begin", "1", "Begin velocity (1–127):");
        dialog.addTextEditor("end", "127", "End velocity (1–127):");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        const auto begin = juce::jlimit(1, 127, dialog.getTextEditorContents("begin").getIntValue());
        const auto end = juce::jlimit(1, 127, dialog.getTextEditorContents("end").getIntValue());
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        auto data = capturePatternData(trackId, patternIndex);
        std::vector<std::pair<int, int>> locations;
        for (int step = 0; step < 64; ++step)
            for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                if (data.notes[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)])
                    locations.emplace_back(pitch, step);
        if (locations.empty()) return;
        rememberUndo("Scale velocities");
        for (size_t i = 0; i < locations.size(); ++i)
        {
            const auto fraction = locations.size() <= 1 ? 0.0 : static_cast<double>(i) / (locations.size() - 1);
            const auto velocity = juce::roundToInt(begin + (end - begin) * fraction);
            project.setPatternNoteVelocity(trackId, patternIndex, locations[i].first, locations[i].second, velocity);
        }
        finishPatternEdit("Note velocities scaled.");
    }

    void showLengthDialog()
    {
        if (!hasPattern) return;
        juce::AlertWindow dialog("Length", "Scale pattern note positions and/or durations.",
                                 juce::MessageBoxIconType::NoIcon);
        dialog.addTextEditor("percent", "100", "Scale percent:");
        dialog.addTextEditor("starts", "0", "Scale start times? 1 = yes, 0 = no:");
        dialog.addTextEditor("durations", "1", "Scale durations? 1 = yes, 0 = no:");
        dialog.addButton("OK", 1);
        dialog.addButton("Cancel", 0);
        if (dialog.runModalLoop() != 1) return;
        const auto percent = juce::jlimit(10, 800, dialog.getTextEditorContents("percent").getIntValue());
        const auto scaleStarts = dialog.getTextEditorContents("starts").getIntValue() != 0;
        const auto scaleDurations = dialog.getTextEditorContents("durations").getIntValue() != 0;
        if (!scaleStarts && !scaleDurations) return;
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        auto data = capturePatternData(trackId, patternIndex);
        PatternData result = data;
        for (auto& row : result.notes) row.fill(false);
        if (scaleStarts)
            result.lengthSteps = juce::jlimit(1, 64, juce::roundToInt(data.lengthSteps * percent / 100.0));
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
            {
                const auto row = static_cast<size_t>(pitch - minimumPitch);
                const auto column = static_cast<size_t>(step);
                if (!data.notes[row][column]) continue;
                const auto moved = scaleStarts ? juce::jlimit(0, result.lengthSteps - 1,
                    juce::roundToInt(step * percent / 100.0)) : step;
                const auto outColumn = static_cast<size_t>(moved);
                result.notes[row][outColumn] = true;
                result.lengths[row][outColumn] = scaleDurations
                    ? juce::jlimit(1, 64 - moved, juce::roundToInt(data.lengths[row][column] * percent / 100.0))
                    : data.lengths[row][column];
                result.velocities[row][outColumn] = data.velocities[row][column];
            }
        rememberUndo("Scale pattern length");
        writePatternData(trackId, patternIndex, result);
        finishPatternEdit("Pattern length updated.");
    }

    void insertBuiltInInstrument()
    {
        if (!hasProject || edit == nullptr)
            createProject();
        if (hasProject && edit != nullptr)
            addInstrumentTrack();
    }

    ProjectSnapshot captureProjectSnapshot()
    {
        ProjectSnapshot snapshot;
        snapshot.document = project;
        snapshot.hadProject = hasProject && edit != nullptr;
        snapshot.dirty = isDirty;
        snapshot.bank = selectedBankIndex;
        snapshot.slot = selectedSlotIndex;
        snapshot.pitch = selectedPitch();
        if (snapshot.hadProject)
        {
            edit->flushState();
            snapshot.editState = edit->state.createCopy();
            snapshot.document.setTracktionEditState(snapshot.editState);
            const auto tracks = getGeneratorTracks();
            const auto selectedIndex = tracks.indexOf(instrumentTrack);
            if (juce::isPositiveAndBelow(selectedIndex, project.getTrackCount()))
                snapshot.selectedTrackId = project.getTrackId(selectedIndex);
        }
        return snapshot;
    }

    void rememberUndo(const juce::String& label)
    {
        undoHistory.push_back({ captureProjectSnapshot(), label });
        if (undoHistory.size() > 50)
            undoHistory.erase(undoHistory.begin());
        redoHistory.clear();
    }

    void restoreProjectSnapshot(const ProjectSnapshot& snapshot, const juce::String& message)
    {
        resetPatternButtons();
        project = snapshot.document;
        edit.reset();
        if (snapshot.hadProject && snapshot.editState.isValid())
            edit = te::loadEditFromState(engine, snapshot.editState.createCopy());
        hasProject = snapshot.hadProject && edit != nullptr;
        isDirty = snapshot.dirty;
        selectedBankIndex = snapshot.bank;
        selectedSlotIndex = snapshot.slot;
        pitchSelector.setSelectedId(pitchToItemId(snapshot.pitch), juce::dontSendNotification);
        for (int i = 0; i < 8; ++i)
        {
            patternBankButtons[static_cast<size_t>(i)]->setToggleState(i == selectedBankIndex, juce::dontSendNotification);
            patternSlotButtons[static_cast<size_t>(i)]->setToggleState(i == selectedSlotIndex, juce::dontSendNotification);
        }
        if (hasProject)
        {
            const auto tracks = getGeneratorTracks();
            for (int i = 0; i < tracks.size() && i < project.getTrackCount(); ++i)
                if (project.getTrackId(i) == snapshot.selectedTrackId)
                {
                    instrumentTrack = tracks.getUnchecked(i);
                    break;
                }
            if (instrumentTrack == nullptr)
                findFirstInstrumentTrack();
            if (instrumentTrack != nullptr)
            {
                const auto selectedIndex = tracks.indexOf(instrumentTrack);
                trackSelector.setSelectedId(selectedIndex + 1, juce::dontSendNotification);
                readPatternSteps();
                refreshMixerControls();
            }
        }
        refreshTrackSelector();
        refreshProjectView(message);
    }

    void undoProjectAction()
    {
        if (undoHistory.empty()) return;
        auto current = captureProjectSnapshot();
        const auto label = undoHistory.back().label;
        redoHistory.push_back({ std::move(current), label });
        auto target = std::move(undoHistory.back());
        undoHistory.pop_back();
        restoreProjectSnapshot(target.snapshot, "Undid " + label + ".");
    }

    void redoProjectAction()
    {
        if (redoHistory.empty()) return;
        auto current = captureProjectSnapshot();
        const auto label = redoHistory.back().label;
        undoHistory.push_back({ std::move(current), label });
        auto target = std::move(redoHistory.back());
        redoHistory.pop_back();
        restoreProjectSnapshot(target.snapshot, "Redid " + label + ".");
    }

    void rewindPlayback()
    {
        if (edit == nullptr) return;
        edit->getTransport().stop(false, false);
        edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));
        playButton.setButtonText("Play");
        status.setText("Returned to the start.", juce::dontSendNotification);
    }

    void skipTransport(int bars)
    {
        if (edit == nullptr) return;
        auto& transport = edit->getTransport();
        const auto barSeconds = 240.0 / juce::jmax(20.0, tempoSlider.getValue());
        const auto current = transport.getPosition().inSeconds();
        transport.setPosition(tracktion::TimePosition::fromSeconds(juce::jmax(0.0, current + bars * barSeconds)));
    }

    void updateLoopRange()
    {
        if (edit == nullptr) return;
        const auto bpm = juce::jmax(20.0, tempoSlider.getValue());
        const auto meter = project.getInitialTimeSignature();
        const auto secondsPerBar = (60.0 / bpm) * meter.numerator * (4.0 / juce::jmax(1, meter.denominator));
        const auto startBar = juce::roundToInt(loopStartSlider.getValue());
        const auto lengthBars = juce::roundToInt(loopLengthSlider.getValue());
        const auto loopEnabled = loopButton.getToggleState();
        const auto startSeconds = (loopStartSlider.getValue() - 1.0) * secondsPerBar;
        const auto lengthSeconds = loopLengthSlider.getValue() * secondsPerBar;
        auto& transport = edit->getTransport();
        transport.setLoopRange({ tracktion::TimePosition::fromSeconds(startSeconds),
                                 tracktion::TimePosition::fromSeconds(startSeconds + lengthSeconds) });
        transport.looping = loopEnabled;
        if (project.setLoopSettings(startBar, lengthBars, loopEnabled))
        {
            isDirty = true;
            projectDetails.setText("Project: " + project.getName() + "   ·   ID: " + project.getProjectId()
                                       + "   ·   Unsaved", juce::dontSendNotification);
        }
        if (playlistWindow != nullptr)
            playlistWindow->setLoopRange(startBar, lengthBars, loopEnabled);
    }

    void auditionSelectedInstrument(int pitch, bool isDown)
    {
        constexpr int auditionChannel = 1;
        if (isDown)
        {
            auditionTrack = instrumentTrack;
            if (auditionTrack == nullptr) return;
            ++midiAuditionNoteOns;
            lastMidiActivity = "Instrument keyboard pitch " + juce::String(pitch) + " on";
            // Tracktion Engine suppresses guide-note audition by default for
            // hosted apps. Force the note so the visible host keyboard works
            // immediately, without first starting transport or opening Piano Roll.
            auditionTrack->playGuideNote(pitch, te::MidiChannel(auditionChannel), 100, true, true, false);
            return;
        }
        if (auditionTrack != nullptr)
        {
            lastMidiActivity = "Instrument keyboard pitch " + juce::String(pitch) + " off";
            auditionTrack->turnOffGuideNotes(te::MidiChannel(auditionChannel));
            auditionTrack = nullptr;
        }
    }

    void setLoopEnabled(bool enabled)
    {
        if (edit != nullptr)
        {
            updateLoopRange();
            edit->getTransport().looping = enabled;
        }
        status.setText(enabled ? "Loop enabled." : "Loop disabled.", juce::dontSendNotification);
    }

    void setMetronomeEnabled(bool enabled)
    {
        if (edit != nullptr)
        {
            edit->clickTrackEnabled = enabled;
            edit->clickTrackEmphasiseBars = true;
            isDirty = true;
        }
        status.setText(enabled ? "Metronome enabled." : "Metronome disabled.", juce::dontSendNotification);
    }

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
        const auto titleText = getSelectedPatternTitle();
        generatorDragHandle.setText(titleText, juce::dontSendNotification);
        patternTitle.setText({}, juce::dontSendNotification);
        refreshPatternLengthDisplay();
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(titleText);
    }

    juce::String getSelectedPatternTitle() const
    {
        const auto patternIndex = selectedPatternIndex();
        const auto patternLabel = juce::String::charToString(
            static_cast<juce::juce_wchar>('A' + patternIndex / 8)) + juce::String(patternIndex % 8 + 1);
        const auto generatorName = instrumentTrack != nullptr ? instrumentTrack->getName() : juce::String("Generator");
        return generatorName + "  ·  Pattern " + patternLabel;
    }

    int selectedPatternLength() const
    {
        return juce::jlimit(1, 64, project.getPatternLengthSteps(selectedProjectTrackId(), selectedPatternIndex()));
    }

    void refreshPatternLengthDisplay()
    {
        const auto length = selectedPatternLength();
        patternLengthDisplay.setText(juce::String(length).paddedLeft('0', 3), juce::dontSendNotification);
        patternLengthDisplay.setTooltip("Pattern length: " + juce::String(length) + " steps. Use + and - to adjust.");
    }

    void adjustPatternLength(int delta)
    {
        if (instrumentTrack == nullptr || edit == nullptr) return;
        const auto nextLength = juce::jlimit(1, 64, selectedPatternLength() + delta);
        if (nextLength == selectedPatternLength()) return;
        rememberUndo("Change pattern length");
        project.setPatternLengthSteps(selectedProjectTrackId(), selectedPatternIndex(), nextLength);
        isDirty = true;
        refreshPatternLengthDisplay();
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
        rebuildPatternClip();
        status.setText("Pattern length: " + juce::String(nextLength) + " steps.", juce::dontSendNotification);
    }

    void showPianoRoll()
    {
        if (pianoRollWindow == nullptr || instrumentTrack == nullptr) return;
        if (pianoRollWindow->isVisible())
        {
            closePianoRollView();
            return;
        }
        if (generatorCollapsed)
        {
            generatorCollapsed = false;
            if (!expandedGeneratorBounds.isEmpty()) generatorPanelBounds = expandedGeneratorBounds;
            generatorCollapseButton.setKind(PlaylistWindowIconButton::Kind::minimize);
            generatorCollapseButton.setTooltip("Minimize this generator");
            pianoRollWasVisibleWhenCollapsed = false;
        }
        const auto previousPanelBounds = generatorPanelBounds;
        allGeneratorsMinimized = false;
        refreshPianoRollAutomationTargets();
        pianoRollWindow->refresh(getSelectedPatternTitle());
        pianoRollRestoreBounds = generatorPanelBounds;
        pianoRollMaximized = true;
        generatorPanelBounds = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(50).reduced(14, 10);
        pianoRollButton.setButtonText("INSTRUMENT");
        pianoRollButton.setTooltip("Return to the VST or built-in instrument");
        pianoRollWindow->setVisible(true);
        pianoRollWindow->focusGrid();
        if (activeInstrumentEditor != nullptr) activeInstrumentEditor->setVisible(false);
        resized();
        bringGeneratorToFront();
        // The maximized roll expands the generator panel beyond the embedded
        // plugin editor. Invalidate both extents so the newly exposed strip is
        // repainted with the same panel background instead of showing stale
        // pixels from the previous editor bounds.
        repaint(previousPanelBounds.getUnion(generatorPanelBounds));
        if (pianoRollWindow != nullptr) pianoRollWindow->repaint();
        status.setText("Piano Roll expanded across the generator workspace.", juce::dontSendNotification);
    }

    void closePianoRollView()
    {
        const auto previousPanelBounds = generatorPanelBounds;
        if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
        if (pianoRollMaximized && !pianoRollRestoreBounds.isEmpty())
            generatorPanelBounds = pianoRollRestoreBounds;
        pianoRollMaximized = false;
        pianoRollButton.setButtonText("PIANO ROLL");
        pianoRollButton.setTooltip("Open the selected pattern in the piano roll");
        resized();
        if (activeInstrumentEditor != nullptr && instrumentTrack != nullptr && !generatorCollapsed)
        {
            activeInstrumentEditor->setVisible(true);
            activeInstrumentEditor->repaint();
        }
        bringGeneratorToFront();
        // Clearing the roll's former maximized area is necessary to prevent
        // its background from lingering beside a narrower instrument GUI.
        repaint(previousPanelBounds.getUnion(generatorPanelBounds));
    }

    void toggleGeneratorCollapsed()
    {
        if (instrumentTrack == nullptr) return;
        if (!generatorCollapsed)
        {
            if (pianoRollWindow != nullptr && pianoRollWindow->isVisible()) closePianoRollView();
            expandedGeneratorBounds = generatorPanelBounds;
            pianoRollWasVisibleWhenCollapsed = false;
            generatorCollapsed = true;
            allGeneratorsMinimized = true;
            generatorCollapseButton.setKind(PlaylistWindowIconButton::Kind::restore);
            generatorCollapseButton.setTooltip("Restore generator");
            if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
        }
        else
        {
            generatorCollapsed = false;
            allGeneratorsMinimized = false;
            if (!expandedGeneratorBounds.isEmpty()) generatorPanelBounds = expandedGeneratorBounds;
            generatorCollapseButton.setKind(PlaylistWindowIconButton::Kind::minimize);
            generatorCollapseButton.setTooltip("Minimize this generator");
        }
        resized();
        if (generatorCollapsed)
            status.setText("Generator minimized. Use its tab at the bottom to restore it or switch generators.",
                           juce::dontSendNotification);
        else
        {
            if (pianoRollWindow != nullptr && pianoRollWasVisibleWhenCollapsed)
            {
                pianoRollWindow->refresh(getSelectedPatternTitle());
                pianoRollWindow->setVisible(true);
                pianoRollWindow->toFront(false);
            }
            pianoRollWasVisibleWhenCollapsed = false;
            status.setText("Generator restored.", juce::dontSendNotification);
        }
        repaint();
    }

    void togglePianoRollNote(int pitch, int step, bool erase)
    {
        if (instrumentTrack == nullptr || pitch < minimumPitch || pitch > maximumPitch
            || !juce::isPositiveAndBelow(step, 64)) return;
        if (!hasPattern)
        {
            project.setPatternCreated(selectedProjectTrackId(), selectedPatternIndex(), true);
            hasPattern = true;
        }
        const auto row = static_cast<size_t>(pitch - minimumPitch);
        const auto column = static_cast<size_t>(step);
        const bool enabled = !erase;
        if (noteGrid[row][column] == enabled) return;
        allNotesSelected = false;
        noteGrid[row][column] = enabled;
        project.setPatternNote(selectedProjectTrackId(), selectedPatternIndex(), pitch, step, enabled);
        if (pitch == selectedPitch() && step < static_cast<int>(steps.size()))
            steps[column]->setToggleState(enabled, juce::dontSendNotification);
        isDirty = true;
        if (pianoRollGestureActive)
        {
            pianoRollGestureChanged = true;
            status.setText(erase ? "Piano roll note removed." : "Piano roll note added.", juce::dontSendNotification);
            return;
        }
        status.setText(erase ? "Piano roll note removed." : "Piano roll note added.", juce::dontSendNotification);
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
        rebuildPatternClip();
    }

    void setPianoRollNoteLength(int pitch, int step, int length)
    {
        if (instrumentTrack == nullptr || !juce::isPositiveAndBelow(step, 64)) return;
        project.setPatternNoteLength(selectedProjectTrackId(), selectedPatternIndex(), pitch, step, length);
        isDirty = true;
        if (pianoRollGestureActive)
        {
            pianoRollGestureChanged = true;
            return;
        }
        if (pianoRollWindow != nullptr) pianoRollWindow->refresh(getSelectedPatternTitle());
        rebuildPatternClip();
    }

    void createProject()
    {
        if (hasProject && isDirty)
        {
            auto* dialog = new juce::AlertWindow("Create New Project?",
                "Discard the current unsaved project and start a new one?",
                juce::MessageBoxIconType::WarningIcon, this);
            dialog->addButton("Discard", 1);
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            dialog->centreAroundComponent(this, 420, 150);
            dialog->enterModalState(true, juce::ModalCallbackFunction::create(
                [safeThis = juce::Component::SafePointer<ProjectView>(this)](int result)
                {
                    if (safeThis != nullptr && result == 1)
                        safeThis->createProjectDiscardingCurrent();
                }), true);
            return;
        }
        createProjectDiscardingCurrent();
    }

    void createProjectDiscardingCurrent()
    {
        // Release references into the current Edit before replacing it. Tracktion
        // owns the tracks, and releasing an AudioTrack after its Edit is gone is unsafe.
        resetPatternButtons();
        project = ProjectDocument::createNew();
        edit = te::loadEditFromState(engine, te::createEmptyEdit(engine));
        hasProject = edit != nullptr;
        isDirty = true;
        undoHistory.clear();
        redoHistory.clear();
        refreshTrackSelector();
        refreshMixerControls();
        if (mixerWindow != nullptr) mixerWindow->refresh(-1);
        refreshProjectView(hasProject ? "New project created. Add an instrument and a pattern." : "Tracktion could not create an edit.");
    }

    void openProject()
    {
        auto startLocation = juce::File{};
        if (auto* settings = applicationProperties.getUserSettings())
        {
            const auto configured = settings->getValue("defaultProjectPath").trim();
            if (configured.isNotEmpty())
            {
                const juce::File configuredFile(configured);
                startLocation = configuredFile.isDirectory() ? configuredFile : configuredFile.getParentDirectory();
            }
        }
        fileChooser = std::make_unique<juce::FileChooser>("Open an Orbit project", startLocation, "*.orbit");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();
                if (file == juce::File{}) return;
                if (hasProject && isDirty)
                {
                    auto* dialog = new juce::AlertWindow("Open another project?",
                        "Discard the unsaved changes and open the selected project?",
                        juce::MessageBoxIconType::WarningIcon, this);
                    dialog->addButton("Discard and Open", 1);
                    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                    dialog->centreAroundComponent(this, 440, 160);
                    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
                        [safeThis = juce::Component::SafePointer<ProjectView>(this), file](int result)
                        {
                            if (safeThis != nullptr && result == 1)
                                safeThis->loadProjectFile(file);
                        }), true);
                    return;
                }
                loadProjectFile(file);
            });
    }

    void loadProjectFile(const juce::File& file)
    {
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
        undoHistory.clear();
        redoHistory.clear();
        migratePlaylistState();
        ensureExistingFxReturns();
        findFirstInstrumentTrack();
        updateActiveInstrumentEditor();
        refreshTrackSelector();
        readPatternSteps();
        refreshMixerControls();
        refreshProjectView("Opened project and restored its Tracktion session.");
    }

    void saveProject()
    {
        if (!hasProject || edit == nullptr) return;
        const auto currentFile = project.getFile();
        if (currentFile.existsAsFile()) { saveToFile(currentFile); return; }
        auto suggestedFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                 .getChildFile("Untitled Project.orbit");
        if (auto* settings = applicationProperties.getUserSettings())
        {
            const auto configured = settings->getValue("defaultProjectPath").trim();
            if (configured.isNotEmpty())
            {
                auto configuredFile = juce::File(configured);
                if (configuredFile.isDirectory()) configuredFile = configuredFile.getChildFile("Untitled Project.orbit");
                if (configuredFile.getFileName().isNotEmpty()) suggestedFile = configuredFile;
            }
        }
        fileChooser = std::make_unique<juce::FileChooser>("Save Orbit project", suggestedFile, "*.orbit");
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
        rememberUndo("Insert Instrument");
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
        edit->restartPlayback();
        instrumentTrack = track;
        refreshTrackSelector();
        readPatternSteps();
        refreshPianoRollAutomationTargets();
        refreshMixerControls();
        isDirty = true;
        refreshProjectView("Added a Tracktion instrument track with the built-in 4OSC synth.");
    }

    void addAudioTrack()
    {
        createAudioTrack({});
    }

    void importAudioFile(int targetTrackIndex = -1, int targetBar = 0)
    {
        if (!hasProject || edit == nullptr) return;
        if (!juce::isPositiveAndBelow(targetTrackIndex, project.getTrackCount())
            && juce::isPositiveAndBelow(trackSelector.getSelectedId() - 1, project.getTrackCount())
            && project.getTrackKind(trackSelector.getSelectedId() - 1) == ProjectDocument::TrackKind::audio)
            targetTrackIndex = trackSelector.getSelectedId() - 1;
        fileChooser = std::make_unique<juce::FileChooser>("Import audio file",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        auto* chooser = fileChooser.get();
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectFiles,
            [this, targetTrackIndex, targetBar](const juce::FileChooser& result)
            {
                const auto audioFile = result.getResult();
                if (audioFile.existsAsFile()) createAudioTrack(audioFile, targetTrackIndex, targetBar);
            });
    }

    void createAudioTrack(const juce::File& sourceFile, int targetTrackIndex = -1, int targetBar = 0)
    {
        if (!hasProject || edit == nullptr) return;
        rememberUndo(sourceFile.existsAsFile() ? "Import Audio Track" : "Add Audio Track");

        const bool useExistingTrack = sourceFile.existsAsFile()
            && juce::isPositiveAndBelow(targetTrackIndex, project.getTrackCount())
            && project.getTrackKind(targetTrackIndex) == ProjectDocument::TrackKind::audio;
        const auto trackName = sourceFile.existsAsFile() && !useExistingTrack
            ? sourceFile.getFileNameWithoutExtension()
            : "Audio " + juce::String(project.getTrackCount() + 1);
        const auto existingTracks = getGeneratorTracks();
        const bool reusingInitialTrack = project.getTrackCount() == 0 && existingTracks.size() == 1;
        te::AudioTrack* track = useExistingTrack && juce::isPositiveAndBelow(targetTrackIndex, existingTracks.size())
            ? existingTracks.getUnchecked(targetTrackIndex)
            : reusingInitialTrack ? existingTracks.getUnchecked(0) : nullptr;
        if (track == nullptr)
        {
            auto insertedTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr);
            track = insertedTrack.get();
        }
        if (track == nullptr)
        {
            showError("Could not add Audio Track", "Tracktion Engine could not create an audio track.");
            return;
        }

        if (!useExistingTrack)
        {
            track->setName(trackName);
            project.addTrack(ProjectDocument::TrackKind::audio, trackName);
            targetTrackIndex = project.getTrackCount() - 1;
        }
        if (sourceFile.existsAsFile())
        {
            te::AudioFile audioFile(edit->engine, sourceFile);
            if (!audioFile.isValid() || audioFile.getLength() <= 0.0)
            {
                if (!useExistingTrack)
                {
                    project.removeTrack(project.getTrackCount() - 1);
                    if (!reusingInitialTrack) edit->deleteTrack(track);
                }
                showError("Could not import audio", "Orbit could not read the selected audio file.");
                return;
            }

            const auto secondsPerBar = 60.0 / juce::jmax(20.0, project.getInitialTempoBpm()) * 4.0;
            const auto clipStart = juce::jmax(0, targetBar) * secondsPerBar;
            const auto clipEnd = clipStart + audioFile.getLength();
            auto clip = track->insertWaveClip(sourceFile.getFileNameWithoutExtension(), sourceFile,
                { { tracktion::TimePosition::fromSeconds(clipStart), tracktion::TimePosition::fromSeconds(clipEnd) }, {} }, false);
            if (clip == nullptr)
            {
                if (!useExistingTrack)
                {
                    project.removeTrack(project.getTrackCount() - 1);
                    if (!reusingInitialTrack) edit->deleteTrack(track);
                }
                showError("Could not import audio", "Tracktion Engine could not create a clip from the selected file.");
                return;
            }
            const auto tracksAfterImport = getGeneratorTracks();
            if (juce::isPositiveAndBelow(targetTrackIndex, tracksAfterImport.size()))
            {
                auto* destinationTrack = tracksAfterImport.getUnchecked(targetTrackIndex);
                for (int i = destinationTrack->getNumTrackItems(); --i >= 0;)
                    if (auto* previous = dynamic_cast<te::WaveAudioClip*>(destinationTrack->getTrackItem(i));
                        previous != nullptr && previous != clip.get())
                    {
                        const auto range = previous->getEditTimeRange();
                        if (range.getStart().inSeconds() < clipEnd && range.getEnd().inSeconds() > clipStart)
                            previous->removeFromParent();
                    }
            }
        }

        edit->restartPlayback();
        instrumentTrack = track;
        if (useExistingTrack)
            status.setText("Imported audio at Playlist bar " + juce::String(targetBar + 1) + ". Drag the clip to move it on the grid.",
                           juce::dontSendNotification);
        patternClip = nullptr;
        refreshTrackSelector();
        refreshMixerControls();
        isDirty = true;
        if (playlistWindow != nullptr) playlistWindow->refresh();
        refreshProjectView(sourceFile.existsAsFile()
            ? "Imported audio onto " + trackName + ". Save to keep the change."
            : "Added an empty audio track. Use Insert > Audio Track > Import Audio File to add a clip.");
    }

    te::SamplerPlugin* findSamplerPlugin(const juce::String& trackId) const
    {
        const auto tracks = getGeneratorTracks();
        for (int index = 0; index < tracks.size() && index < project.getTrackCount(); ++index)
        {
            if (project.getTrackId(index) != trackId) continue;
            for (auto* plugin : tracks.getUnchecked(index)->pluginList)
                if (auto* sampler = dynamic_cast<te::SamplerPlugin*>(plugin)) return sampler;
        }
        return nullptr;
    }

    void addDrumRackTrack()
    {
        if (!hasProject || edit == nullptr) return;
        rememberUndo("Insert Drum Rack");
        const auto trackName = "Drum Rack " + juce::String(project.getTrackCount() + 1);
        const auto existingTracks = getGeneratorTracks();
        te::AudioTrack* track = existingTracks.size() == 1 && project.getTrackCount() == 0
                                    ? existingTracks.getUnchecked(0) : nullptr;
        if (track == nullptr)
        {
            auto insertedTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr);
            track = insertedTrack.get();
        }
        if (track == nullptr)
        {
            showError("Could not add Drum Rack", "Tracktion Engine could not create an instrument track.");
            return;
        }

        auto sampler = edit->getPluginCache().createNewPlugin(te::SamplerPlugin::xmlTypeName, {});
        if (sampler == nullptr)
        {
            showError("Could not add Drum Rack", "Tracktion Engine could not create its built-in sampler.");
            return;
        }
        track->setName(trackName);
        track->pluginList.insertPlugin(*sampler, 0, nullptr);
        project.addInstrumentTrack();
        project.setTrackName(project.getTrackCount() - 1, trackName);
        edit->restartPlayback();
        instrumentTrack = track;
        patternClip = nullptr;
        refreshTrackSelector();
        readPatternSteps();
        refreshMixerControls();
        isDirty = true;
        openDrumRackWindow(project.getTrackId(project.getTrackCount() - 1));
        refreshProjectView("Added a sampler-backed Drum Rack. Load samples into pads, then program its steps.");
    }

    void openDrumRackWindow(const juce::String& trackId)
    {
        drumRackTrackId = trackId;
        drumRackWindow = std::make_unique<DrumRackWindow>(
            [this, trackId](int pad)
            {
                const auto note = 48 + pad;
                fileChooser = std::make_unique<juce::FileChooser>("Load a drum sample", juce::File{},
                                                                  "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
                fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                             | juce::FileBrowserComponent::canSelectFiles,
                    [this, trackId, pad, note](const juce::FileChooser& chooser)
                    {
                        const auto file = chooser.getResult();
                        if (!file.existsAsFile()) return;
                        auto* sampler = findSamplerPlugin(trackId);
                        if (sampler == nullptr) return;
                        for (int index = sampler->getNumSounds(); --index >= 0;)
                            if (sampler->getKeyNote(index) == note) sampler->removeSound(index);
                        const auto error = sampler->addSound(file.getFullPathName(), file.getFileNameWithoutExtension(),
                                                             0.0, 0.0, 0.0f);
                        if (error.isNotEmpty())
                        {
                            showError("Could not load sample", error);
                            return;
                        }
                        const int soundIndex = sampler->getNumSounds() - 1;
                        sampler->setSoundParams(soundIndex, note, note, note);
                        drumRackWindow->setPadSampleName(pad, file.getFileNameWithoutExtension());
                        isDirty = true;
                        status.setText("Loaded " + file.getFileName() + " into Drum Rack pad "
                                           + juce::String(pad + 1) + ". Save to keep the change.",
                                       juce::dontSendNotification);
                    });
            },
            [this, trackId](int pad)
            {
                drumRackWindow->setSteps(pad, getDrumRackSteps(trackId, selectedPatternIndex(), pad));
            },
            [this, trackId](int pad, int step, bool enabled)
            {
                const int slot = selectedPatternIndex();
                if (!project.isPatternCreated(trackId, slot))
                {
                    project.setPatternCreated(trackId, slot, true);
                    project.setPatternLengthSteps(trackId, slot, 16);
                }
                const int pitch = 48 + pad;
                project.setPatternNote(trackId, slot, pitch, step, enabled);
                if (enabled)
                {
                    project.setPatternNoteVelocity(trackId, slot, pitch, step, 100);
                    project.setPatternNoteLength(trackId, slot, pitch, step, 1);
                }
                drumRackWindow->setSteps(pad, getDrumRackSteps(trackId, slot, pad));
                rebuildPatternClip();
                isDirty = true;
                status.setText("Drum Rack pattern edited. Save to keep the change.", juce::dontSendNotification);
            });

        const auto* sampler = findSamplerPlugin(trackId);
        if (sampler != nullptr)
            for (int pad = 0; pad < 16; ++pad)
                for (int sound = 0; sound < sampler->getNumSounds(); ++sound)
                    if (sampler->getKeyNote(sound) == 48 + pad)
                        drumRackWindow->setPadSampleName(pad, sampler->getSoundName(sound));
        for (int pad = 0; pad < 16; ++pad)
            drumRackWindow->setSteps(pad, getDrumRackSteps(trackId, selectedPatternIndex(), pad));
    }

    std::array<bool, 16> getDrumRackSteps(const juce::String& trackId, int slot, int pad) const
    {
        std::array<bool, 16> steps{};
        for (int step = 0; step < 16; ++step)
            steps[static_cast<size_t>(step)] = project.getPatternNote(trackId, slot, 48 + pad, step);
        return steps;
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
            for (int step = 0; step < 64; ++step)
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
        playlistPreviewCache.clear();
        patternClip = nullptr;
        const auto engineTracks = getGeneratorTracks();
        const auto secondsPerStep = 15.0 / juce::jmax(20.0, project.getInitialTempoBpm());
        const auto secondsPerBar = secondsPerStep * 16.0;
        for (int trackIndex = 0; trackIndex < engineTracks.size() && trackIndex < project.getTrackCount(); ++trackIndex)
        {
            if (project.getTrackKind(trackIndex) == ProjectDocument::TrackKind::audio)
                continue;
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
                const auto start = bar * secondsPerBar;
                const auto patternLength = project.getPatternLengthSteps(trackId, patternIndex);
                const auto clipSeconds = juce::jmax(secondsPerBar, patternLength * secondsPerStep);
                auto inserted = track->insertMIDIClip(name, { tracktion::TimePosition::fromSeconds(start),
                                                              tracktion::TimePosition::fromSeconds(start + clipSeconds) }, nullptr);
                auto* clip = inserted.get();
                if (clip == nullptr) continue;
                clip->setLoopRangeBeats({ tracktion::BeatPosition::fromBeats(0.0), tracktion::BeatPosition::fromBeats(patternLength * 0.25) });
                for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                    for (int step = 0; step < patternLength; ++step)
                        if (project.getPatternNote(trackId, patternIndex, pitch, step))
                            clip->getSequence().addNote(pitch, tracktion::BeatPosition::fromBeats(step * 0.25),
                                                        tracktion::BeatDuration::fromBeats(project.getPatternNoteLength(trackId, patternIndex, pitch, step) * 0.25),
                                                        project.getPatternNoteVelocity(trackId, patternIndex, pitch, step), 0, nullptr);
                if (track == instrumentTrack && patternIndex == selectedPatternIndex())
                    patternClip = clip;
            }
        }
        if (auditionPatternPlayback)
        {
            for (int trackIndex = 0; trackIndex < engineTracks.size() && trackIndex < project.getTrackCount(); ++trackIndex)
            {
                auto* track = engineTracks.getUnchecked(trackIndex);
                const auto trackId = project.getTrackId(trackIndex);
                const auto patternIndex = project.getTrackSelectedPatternIndex(trackId);
                if (!patternHasNotes(trackId, patternIndex)) continue;

                const auto patternLength = project.getPatternLengthSteps(trackId, patternIndex);
                const auto lengthBeats = juce::jmax(1, patternLength) * 0.25;
                const auto clipSeconds = juce::jmax(0.125, lengthBeats * 60.0 / project.getInitialTempoBpm());
                const auto name = project.getPatternName(trackId, patternIndex);
                auto inserted = track->insertMIDIClip(name,
                    { tracktion::TimePosition::fromSeconds(0.0),
                      tracktion::TimePosition::fromSeconds(clipSeconds) }, nullptr);
                auto* clip = inserted.get();
                if (clip != nullptr)
                {
                    clip->setLoopRangeBeats({ tracktion::BeatPosition::fromBeats(0.0),
                                              tracktion::BeatPosition::fromBeats(lengthBeats) });
                    for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                        for (int step = 0; step < patternLength; ++step)
                            if (project.getPatternNote(trackId, patternIndex, pitch, step))
                                clip->getSequence().addNote(pitch,
                                    tracktion::BeatPosition::fromBeats(step * 0.25),
                                    tracktion::BeatDuration::fromBeats(
                                        project.getPatternNoteLength(trackId, patternIndex, pitch, step) * 0.25),
                                    project.getPatternNoteVelocity(trackId, patternIndex, pitch, step), 0, nullptr);
                    if (track == instrumentTrack)
                        patternClip = clip;
                }
            }
        }
        updateEngineAutomation();
        if (playlistWindow != nullptr) playlistWindow->refresh();
    }

    void refreshEnginePlaylistClip(int trackIndex, int bar)
    {
        if (edit == nullptr || auditionPatternPlayback)
        {
            if (edit != nullptr) refreshEnginePlaylist();
            return;
        }
        const auto tracks = getGeneratorTracks();
        if (!juce::isPositiveAndBelow(trackIndex, tracks.size())
            || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;

        auto* track = tracks.getUnchecked(trackIndex);
        const auto trackId = project.getTrackId(trackIndex);
        const auto secondsPerStep = 15.0 / juce::jmax(20.0, project.getInitialTempoBpm());
        const auto secondsPerBar = secondsPerStep * 16.0;
        const auto targetStart = static_cast<double>(bar) * secondsPerBar;
        std::vector<te::MidiClip*> oldClips;
        for (int item = 0; item < track->getNumTrackItems(); ++item)
            if (auto* clip = dynamic_cast<te::MidiClip*>(track->getTrackItem(item)); clip != nullptr
                && std::abs(clip->getEditTimeRange().getStart().inSeconds() - targetStart) < 0.001)
                oldClips.push_back(clip);

        for (auto* clip : oldClips)
        {
            if (clip == patternClip) patternClip = nullptr;
            track->deleteRegionOfClip(clip, clip->getEditTimeRange(), nullptr);
        }

        const auto patternId = project.getPlaylistPatternId(trackId, bar);
        const auto patternIndex = project.getPatternIndexForId(trackId, patternId);
        if (patternIndex < 0 || !project.isPatternCreated(trackId, patternIndex)) return;

        const auto patternLength = project.getPatternLengthSteps(trackId, patternIndex);
        const auto clipSeconds = juce::jmax(secondsPerBar, patternLength * secondsPerStep);
        auto inserted = track->insertMIDIClip(project.getPatternName(trackId, patternIndex),
            { tracktion::TimePosition::fromSeconds(targetStart),
              tracktion::TimePosition::fromSeconds(targetStart + clipSeconds) }, nullptr);
        auto* clip = inserted.get();
        if (clip == nullptr) return;
        clip->setLoopRangeBeats({ tracktion::BeatPosition::fromBeats(0.0),
                                  tracktion::BeatPosition::fromBeats(patternLength * 0.25) });
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < patternLength; ++step)
                if (project.getPatternNote(trackId, patternIndex, pitch, step))
                    clip->getSequence().addNote(pitch, tracktion::BeatPosition::fromBeats(step * 0.25),
                        tracktion::BeatDuration::fromBeats(
                            project.getPatternNoteLength(trackId, patternIndex, pitch, step) * 0.25),
                        project.getPatternNoteVelocity(trackId, patternIndex, pitch, step), 0, nullptr);
        if (track == instrumentTrack && patternIndex == selectedPatternIndex())
            patternClip = clip;
    }

    static float evaluateAutomationLane(const ProjectDocument::AutomationLane& lane, int position)
    {
        if (lane.points.isEmpty()) return 0.5f;
        const ProjectDocument::AutomationPoint* before = nullptr;
        const ProjectDocument::AutomationPoint* after = nullptr;
        for (const auto& point : lane.points)
        {
            if (point.position <= position && (before == nullptr || point.position > before->position)) before = &point;
            if (point.position >= position && (after == nullptr || point.position < after->position)) after = &point;
        }
        if (before == nullptr) return after->value;
        if (after == nullptr) return before->value;
        if (before->position == after->position) return before->value;
        const auto amount = static_cast<float>(position - before->position)
            / static_cast<float>(after->position - before->position);
        return before->value + amount * (after->value - before->value);
    }

    void refreshPianoRollAutomationTargets()
    {
        if (pianoRollWindow == nullptr) return;
        juce::StringArray labels, ids;
        if (instrumentTrack != nullptr)
        {
            for (auto* plugin : instrumentTrack->pluginList)
            {
                if (plugin == nullptr) continue;
                for (auto* parameter : plugin->getAutomatableParameters())
                {
                    if (parameter == nullptr) continue;
                    auto parameterName = parameter->getParameterName();
                    if (parameterName.equalsIgnoreCase("Filter Freq"))
                        parameterName = "Filter Cutoff";
                    labels.add(plugin->getName() + " / " + parameterName);
                    ids.add(plugin->getName() + "\n" + parameter->paramID + "\n" + parameterName);
                }
            }
        }
        pianoRollWindow->setAutomationTargets(labels, ids);
    }

    void updateEngineAutomation()
    {
        if (edit == nullptr) return;
        const auto tracks = getGeneratorTracks();
        const auto bpm = juce::jmax(20.0, project.getInitialTempoBpm());
        const auto secondsPerStep = 15.0 / bpm;
        for (int trackIndex = 0; trackIndex < tracks.size() && trackIndex < project.getTrackCount(); ++trackIndex)
        {
            auto* track = tracks.getUnchecked(trackIndex);
            const auto trackId = project.getTrackId(trackIndex);
            for (auto* plugin : track->pluginList)
            {
                if (plugin == nullptr) continue;
                for (auto* parameter : plugin->getAutomatableParameters())
                {
                    if (parameter == nullptr) continue;
                    ProjectDocument::AutomationLane patternLane, songLane;
                    bool hasPatternLane = false, hasSongLane = false;
                    for (int i = 0; i < project.getSongAutomationLaneCount(trackId); ++i)
                    {
                        auto candidate = project.getSongAutomationLane(trackId, i);
                        if (candidate.pluginName == plugin->getName() && candidate.parameterId == parameter->paramID)
                        { songLane = std::move(candidate); hasSongLane = true; break; }
                    }
                    const auto patternIndex = project.getTrackSelectedPatternIndex(trackId);
                    for (int i = 0; i < project.getPatternAutomationLaneCount(trackId, patternIndex); ++i)
                    {
                        auto candidate = project.getPatternAutomationLane(trackId, patternIndex, i);
                        if (candidate.pluginName == plugin->getName() && candidate.parameterId == parameter->paramID)
                        { patternLane = std::move(candidate); hasPatternLane = true; break; }
                    }

                    // Only replace curves owned by Orbit's automation editor. Leaving
                    // unrelated Tracktion curves untouched avoids deleting host/plugin automation.
                    if (!hasPatternLane && !hasSongLane) continue;
                    auto& curve = parameter->getCurve();
                    curve.clear();
                    const auto& lane = playbackMode == PlaybackMode::song ? songLane : patternLane;
                    const bool hasActiveLane = playbackMode == PlaybackMode::song ? hasSongLane : hasPatternLane;
                    if (!hasActiveLane) continue;
                    for (const auto& point : lane.points)
                    {
                        const auto seconds = point.position * secondsPerStep;
                        curve.addPoint(tracktion::TimePosition::fromSeconds(seconds),
                            parameter->valueRange.convertFrom0to1(juce::jlimit(0.0f, 1.0f, point.value)), 0.0f);
                    }
                }
            }
        }
    }

    void showPlaylist()
    {
        if (playlistWindow == nullptr) return;
        if (!playlistBoundsInitialised) initialisePlaylistPanelBounds();
        if (playlistWindow->isMinimized()) playlistWindow->setMinimized(false);
        playlistWindow->refresh();
        playlistWindow->setVisible(true);
        if (auto* settings = applicationProperties.getUserSettings())
        {
            settings->setValue("playlistWindowOpen", true);
            settings->saveIfNeeded();
        }
        // Keep generator pattern controls behind the Playlist while it is open.
        playlistWindow->toFront(false);
    }

    void initialisePlaylistPanelBounds()
    {
        if (playlistBoundsInitialised) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        playlistPanelBounds = area.withHeight(juce::jmin(700, area.getHeight()));
        playlistBoundsInitialised = true;
        resized();
    }

    void movePlaylistPanel(juce::Point<int> position)
    {
        if (playlistWindow == nullptr || playlistWindow->isMaximized() || playlistWindow->isMinimized()) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        playlistPanelBounds.setPosition(
            juce::jlimit(48 - playlistPanelBounds.getWidth(), getWidth() - 48, position.x),
            juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - 28), position.y));
        playlistWindow->setBounds(playlistPanelBounds);
    }

    void resizePlaylistPanel(juce::Rectangle<int> bounds)
    {
        if (playlistWindow == nullptr || playlistWindow->isMaximized() || playlistWindow->isMinimized()) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        bounds.setSize(juce::jlimit(juce::jmin(440, area.getWidth()), area.getWidth(), bounds.getWidth()),
                       juce::jlimit(juce::jmin(250, area.getHeight()), area.getHeight(), bounds.getHeight()));
        bounds.setPosition(juce::jlimit(area.getX(), juce::jmax(area.getX(), area.getRight() - bounds.getWidth()), bounds.getX()),
                           juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - 28), bounds.getY()));
        playlistPanelBounds = bounds;
        playlistWindow->setBounds(playlistPanelBounds);
    }

    void togglePlaylistMaximized()
    {
        if (playlistWindow == nullptr) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        if (!playlistWindow->isMaximized())
        {
            if (!playlistWindow->isMinimized()) playlistRestoreBounds = playlistPanelBounds;
            playlistWindow->setMinimized(false);
            playlistWindow->setMaximized(true);
            playlistPanelBounds = area;
        }
        else
        {
            playlistWindow->setMaximized(false);
            playlistPanelBounds = playlistRestoreBounds.isEmpty() ? area : playlistRestoreBounds;
        }
        playlistWindow->setBounds(playlistPanelBounds);
        playlistWindow->toFront(false);
    }

    void togglePlaylistView()
    {
        if (playlistWindow == nullptr) return;
        if (playlistWindow->isVisible() && playlistWindow->isMinimized())
            playlistWindow->setMinimized(false);
        else if (playlistWindow->isVisible())
        {
            playlistWindow->setVisible(false);
            if (auto* settings = applicationProperties.getUserSettings())
            {
                settings->setValue("playlistWindowOpen", false);
                settings->saveIfNeeded();
            }
        }
        else showPlaylist();
    }

    void cascadeOpenWindows()
    {
        const auto mainBounds = getScreenBounds();
        int offset = 0;
        for (auto* window : { static_cast<juce::DocumentWindow*>(equaliserWindow.get()),
                              static_cast<juce::DocumentWindow*>(audioSettingsWindow.get()) })
        {
            if (window == nullptr || !window->isVisible()) continue;
            window->setMinimised(false);
            window->setTopLeftPosition(mainBounds.getX() + 30 + offset, mainBounds.getY() + 90 + offset);
            window->toFront(false);
            offset += 28;
        }
        if (pianoRollWindow != nullptr && pianoRollWindow->isVisible())
            pianoRollWindow->toFront(false);
    }

    void stopPlayback()
    {
        if (edit != nullptr && playbackMode == PlaybackMode::song)
            rememberSongPatternsAt(edit->getTransport().getPosition().inSeconds());
        if (edit != nullptr) edit->getTransport().stop(false, false);
        if (auditionPatternPlayback)
        {
            auditionPatternPlayback = false;
            refreshEnginePlaylist();
        }
        playButton.setButtonText("Play");
        status.setText("Stopped.", juce::dontSendNotification);
    }

    void showMixer(bool focusMaster = false)
    {
        if (mixerWindow == nullptr) return;
        mixerWindow->setMasterSectionMode(focusMaster);
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        mixerWindow->refresh(tracks.indexOf(instrumentTrack));
        initialiseMixerPanelBounds();
        if (mixerWindow->isMinimized() && !mixerRestoreBounds.isEmpty())
            mixerPanelBounds = mixerRestoreBounds;
        mixerWindow->setMinimized(false);
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto stripCount = mixerWindow->getVisibleTrackCount();
        // The master strip is shown in its own view, so the instrument mixer
        // must size to exactly the strips currently visible.
        const auto contentWidth = stripCount * 132 + 20;
        const auto width = juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(), contentWidth);
        mixerPanelBounds.setWidth(width);
        mixerPanelBounds.setHeight(mixerMaximized ? area.getHeight()
            : juce::jlimit(juce::jmin(420, area.getHeight()), area.getHeight(), mixerPanelBounds.getHeight()));
        mixerPanelBounds.setPosition(constrainMixerPosition(mixerPanelBounds.getPosition()));
        mixerWindow->setMaximized(mixerMaximized);
        mixerWindow->setVisible(true);
        if (auto* settings = applicationProperties.getUserSettings())
        {
            settings->setValue("mixerWindowOpen", true);
            settings->saveIfNeeded();
        }
        resized();
        // resized() raises the generator-tab dock so it remains available
        // above the workspace. Re-raise the focused mixer afterward so the
        // dock cannot cover the channel routing buttons at the bottom.
        mixerWindow->toFront(false);
        if (focusMaster) mixerWindow->focusMaster();
    }

    void fitMixerWidthToStrips()
    {
        if (mixerWindow == nullptr || mixerWindow->isMaximized() || mixerWindow->isMinimized()) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto width = juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(),
                                        mixerWindow->getPreferredWidth());
        mixerPanelBounds = mixerWindow->getBounds();
        mixerPanelBounds.setWidth(width);
        mixerPanelBounds.setX(constrainMixerPosition(mixerPanelBounds.getPosition()).x);
        mixerWindow->setBounds(mixerPanelBounds);
    }

    void initialiseMixerPanelBounds()
    {
        if (mixerBoundsInitialised || mixerWindow == nullptr) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto stripCount = mixerWindow->getTrackCount ? mixerWindow->getTrackCount() : 0;
        const auto stripContentWidth = stripCount * 132;
        const auto defaultWidth = juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(),
                                               juce::jmax(152, stripContentWidth + 20));
        mixerPanelBounds = { juce::jmax(area.getX(), area.getRight() - defaultWidth), area.getY(),
                             defaultWidth, juce::jmin(650, area.getHeight()) };

        if (auto* settings = applicationProperties.getUserSettings();
            settings != nullptr && settings->getBoolValue("mixerPanelBoundsSet", false))
        {
            mixerPanelBounds.setPosition(settings->getIntValue("mixerPanelX", area.getX()),
                                         settings->getIntValue("mixerPanelY", area.getY()));
            mixerPanelBounds.setSize(settings->getIntValue("mixerPanelWidth", defaultWidth),
                                     settings->getIntValue("mixerPanelHeight", juce::jmin(650, area.getHeight())));
            if (mixerPanelBounds.getHeight() >= area.getHeight())
                mixerPanelBounds.setHeight(juce::jmin(650, area.getHeight()));
        }
        mixerPanelBounds.setPosition(constrainMixerPosition(mixerPanelBounds.getPosition()));
        mixerBoundsInitialised = true;
        resized();
    }

    void layoutMixerHandles()
    {
        // Mixer movement is handled by its compact title bar. Keep the legacy
        // overlay handles hidden so they cannot cover generator controls.
        mixerMoveHandle.setVisible(false);
        mixerResizeHandle.setVisible(false);
    }

    void moveMixerPanel(juce::Point<int> point)
    {
        if (!draggingMixer || mixerWindow == nullptr) return;
        mixerPanelBounds.setPosition(constrainMixerPosition(point - mixerDragOffset));
        mixerWindow->setBounds(mixerPanelBounds);
        layoutMixerHandles();
    }

    juce::Point<int> constrainMixerPosition(juce::Point<int> position) const
    {
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        // Let the mixer slide past either side of the workspace while keeping
        // a 48 px strip available to grab and bring it back.
        const auto minX = 48 - mixerPanelBounds.getWidth();
        const auto maxX = juce::jmax(minX, getWidth() - 48);
        return { juce::jlimit(minX, maxX, position.x),
                 juce::jlimit(area.getY(), juce::jmax(area.getY(), area.getBottom() - 28), position.y) };
    }

    void resizeMixerPanel(juce::Point<int> point)
    {
        if (!resizingMixer || mixerWindow == nullptr) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        const auto delta = point - mixerResizeStart;
        mixerPanelBounds.setSize(
            juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(), mixerWindow->getPreferredWidth()),
            juce::jlimit(juce::jmin(420, area.getHeight()), area.getHeight(), mixerResizeStartBounds.getHeight() + delta.y));
        mixerPanelBounds.setX(constrainMixerPosition(mixerResizeStartBounds.getPosition()).x);
        mixerWindow->setBounds(mixerPanelBounds);
        layoutMixerHandles();
    }

    void endMixerDrag()
    {
        draggingMixer = false;
        saveMixerPanelBounds();
        status.setText("Mixer moved. Its size and position are saved.", juce::dontSendNotification);
    }

    void endMixerResize()
    {
        resizingMixer = false;
        saveMixerPanelBounds();
        status.setText("Mixer resized. Its size and position are saved.", juce::dontSendNotification);
    }

    void saveMixerPanelBounds()
    {
        if (auto* settings = applicationProperties.getUserSettings())
        {
            settings->setValue("mixerPanelBoundsSet", true);
            settings->setValue("mixerPanelX", mixerPanelBounds.getX());
            settings->setValue("mixerPanelY", mixerPanelBounds.getY());
            settings->setValue("mixerPanelWidth", mixerPanelBounds.getWidth());
            settings->setValue("mixerPanelHeight", mixerPanelBounds.getHeight());
            settings->saveIfNeeded();
        }
    }

    void toggleMixerView()
    {
        if (mixerWindow == nullptr) return;
        if (mixerWindow->isVisible())
        {
            if (mixerWindow->isMasterSectionMode())
            {
                showMixer(false);
                return;
            }
            mixerWindow->setVisible(false);
            if (auto* settings = applicationProperties.getUserSettings())
            {
                settings->setValue("mixerWindowOpen", false);
                settings->saveIfNeeded();
            }
            layoutMixerHandles();
            repaint();
            return;
        }
        showMixer();
    }

    void setMixerPanelSize(bool expanded)
    {
        if (mixerWindow == nullptr) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        initialiseMixerPanelBounds();
        const auto width = juce::jlimit(juce::jmin(152, area.getWidth()), area.getWidth(),
                                        mixerWindow->getVisibleTrackCount() * 132 + 20);
        if (expanded)
        {
            if (!mixerMaximized) mixerRestoreBounds = mixerPanelBounds;
            mixerMaximized = true;
            mixerPanelBounds = area;
        }
        else
        {
            mixerMaximized = false;
            const auto height = juce::jmin(area.getHeight(), 520);
            mixerPanelBounds = { juce::jmax(area.getX(), area.getRight() - width), area.getBottom() - height, width, height };
            mixerRestoreBounds = mixerPanelBounds;
        }
        mixerWindow->setMaximized(mixerMaximized);
        mixerWindow->setMinimized(false);
        mixerWindow->setVisible(true);
        mixerWindow->setBounds(mixerPanelBounds);
        layoutMixerHandles();
        mixerWindow->toFront(false);
        mixerMoveHandle.toFront(false);
        mixerResizeHandle.toFront(false);
        saveMixerPanelBounds();
        status.setText(expanded ? "Mixer maximized inside Orbit." : "Mixer restored to compact view.",
                       juce::dontSendNotification);
    }

    void toggleMixerMaximized()
    {
        if (mixerWindow == nullptr) return;
        const auto area = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(22).reduced(14, 10);
        if (!mixerWindow->isMaximized())
        {
            if (!mixerWindow->isMinimized()) mixerRestoreBounds = mixerPanelBounds;
            mixerWindow->setMinimized(false);
            mixerMaximized = true;
            mixerWindow->setMaximized(true);
            mixerPanelBounds = area;
        }
        else
        {
            mixerMaximized = false;
            mixerWindow->setMaximized(false);
            mixerPanelBounds = mixerRestoreBounds.isEmpty() ? mixerPanelBounds : mixerRestoreBounds;
        }
        mixerWindow->setBounds(mixerPanelBounds);
        mixerWindow->toFront(false);
        saveMixerPanelBounds();
    }

    juce::Array<te::AudioTrack*> getGeneratorTracks() const
    {
        juce::Array<te::AudioTrack*> result;
        if (edit == nullptr) return result;
        for (auto* track : te::getAudioTracks(*edit))
            if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1)) < 0
                && static_cast<int>(track->state.getProperty(juce::Identifier("orbitMixerBus"), -1)) < 0)
                result.add(track);
        return result;
    }

    juce::Array<te::AudioTrack*> getMixerTracks() const
    {
        auto result = getGeneratorTracks();
        for (int bus = 0; bus < 8; ++bus)
            if (auto* returnTrack = getFxReturnTrack(bus))
                result.add(returnTrack);
        for (int bus = 0; bus < 8; ++bus)
            if (auto* outputBus = getOutputBusTrack(bus))
                result.add(outputBus);
        return result;
    }

    te::AudioTrack* getOutputBusTrack(int bus) const
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 8)) return nullptr;
        for (auto* track : te::getAudioTracks(*edit))
            if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitMixerBus"), -1)) == bus)
                return track;
        return nullptr;
    }

    te::AudioTrack* ensureOutputBusTrack(int bus)
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 8)) return nullptr;
        if (auto* existing = getOutputBusTrack(bus)) return existing;
        auto busTrack = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr, true);
        if (busTrack == nullptr) return nullptr;
        busTrack->setName("Bus " + juce::String(bus + 1));
        busTrack->state.setProperty(juce::Identifier("orbitMixerBus"), bus, &edit->getUndoManager());
        busTrack->state.setProperty(juce::Identifier("orbitMixerChannelId"), juce::Uuid().toString(),
                                     &edit->getUndoManager());
        return busTrack.get();
    }

    te::AudioTrack* getFxReturnTrack(int bus) const
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 8)) return nullptr;
        for (auto* track : te::getAudioTracks(*edit))
            if (static_cast<int>(track->state.getProperty(juce::Identifier("orbitFxReturnBus"), -1)) == bus)
                return track;
        return nullptr;
    }

    te::AudioTrack* ensureFxReturnTrack(int bus)
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 8)) return nullptr;
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

        const std::array<juce::String, 8> effectTypes {
            te::ReverbPlugin::xmlTypeName, te::DelayPlugin::xmlTypeName,
            te::ChorusPlugin::xmlTypeName, te::PhaserPlugin::xmlTypeName,
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
        for (int bus = 0; bus < 8; ++bus)
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
            if (!juce::isPositiveAndBelow(bus, 8)) return;

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
        for (int bus = 0; bus < 8; ++bus)
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
        if (playlistWindow != nullptr) playlistWindow->refresh();
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
        if (playlistWindow != nullptr) playlistWindow->refresh();
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
        eq->setEnabled(true);
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
        eq->setEnabled(true);
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
                && dynamic_cast<te::AuxSendPlugin*>(plugin) == nullptr
                && (dynamic_cast<te::ExternalPlugin*>(plugin) == nullptr
                    || !dynamic_cast<te::ExternalPlugin*>(plugin)->isSynth()))
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
        if (getInsertPlugins(index).size() >= 3)
        {
            status.setText("This mixer strip has three insert slots. Remove one before adding another.", juce::dontSendNotification);
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
        if (!juce::isPositiveAndBelow(index, tracks.size()) || !juce::isPositiveAndBelow(bus, 8)) return -60.0;
        if (auto* send = tracks.getUnchecked(index)->getAuxSendPlugin(bus))
            return send->getGainDb();
        return -60.0;
    }

    int getTrackOutputBus(int index) const
    {
        const auto tracks = edit != nullptr ? getMixerTracks() : juce::Array<te::AudioTrack*>{};
        const auto generatorCount = getGeneratorTracks().size();
        if (!juce::isPositiveAndBelow(index, generatorCount)
            || !juce::isPositiveAndBelow(index, tracks.size())) return -1;
        auto* destination = tracks.getUnchecked(index)->getOutput().getDestinationTrack();
        return destination != nullptr
            ? static_cast<int>(destination->state.getProperty(juce::Identifier("orbitMixerBus"), -1))
            : -1;
    }

    void setTrackOutputBus(int index, int bus)
    {
        auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        if (edit == nullptr || !juce::isPositiveAndBelow(index, tracks.size())
            || bus < -1 || bus >= 8) return;
        auto* track = tracks.getUnchecked(index);
        if (bus < 0)
            track->getOutput().setOutputToDefaultDevice(false);
        else
        {
            auto* destination = ensureOutputBusTrack(bus);
            if (destination == nullptr) return;
            track->getOutput().setOutputToTrack(destination);
        }
        isDirty = true;
        if (mixerWindow != nullptr) mixerWindow->refresh(getGeneratorTracks().indexOf(instrumentTrack));
        status.setText(bus < 0 ? "Channel routed to Master." : "Channel routed to Bus " + juce::String(bus + 1) + ".",
                       juce::dontSendNotification);
    }

    void setTrackSendGainDb(int index, int bus, double value)
    {
        if (edit == nullptr || !juce::isPositiveAndBelow(bus, 8)) return;
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
                           + fxNames[static_cast<size_t>(bus % 4)] + " return. Save to keep the change.", juce::dontSendNotification);
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

    te::WaveAudioClip* findAudioClipAtBar(int trackIndex, int bar, int* startBarOut, int* lengthBarsOut) const
    {
        if (startBarOut != nullptr) *startBarOut = -1;
        if (lengthBarsOut != nullptr) *lengthBarsOut = 1;
        if (edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || project.getTrackKind(trackIndex) != ProjectDocument::TrackKind::audio
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return nullptr;
        const auto tracks = getGeneratorTracks();
        if (!juce::isPositiveAndBelow(trackIndex, tracks.size())) return nullptr;
        const auto secondsPerBar = 60.0 / juce::jmax(20.0, project.getInitialTempoBpm()) * 4.0;
        const auto barStart = bar * secondsPerBar;
        const auto barEnd = barStart + secondsPerBar;
        auto* track = tracks.getUnchecked(trackIndex);
        for (int item = 0; item < track->getNumTrackItems(); ++item)
            if (auto* clip = dynamic_cast<te::WaveAudioClip*>(track->getTrackItem(item)))
            {
                const auto range = clip->getEditTimeRange();
                const auto clipStart = range.getStart().inSeconds();
                const auto clipEnd = range.getEnd().inSeconds();
                if (clipStart < barEnd && clipEnd > barStart)
                {
                    if (startBarOut != nullptr)
                        *startBarOut = juce::jmax(0, static_cast<int>(std::floor(clipStart / secondsPerBar + 1.0e-6)));
                    if (lengthBarsOut != nullptr)
                        *lengthBarsOut = juce::jmax(1, static_cast<int>(std::ceil((clipEnd - clipStart) / secondsPerBar - 1.0e-6)));
                    return clip;
                }
            }
        return nullptr;
    }

    juce::String getPlaylistClipLabel(int trackIndex, int bar) const
    {
        if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())) return {};
        if (project.getTrackKind(trackIndex) == ProjectDocument::TrackKind::audio && edit != nullptr)
        {
            if (auto* clip = findAudioClipAtBar(trackIndex, bar, nullptr, nullptr)) return clip->getName();
            return {};
        }
        const auto trackId = project.getTrackId(trackIndex);
        const auto patternId = project.getPlaylistPatternId(trackId, bar);
        const auto patternIndex = project.getPatternIndexForId(trackId, patternId);
        return patternIndex >= 0 ? project.getPatternName(trackId, patternIndex) : juce::String{};
    }

    std::vector<juce::Point<int>> getPlaylistPreviewNotes(int trackIndex, int bar) const
    {
        if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return {};
        const auto trackId = project.getTrackId(trackIndex);
        const auto patternIndex = project.getPatternIndexForId(trackId, project.getPlaylistPatternId(trackId, bar));
        if (patternIndex < 0) return {};
        const auto cacheKey = trackId + ":" + juce::String(patternIndex);
        if (const auto found = playlistPreviewCache.find(cacheKey); found != playlistPreviewCache.end())
            return found->second;
        std::vector<juce::Point<int>> notes;
        const auto length = project.getPatternLengthSteps(trackId, patternIndex);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < length; ++step)
                if (project.getPatternNote(trackId, patternIndex, pitch, step))
                    notes.emplace_back(step * 64 / juce::jmax(1, length), pitch - minimumPitch);
        playlistPreviewCache.emplace(cacheKey, notes);
        return notes;
    }

    bool getTrackRecordArm(int trackIndex) const
    {
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        if (edit == nullptr || !juce::isPositiveAndBelow(trackIndex, tracks.size())) return false;
        for (auto* input : edit->getAllInputDevices())
            if (input != nullptr && input->isRecordingEnabled(tracks.getUnchecked(trackIndex)->itemID)) return true;
        return false;
    }

    void setTrackRecordArm(int trackIndex, bool armed)
    {
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        if (edit == nullptr || !juce::isPositiveAndBelow(trackIndex, tracks.size())) return;
        const auto devices = edit->getAllInputDevices();
        if (devices.isEmpty())
        {
            status.setText("No MIDI input is available to arm for recording.", juce::dontSendNotification);
            if (playlistWindow != nullptr) playlistWindow->refresh();
            return;
        }
        for (auto* input : devices)
            if (input != nullptr) input->setRecordingEnabled(tracks.getUnchecked(trackIndex)->itemID, armed);
        isDirty = true;
        status.setText(armed ? "Generator armed for recording." : "Generator disarmed.", juce::dontSendNotification);
        if (playlistWindow != nullptr) playlistWindow->refresh();
    }

    void paintPlaylistClip(int trackIndex, int bar)
    {
        if (!hasProject || edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;
        if (project.getTrackKind(trackIndex) == ProjectDocument::TrackKind::audio)
        {
            importAudioFile(trackIndex, bar);
            return;
        }
        const auto trackId = project.getTrackId(trackIndex);
        // Use the current A1-H8 slot as the drawing tool on the clicked lane;
        // each destination generator retains its own note data for that slot.
        const auto patternIndex = selectedPatternIndex();
        const bool createdPattern = !project.isPatternCreated(trackId, patternIndex);
        const auto patternId = project.getPatternId(trackId, patternIndex);
        const auto currentPatternId = project.getPlaylistPatternId(trackId, bar);
        if (!createdPattern && currentPatternId == patternId)
            return;

        if (!playlistPaintGestureActive)
        {
            rememberUndo("Draw playlist pattern");
            playlistPaintGestureActive = true;
        }
        if (createdPattern)
            project.setPatternCreated(trackId, patternIndex, true);
        project.setPlaylistClip(trackId, patternId, bar, 1, true);
        refreshEnginePlaylistClip(trackIndex, bar);
        isDirty = true;
        if (createdPattern) readPatternSteps();
        if (playlistWindow != nullptr) playlistWindow->repaintGrid();
        status.setText("Drew " + project.getPatternName(trackId, patternIndex)
            + " in Playlist bar " + juce::String(bar + 1) + ".", juce::dontSendNotification);
    }

    void togglePlaylistClip(int trackIndex, int bar)
    {
        if (!hasProject || edit == nullptr || trackIndex < 0 || trackIndex >= project.getTrackCount()) return;
        if (!juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;
        if (project.getTrackKind(trackIndex) == ProjectDocument::TrackKind::audio)
        {
            importAudioFile(trackIndex, bar);
            return;
        }
        const auto trackId = project.getTrackId(trackIndex);
        const auto current = project.getPlaylistPatternId(trackId, bar);
        if (current.isNotEmpty())
        {
            const auto patternIndex = project.getPatternIndexForId(trackId, current);
            if (patternIndex >= 0)
            {
                trackSelector.setSelectedId(trackIndex + 1, juce::sendNotificationSync);
                selectPatternBank(patternIndex / ProjectDocument::patternsPerBank);
                selectPatternSlot(patternIndex % ProjectDocument::patternsPerBank);
                status.setText("Selected " + project.getPatternName(trackId, patternIndex)
                                   + ". Scroll over the clip to cycle patterns, or double-click for the available patterns.",
                               juce::dontSendNotification);
            }
            return;
        }
        status.setText("Move mode selected. Switch to Draw to place the selected pattern.",
                       juce::dontSendNotification);
    }

    void deletePlaylistClip(int trackIndex, int bar)
    {
        if (!hasProject || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;
        const auto trackId = project.getTrackId(trackIndex);
        const auto patternId = project.getPlaylistPatternId(trackId, bar);
        if (patternId.isEmpty()) return;
        rememberUndo("Delete Playlist clip");
        project.setPlaylistClip(trackId, {}, bar, 1, false);
        refreshEnginePlaylistClip(trackIndex, bar);
        isDirty = true;
        if (playlistWindow != nullptr) playlistWindow->repaintGrid();
        status.setText("Deleted the Playlist clip at bar " + juce::String(bar + 1) + ". Save to keep the change.",
                       juce::dontSendNotification);
    }

    void setPlaylistPattern(int trackIndex, int bar, int patternIndex)
    {
        if (!hasProject || edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)
            || !juce::isPositiveAndBelow(patternIndex, ProjectDocument::patternSlotCount)) return;
        const auto trackId = project.getTrackId(trackIndex);
        if (!project.isPatternCreated(trackId, patternIndex)) return;
        const auto currentId = project.getPlaylistPatternId(trackId, bar);
        const auto patternId = project.getPatternId(trackId, patternIndex);
        if (currentId == patternId) return;

        rememberUndo("Choose Playlist pattern");
        project.setPlaylistClip(trackId, patternId, bar, 1, true);
        trackSelector.setSelectedId(trackIndex + 1, juce::sendNotificationSync);
        selectPatternBank(patternIndex / ProjectDocument::patternsPerBank);
        selectPatternSlot(patternIndex % ProjectDocument::patternsPerBank);
        refreshEnginePlaylistClip(trackIndex, bar);
        isDirty = true;
        if (playlistWindow != nullptr) playlistWindow->repaintGrid();
        status.setText("Selected " + project.getPatternName(trackId, patternIndex)
                           + " for Playlist bar " + juce::String(bar + 1) + ".", juce::dontSendNotification);
    }

    void showPlaylistPatternMenu(int trackIndex, int bar, juce::Component* target)
    {
        if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;
        const auto trackId = project.getTrackId(trackIndex);
        const auto currentIndex = project.getPatternIndexForId(trackId, project.getPlaylistPatternId(trackId, bar));
        juce::PopupMenu patterns;
        int available = 0;
        for (int index = 0; index < ProjectDocument::patternSlotCount; ++index)
        {
            if (!project.isPatternCreated(trackId, index)) continue;
            const auto bank = juce::String::charToString(static_cast<juce::juce_wchar>('A' + index / 8));
            patterns.addItem(index + 1, bank + juce::String(index % 8 + 1) + "  ·  "
                + project.getPatternName(trackId, index), true, index == currentIndex);
            ++available;
        }
        if (available == 0)
        {
            status.setText("This generator has no created patterns yet. Draw its active pattern first.",
                           juce::dontSendNotification);
            return;
        }
        patterns.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target != nullptr ? target : playlistWindow.get()),
            [this, trackIndex, bar](int result)
            {
                if (result > 0) setPlaylistPattern(trackIndex, bar, result - 1);
            });
    }

    void scrollPlaylistPattern(int trackIndex, int bar, int direction)
    {
        if (!juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(bar, PlaylistGrid::barCount)) return;
        const auto trackId = project.getTrackId(trackIndex);
        const auto currentIndex = project.getPatternIndexForId(trackId, project.getPlaylistPatternId(trackId, bar));
        std::vector<int> available;
        for (int index = 0; index < ProjectDocument::patternSlotCount; ++index)
            if (project.isPatternCreated(trackId, index)) available.push_back(index);
        if (available.size() < 2) return;
        const auto current = std::find(available.begin(), available.end(), currentIndex);
        auto position = current == available.end() ? 0 : static_cast<int>(current - available.begin());
        const auto count = static_cast<int>(available.size());
        position = (position + direction + count) % count;
        setPlaylistPattern(trackIndex, bar, available[static_cast<size_t>(position)]);
    }

    void showPlaylistAudioContextMenu(int trackIndex, int bar, juce::Component* target)
    {
        if (!hasProject || edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || project.getTrackKind(trackIndex) != ProjectDocument::TrackKind::audio) return;
        int clipStartBar = -1;
        int clipLengthBars = 1;
        auto* clip = bar >= 0 ? findAudioClipAtBar(trackIndex, bar, &clipStartBar, &clipLengthBars) : nullptr;
        const auto trackId = project.getTrackId(trackIndex);
        juce::PopupMenu menu;
        menu.addItem(9201, clip != nullptr ? "Replace / Import Audio File…" : "Open / Import Audio File…");
        if (clip != nullptr)
        {
            menu.addItem(9202, "Rename Audio Clip…");
            menu.addItem(9203, "Delete Audio Clip");
        }
        menu.addSeparator();
        menu.addItem(9204, "Rename Audio Track…");
        menu.addItem(9205, "Delete Audio Track…");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target != nullptr ? target : playlistWindow.get()),
            [this, trackId, clipStartBar, bar](int result)
            {
                auto findTrackIndex = [this, trackId]
                {
                    for (int i = 0; i < project.getTrackCount(); ++i)
                        if (project.getTrackId(i) == trackId) return i;
                    return -1;
                };
                const auto row = findTrackIndex();
                if (!juce::isPositiveAndBelow(row, project.getTrackCount())) return;
                auto findClip = [this, row, clipStartBar, bar]() -> te::WaveAudioClip*
                {
                    const auto targetBar = clipStartBar >= 0 ? clipStartBar : bar;
                    return targetBar >= 0 ? findAudioClipAtBar(row, targetBar, nullptr, nullptr) : nullptr;
                };
                if (result == 9201)
                {
                    const auto importBar = clipStartBar >= 0 ? clipStartBar : juce::jmax(0, bar);
                    importAudioFile(row, importBar);
                    return;
                }
                if (result == 9202)
                {
                    auto* audioClip = findClip();
                    if (audioClip == nullptr) return;
                    juce::AlertWindow dialog("Rename Audio Clip", "Enter a new clip name.", juce::MessageBoxIconType::NoIcon);
                    dialog.addTextEditor("name", audioClip->getName());
                    dialog.addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                    if (dialog.runModalLoop() != 1) return;
                    const auto name = dialog.getTextEditorContents("name").trim();
                    if (name.isEmpty()) return;
                    rememberUndo("Rename audio clip");
                    audioClip->setName(name);
                    isDirty = true;
                    if (playlistWindow != nullptr) playlistWindow->refresh();
                    status.setText("Renamed audio clip. Save to keep the change.", juce::dontSendNotification);
                    return;
                }
                if (result == 9203)
                {
                    auto* audioClip = findClip();
                    if (audioClip == nullptr) return;
                    rememberUndo("Delete audio clip");
                    audioClip->removeFromParent();
                    edit->restartPlayback();
                    isDirty = true;
                    if (playlistWindow != nullptr) playlistWindow->refresh();
                    status.setText("Deleted audio clip. Save to keep the change.", juce::dontSendNotification);
                    return;
                }
                if (result == 9204)
                {
                    juce::AlertWindow dialog("Rename Audio Track", "Enter a new track name.", juce::MessageBoxIconType::NoIcon);
                    dialog.addTextEditor("name", project.getTrackName(row));
                    dialog.addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                    if (dialog.runModalLoop() != 1) return;
                    const auto name = dialog.getTextEditorContents("name").trim();
                    if (name.isEmpty()) return;
                    rememberUndo("Rename audio track");
                    project.setTrackName(row, name);
                    const auto tracks = getGeneratorTracks();
                    if (juce::isPositiveAndBelow(row, tracks.size())) tracks.getUnchecked(row)->setName(name);
                    isDirty = true;
                    refreshTrackSelector();
                    if (playlistWindow != nullptr) playlistWindow->refresh();
                    status.setText("Renamed audio track. Save to keep the change.", juce::dontSendNotification);
                    return;
                }
                if (result == 9205)
                {
                    const auto name = project.getTrackName(row);
                    juce::AlertWindow dialog("Delete Audio Track", "Delete " + name + " and all of its audio clips?",
                                             juce::MessageBoxIconType::NoIcon);
                    dialog.addButton("Delete", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                    if (dialog.runModalLoop() != 1) return;
                    const auto tracks = getGeneratorTracks();
                    if (!juce::isPositiveAndBelow(row, tracks.size())) return;
                    rememberUndo("Delete audio track");
                    auto* removedTrack = tracks.getUnchecked(row);
                    if (instrumentTrack == removedTrack) instrumentTrack = nullptr;
                    edit->deleteTrack(removedTrack);
                    project.removeTrack(row);
                    findFirstInstrumentTrack();
                    refreshTrackSelector();
                    refreshMixerControls();
                    refreshEnginePlaylist();
                    updateActiveInstrumentEditor();
                    isDirty = true;
                    refreshProjectView("Deleted " + name + ". Save to keep the change.");
                }
            });
    }

    void showPlaylistTrackContextMenu(int sourceIndex, juce::Component* target)
    {
        if (!hasProject || !juce::isPositiveAndBelow(sourceIndex, project.getTrackCount())) return;
        if (project.getTrackKind(sourceIndex) == ProjectDocument::TrackKind::audio)
        {
            showPlaylistAudioContextMenu(sourceIndex, -1, target);
            return;
        }
        const auto sourceTrackId = project.getTrackId(sourceIndex);
        juce::PopupMenu menu, cloneMenu, colourMenu;
        std::vector<juce::String> destinations;
        for (int index = 0; index < project.getTrackCount(); ++index)
        {
            if (index == sourceIndex) continue;
            cloneMenu.addItem(static_cast<int>(destinations.size()) + 1, project.getTrackName(index));
            destinations.push_back(project.getTrackId(index));
        }
        menu.addItem(9001, "Show Generator");
        menu.addItem(9002, "Rename Generator...");
        menu.addItem(9003, "Delete Generator...");
        menu.addSeparator();
        menu.addSubMenu("Clone Track To", cloneMenu, !destinations.empty());
        menu.addItem(9004, "Edit Song Events");
        menu.addSeparator();

        const std::array<std::pair<const char*, juce::Colour>, 12> palette {{
            { "Red", juce::Colour::fromRGB(210, 77, 77) },
            { "Orange", juce::Colour::fromRGB(225, 133, 55) },
            { "Yellow", juce::Colour::fromRGB(207, 174, 61) },
            { "Lime", juce::Colour::fromRGB(137, 181, 70) },
            { "Green", juce::Colour::fromRGB(65, 166, 106) },
            { "Teal", juce::Colour::fromRGB(53, 174, 164) },
            { "Cyan", juce::Colour::fromRGB(58, 171, 205) },
            { "Blue", juce::Colour::fromRGB(68, 128, 211) },
            { "Indigo", juce::Colour::fromRGB(103, 104, 197) },
            { "Violet", juce::Colour::fromRGB(158, 91, 198) },
            { "Magenta", juce::Colour::fromRGB(205, 83, 161) },
            { "Pink", juce::Colour::fromRGB(224, 116, 143) }
        }};
        const auto currentColour = project.getTrackColour(sourceIndex);
        colourMenu.addItem(500, "Default", true, currentColour.isTransparent());
        for (int i = 0; i < static_cast<int>(palette.size()); ++i)
            colourMenu.addItem(501 + i, palette[static_cast<size_t>(i)].first,
                true, currentColour == palette[static_cast<size_t>(i)].second);
        menu.addSubMenu("Track Color", colourMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target != nullptr ? target : playlistWindow.get()),
            [this, sourceIndex, sourceTrackId, destinations = std::move(destinations), palette](int result)
            {
                auto findSourceIndex = [this, sourceTrackId]
                {
                    for (int i = 0; i < project.getTrackCount(); ++i)
                        if (project.getTrackId(i) == sourceTrackId) return i;
                    return -1;
                };
                auto currentSourceIndex = findSourceIndex();
                if (currentSourceIndex < 0) return;
                if (result == 9001)
                {
                    const auto tracks = getGeneratorTracks();
                    if (juce::isPositiveAndBelow(currentSourceIndex, tracks.size()))
                    {
                        trackSelector.setSelectedId(currentSourceIndex + 1, juce::sendNotificationSync);
                        closePianoRollView();
                        allGeneratorsMinimized = false;
                        generatorCollapsed = false;
                        bringGeneratorToFront();
                        status.setText("Showing " + project.getTrackName(currentSourceIndex) + ".", juce::dontSendNotification);
                    }
                    return;
                }
                if (result == 9002)
                {
                    juce::AlertWindow dialog("Rename Generator", "Enter a new generator name.", juce::MessageBoxIconType::NoIcon);
                    dialog.addTextEditor("name", project.getTrackName(currentSourceIndex));
                    dialog.addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                    if (dialog.runModalLoop() != 1) return;
                    const auto newName = dialog.getTextEditorContents("name").trim();
                    if (newName.isEmpty()) return;
                    rememberUndo("Rename generator");
                    const auto tracks = getGeneratorTracks();
                    project.setTrackName(currentSourceIndex, newName);
                    if (juce::isPositiveAndBelow(currentSourceIndex, tracks.size()))
                        tracks.getUnchecked(currentSourceIndex)->setName(newName);
                    isDirty = true;
                    refreshTrackSelector();
                    refreshMixerControls();
                    if (playlistWindow != nullptr) playlistWindow->refresh();
                    status.setText("Renamed generator to " + newName + ". Save to keep the change.", juce::dontSendNotification);
                    return;
                }
                if (result == 9003)
                {
                    const auto trackName = project.getTrackName(currentSourceIndex);
                    juce::AlertWindow dialog("Delete Generator", "Delete " + trackName + " and its patterns and Playlist clips?",
                                             juce::MessageBoxIconType::NoIcon);
                    dialog.addButton("Delete", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                    if (dialog.runModalLoop() != 1) return;
                    const auto tracks = getGeneratorTracks();
                    if (!juce::isPositiveAndBelow(currentSourceIndex, tracks.size())) return;
                    rememberUndo("Delete generator");
                    auto* removedTrack = tracks.getUnchecked(currentSourceIndex);
                    closePianoRollView();
                    if (instrumentTrack == removedTrack)
                    {
                        instrumentTrack = nullptr;
                        patternClip = nullptr;
                    }
                    edit->deleteTrack(removedTrack);
                    project.removeTrack(currentSourceIndex);
                    findFirstInstrumentTrack();
                    refreshTrackSelector();
                    readPatternSteps();
                    refreshMixerControls();
                    refreshEnginePlaylist();
                    updateActiveInstrumentEditor();
                    isDirty = true;
                    refreshProjectView("Deleted " + trackName + ". Save to keep the change.");
                    return;
                }
                if (result == 9004)
                {
                    const auto tracks = getGeneratorTracks();
                    if (juce::isPositiveAndBelow(currentSourceIndex, tracks.size()))
                    {
                        trackSelector.setSelectedId(currentSourceIndex + 1, juce::sendNotificationSync);
                        showPlaylist();
                        if (playlistWindow != nullptr) playlistWindow->setAutomationMode(true);
                        status.setText("Song Events: select a parameter in the Piano Roll, then draw automation in the Playlist.",
                                       juce::dontSendNotification);
                    }
                    return;
                }
                if (result >= 500 && result <= 512)
                {
                    const auto colour = result == 500 ? juce::Colour{}
                        : palette[static_cast<size_t>(result - 501)].second;
                    project.setTrackColour(currentSourceIndex, colour);
                    isDirty = true;
                    if (playlistWindow != nullptr) playlistWindow->refresh();
                    status.setText(colour.isTransparent() ? "Track color reset." : "Track color updated. Save to keep the change.",
                                   juce::dontSendNotification);
                    return;
                }
                if (result <= 0 || result > static_cast<int>(destinations.size())) return;
                const auto destinationId = destinations[static_cast<size_t>(result - 1)];
                const auto sourceName = project.getTrackName(currentSourceIndex);
                const auto destinationIndex = [&]
                {
                    for (int i = 0; i < project.getTrackCount(); ++i)
                        if (project.getTrackId(i) == destinationId) return i;
                    return -1;
                }();
                if (destinationIndex < 0) return;

                juce::AlertWindow dialog("Clone Options",
                    "Copy all patterns and Playlist arrangement from " + sourceName + " to "
                        + project.getTrackName(destinationIndex) + "? Existing destination patterns and clips will be replaced.",
                    juce::MessageBoxIconType::NoIcon);
                dialog.addButton("Clone", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
                if (dialog.runModalLoop() != 1) return;

                rememberUndo("Clone patterns and arrangement");
                if (!project.cloneTrackPatternsAndArrangement(sourceTrackId, destinationId))
                {
                    status.setText("Clone canceled. Both generators must be instrument tracks.", juce::dontSendNotification);
                    return;
                }
                rebuildPatternClip();
                refreshEnginePlaylist();
                isDirty = true;
                if (playlistWindow != nullptr) playlistWindow->refresh();
                refreshProjectView("Cloned patterns and Playlist arrangement to "
                    + project.getTrackName(destinationIndex) + ". Save to keep the change.");
            });
    }

    void movePlaylistClip(int trackIndex, int sourceBar, int destinationTrackIndex, int destinationBar)
    {
        if (!hasProject || edit == nullptr || !juce::isPositiveAndBelow(trackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(destinationTrackIndex, project.getTrackCount())
            || !juce::isPositiveAndBelow(sourceBar, PlaylistGrid::barCount)
            || !juce::isPositiveAndBelow(destinationBar, PlaylistGrid::barCount)
            || (sourceBar == destinationBar && trackIndex == destinationTrackIndex))
            return;

        if (project.getTrackKind(trackIndex) == ProjectDocument::TrackKind::audio)
        {
            if (project.getTrackKind(destinationTrackIndex) != ProjectDocument::TrackKind::audio)
            {
                status.setText("Audio clips can only be moved to another audio track.", juce::dontSendNotification);
                return;
            }
            int clipStartBar = -1;
            int clipLengthBars = 1;
            auto* clip = findAudioClipAtBar(trackIndex, sourceBar, &clipStartBar, &clipLengthBars);
            const auto tracks = getGeneratorTracks();
            if (clip == nullptr || !juce::isPositiveAndBelow(trackIndex, tracks.size())
                || !juce::isPositiveAndBelow(destinationTrackIndex, tracks.size())) return;
            const int newStartBar = destinationBar - (sourceBar - clipStartBar);
            if (newStartBar < 0 || newStartBar + clipLengthBars > PlaylistGrid::barCount)
            {
                status.setText("Move canceled. The audio clip would extend outside the Playlist grid.", juce::dontSendNotification);
                return;
            }
            auto* destinationTrack = tracks.getUnchecked(destinationTrackIndex);
            if (destinationTrackIndex != trackIndex)
            {
                const auto secondsPerBar = 60.0 / juce::jmax(20.0, project.getInitialTempoBpm()) * 4.0;
                const auto start = newStartBar * secondsPerBar;
                const auto end = start + clip->getEditTimeRange().getLength().inSeconds();
                for (int i = 0; i < destinationTrack->getNumTrackItems(); ++i)
                    if (auto* other = dynamic_cast<te::WaveAudioClip*>(destinationTrack->getTrackItem(i));
                        other != nullptr)
                    {
                        const auto range = other->getEditTimeRange();
                        if (range.getStart().inSeconds() < end && range.getEnd().inSeconds() > start)
                        {
                            status.setText("Move canceled. The destination audio lane already has a clip there.",
                                           juce::dontSendNotification);
                            return;
                        }
                    }
            }
            rememberUndo("Move audio clip");
            if (destinationTrackIndex != trackIndex && !clip->moveTo(*destinationTrack))
            {
                status.setText("Move canceled. Tracktion Engine could not move the audio clip.", juce::dontSendNotification);
                return;
            }
            const auto secondsPerBar = 60.0 / juce::jmax(20.0, project.getInitialTempoBpm()) * 4.0;
            clip->setStart(tracktion::TimePosition::fromSeconds(newStartBar * secondsPerBar), true, true);
            edit->restartPlayback();
            isDirty = true;
            refreshProjectView("Moved audio clip to Playlist bar " + juce::String(newStartBar + 1) + ".");
            return;
        }

        const auto trackId = project.getTrackId(trackIndex);
        const auto patternId = project.getPlaylistPatternId(trackId, sourceBar);
        if (patternId.isEmpty()) return;

        auto destinationPatternId = patternId;
        const int sourcePatternIndex = project.getPatternIndexForId(trackId, patternId);
        if (sourcePatternIndex < 0) return;
        const auto destinationTrackId = project.getTrackId(destinationTrackIndex);
        int destinationPatternIndex = sourcePatternIndex;
        if (destinationTrackIndex != trackIndex)
        {
            const auto occupied = project.getPlaylistPatternId(destinationTrackId, destinationBar);
            if (occupied.isNotEmpty())
            {
                status.setText("Move canceled. The destination Playlist cell already contains a pattern.",
                               juce::dontSendNotification);
                return;
            }
            destinationPatternIndex = -1;
            for (int candidate = 0; candidate < ProjectDocument::patternSlotCount; ++candidate)
                if (!project.isPatternCreated(destinationTrackId, candidate))
                {
                    destinationPatternIndex = candidate;
                    break;
                }
            if (destinationPatternIndex < 0)
            {
                status.setText("Move canceled. The destination generator has no empty pattern slot.",
                               juce::dontSendNotification);
                return;
            }
        }
        else if (project.getPlaylistPatternId(destinationTrackId, destinationBar).isNotEmpty())
        {
            status.setText("Move canceled. The destination Playlist cell already contains a pattern.",
                           juce::dontSendNotification);
            return;
        }

        rememberUndo("Move playlist pattern");
        if (destinationTrackIndex != trackIndex)
        {
            project.setPatternCreated(destinationTrackId, destinationPatternIndex, true);
            project.setPatternLengthSteps(destinationTrackId, destinationPatternIndex,
                project.getPatternLengthSteps(trackId, sourcePatternIndex));
            for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                for (int step = 0; step < 64; ++step)
                {
                    const bool on = project.getPatternNote(trackId, sourcePatternIndex, pitch, step);
                    project.setPatternNote(destinationTrackId, destinationPatternIndex, pitch, step, on);
                    if (on)
                    {
                        project.setPatternNoteLength(destinationTrackId, destinationPatternIndex, pitch, step,
                            project.getPatternNoteLength(trackId, sourcePatternIndex, pitch, step));
                        project.setPatternNoteVelocity(destinationTrackId, destinationPatternIndex, pitch, step,
                            project.getPatternNoteVelocity(trackId, sourcePatternIndex, pitch, step));
                    }
                }
            destinationPatternId = project.getPatternId(destinationTrackId, destinationPatternIndex);
        }
        project.setPlaylistClip(trackId, {}, sourceBar, 1, false);
        project.setPlaylistClip(destinationTrackId, destinationPatternId, destinationBar, 1, true);
        refreshEnginePlaylistClip(trackIndex, sourceBar);
        refreshEnginePlaylistClip(destinationTrackIndex, destinationBar);
        isDirty = true;
        const auto destinationPattern = project.getPatternIndexForId(destinationTrackId, destinationPatternId);
        const juce::String name = destinationPattern >= 0 ? project.getPatternName(destinationTrackId, destinationPattern) : juce::String("pattern");
        refreshProjectView("Moved " + name + " to generator " + juce::String(destinationTrackIndex + 1)
            + ", Playlist bar " + juce::String(destinationBar + 1) + ".");
    }

    void finishPlaylistPaintGesture()
    {
        if (playlistPaintGestureActive)
        {
            playlistPaintGestureActive = false;
            refreshProjectView("Playlist pattern placement updated. Save to keep the change.");
        }
        else if (playlistWindow != nullptr)
            playlistWindow->refresh();
    }

    void setPlaybackMode(PlaybackMode mode)
    {
        if (playbackMode == mode) return;
        if (edit != nullptr && edit->getTransport().isPlaying())
        {
            if (playbackMode == PlaybackMode::song)
                rememberSongPatternsAt(edit->getTransport().getPosition().inSeconds());
            edit->getTransport().stop(false, false);
        }
        if (auditionPatternPlayback)
        {
            auditionPatternPlayback = false;
            refreshEnginePlaylist();
        }
        playbackMode = mode;
        updatePlaybackModeButtons();
        updateEngineAutomation();
        playButton.setButtonText("Play");
        status.setText(mode == PlaybackMode::pattern
            ? "Pattern mode: Play loops each generator's remembered pattern."
            : "Song mode: Play runs clips arranged in the Playlist.", juce::dontSendNotification);
    }

    void updatePlaybackModeButtons()
    {
        patternModeButton.setToggleState(playbackMode == PlaybackMode::pattern, juce::dontSendNotification);
        songModeButton.setToggleState(playbackMode == PlaybackMode::song, juce::dontSendNotification);
        patternModeButton.repaint();
        songModeButton.repaint();
    }

    bool patternHasNotes(const juce::String& trackId, int patternIndex) const
    {
        if (trackId.isEmpty() || !project.isPatternCreated(trackId, patternIndex)) return false;
        const auto patternLength = project.getPatternLengthSteps(trackId, patternIndex);
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < patternLength; ++step)
                if (project.getPatternNote(trackId, patternIndex, pitch, step)) return true;
        return false;
    }

    void rememberSongPatternsAt(double seconds)
    {
        if (seconds < 0.0) return;
        const auto bar = juce::jlimit(0, PlaylistGrid::barCount - 1,
            static_cast<int>(seconds * project.getInitialTempoBpm() / 60.0 / 4.0));
        const auto generatorTracks = getGeneratorTracks();
        const auto activeGeneratorIndex = generatorTracks.indexOf(instrumentTrack);
        for (int trackIndex = 0; trackIndex < project.getTrackCount(); ++trackIndex)
        {
            const auto trackId = project.getTrackId(trackIndex);
            const auto patternId = project.getPlaylistPatternId(trackId, bar);
            const auto patternIndex = project.getPatternIndexForId(trackId, patternId);
            if (patternIndex < 0 || !project.isPatternCreated(trackId, patternIndex)) continue;
            if (project.getTrackSelectedPatternIndex(trackId) != patternIndex)
            {
                project.setTrackSelectedPatternIndex(trackId, patternIndex);
            }
            if (trackIndex == activeGeneratorIndex)
            {
                selectedBankIndex = patternIndex / ProjectDocument::patternsPerBank;
                selectedSlotIndex = patternIndex % ProjectDocument::patternsPerBank;
                for (int i = 0; i < 8; ++i)
                {
                    patternBankButtons[static_cast<size_t>(i)]->setToggleState(i == selectedBankIndex, juce::dontSendNotification);
                    patternSlotButtons[static_cast<size_t>(i)]->setToggleState(i == selectedSlotIndex, juce::dontSendNotification);
                }
                readPatternSteps();
            }
        }
    }

    void togglePlayback()
    {
        if (edit == nullptr) return;
        auto* activeDevice = engine.getDeviceManager().deviceManager.getCurrentAudioDevice();
        if (activeDevice == nullptr)
        {
            status.setText("No active audio output. Choose a device in Audio Settings.", juce::dontSendNotification);
            return;
        }
        auto& transport = edit->getTransport();
        if (transport.isPlaying())
        {
            if (playbackMode == PlaybackMode::song)
                rememberSongPatternsAt(transport.getPosition().inSeconds());
            transport.stop(false, false);
            if (auditionPatternPlayback)
            {
                auditionPatternPlayback = false;
                refreshEnginePlaylist();
            }
            playButton.setButtonText("Play");
            return;
        }
        auditionPatternPlayback = playbackMode == PlaybackMode::pattern;
        if (auditionPatternPlayback)
        {
            bool anySelectedPatternHasNotes = false;
            const auto trackCount = juce::jmin(project.getTrackCount(), getGeneratorTracks().size());
            for (int index = 0; index < trackCount && !anySelectedPatternHasNotes; ++index)
            {
                const auto trackId = project.getTrackId(index);
                anySelectedPatternHasNotes = patternHasNotes(trackId, project.getTrackSelectedPatternIndex(trackId));
            }
            if (!anySelectedPatternHasNotes)
            {
                auditionPatternPlayback = false;
                status.setText("Add notes to a generator's selected pattern before Pattern mode playback.",
                               juce::dontSendNotification);
                return;
            }
            refreshEnginePlaylist();
        }
        else
        {
            bool hasPlaylistContent = false;
            for (int trackIndex = 0; trackIndex < project.getTrackCount() && !hasPlaylistContent; ++trackIndex)
                for (int bar = 0; bar < PlaylistGrid::barCount; ++bar)
                    hasPlaylistContent |= project.getPlaylistPatternId(project.getTrackId(trackIndex), bar).isNotEmpty();
            if (!hasPlaylistContent)
                for (auto* track : getGeneratorTracks())
                    for (int item = 0; item < track->getNumTrackItems() && !hasPlaylistContent; ++item)
                        hasPlaylistContent = dynamic_cast<te::WaveAudioClip*>(track->getTrackItem(item)) != nullptr;
            if (!hasPlaylistContent)
            {
                status.setText("Song mode: place pattern clips or import audio into the Playlist before playback.",
                               juce::dontSendNotification);
                return;
            }
        }
        if (auditionPatternPlayback)
        {
            double longestPatternBeats = 1.0;
            const auto trackCount = juce::jmin(project.getTrackCount(), getGeneratorTracks().size());
            for (int index = 0; index < trackCount; ++index)
            {
                const auto trackId = project.getTrackId(index);
                const auto patternIndex = project.getTrackSelectedPatternIndex(trackId);
                if (patternHasNotes(trackId, patternIndex))
                    longestPatternBeats = juce::jmax(longestPatternBeats,
                        project.getPatternLengthSteps(trackId, patternIndex) * 0.25);
            }
            const auto endSeconds = juce::jmax(0.125, longestPatternBeats * 60.0 / project.getInitialTempoBpm());
            transport.setLoopRange({ tracktion::TimePosition::fromSeconds(0.0),
                                     tracktion::TimePosition::fromSeconds(endSeconds) });
            transport.looping = true;
        }
        else
        {
            auditionPatternPlayback = false;
            updateLoopRange();
            transport.looping = loopButton.getToggleState();
        }
        transport.playFromStart(false);
        playButton.setButtonText("Stop");
        status.setText(auditionPatternPlayback
            ? "Pattern mode: playing each generator's remembered pattern through " + activeDevice->getName() + ". Press Stop to finish."
            : "Playing the Playlist through " + activeDevice->getName(), juce::dontSendNotification);
    }

    void showAudioSettings()
    {
        if (audioSettingsWindow == nullptr)
            audioSettingsWindow = std::make_unique<AudioSettingsWindow>(engine.getDeviceManager().deviceManager);
        audioSettingsWindow->setVisible(true);
        audioSettingsWindow->toFront(true);
    }

    void showAudioDiagnostics()
    {
        auto* device = engine.getDeviceManager().deviceManager.getCurrentAudioDevice();
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        const auto trackIndex = tracks.indexOf(instrumentTrack);
        const auto trackId = selectedProjectTrackId();
        int patternNotes = 0;
        if (trackId.isNotEmpty())
            for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
                for (int step = 0; step < project.getPatternLengthSteps(trackId, selectedPatternIndex()); ++step)
                    patternNotes += project.getPatternNote(trackId, selectedPatternIndex(), pitch, step) ? 1 : 0;

        juce::String details;
        details << "Audio device: " << (device != nullptr ? device->getName() : "No active output") << "\n";
        if (device != nullptr)
            details << "Output channels: " << device->getActiveOutputChannels().countNumberOfSetBits()
                    << "   Sample rate: " << juce::String(device->getCurrentSampleRate(), 0) << " Hz\n";
        details << "Transport: " << (edit != nullptr && edit->getTransport().isPlaying() ? "Playing" : "Stopped")
                << "   Position: " << (edit != nullptr
                    ? juce::String(edit->getTransport().getPosition().inSeconds(), 2) : juce::String("n/a")) << " s\n";
        details << "Mode: " << (playbackMode == PlaybackMode::pattern ? "Pattern" : "Song")
                << "   Selected generator: " << (instrumentTrack != nullptr ? instrumentTrack->getName() : "None") << "\n";
        details << "Generator mapping: engine " << (trackIndex >= 0 ? juce::String(trackIndex + 1) : "unmapped")
                << " of " << tracks.size() << "   Orbit project tracks: " << project.getTrackCount() << "\n";
        details << "Selected pattern: " << getSelectedPatternTitle() << "   Notes: " << patternNotes << "\n";
        details << "Track meter: " << (trackMeter != nullptr
            ? (displayedMeterPeakDb > -90.0f ? juce::String(displayedMeterPeakDb, 1) + " dBFS" : "no recent signal")
            : "not attached") << "\n";
        details << "Piano-key note-ons this session: " << midiAuditionNoteOns
                << "   Last key event: " << (lastMidiActivity.isNotEmpty() ? lastMidiActivity : "none");

        juce::AlertWindow dialog("Audio / MIDI Diagnostics", details,
                                 juce::MessageBoxIconType::NoIcon, this);
        dialog.addButton("Play Test Note", 1, juce::KeyPress());
        dialog.addButton("Close", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        dialog.centreAroundComponent(this, 560, 290);
        if (dialog.runModalLoop() == 1)
        {
            if (instrumentTrack == nullptr)
            {
                status.setText("Test note unavailable: select an instrument generator first.", juce::dontSendNotification);
                return;
            }
            ++midiAuditionNoteOns;
            lastMidiActivity = "Diagnostics: C4 test note";
            instrumentTrack->playGuideNote(60, te::MidiChannel(1), 100, true, true, true);
            status.setText("Sent a C4 test note to " + instrumentTrack->getName()
                           + ". Check the track meter and output device.", juce::dontSendNotification);
        }
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
        allNotesSelected = false;
        for (auto& pitchSteps : noteGrid) pitchSteps.fill(false);
        const auto trackId = selectedProjectTrackId();
        hasPattern = project.isPatternCreated(trackId, selectedPatternIndex());
        for (int pitch = minimumPitch; pitch <= maximumPitch; ++pitch)
            for (int step = 0; step < 64; ++step)
                noteGrid[static_cast<size_t>(pitch - minimumPitch)][static_cast<size_t>(step)] =
                    project.getPatternNote(trackId, selectedPatternIndex(), pitch, step);
        pitchSelector.setEnabled(false);
        pitchSelector.setVisible(false);
        pianoRollButton.setEnabled(instrumentTrack != nullptr);
        pianoRollButton.setVisible(instrumentTrack != nullptr);
        generatorDragHandle.setVisible(instrumentTrack != nullptr);
        generatorResizeHandle.setVisible(instrumentTrack != nullptr);
        generatorCollapseButton.setVisible(instrumentTrack != nullptr);
        patternTitle.setVisible(instrumentTrack != nullptr);
        for (auto& step : steps)
        {
            step->setEnabled(false);
            step->setVisible(false);
        }
        for (auto& button : patternBankButtons)
        {
            button->setEnabled(instrumentTrack != nullptr);
            button->setVisible(instrumentTrack != nullptr);
        }
        generatorMuteButton.setVisible(instrumentTrack != nullptr);
        generatorSoloButton.setVisible(instrumentTrack != nullptr);
        generatorMuteButton.setEnabled(instrumentTrack != nullptr);
        generatorSoloButton.setEnabled(instrumentTrack != nullptr);
        generatorSettingsButton.setVisible(instrumentTrack != nullptr);
        generatorSettingsButton.setEnabled(instrumentTrack != nullptr);
        openPresetButton.setVisible(instrumentTrack != nullptr);
        savePresetButton.setVisible(instrumentTrack != nullptr);
        openPresetButton.setEnabled(instrumentTrack != nullptr);
        savePresetButton.setEnabled(instrumentTrack != nullptr);
        patternPresetSelector.setVisible(instrumentTrack != nullptr);
        patternPresetSelector.setEnabled(instrumentTrack != nullptr);
        patternLengthDisplay.setVisible(instrumentTrack != nullptr);
        patternLengthDownButton.setVisible(instrumentTrack != nullptr);
        patternLengthUpButton.setVisible(instrumentTrack != nullptr);
        patternLengthDownButton.setEnabled(instrumentTrack != nullptr);
        patternLengthUpButton.setEnabled(instrumentTrack != nullptr);
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

    juce::String automationPluginName, automationParameterId, automationParameterName;

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
        const auto trackId = selectedProjectTrackId();
        const auto patternIndex = selectedPatternIndex();
        if (project.getTrackSelectedPatternIndex(trackId) != patternIndex)
        {
            project.setTrackSelectedPatternIndex(trackId, patternIndex);
        }
        readPatternSteps();
        if (drumRackWindow != nullptr && drumRackTrackId == trackId)
            for (int pad = 0; pad < 16; ++pad)
                drumRackWindow->setSteps(pad, getDrumRackSteps(trackId, patternIndex, pad));
        if (auditionPatternPlayback && edit->getTransport().isPlaying())
            refreshEnginePlaylist();
        status.setText(hasPattern ? "Selected pattern. Click Playlist to place it, or edit its notes in Piano Roll."
                                  : "Selected empty pattern slot. Click Playlist to place it or add notes in Piano Roll.",
                       juce::dontSendNotification);
    }

    void resetPatternButtons()
    {
        hasPattern = false;
        if (activeInstrumentEditor != nullptr)
        {
            removeChildComponent(activeInstrumentEditor.get());
            activeInstrumentEditor.reset();
        }
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
        generatorCollapseButton.setVisible(false);
        generatorCollapsed = false;
        allGeneratorsMinimized = false;
        pianoRollMaximized = false;
        pianoRollRestoreBounds = {};
        pianoRollWasVisibleWhenCollapsed = false;
        expandedGeneratorBounds = {};
        if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
        generatorMuteButton.setEnabled(false);
        generatorMuteButton.setToggleState(false, juce::dontSendNotification);
        generatorMuteButton.setVisible(false);
        generatorSoloButton.setEnabled(false);
        generatorSoloButton.setToggleState(false, juce::dontSendNotification);
        generatorSoloButton.setVisible(false);
        generatorSettingsButton.setEnabled(false);
        generatorSettingsButton.setVisible(false);
        openPresetButton.setEnabled(false);
        openPresetButton.setVisible(false);
        savePresetButton.setEnabled(false);
        savePresetButton.setVisible(false);
        patternPresetSelector.setEnabled(false);
        patternPresetSelector.setVisible(false);
        patternLengthDisplay.setVisible(false);
        patternLengthDownButton.setVisible(false);
        patternLengthUpButton.setVisible(false);
        generatorDragHandle.setVisible(false);
        generatorResizeHandle.setVisible(false);
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
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        const auto selectedIndex = tracks.indexOf(instrumentTrack);
        const bool trackAvailable = juce::isPositiveAndBelow(selectedIndex, tracks.size());
        generatorMuteButton.setToggleState(trackAvailable && tracks.getUnchecked(selectedIndex)->isMuted(false), juce::dontSendNotification);
        generatorSoloButton.setToggleState(trackAvailable && tracks.getUnchecked(selectedIndex)->isSolo(false), juce::dontSendNotification);
        generatorMuteButton.setEnabled(trackAvailable);
        generatorSoloButton.setEnabled(trackAvailable);
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
        trackSelector.setVisible(false);
        for (auto& button : patternBankButtons) button->setEnabled(instrumentTrack != nullptr);
        for (auto& button : patternSlotButtons) button->setEnabled(instrumentTrack != nullptr);
        refreshingTrackSelector = false;
        refreshGeneratorDock();
        if (playlistWindow != nullptr) playlistWindow->refresh();
    }

    void refreshGeneratorDock()
    {
        for (auto& button : generatorDockButtons)
            if (button != nullptr) generatorDockStrip.removeChildComponent(button.get());
        generatorDockButtons.clear();
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        for (int index = 0; index < tracks.size(); ++index)
        {
            auto button = std::make_unique<juce::TextButton>(tracks.getUnchecked(index)->getName());
            button->setTooltip("Select and restore " + tracks.getUnchecked(index)->getName());
            button->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(36, 53, 64));
            button->setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(46, 112, 139));
            button->setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(206, 226, 236));
            button->setClickingTogglesState(false);
            button->setToggleState(index == tracks.indexOf(instrumentTrack), juce::dontSendNotification);
            button->onClick = [this, index]
            {
                if (trackSelector.getSelectedId() != index + 1)
                    trackSelector.setSelectedId(index + 1, juce::dontSendNotification);
                selectInstrumentTrack();
                allGeneratorsMinimized = false;
                generatorCollapsed = false;
                pianoRollMaximized = false;
                if (pianoRollWindow != nullptr) pianoRollWindow->setVisible(false);
                if (!pianoRollRestoreBounds.isEmpty()) generatorPanelBounds = pianoRollRestoreBounds;
                pianoRollButton.setButtonText("PIANO ROLL");
                if (generatorPanelBounds.isEmpty())
                    generatorPanelBounds = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(50).reduced(14, 10)
                        .withSizeKeepingCentre(juce::jmin(1040, getWidth() - 28), juce::jmin(560, getHeight() - 150));
                resized();
                bringGeneratorToFront();
            };
            generatorDockStrip.addAndMakeVisible(*button);
            generatorDockButtons.push_back(std::move(button));
        }
        generatorDockViewport.setVisible(!generatorDockButtons.empty());
        resized();
    }

    void bringGeneratorToFront()
    {
        if (activeInstrumentEditor != nullptr && activeInstrumentEditor->isVisible())
            activeInstrumentEditor->toFront(false);
        if (pianoRollWindow != nullptr && pianoRollWindow->isVisible())
            pianoRollWindow->toFront(false);
        for (auto& button : patternBankButtons) button->toFront(false);
        for (auto& button : patternSlotButtons) button->toFront(false);
        for (auto* control : std::array<juce::Component*, 14> {
                 &pianoRollButton, &generatorMuteButton, &generatorSoloButton, &generatorSettingsButton,
                 &patternPresetSelector, &openPresetButton, &savePresetButton, &patternLengthDownButton,
                 &patternLengthDisplay, &patternLengthUpButton, &trackSelector,
                 &generatorCollapseButton, &generatorDragHandle, &generatorResizeHandle })
            control->toFront(false);
    }

    void updateGeneratorDockSelection()
    {
        const auto tracks = edit != nullptr ? getGeneratorTracks() : juce::Array<te::AudioTrack*>{};
        const auto selected = tracks.indexOf(instrumentTrack);
        for (int i = 0; i < static_cast<int>(generatorDockButtons.size()); ++i)
            generatorDockButtons[static_cast<size_t>(i)]->setToggleState(i == selected, juce::dontSendNotification);
    }

    void selectInstrumentTrack()
    {
        if (refreshingTrackSelector || edit == nullptr) return;
        const auto tracks = getGeneratorTracks();
        const auto index = trackSelector.getSelectedId() - 1;
        if (index < 0 || index >= tracks.size()) return;
        if (instrumentTrack != nullptr)
        {
            const auto previousIndex = tracks.indexOf(instrumentTrack);
            if (juce::isPositiveAndBelow(previousIndex, project.getTrackCount()))
            {
                const auto previousTrackId = project.getTrackId(previousIndex);
                if (project.getTrackSelectedPatternIndex(previousTrackId) != selectedPatternIndex())
                {
                    project.setTrackSelectedPatternIndex(previousTrackId, selectedPatternIndex());
                }
            }
        }
        instrumentTrack = tracks.getUnchecked(index);
        const auto rememberedPattern = project.getTrackSelectedPatternIndex(project.getTrackId(index));
        selectedBankIndex = rememberedPattern / ProjectDocument::patternsPerBank;
        selectedSlotIndex = rememberedPattern % ProjectDocument::patternsPerBank;
        for (int buttonIndex = 0; buttonIndex < 8; ++buttonIndex)
        {
            patternBankButtons[static_cast<size_t>(buttonIndex)]->setToggleState(buttonIndex == selectedBankIndex, juce::dontSendNotification);
            patternSlotButtons[static_cast<size_t>(buttonIndex)]->setToggleState(buttonIndex == selectedSlotIndex, juce::dontSendNotification);
        }
        allGeneratorsMinimized = false;
        if (generatorCollapsed)
        {
            generatorCollapsed = false;
            if (!expandedGeneratorBounds.isEmpty()) generatorPanelBounds = expandedGeneratorBounds;
            generatorCollapseButton.setKind(PlaylistWindowIconButton::Kind::minimize);
        }
        if (generatorPanelBounds.isEmpty())
            generatorPanelBounds = getLocalBounds().withTrimmedTop(82).withTrimmedBottom(50).reduced(14, 10)
                .withSizeKeepingCentre(juce::jmin(1040, getWidth() - 28), juce::jmin(560, getHeight() - 150));
        patternClip = nullptr;
        for (int i = 0; i < instrumentTrack->getNumTrackItems(); ++i)
            if (auto* clip = dynamic_cast<te::MidiClip*>(instrumentTrack->getTrackItem(i)))
            {
                patternClip = clip;
                break;
            }
        updateActiveInstrumentEditor();
        readPatternSteps();
        refreshPianoRollAutomationTargets();
        trackSelector.setVisible(false);
        refreshMixerControls();
        if (playlistWindow != nullptr) playlistWindow->refresh();
        updateGeneratorDockSelection();
        bringGeneratorToFront();
        status.setText("Selected " + instrumentTrack->getName() + ". Pattern bank A-H x 1-8 is ready.", juce::dontSendNotification);
    }

    void updateActiveInstrumentEditor()
    {
        if (activeInstrumentEditor != nullptr)
        {
            removeChildComponent(activeInstrumentEditor.get());
            activeInstrumentEditor.reset();
        }
        if (instrumentTrack == nullptr) return;

        for (auto* plugin : instrumentTrack->pluginList)
        {
            if (auto* external = dynamic_cast<te::ExternalPlugin*>(plugin))
            {
                activeInstrumentEditor = std::make_unique<OrbitPluginEditorContent>(*external);
                addAndMakeVisible(*activeInstrumentEditor);
                activeInstrumentEditor->addMouseListener(this, true);
                resized();
                if (pianoRollWindow != nullptr && pianoRollWindow->isVisible())
                    pianoRollWindow->toFront(false);
                return;
            }
        }
        resized();
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
            displayedMeterPeakDb = -100.0f;
            meterStatus.setText("Track output: no instrument meter", juce::dontSendNotification);
            return;
        }

        const auto leftDb = trackMeterClient.getAndClearAudioLevel(0).dB;
        const auto rightDb = trackMeterClient.getAndClearAudioLevel(1).dB;
        const auto peakDb = juce::jmax(leftDb, rightDb);
        auto* settings = applicationProperties.getUserSettings();
        const auto decay = settings != nullptr ? juce::jlimit(1, 100, settings->getIntValue("vuDecay", 75)) : 75;
        const auto updateRate = settings != nullptr ? juce::jlimit(2, 60, settings->getIntValue("updateRateHz", 30)) : 30;
        displayedMeterPeakDb = juce::jmax(peakDb, displayedMeterPeakDb - 60.0f * (decay / 100.0f) / updateRate);
        if (displayedMeterPeakDb > -90.0f)
            meterStatus.setText("Track output: audio signal detected · peak " + juce::String(displayedMeterPeakDb, 1) + " dBFS",
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
        masterVolumeKnob.setEnabled(ready);
        tempoLabel.setEnabled(ready);
        tempoSlider.setEnabled(ready);
        stopButton.setEnabled(ready);
        loopButton.setEnabled(ready);
        loopStartSlider.setEnabled(ready);
        loopLengthSlider.setEnabled(ready);
        metronomeButton.setEnabled(ready);
        if (ready)
        {
            tempoSlider.setValue(project.getInitialTempoBpm(), juce::dontSendNotification);
            masterVolumeKnob.setValue(getMasterGainDb(), juce::dontSendNotification);
            loopStartSlider.setValue(project.getLoopStartBar(), juce::dontSendNotification);
            loopLengthSlider.setValue(project.getLoopLengthBars(), juce::dontSendNotification);
            loopButton.setToggleState(project.isLoopEnabled(), juce::dontSendNotification);
            metronomeButton.setToggleState(edit->clickTrackEnabled.get(), juce::dontSendNotification);
            const auto meter = project.getInitialTimeSignature();
            timeSignatureLabel.setText(juce::String(meter.numerator) + "/" + juce::String(meter.denominator),
                                       juce::dontSendNotification);
            updateLoopRange();
            if (mixerWindow != nullptr)
            {
                const auto tracks = getGeneratorTracks();
                mixerWindow->refresh(tracks.indexOf(instrumentTrack));
                if (mixerWindow->isVisible() && !mixerWindow->isMaximized() && !mixerWindow->isMinimized())
                    fitMixerWidthToStrips();
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
    juce::ApplicationProperties applicationProperties;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    juce::uint32 lastAutoSaveMs = 0;
    float displayedMeterPeakDb = -100.0f;
    int midiAuditionNoteOns = 0;
    juce::String lastMidiActivity;
    std::unique_ptr<te::Edit> edit;
    te::AudioTrack* instrumentTrack = nullptr;
    te::AudioTrack* auditionTrack = nullptr;
    te::MidiClip* patternClip = nullptr;
    te::LevelMeterPlugin* trackMeter = nullptr;
    te::LevelMeasurer::Client trackMeterClient;
    ProjectDocument project;
    mutable std::map<juce::String, std::vector<juce::Point<int>>> playlistPreviewCache;
    bool hasProject = false;
    bool isDirty = false;
    bool pianoRollGestureActive = false;
    bool pianoRollGestureChanged = false;
    bool auditionPatternPlayback = false;
    bool hasPattern = false;
    bool refreshingTrackSelector = false;
    bool allNotesSelected = false;
    bool generatorCollapsed = false;
    bool allGeneratorsMinimized = false;
    bool pianoRollWasVisibleWhenCollapsed = false;
    bool pianoRollMaximized = false;
    juce::Rectangle<int> pianoRollRestoreBounds;
    juce::Rectangle<int> expandedGeneratorBounds;
    static constexpr int minimumPitch = 48; // C3
    static constexpr int maximumPitch = 84; // C6
    std::array<std::array<bool, 64>, maximumPitch - minimumPitch + 1> noteGrid{};
    std::array<std::unique_ptr<juce::TextButton>, 16> steps;
    std::array<std::unique_ptr<juce::TextButton>, 8> patternBankButtons, patternSlotButtons;
    std::vector<UndoEntry> undoHistory, redoHistory;
    PatternClipboard patternClipboard;
    int randomDensityPercent = 25;
    int randomPitchLow = 48;
    int randomPitchHigh = 84;
    int selectedBankIndex = 0;
    int selectedSlotIndex = 0;
    PlaybackMode playbackMode = PlaybackMode::pattern;
    juce::ComboBox pitchSelector;
    PianoRollButton pianoRollButton;
    juce::ComboBox patternPresetSelector;
    juce::Label patternLengthDisplay;
    juce::TextButton patternLengthDownButton, patternLengthUpButton;
    juce::ComboBox trackSelector;
    juce::Slider levelSlider, panSlider;
    juce::Slider tempoSlider;
    juce::Slider loopStartSlider, loopLengthSlider;
    juce::Slider masterVolumeKnob;
    OrbitAuditionKeyboard auditionKeyboard;
    juce::Label title, status, projectDetails, musicalDetails, audioDetails, patternTitle,
                arrangementTitle, mixerTitle, levelLabel, panLabel, meterStatus, buildLabel, tempoLabel,
                loopStartLabel, loopLengthLabel, timeSignatureLabel, barLabel, barReadoutLabel;
    GeneratorDragLabel generatorDragHandle;
    GeneratorResizeHandle generatorResizeHandle;
    MixerMoveHandle mixerMoveHandle;
    GeneratorResizeHandle mixerResizeHandle { "Resize mixer" };
    PlaylistWindowIconButton generatorCollapseButton { PlaylistWindowIconButton::Kind::minimize };
    juce::Viewport generatorDockViewport;
    juce::Component generatorDockStrip;
    std::vector<std::unique_ptr<juce::TextButton>> generatorDockButtons;
    ModeSelectorButton patternModeButton, songModeButton;
    juce::TextButton newProjectButton, openButton, saveButton, midiExportButton, audioSettingsButton, addTrackButton,
                     patternButton, fileMenuButton, editMenuButton, viewMenuButton, insertMenuButton, optionsMenuButton,
                     windowMenuButton, helpMenuButton, loopButton, metronomeButton;
    ToolbarButton playlistButton{ToolbarButton::Symbol::playlist}, playButton{ToolbarButton::Symbol::play},
                  stopButton{ToolbarButton::Symbol::stop}, mixerButton{ToolbarButton::Symbol::mixer},
                  masterButton{ToolbarButton::Symbol::master}, recordButton{ToolbarButton::Symbol::record},
                  rewindButton{ToolbarButton::Symbol::rewind}, previousButton{ToolbarButton::Symbol::previous},
                  nextButton{ToolbarButton::Symbol::next}, generatorMuteButton, generatorSoloButton,
                  generatorSettingsButton{ToolbarButton::Symbol::settings}, openPresetButton{ToolbarButton::Symbol::open},
                  savePresetButton{ToolbarButton::Symbol::save};
    OutputMeterDisplay outputMeter;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<AudioSettingsWindow> audioSettingsWindow;
    std::unique_ptr<SearchPathsWindow> samplePathsWindow, vstPathsWindow;
    std::unique_ptr<Vst3InstrumentBrowser> vst3InstrumentBrowser;
    juce::Array<juce::PluginDescription> vst3MenuDescriptions;
    std::unique_ptr<PlaylistWindow> playlistWindow;
    std::unique_ptr<MixerWindow> mixerWindow;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<DrumRackWindow> drumRackWindow;
    juce::String drumRackTrackId;
    std::unique_ptr<te::Plugin::EditorComponent> activeInstrumentEditor;
    std::unique_ptr<EqualiserWindow> equaliserWindow;
    juce::Rectangle<int> generatorPanelBounds, generatorInstrumentBounds, generatorResizeStart;
    juce::Rectangle<int> pianoRollBounds;
    juce::Point<int> generatorDragOffset, generatorDragStart, generatorDragStartPosition;
    juce::Rectangle<int> playlistPanelBounds, playlistRestoreBounds, playlistResizeStartBounds;
    juce::Rectangle<int> mixerPanelBounds, mixerRestoreBounds, mixerResizeStartBounds;
    juce::Point<int> mixerDragOffset, mixerResizeStart;
    bool draggingGenerator = false, resizingGenerator = false, playlistPaintGestureActive = false;
    bool playlistBoundsInitialised = false, mixerBoundsInitialised = false;
    bool mixerMaximized = false, draggingMixer = false, resizingMixer = false;
    bool showHints = true;
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
        centreWithSize(1220, 760);
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
    const juce::String getApplicationVersion() override { return "0.3.110"; }
    void initialise(const juce::String&) override
    {
        installOrbitCrashReportHandler();
        mainWindow = std::make_unique<MainWindow>(*this);
    }
    void shutdown() override { mainWindow.reset(); }
private:
    std::unique_ptr<MainWindow> mainWindow;
};
}

START_JUCE_APPLICATION(OrbitApplication)
