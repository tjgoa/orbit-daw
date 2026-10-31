#include "OrionSfsImportDialog.h"
#include <iostream>
#include <stdexcept>

static void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T, class Predicate> static T* find(juce::Component& parent, Predicate predicate)
{
    for (auto* child : parent.getChildren())
    {
        if (auto* result=dynamic_cast<T*>(child);result && predicate(*result)) return result;
        if (auto* result=find<T>(*child,predicate)) return result;
    }
    return nullptr;
}
int main(int argc,char** argv)
{
    try
    {
        check(argc==3,"Usage: test-sfs-preview-ui Song1.sfs render-folder");
        juce::ScopedJuceInitialiser_GUI gui;
        const juce::File source(argv[1]),folder(argv[2]);
        juce::MemoryBlock before;check(source.loadFileAsData(before),"could not read source fixture");
        int commits=0;bool refuse=true;orbit::import::ProjectImportPlan captured;
        std::cout<<"CHECK: construct dialog\n"<<std::flush;
        auto component=orbit::import::createOrionSfsPreview(source,[&](const auto& plan,std::string& error)
        {
            ++commits;captured=plan;
            if(refuse){error="Test destination mismatch";return false;}return true;
        });
        std::cout<<"CHECK: dialog ready\n"<<std::flush;
        auto editor=[&](const char* name)
        {auto* p=find<juce::TextEditor>(*component,[&](auto& c){return c.getTitle()==name;});check(p,"missing field");return p;};
        auto button=[&](const char* prefix)
        {auto* p=find<juce::Button>(*component,[&](auto& c){return c.getButtonText().startsWith(prefix);});check(p,"missing button");return p;};
        auto* status=find<juce::Label>(*component,[](auto& c){return c.getName()=="Recovery status";});check(status,"missing visible status");
        auto* list=find<juce::ListBox>(*component,[](auto&){return true;});check(list,"missing row list");
        auto* merge=button("Merge selected");auto* preview=button("Preview recovery");
        auto set=[&](const char* name,const char* value){auto* field=editor(name);field->setText(value,false);field->onTextChange();};
        auto toggle=[&](const char* name,bool value){auto* b=button(name);b->setToggleState(value,juce::dontSendNotification);b->onClick();};
        auto render=[&](const char* name)
        {juce::FileOutputStream out(folder.getChildFile(name));juce::PNGImageFormat png;check(png.writeImageToStream(component->createComponentSnapshot(component->getLocalBounds()),out),"render failed");};
        check(editor("Source PPQ")->getText()=="96" && editor("Meter N/D")->getText()=="4/4","requested defaults absent");
        check(merge->isEnabled() && list->getNumSelectedRows()==3 && !button("Use tentative")->getToggleState() && !button("Accept omitted")->getToggleState(),"initial selection or consent changed");
        int first=-1,audio=-1;
        for(int row=0;row<list->getModel()->getNumRows();++row)
        {
            auto name=list->getModel()->getNameForRow(row);
            if(name.contains(" / Generator /") && first<0)first=row;
            if(name.contains(" / Audio /"))audio=row;
        }
        check(first>=0 && audio>=0,"Song1 fixture row inventory changed");
        list->selectRow(first);check(merge->isEnabled(),"Merge still requires a separate preview");
        std::cout<<"CHECK: missing consent\n"<<std::flush;
        merge->onClick();check(commits==0 && status->getText().startsWith("Cannot merge:"),"missing consent did not produce a visible failure");
        toggle("Use tentative",true);toggle("Accept omitted",true);toggle("Omit unsupported",true);
        std::cout<<"CHECK: preview generator\n"<<std::flush;
        preview->onClick();check(merge->isEnabled() && commits==0 && status->getText().startsWith("Ready to merge:"),"preview did not unlock a valid plan");
        merge->onClick();check(commits==1 && status->getText().contains("Test destination mismatch"),"host rejection was hidden");
        refuse=false;merge->onClick();check(commits==2 && captured.tracks.size()==1 && captured.tracks[0].patterns.size()==2,"direct retry did not rebuild and commit");
        set("Meter N/D","4/3");merge->onClick();check(commits==2 && status->getText().startsWith("Cannot merge:"),"invalid meter imported");
        set("Meter N/D","4/4");set("Tempo BPM","nan");merge->onClick();check(commits==2,"nonfinite tempo imported");
        set("Tempo BPM","120");set("Source PPQ","48");merge->onClick();
        check(commits==3 && captured.tracks[0].patterns[0].lengthTicks==6144,"edited PPQ reused a stale preview");set("Source PPQ","96");
        toggle("Recover tentative playlist",true);toggle("Recover tentative dry",true);toggle("Omit unsupported",true);toggle("Allow rounding",true);
        list->selectRangeOfRows(0,list->getModel()->getNumRows()-1);toggle("Keep missing",false);
        check(status->getText().contains("locate WAVs") && status->getText().contains("placeholders"),"missing media guidance absent");
        render("sfs-required-media.png");merge->onClick();check(commits==3 && status->getText().startsWith("Cannot merge:"),"unresolved audio imported");
        std::cout<<"CHECK: placeholders\n"<<std::flush;
        toggle("Keep missing",true);merge->onClick();
        check(commits==4 && captured.tracks.size()==3 && captured.tracks[1].audioClips.size()==3,"missing media still prevents generator merge");
        for(const auto& clip:captured.tracks[1].audioClips)check(clip.missingMedia && !clip.originalReference.empty(),"placeholder metadata missing");
        list->deselectRow(audio);merge->onClick();
        std::size_t notes=0,placements=0;for(const auto& track:captured.tracks){placements+=track.playlist.size();for(const auto& p:track.patterns)notes+=p.notes.size();}
        check(commits==5 && captured.tracks.size()==2 && notes==32 && placements==62 && captured.unconvertedRecords.size()==2,"notes-only direct merge plan changed");
        check(status->getText().contains("32 notes") && status->getText().contains("62 Playlist blocks"),"ready summary does not describe plan");
        render("sfs-ready-to-merge.png");
        juce::MemoryBlock after;check(source.loadFileAsData(after) && before==after,"source was modified");
        check(status->getBounds().getBottom()<merge->getY() && status->getHeight()>=60,"visible status clipped by buttons");
        std::cout<<"PASS: 96 PPQ/4/4 defaults, explicit consent, optional preview/direct merge, stale-plan invalidation, visible validation/host failure and retry, missing-media guidance, Song1 notes-only 32-note/62-placement plan and two retained records; original source unchanged.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
