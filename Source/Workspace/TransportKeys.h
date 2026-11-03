#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
namespace orbit::workspace
{
// A Mac native plugin may consume keys before JUCE sees them. Route an unmodified
// Space in Orbit's own peer to transport, leaving text input and modal forms alone.
class TransportKeys
{
public:
#if JUCE_MAC
    TransportKeys();
    ~TransportKeys();
    void set(juce::Component*,std::function<void()>);
private:
    struct Impl;std::unique_ptr<Impl> impl;
#else
    void set(juce::Component*,std::function<void()>){ }
#endif
};
}
