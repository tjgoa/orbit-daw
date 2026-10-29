#pragma once
#include <JuceHeader.h>
#include <map>

// Consistent, accessible controls shared by the Edit menu's musical tools.
class EditToolPanel final : public juce::Component
{
public:
    explicit EditToolPanel(const juce::String& explanation)
    {
        description.setText(explanation,juce::dontSendNotification);
        description.setColour(juce::Label::textColourId,juce::Colour::fromRGB(191,215,227));
        description.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(description);addAndMakeVisible(apply);addAndMakeVisible(cancel);
        setSize(520,150);
    }
    juce::Slider& number(const juce::String& id,const juce::String& label,double value,double minimum,double maximum,double interval=1)
    {
        auto field=std::make_unique<juce::Slider>();auto* result=field.get();
        field->setSliderStyle(juce::Slider::IncDecButtons);field->setTextBoxStyle(juce::Slider::TextBoxLeft,false,110,26);
        field->setRange(minimum,maximum,interval);field->setValue(value,juce::dontSendNotification);
        numbers[id]=result;add(id,label,std::move(field));return *result;
    }
    juce::Slider& strength(const juce::String& id,const juce::String& label,double value)
    {
        auto field=std::make_unique<juce::Slider>();auto* result=field.get();
        field->setSliderStyle(juce::Slider::LinearHorizontal);field->setTextBoxStyle(juce::Slider::TextBoxRight,false,68,26);
        field->setRange(0,100,1);field->setValue(value,juce::dontSendNotification);field->setTextValueSuffix(" %");
        numbers[id]=result;add(id,label,std::move(field));return *result;
    }
    juce::ComboBox& choice(const juce::String& id,const juce::String& label,const juce::StringArray& items,int index)
    {
        auto field=std::make_unique<juce::ComboBox>();auto* result=field.get();field->addItemList(items,1);
        field->setSelectedItemIndex(juce::jlimit(0,juce::jmax(0,items.size()-1),index),juce::dontSendNotification);
        choices[id]=result;add(id,label,std::move(field));return *result;
    }
    juce::ToggleButton& check(const juce::String& id,const juce::String& label,bool value)
    {
        auto field=std::make_unique<juce::ToggleButton>(label);auto* result=field.get();field->setToggleState(value,juce::dontSendNotification);
        checks[id]=result;add(id,{},std::move(field));return *result;
    }
    double value(const juce::String& id) const { return numbers.at(id)->getValue(); }
    int index(const juce::String& id) const { return choices.at(id)->getSelectedItemIndex(); }
    bool checked(const juce::String& id) const { return checks.at(id)->getToggleState(); }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour::fromRGB(27,43,55));}
    void resized() override
    {
        auto area=getLocalBounds().reduced(16);description.setBounds(area.removeFromTop(48));area.removeFromTop(8);
        for(auto& row:rows)
        {
            auto bounds=area.removeFromTop(34);
            if(row->label.getText().isNotEmpty()) row->label.setBounds(bounds.removeFromLeft(210));
            row->control->setBounds(bounds.reduced(2,3));
        }
        area.removeFromTop(12);auto buttons=area.removeFromTop(30);cancel.setBounds(buttons.removeFromRight(94));buttons.removeFromRight(8);apply.setBounds(buttons.removeFromRight(94));
    }
    void setMessage(const juce::String& text) {description.setText(text,juce::dontSendNotification);}
    juce::TextButton apply{"Apply"},cancel{"Cancel"};
private:
    struct Row {juce::Label label;std::unique_ptr<juce::Component> control;};
    void add(const juce::String& id,const juce::String& label,std::unique_ptr<juce::Component> field)
    {
        auto row=std::make_unique<Row>();row->label.setText(label,juce::dontSendNotification);
        row->label.setColour(juce::Label::textColourId,juce::Colour::fromRGB(225,239,247));
        field->setComponentID(id);field->setTitle(label.isNotEmpty()?label:id);
        addChildComponent(row->label);row->label.setVisible(label.isNotEmpty());addAndMakeVisible(*field);row->control=std::move(field);rows.push_back(std::move(row));
        setSize(520,136+int(rows.size())*34);resized();
    }
    juce::Label description;
    std::vector<std::unique_ptr<Row>> rows;
    std::map<juce::String,juce::Slider*> numbers;
    std::map<juce::String,juce::ComboBox*> choices;
    std::map<juce::String,juce::ToggleButton*> checks;
};
