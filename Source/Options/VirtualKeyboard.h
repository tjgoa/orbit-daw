#pragma once
#include <JuceHeader.h>

namespace orbit::options
{
class VirtualKeyboard final : public juce::DocumentWindow
{
public:
    VirtualKeyboard(std::function<void(const juce::MidiMessage&)> send, std::function<void()> hidden)
        : DocumentWindow("Virtual MIDI Keyboard", juce::Colour(0xff1b2b37), closeButton),
          content(std::move(send)), onHidden(std::move(hidden))
    {
        setUsingNativeTitleBar(true); setContentNonOwned(&content, false);
        setResizable(true, false); setResizeLimits(580, 180, 1400, 300);
        centreWithSize(760, 210);
    }
    ~VirtualKeyboard() override { content.release(); clearContentComponent(); }
    void closeButtonPressed() override
    { content.release(); setVisible(false); if (onHidden) onHidden(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::F12Key) { closeButtonPressed(); return true; }
        return DocumentWindow::keyPressed(key);
    }
    void release() { content.release(); }
private:
    struct Content final : juce::Component, juce::MidiKeyboardStateListener
    {
        explicit Content(std::function<void(const juce::MidiMessage&)> callback) : send(std::move(callback))
        {
            state.addListener(this);
            addAndMakeVisible(keys); keys.setAvailableRange(0, 127); keys.setLowestVisibleKey(48);
            keys.setKeyPressBaseOctave(5); keys.setVelocity(100.0f / 127, false);
            for (auto* label : { &channelLabel, &velocityLabel, &octaveLabel }) addAndMakeVisible(*label);
            channelLabel.setText("Channel", juce::dontSendNotification);
            velocityLabel.setText("Velocity", juce::dontSendNotification);
            octaveLabel.setText("Octave", juce::dontSendNotification);
            for (int i = 1; i <= 16; ++i) channel.addItem(juce::String(i), i);
            channel.setSelectedId(1); addAndMakeVisible(channel);
            channel.onChange = [this] { release(); keys.setMidiChannel(channel.getSelectedId()); };
            velocity.setRange(1, 127, 1); velocity.setValue(100); velocity.setSliderStyle(juce::Slider::IncDecButtons);
            velocity.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 24); addAndMakeVisible(velocity);
            velocity.onValueChange = [this] { keys.setVelocity(float(velocity.getValue() / 127), false); };
            for (int i = 0; i <= 9; ++i) octave.addItem(juce::String(i - 1), i + 1);
            octave.setSelectedId(6); addAndMakeVisible(octave);
            octave.onChange = [this] { release(); keys.setKeyPressBaseOctave(octave.getSelectedId() - 1); keys.setLowestVisibleKey(juce::jmin(108, (octave.getSelectedId()-1)*12)); };
            addAndMakeVisible(panic); panic.onClick = [this] { release(); };
            hint.setText("Play with the mouse or A W S E D F T G Y H U J K. Click the keys to focus the computer keyboard.", juce::dontSendNotification);
            addAndMakeVisible(hint); setSize(760, 180);
        }
        ~Content() override { state.removeListener(this); }
        void release()
        {
            state.allNotesOff(0);
            if (send) for (int channelNumber = 1; channelNumber <= 16; ++channelNumber)
            {
                send(juce::MidiMessage::controllerEvent(channelNumber, 64, 0));
                send(juce::MidiMessage::allNotesOff(channelNumber));
            }
        }
        void handleNoteOn(juce::MidiKeyboardState*, int channelNumber, int note, float v) override
        { if (send) send(juce::MidiMessage::noteOn(channelNumber, note, v)); }
        void handleNoteOff(juce::MidiKeyboardState*, int channelNumber, int note, float) override
        { if (send) send(juce::MidiMessage::noteOff(channelNumber, note)); }
        void resized() override
        {
            auto area = getLocalBounds().reduced(10); auto top = area.removeFromTop(28);
            channelLabel.setBounds(top.removeFromLeft(58)); channel.setBounds(top.removeFromLeft(66)); top.removeFromLeft(12);
            velocityLabel.setBounds(top.removeFromLeft(58)); velocity.setBounds(top.removeFromLeft(100)); top.removeFromLeft(12);
            octaveLabel.setBounds(top.removeFromLeft(56)); octave.setBounds(top.removeFromLeft(66));
            panic.setBounds(top.removeFromRight(86)); hint.setBounds(area.removeFromBottom(28));
            area.removeFromTop(8); keys.setBounds(area);
        }
        std::function<void(const juce::MidiMessage&)> send;
        juce::MidiKeyboardState state;
        juce::MidiKeyboardComponent keys { state, juce::MidiKeyboardComponent::horizontalKeyboard };
        juce::ComboBox channel, octave; juce::Slider velocity;
        juce::Label channelLabel, velocityLabel, octaveLabel, hint;
        juce::TextButton panic { "All Notes Off" };
    } content;
    std::function<void()> onHidden;
};
}
