#pragma once
#include <JuceHeader.h>
#include <deque>

namespace orbit::options
{
// Device callbacks never touch the document, plug-ins or UI. A bounded queue
// delivers events promptly on the JUCE message thread, without timer polling.
class MidiInbox final : public juce::MidiInputCallback, private juce::AsyncUpdater
{
public:
    struct Event { juce::String device; juce::MidiMessage message; double receivedAtMs=0; };
    std::function<void(const std::vector<Event>&, bool)> deliver;
    ~MidiInbox() override { cancelPendingUpdate(); }
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        if (message.isActiveSense() || message.isMidiClock() || message.isSysEx()) return;
        {
            const juce::ScopedLock guard(lock);
            if (pending.size() >= 4096) { pending.clear(); overflow = true; }
            pending.push_back({ source != nullptr ? source->getIdentifier() : juce::String{}, message,
                                juce::Time::getMillisecondCounterHiRes() });
        }
        triggerAsyncUpdate();
    }
    void clear()
    {
        cancelPendingUpdate(); const juce::ScopedLock guard(lock); pending.clear(); overflow = false;
    }
    void dispatchNow() { handleUpdateNowIfNeeded(); }
private:
    void handleAsyncUpdate() override
    {
        std::vector<Event> events; bool lost = false;
        {
            const juce::ScopedLock guard(lock);
            events.swap(pending); lost = overflow; overflow = false;
        }
        if (deliver) deliver(events, lost);
    }
    juce::CriticalSection lock;
    std::vector<Event> pending;
    bool overflow = false;
};
}
