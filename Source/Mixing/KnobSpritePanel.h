#pragma once
#include "../UI/KnobSprite.h"
#include <atomic>
#include <mutex>

namespace orbit::mixing
{
// Metadata and the selected image are read on one background worker. Generations
// discard outdated selections, including rapid previous/next clicks and rescans.
class SpriteBrowser final:private juce::Timer
{
public:
    struct Result
    {
        int generation=0;
        bool catalogueChanged=false;
        ui::sprites::Catalogue catalogue;
        ui::sprites::Ptr sprite;
        juce::String error;
    };
    SpriteBrowser():worker(1){}
    ~SpriteBrowser()override{stopTimer();++state->generation;worker.removeAllJobs(true,10000);}
    std::function<void()> onChanged;
    std::function<void(ui::sprites::Ptr)> onApplied;
    ui::sprites::Catalogue catalogue;
    ui::sprites::Ptr active;
    juce::String status="Choose an export folder to preview its knobs on the Mixer and Master.";
    bool loading=false;
    void open(juce::File folder,juce::String preferred={})
    {
        const auto generation=begin("Reading sprite library...");const auto shared=state;
        worker.addJob([shared,generation,folder,preferred]
        {
            if(shared->generation!=generation)return;
            auto result=std::make_unique<Result>();result->generation=generation;result->catalogueChanged=true;
            result->catalogue=ui::sprites::scan(folder);result->error=result->catalogue.error;
            if(result->error.isEmpty() && preferred.isNotEmpty())
            {
                const auto& entries=result->catalogue.entries;
                auto i=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.id==preferred;});
                if(i==entries.end())result->error="The saved sprite is absent from this library. Using the current/default knobs.";
                else result->sprite=ui::sprites::load(*i,result->error);
            }
            publish(shared,std::move(result));
        });
    }
    void select(const ui::sprites::Entry& entry)
    {
        if(active!=nullptr && active->entry.id==entry.id && active->entry.file==entry.file && !loading)return;
        const auto generation=begin("Loading "+entry.name+"...");const auto shared=state;
        worker.addJob([shared,generation,entry]
        {
            if(shared->generation!=generation)return;
            auto result=std::make_unique<Result>();result->generation=generation;
            result->sprite=ui::sprites::load(entry,result->error);publish(shared,std::move(result));
        });
    }
    void useDefault()
    {
        ++state->generation;loading=false;stopTimer();active.reset();
        status="Using the normal mixer knob style. Your sprite library is still available.";
        if(onApplied)onApplied({});if(onChanged)onChanged();
    }
    void poll()
    {
        std::unique_ptr<Result> result;
        {std::lock_guard<std::mutex> lock(state->mutex);result=std::move(state->result);}
        if(result==nullptr || result->generation!=state->generation)return;
        loading=false;stopTimer();
        if(result->catalogueChanged && result->catalogue.error.isEmpty())catalogue=std::move(result->catalogue);
        if(result->sprite!=nullptr)
        {
            active=std::move(result->sprite);status="Live in Mixer and Master: "+active->entry.name;
            if(onApplied)onApplied(active);
        }
        else if(result->error.isEmpty())status=juce::String(int(catalogue.entries.size()))+" sprites available. Select one to apply it live.";
        if(result->error.isNotEmpty())status=result->error+" The previous knob selection is retained.";
        if(onChanged)onChanged();
    }
private:
    struct State{std::atomic<int> generation{0};std::mutex mutex;std::unique_ptr<Result> result;};
    std::shared_ptr<State> state=std::make_shared<State>();
    juce::ThreadPool worker;
    int begin(const juce::String& message)
    {const auto g=++state->generation;loading=true;status=message;startTimerHz(30);if(onChanged)onChanged();return g;}
    static void publish(const std::shared_ptr<State>& s,std::unique_ptr<Result> result)
    {if(s->generation!=result->generation)return;std::lock_guard<std::mutex> lock(s->mutex);if(s->generation==result->generation)s->result=std::move(result);}
    void timerCallback()override{poll();}
};

class KnobSpritePanel final:public juce::Component,public juce::ListBoxModel
{
public:
    explicit KnobSpritePanel(SpriteBrowser& b):browser(b),list("Exported knob sprites",this)
    {
        heading.setText("Select a sprite to override Mixer and Master knob artwork immediately, across all skins. Drag the mixer knobs as usual. Use normal knobs restores the global style.",juce::dontSendNotification);
        heading.setColour(juce::Label::textColourId,juce::Colour(0xff202731));heading.setFont(juce::FontOptions(13));
        filter.setTextToShowWhenEmpty("Search sprites",juce::Colour(0xff65717c));filter.setColour(juce::TextEditor::backgroundColourId,juce::Colours::white);filter.setColour(juce::TextEditor::textColourId,juce::Colour(0xff202731));
        filter.onTextChange=[this]{refreshRows();};
        list.setRowHeight(27);list.setColour(juce::ListBox::backgroundColourId,juce::Colours::white);list.setColour(juce::ListBox::textColourId,juce::Colour(0xff202731));list.setMultipleSelectionEnabled(false);
        for(auto* c:std::initializer_list<juce::Component*>{&heading,&filter,&list,&folder,&reload,&previous,&next,&normal,&layout,&close,&info,&location,&preview,&frame,&surface})addAndMakeVisible(c);
        info.setColour(juce::Label::textColourId,juce::Colour(0xff202731));info.setFont(juce::FontOptions(12));
        location.setColour(juce::Label::textColourId,juce::Colour(0xff445563));location.setFont(juce::FontOptions(11));
        folder.onClick=[this]{chooseFolder();};reload.onClick=[this]{if(browser.catalogue.folder.isDirectory())browser.open(browser.catalogue.folder,browser.active?browser.active->entry.id:juce::String{});};
        previous.onClick=[this]{step(-1);};next.onClick=[this]{step(1);};normal.onClick=[this]{browser.useDefault();};layout.onClick=[this]{if(onLayout)onLayout();};close.onClick=[this]{if(onClose)onClose();};
        frame.setSliderStyle(juce::Slider::LinearHorizontal);frame.setRange(0,1,.001);frame.setValue(.5);frame.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);frame.setTitle("Preview frame (does not change mixer parameters)");frame.onValueChange=[this]{preview.position=float(frame.getValue());preview.repaint();};
        surface.addItemList({"Preview: Silver","Preview: White","Preview: Navy","Preview: Transparency"},1);surface.setSelectedId(1);surface.onChange=[this]{preview.surface=surface.getSelectedItemIndex();preview.repaint();};
        refresh();setSize(710,520);
    }
    std::function<void()> onClose,onLayout;
    int getNumRows()override{return int(rows.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected)override
    {
        if(!juce::isPositiveAndBelow(row,int(rows.size())))return;
        if(selected)g.fillAll(juce::Colour(0xffc9e3ef));
        const auto& entry=browser.catalogue.entries[size_t(rows[size_t(row)])];
        g.setColour(juce::Colour(0xff202731));g.setFont(13.f);g.drawText(entry.name,8,0,width-16,height,juce::Justification::centredLeft,true);
    }
    void selectedRowsChanged(int row)override
    {if(!updating && juce::isPositiveAndBelow(row,int(rows.size())))browser.select(browser.catalogue.entries[size_t(rows[size_t(row)])]);}
    void refresh()
    {
        refreshRows();preview.sprite=browser.active;preview.repaint();
        info.setText(browser.status,juce::dontSendNotification);
        const auto& c=browser.catalogue;location.setText(c.folder==juce::File{}?"No library loaded":c.folder.getFullPathName()+"  |  "+juce::String(int(c.entries.size()))+" available, "+juce::String(c.skipped)+" skipped",juce::dontSendNotification);
        location.setTooltip(location.getText());reload.setEnabled(c.folder.isDirectory());
        if(browser.active)preview.setTooltip(browser.active->entry.name+" | "+juce::String(browser.active->entry.width)+" x "+juce::String(browser.active->entry.height)+" | "+juce::String(browser.active->entry.frames)+" frames");
    }
    void step(int delta)
    {if(rows.empty())return;const int selected=list.getSelectedRow();const int row=selected<0?(delta<0?int(rows.size())-1:0):(selected+delta+int(rows.size()))%int(rows.size());list.selectRow(row);list.scrollToEnsureRowIsOnscreen(row);}
    void chooseFolder()
    {
        if(chooser!=nullptr)return;
        auto start=browser.catalogue.folder.isDirectory()?browser.catalogue.folder:juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        chooser=std::make_unique<juce::FileChooser>("Choose the KnobMan export folder containing manifest.json",start,juce::String{});
        const juce::Component::SafePointer<KnobSpritePanel> owner(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[owner](const auto& chosen)
        {if(owner==nullptr)return;const auto result=chosen.getResult();if(result.isDirectory())owner->browser.open(result);owner->chooser.reset();});
    }
    void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xffedf0f2));}
    void resized()override
    {
        auto a=getLocalBounds().reduced(14);heading.setBounds(a.removeFromTop(42));
        auto row=a.removeFromTop(30);folder.setBounds(row.removeFromLeft(160));row.removeFromLeft(8);reload.setBounds(row.removeFromLeft(80));row.removeFromLeft(10);filter.setBounds(row);
        a.removeFromTop(8);location.setBounds(a.removeFromTop(28));
        auto bottom=a.removeFromBottom(34);normal.setBounds(bottom.removeFromLeft(150));bottom.removeFromLeft(8);layout.setBounds(bottom.removeFromLeft(140));close.setBounds(bottom.removeFromRight(80));
        info.setBounds(a.removeFromBottom(55));a.removeFromBottom(8);
        auto right=a.removeFromRight(225);a.removeFromRight(12);list.setBounds(a);
        auto nav=right.removeFromBottom(30);previous.setBounds(nav.removeFromLeft(107));next.setBounds(nav.removeFromRight(107));right.removeFromBottom(6);frame.setBounds(right.removeFromBottom(28));surface.setBounds(right.removeFromBottom(28));right.removeFromBottom(8);preview.setBounds(right);
    }
    SpriteBrowser& browser;
    juce::TextEditor filter;
    juce::ListBox list;
    juce::TextButton folder{"Choose export folder..."},reload{"Refresh"},previous{"Previous"},next{"Next"},normal{"Use normal knobs"},layout{"Mixer Layout..."},close{"Close"};
    juce::Slider frame;
    juce::ComboBox surface;
private:
    struct Preview:juce::Component,juce::SettableTooltipClient
    {
        ui::sprites::Ptr sprite;float position=.5f;int surface=0;
        void paint(juce::Graphics& g)override
        {
            g.fillAll(surface==1?juce::Colours::white:surface==2?juce::Colour(0xff1e2c36):juce::Colour(0xffd4d9df));
            if(surface==3)for(int y=0;y<getHeight();y+=12)for(int x=0;x<getWidth();x+=12){g.setColour(((x/12+y/12)%2)==0?juce::Colour(0xffadb5be):juce::Colour(0xffe6e9ed));g.fillRect(x,y,12,12);}
            if(sprite)sprite->draw(g,getLocalBounds().toFloat().reduced(12),position);
            else{g.setColour(surface==2?juce::Colours::white:juce::Colour(0xff202731));g.drawFittedText("Select a sprite",getLocalBounds().reduced(10),juce::Justification::centred,2);}
        }
    } preview;
    juce::Label heading,info,location;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<int> rows;
    bool updating=false;
    void refreshRows()
    {
        const auto query=filter.getText().trim();rows.clear();int selected=-1;
        for(size_t i=0;i<browser.catalogue.entries.size();++i){const auto& e=browser.catalogue.entries[i];if(!e.name.containsIgnoreCase(query))continue;if(browser.active && browser.active->entry.id==e.id && browser.active->entry.file==e.file)selected=int(rows.size());rows.push_back(int(i));}
        // Keep a pending selection visible during background decode.
        updating=true;const auto pending=list.getSelectedRow();list.updateContent();
        if(!browser.loading){if(selected>=0)list.selectRow(selected,false,false);else list.deselectAllRows();}
        else if(juce::isPositiveAndBelow(pending,int(rows.size())))list.selectRow(pending,true,false);
        updating=false;list.repaint();previous.setEnabled(!rows.empty());next.setEnabled(!rows.empty());
    }
};
}
