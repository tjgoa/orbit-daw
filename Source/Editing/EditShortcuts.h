#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace orbit::editing
{
// JUCE's macOS peer intentionally sets textCharacter=0 for Command events.
// Ctrl events may carry control characters. Letter key codes remain available.
inline juce::juce_wchar shortcutLetter(const juce::KeyPress& key)
{
    const auto code=key.getKeyCode();
    if((code>='A' && code<='Z') || (code>='a' && code<='z'))
        return juce::CharacterFunctions::toLowerCase(juce::juce_wchar(code));
    return juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
}
}
