static void layoutCheck(bool good,const char* message){if(!good)throw std::runtime_error(message);}
static void layoutImage(juce::Component& c,const char* name)
{auto im=c.createComponentSnapshot(c.getLocalBounds(),true,2);juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name));juce::PNGImageFormat().writeImageToStream(im,out);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);host.createProjectDiscardingCurrent();for(int i=0;i<4;++i)host.addInstrumentTrack(true);host.showMixer(true);host.showMixer();host.setKnobStyle(2);
    auto& mixer=*host.mixerWindow;mixer.setBounds(0,82,660,880);mixer.resized();auto* strip=mixer.channelStrips[0];const auto bounds=mixer.getBounds();const auto original=strip->eqControls[0][0].getBounds();const auto undo=host.undoHistory.size();const auto gain=host.getTrackGainDb(0);const auto pan=host.getTrackPan(0);
    auto base=mixer.layoutSettings;
    for(int skin:{0,1,2,3,4,5,6})for(int width:{112,132,200,260})for(int x:{-20,0,24})for(int y:{-12,0,32})
    {
        auto l=base;l.stripWidth=width;l.horizontal=x;l.vertical=y;l.labelOffset=12;l.knobPercent=500;mixer.setLayout(l);mixer.setMixerSkin(skin);
        layoutCheck(mixer.getBounds()==bounds,"layout resized mixer window");
        for(int band=0;band<4;++band)
        {
            const int count=band==1?3:2;
            for(int i=0;i<count;++i)
            {
                auto& knob=strip->eqControls[band][i];layoutCheck(strip->getLocalBounds().contains(knob.getBounds()),"knob clipped outside strip");
                auto face=orbit::ui::mixerKnobBounds(knob,knob.getBounds().toFloat(),true);const auto diameter=juce::jmax(1.f,(face.getWidth()-13.f)*.53f);auto actual=face.withSizeKeepingCentre(diameter,diameter);
                layoutCheck(strip->getLocalBounds().toFloat().contains(actual),"painted knob face clipped");
                for(int j=i+1;j<count;++j){auto& other=strip->eqControls[band][j];auto body=orbit::ui::mixerKnobBounds(other,other.getBounds().toFloat(),true);auto d=juce::jmax(1.f,(body.getWidth()-13.f)*.53f);layoutCheck(!actual.intersects(body.withSizeKeepingCentre(d,d)),"adjacent knob faces overlap");}
            }
        }
        layoutCheck(strip->gain.getHeight()>=40,"expanded layout lost fader space");
    }
    mixer.setLayout(base);layoutCheck(strip->eqControls[0][0].getBounds()==original,"reset did not restore original knob cells");
    const auto atlas=orbit::ui::homeKnobAtlas();long long white=0,silver=0;int blueBefore=0,blueAfter=0;
    for(int y=0;y<atlas.getHeight();y+=3)for(int x=0;x<atlas.getWidth();x+=3)
    {const auto a=atlas.getPixelAt(x,y),b=orbit::ui::mixerKnobAtlas(2).getPixelAt(x,y),c=orbit::ui::mixerKnobAtlas(3).getPixelAt(x,y);layoutCheck(a.getAlpha()==b.getAlpha() && a.getAlpha()==c.getAlpha(),"finish changed sprite alpha");if(a.getAlpha()>200){white+=a.getRed()+a.getGreen()+a.getBlue();silver+=c.getRed()+c.getGreen()+c.getBlue();if(a.getBlue()>a.getRed()+20)++blueBefore;if(b.getBlue()>b.getRed()+20)++blueAfter;}}
    layoutCheck(silver<white && blueAfter<blueBefore/4,"silver/charcoal appearance ineffective");
    mixer.setMixerSkin(3);layoutImage(mixer,"original-silver.png");mixer.setMixerSkin(5);layoutImage(mixer,"warm-white.png");mixer.setMixerSkin(6);layoutImage(mixer,"silver-knobs.png");
    mixer.handleMixerMenuResult(40);layoutCheck(mixer.layoutWindow!=nullptr && mixer.layoutWindow->isVisible(),"layout menu did not open panel");auto* panel=mixer.layoutPanel;panel->sliders[0].setValue(20,juce::sendNotificationSync);panel->sliders[1].setValue(18,juce::sendNotificationSync);panel->sliders[3].setValue(180,juce::sendNotificationSync);panel->fader.setSelectedItemIndex(1,juce::sendNotificationSync);panel->finish.setSelectedItemIndex(2,juce::sendNotificationSync);panel->commit();
    layoutCheck(mixer.layoutSettings.horizontal==20 && mixer.layoutSettings.vertical==18 && mixer.stripStep()==180,"live layout callbacks failed");
    layoutCheck(orbit::mixing::Layout::fromJson(host.optionString("mixerLayoutV1"))==mixer.layoutSettings,"layout preferences did not persist");
    layoutImage(*panel,"layout-panel.png");layoutImage(mixer,"custom-layout.png");auto custom=mixer.layoutSettings;
    mixer.handleMixerMenuResult(1500);layoutCheck(mixer.layoutSettings.knobPercent==500 && mixer.getBounds()==bounds,"knob size menu resized mixer or lost config");
    panel->revert.onClick();panel->commit();layoutCheck(mixer.layoutSettings==base,"Revert failed");mixer.setLayout(custom);host.showMixer(true);layoutCheck(mixer.layoutSettings==custom,"Master bank lost layout");
    mixer.setBounds(bounds);layoutCheck(mixer.masterStrip.gain.getHeight()>0,"Master fader clipped");layoutImage(mixer,"master-layout.png");
    layoutCheck(host.undoHistory.size()==undo && host.getTrackGainDb(0)==gain && host.getTrackPan(0)==pan,"appearance changed audio state or Undo");
    panel->reset.onClick();panel->commit();layoutCheck(mixer.layoutSettings==orbit::mixing::Layout{},"Reset failed");panel->close.onClick();layoutCheck(!mixer.layoutWindow->isVisible(),"layout Close did not hide panel");
    mixer.setLayout(custom);mixer.showLayoutPanel();panel->sliders[0].setValue(-10,juce::sendNotificationSync);panel->revert.onClick();panel->commit();layoutCheck(mixer.layoutSettings==custom,"reopened panel used stale Revert baseline");mixer.setMinimized(true);layoutCheck(!mixer.layoutWindow->isVisible(),"minimized mixer left layout panel open");mixer.setMinimized(false);mixer.showLayoutPanel();mixer.setVisible(false);layoutCheck(!mixer.layoutWindow->isVisible(),"hidden mixer left orphan layout panel");
    auto invalid=orbit::mixing::Layout::fromJson("{\"version\":1,\"horizontal\":1e100,\"stripWidth\":-200,\"knobFinish\":99}");layoutCheck(invalid.horizontal==24 && invalid.stripWidth==112 && invalid.knobFinish==3,"layout validation failed");
    mixer.setLayout({});if(mixer.onLayoutSaved)mixer.onLayoutSaved({});
    std::cout<<"PASS: 252 palette/width/spacing combinations, independent fixed mixer bounds, no face overlap, fader space, default restore, alpha-preserving charcoal/silver sprites, live panel/menu/persistence/reset/revert, Master bank and unchanged audio/Undo.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
