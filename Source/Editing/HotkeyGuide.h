#pragma once
#include <JuceHeader.h>

namespace orbit::editing
{
class HotkeyGuide final : public juce::Component
{
public:
    static juce::String reference()
    {
        return "GENERAL (when a text field is not being edited)\n"
            "Ctrl/Cmd-Z: Undo   |   Ctrl/Cmd-Shift-Z or Ctrl/Cmd-Y: Redo\n"
            "Ctrl/Cmd-X / C / V: Cut / Copy / Paste pattern notes\n"
            "Ctrl/Cmd-A: Select all notes\n"
            "Ctrl/Cmd-R: Randomize   |   Ctrl-H: Humanize   |   Ctrl-Q: Quantize\n"
            "F12: Show/hide virtual MIDI keyboard\n\n"
            "PIANO ROLL (click the note grid first)\n"
            "T: Draw / Select tool   |   V: Individual note velocity mode\n"
            "Shift-click: Select/deselect a note\n"
            "Shift-drag empty grid: Rectangle selection without leaving Draw\n"
            "Ctrl/Cmd-drag selected notes: Copy group\n"
            "Alt-drag: Paint a brush stroke of separate notes\n"
            "Velocity lane: Drag one note's bar; gold marks the edited note\n"
            "Right-click note: Delete   |   Drag right edge: Resize\n"
            "Up / Down: Transpose by semitone\n"
            "Page Up / Page Down: Transpose by octave\n"
            "Left / Right: Rotate phrase by one sixteenth\n"
            "S: Split notes   |   X: Reverse time   |   Y: Mirror pitch\n"
            "N: Rename pattern   |   1-8: Select slot in current bank\n"
            "Delete / Backspace: Delete selection   |   Escape: Clear selection\n"
            "Tab or Ctrl/Cmd-W: Return to instrument\n"
            "C: Piano Roll settings   |   E: Automation lane\n"
            "G: Grid snapping   |   M: Ghost notes\n"
            "9 / 0: Coarser / finer grid   |   + / -: Time zoom\n"
            "Mouse wheel: Scroll pitches   |   Shift-wheel: Scroll time\n"
            "Numpad 8 / 2: Scroll pitches by octave\n"
            "Numpad 4 / 6: Move edit cursor by one sixteenth\n"
            "No selection: phrase transformations use the whole pattern.\n\n"
            "SONG EVENT EDITOR (View > Event Editor)\n"
            "Ctrl/Cmd-C / V: Copy / Paste the complete Song curve\n"
            "Ctrl/Cmd-Z / Shift-Z: Undo / Redo\n"
            "Escape during a drag: Cancel the pending curve edit\n"
            "Right-click graph: Curve clipboard, Clear, Smooth and Scale\n\n"
            "PLAYLIST MOUSE GESTURES\n"
            "Draw mode: click block, then wheel or hold-drag vertically: Change pattern\n"
            "Draw mode, horizontal drag: Stamp pattern-sized copies\n"
            "Drag block's right edge: Resize placement\n"
            "Select mode: Drag empty space for a rectangle; Shift-click adds/removes\n"
            "Drag selected clips: Move group   |   Ctrl/Cmd-drag: Copy group\n"
            "Playlist focused: Ctrl/Cmd-A selects all clips; Delete removes placements\n"
            "Escape clears clip selection   |   Shift-S toggles Follow Play Cursor\n"
            "Playlist focused, hover a pattern/audio clip and press N: Rename\n"
            "Click ruler: Seek   |   Drag ruler: Set and enable loop\n"
            "Drag three-line Generator grip: Resize name column\n\n"
            "Piano Roll header Restore/Maximize keeps the Piano Roll open.\n"
            "Instrument switches back to the synth; Minimize hides the generator.\n";
    }
    HotkeyGuide()
    {
        search.setTextToShowWhenEmpty("Search hotkeys or actions",juce::Colours::grey);
        search.onTextChange=[this]{refresh();};
        text.setMultiLine(true);text.setReadOnly(true);text.setScrollbarsShown(true);
        text.setFont(juce::FontOptions(14.0f));
        addAndMakeVisible(search);addAndMakeVisible(text);setSize(700,580);refresh();
    }
    void resized() override
    {auto area=getLocalBounds().reduced(12);search.setBounds(area.removeFromTop(28));area.removeFromTop(8);text.setBounds(area);}
    void refresh()
    {
        const auto query=search.getText().trim();
        if(query.isEmpty()){text.setText(reference(),false);return;}
        juce::String result;
        for(const auto& line:juce::StringArray::fromLines(reference()))if(line.containsIgnoreCase(query))result+=line+"\n";
        text.setText(result.isEmpty()?"No matching hotkeys.":result,false);
    }
    juce::TextEditor search,text;
};
}
