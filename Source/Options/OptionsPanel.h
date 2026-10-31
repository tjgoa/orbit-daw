#pragma once
#include <JuceHeader.h>
#include <map>

namespace orbit::options
{
// The form scrolls on smaller displays; action buttons always remain reachable.
class Panel final : public juce::Component
{
public:
    explicit Panel(const juce::String& explanation)
    {
        description.setText(explanation, juce::dontSendNotification);
        description.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(description); addAndMakeVisible(viewport);
        viewport.setViewedComponent(&form, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(apply); addAndMakeVisible(cancel);
        setSize(640, 200);
    }
    juce::Slider& number(const juce::String& id, const juce::String& label, double initial,
                         double low, double high, double step = 1)
    {
        auto field = std::make_unique<juce::Slider>(); auto& ref = *field;
        ref.setSliderStyle(juce::Slider::IncDecButtons);
        ref.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 110, 26);
        ref.setRange(low, high, step); ref.setValue(initial, juce::dontSendNotification);
        numbers[id] = &ref; add(id, label, std::move(field)); return ref;
    }
    juce::ComboBox& choice(const juce::String& id, const juce::String& label,
                          const juce::StringArray& entries, int selected)
    {
        auto field = std::make_unique<juce::ComboBox>(); auto& ref = *field;
        ref.addItemList(entries, 1); ref.setSelectedItemIndex(selected, juce::dontSendNotification);
        choices[id] = &ref; add(id, label, std::move(field)); return ref;
    }
    juce::ToggleButton& check(const juce::String& id, const juce::String& label, bool selected)
    {
        auto field = std::make_unique<juce::ToggleButton>(label); auto& ref = *field;
        ref.setToggleState(selected, juce::dontSendNotification);
        checks[id] = &ref; add(id, {}, std::move(field)); return ref;
    }
    juce::TextEditor& text(const juce::String& id, const juce::String& label, const juce::String& initial)
    {
        auto field = std::make_unique<juce::TextEditor>(); auto& ref = *field;
        ref.setText(initial, false); texts[id] = &ref; add(id, label, std::move(field)); return ref;
    }
    juce::TextButton& button(const juce::String& id, const juce::String& label)
    {
        auto field = std::make_unique<juce::TextButton>(label); auto& ref = *field;
        add(id, {}, std::move(field)); return ref;
    }
    void custom(const juce::String& id, std::unique_ptr<juce::Component> control, int height)
    { add(id, {}, std::move(control), height); }
    double value(const juce::String& id) const { return numbers.at(id)->getValue(); }
    int index(const juce::String& id) const { return choices.at(id)->getSelectedItemIndex(); }
    bool checked(const juce::String& id) const { return checks.at(id)->getToggleState(); }
    juce::String string(const juce::String& id) const { return texts.at(id)->getText().trim(); }
    void setMessage(const juce::String& message) { description.setText(message, juce::dontSendNotification); }
    void paint(juce::Graphics& g) override { g.fillAll(findColour(juce::ResizableWindow::backgroundColourId)); }
    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        description.setBounds(area.removeFromTop(60)); area.removeFromTop(8);
        auto buttons = area.removeFromBottom(32);
        cancel.setBounds(buttons.removeFromRight(94)); buttons.removeFromRight(8);
        apply.setBounds(buttons.removeFromRight(94)); area.removeFromBottom(12);
        viewport.setBounds(area); form.setSize(juce::jmax(1, area.getWidth() - 16), totalHeight);
        int y = 0;
        for (auto& row : rows)
        {
            auto bounds = juce::Rectangle<int>(0, y, form.getWidth(), row->height); y += row->height;
            if (row->label.getText().isNotEmpty()) row->label.setBounds(bounds.removeFromLeft(234));
            row->control->setBounds(bounds.reduced(3, 3));
        }
    }
    juce::TextButton apply {"Apply"}, cancel {"Cancel"};
    std::map<juce::String, juce::Slider*> numbers;
    std::map<juce::String, juce::ComboBox*> choices;
    std::map<juce::String, juce::ToggleButton*> checks;
    std::map<juce::String, juce::TextEditor*> texts;
private:
    struct Row { juce::Label label; std::unique_ptr<juce::Component> control; int height = 36; };
    void add(const juce::String& id, const juce::String& label, std::unique_ptr<juce::Component> field, int height = 36)
    {
        auto row = std::make_unique<Row>(); row->height = height;
        row->label.setText(label, juce::dontSendNotification);
        field->setComponentID(id); field->setTitle(label.isNotEmpty() ? label : id);
        form.addChildComponent(row->label); row->label.setVisible(label.isNotEmpty());
        form.addAndMakeVisible(*field); row->control = std::move(field);
        totalHeight += height; rows.push_back(std::move(row));
        setSize(640, juce::jmin(640, totalHeight + 144)); resized();
    }
    juce::Label description;
    juce::Component form;
    juce::Viewport viewport;
    int totalHeight = 0;
    std::vector<std::unique_ptr<Row>> rows;
};
}
