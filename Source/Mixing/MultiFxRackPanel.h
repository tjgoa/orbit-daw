// Included inside PluginParameterWindow, after ParameterList.
class MultiFxPanel final : public juce::Component
{
public:
    MultiFxPanel(te::RackInstance::Ptr value,std::function<void()> begin,std::function<void()> changed,
                 std::function<void(te::AutomatableParameter&,juce::Component*)> events)
        :instance(std::move(value)),before(std::move(begin)),after(std::move(changed)),editEvents(std::move(events))
    {
        for(auto* c:std::array<juce::Component*,8>{&chain,&add,&enabled,&remove,&earlier,&later,&gui,&summary})addAndMakeVisible(c);
        chain.setComponentID("multiFxChain");add.setComponentID("multiFxAdd");remove.setComponentID("multiFxRemove");earlier.setComponentID("multiFxEarlier");later.setComponentID("multiFxLater");enabled.setComponentID("multiFxEnabled");
        enabled.setButtonText("Enabled");remove.setButtonText("Remove");earlier.setButtonText("Up");later.setButtonText("Down");gui.setButtonText("Plugin GUI");
        add.setTextWhenNothingSelected("Add effect...");chain.setTextWhenNothingSelected("Empty rack — audio passes through");
        for(size_t i=0;i<orbit::mixing::effectNames.size();++i)add.addItem(orbit::mixing::effectNames[i],int(i+1));
        add.addSeparator();
        for(const auto& description:instance->edit.engine.getPluginManager().knownPluginList.getTypes())if(description.pluginFormatName=="VST3" && !description.isInstrument){external.add(description);add.addItem(description.name,1000+external.size()-1);}
        add.onChange=[this]{const int kind=add.getSelectedId();add.setSelectedId(0,juce::dontSendNotification);if(kind==0)return;if(before)before();te::Plugin::Ptr plugin;
            if(kind>=1 && kind<=10)plugin=instance->edit.getPluginCache().createNewPlugin(orbit::mixing::effectTypes[size_t(kind-1)],{});
            else if(juce::isPositiveAndBelow(kind-1000,external.size()))plugin=instance->edit.getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName,external[kind-1000]);
            if(auto* externalPlugin=dynamic_cast<te::ExternalPlugin*>(plugin.get()))
                if(auto* live=externalPlugin->getAudioPluginInstance();live==nullptr || live->getTotalNumInputChannels()==0 || live->getTotalNumOutputChannels()==0){summary.setText("Could not load this audio effect. The chain is unchanged.",juce::dontSendNotification);return;}
            if(orbit::mixing::addRackEffect(*instance->type,plugin)){selected=instance->type->getPlugins().size()-1;if(after)after();refresh();}
        };
        chain.onChange=[this]{selected=chain.getSelectedId()-1;showSelected();};
        enabled.onClick=[this]{if(auto p=selectedPlugin()){if(before)before();p->setEnabled(enabled.getToggleState());if(after)after();refresh();}};
        remove.onClick=[this]{if(auto p=selectedPlugin()){if(before)before();clearEditor();orbit::mixing::removeRackEffect(*instance->type,*p);if(after)after();refresh();}};
        earlier.onClick=[this]{move(-1);};later.onClick=[this]{move(1);};
        gui.onClick=[this]{showSelected(true);};
        addAndMakeVisible(viewport);viewport.setViewedComponent(&parameterList,false);refresh();
    }
    ~MultiFxPanel()override{clearEditor();viewport.setViewedComponent(nullptr,false);}
    te::Plugin::Ptr selectedPlugin()const{const auto plugins=instance->type->getPlugins();return juce::isPositiveAndBelow(selected,plugins.size())?te::Plugin::Ptr(plugins[selected]):te::Plugin::Ptr{};}
    void refresh()
    {
        auto plugins=instance->type->getPlugins();selected=plugins.isEmpty()?-1:juce::jlimit(0,plugins.size()-1,selected);
        chain.clear(juce::dontSendNotification);juce::StringArray names;
        for(int i=0;i<plugins.size();++i){auto name=juce::String(i+1)+". "+plugins[i]->getName()+(plugins[i]->isEnabled()?"":" [bypassed]");chain.addItem(name,i+1);names.add(plugins[i]->getName());}
        chain.setSelectedId(selected+1,juce::dontSendNotification);summary.setText("Input -> "+names.joinIntoString(" -> ")+(names.isEmpty()?"Output":" -> Output"),juce::dontSendNotification);summary.setTooltip(summary.getText());showSelected();
    }
    void resized()override
    {
        auto area=getLocalBounds();auto top=area.removeFromTop(30);add.setBounds(top.removeFromRight(188).reduced(2));chain.setBounds(top.reduced(2));
        auto actions=area.removeFromTop(30);enabled.setBounds(actions.removeFromLeft(100));gui.setBounds(actions.removeFromLeft(110).reduced(2));earlier.setBounds(actions.removeFromLeft(54).reduced(2));later.setBounds(actions.removeFromLeft(54).reduced(2));remove.setBounds(actions.removeFromLeft(78).reduced(2));
        summary.setBounds(area.removeFromTop(24));viewport.setBounds(area);
        if(!nativeEditor)parameterList.setSize(juce::jmax(480,viewport.getMaximumVisibleWidth()),parameterList.getHeight());
        else nativeEditor->syncNativeViews();
    }
    std::unique_ptr<OrbitPluginEditorContent> nativeEditor;
private:
    void clearEditor(){viewport.setViewedComponent(nullptr,false);nativeEditor.reset();parameterList.clearParameters();}
    void showSelected(bool showGui=false)
    {
        clearEditor();auto p=selectedPlugin();enabled.setEnabled(p!=nullptr);enabled.setToggleState(p && p->isEnabled(),juce::dontSendNotification);remove.setEnabled(p!=nullptr);earlier.setEnabled(selected>0);later.setEnabled(selected>=0 && selected+1<instance->type->getPlugins().size());
        auto* externalPlugin=dynamic_cast<te::ExternalPlugin*>(p.get());const bool hasGui=externalPlugin && externalPlugin->getAudioPluginInstance() && externalPlugin->getAudioPluginInstance()->hasEditor();gui.setEnabled(hasGui);
        if(showGui && hasGui){nativeEditor=std::make_unique<OrbitPluginEditorContent>(*externalPlugin);viewport.setViewedComponent(nativeEditor.get(),false);nativeEditor->preferredSizeChanged=[this]{if(nativeEditor){nativeEditor->setSize(nativeEditor->getPreferredWidth(),nativeEditor->getPreferredHeight());resized();}};nativeEditor->ensureEditorCreated();nativeEditor->setSize(nativeEditor->getPreferredWidth(),nativeEditor->getPreferredHeight());}
        else{parameterList.setParameters(p,after,0,-1,editEvents,before);viewport.setViewedComponent(&parameterList,false);}
        viewport.setViewPosition(0,0);resized();
    }
    void move(int delta){if(selected<0)return;if(before)before();clearEditor();orbit::mixing::moveRackEffect(*instance->type,selected,delta);selected+=delta;if(after)after();refresh();}
    te::RackInstance::Ptr instance;int selected=-1;
    std::function<void()> before,after;std::function<void(te::AutomatableParameter&,juce::Component*)> editEvents;
    juce::Array<juce::PluginDescription> external;
    juce::ComboBox chain,add;juce::ToggleButton enabled;juce::TextButton remove,earlier,later,gui;juce::Label summary;
    juce::Viewport viewport;ParameterList parameterList;
};
