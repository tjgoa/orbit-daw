// Included inside ProjectView. Folder scanning never dereferences session objects
// off the message thread; matching and one-Undo application use the current Edit.
int missingMediaCount() const
{return edit!=nullptr?static_cast<int>(orbit::media::collect(*edit).size()):0;}
void relinkMissingMediaFromFiles(const orbit::media::FileIndex& files,bool incomplete=false)
{
    if(edit==nullptr)return;
    const auto matches=orbit::media::plan(orbit::media::collect(*edit),files);
    if(!matches.empty())
    {
        rememberUndo("Find missing media");orbit::media::apply(matches);isDirty=true;
        edit->restartPlayback();if(playlistWindow!=nullptr)playlistWindow->refresh();
        if(drumRackWindow!=nullptr)
            for(int pad=0;pad<16;++pad)drumRackWindow->setPadInfo(pad,getDrumRackPadInfo(findSamplerPlugin(drumRackTrackId),pad));
    }
    const auto remaining=missingMediaCount();
    status.setText("Linked "+juce::String(matches.size())+" media references; "+juce::String(remaining)
        +" still missing. "+(incomplete?"Search incomplete; choose a smaller folder. ":"")
        +(remaining>0?"Choose another folder; ambiguous or mismatched WAVs are left unresolved.":"Save the project to keep these paths."),juce::dontSendNotification);
}
void findMissingMedia()
{
    if(edit==nullptr)return;
    const auto missing=orbit::media::collect(*edit);
    if(missing.empty()){status.setText("No missing audio clips or sampler files.",juce::dontSendNotification);return;}
    fileChooser=std::make_unique<juce::FileChooser>("Find missing media: choose a folder to search, including subfolders",juce::File{});
    const juce::Component::SafePointer<ProjectView> safe(this);const auto projectId=project.getProjectId();
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,
        [safe,projectId](const juce::FileChooser& chooser)
        {
            const auto folder=chooser.getResult();if(!safe || safe->edit==nullptr || !folder.isDirectory())return;
            std::set<juce::String> wanted;
            for(const auto& item:orbit::media::collect(*safe->edit))wanted.insert(orbit::media::leafName(item.reference).toLowerCase());
            safe->status.setText("Searching for missing media in "+folder.getFileName()+"...",juce::dontSendNotification);
            juce::Thread::launch([safe,projectId,folder,wanted]
            {
                bool incomplete=false;auto files=orbit::media::scan(folder,wanted,incomplete);
                juce::MessageManager::callAsync([safe,projectId,files=std::move(files),incomplete]
                {if(safe && safe->edit!=nullptr && safe->project.getProjectId()==projectId)safe->relinkMissingMediaFromFiles(files,incomplete);});
            });
        });
}
void bypassMissingEffects()
{
    if(edit==nullptr)return;
    std::vector<te::Plugin*> missing;
    auto collect=[&](const auto& list){for(auto* plugin:list)
        if(auto* external=dynamic_cast<te::ExternalPlugin*>(plugin);external!=nullptr && external->isMissing() && !external->isSynth() && external->isEnabled())missing.push_back(plugin);};
    for(auto* track:te::getAudioTracks(*edit))collect(track->pluginList);collect(edit->getMasterPluginList());
    if(!missing.empty())
    {
        rememberUndo("Bypass missing effects");for(auto* plugin:missing)plugin->setEnabled(false);
        isDirty=true;edit->restartPlayback();
    }
    status.setText("Bypassed "+juce::String(missing.size())+" missing effects; their saved state is retained. Orion SFS effect state is not converted.",juce::dontSendNotification);
}
