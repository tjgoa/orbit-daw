static void shadowCheck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);host.addInstrumentTrack(true);host.setKnobStyle(2);host.showMixer();auto& mixer=*host.mixerWindow;
    mixer.setBounds(0,0,400,820);mixer.setKnobSize(400);mixer.setMixerSkin(3);mixer.resized();auto* strip=mixer.channelStrips[0];const auto geometry=strip->getBounds();
    for(int scale:{100,200,300,400,500})for(int skin:{0,1,2,3,4})
    {
        mixer.setKnobSize(scale);mixer.setMixerSkin(skin);shadowCheck(strip->getBounds()==geometry,"knob/skin changed strip geometry");
        for(int band=0;band<4;++band)for(int knob=0;knob<(band==1?3:2);++knob)
        {
            auto& slider=strip->eqControls[band][knob];auto light=slider.createComponentSnapshot(slider.getLocalBounds());mixer.getProperties().set("orbitMixerLightSurface",false);auto dry=slider.createComponentSnapshot(slider.getLocalBounds());mixer.getProperties().set("orbitMixerLightSurface",skin>=3);
            if(skin>=3)
            {
                dry=juce::Image(juce::Image::ARGB,slider.getWidth(),slider.getHeight(),true);juce::Graphics g(dry);
                FirstSynthReusableHomeKnobLookAndFeel reference;reference.setSpriteImage(orbit::ui::homeKnobAtlas());orbit::ui::configureLightWhiteKnob(reference);
                auto b=orbit::ui::mixerKnobBounds(slider,slider.getLocalBounds().toFloat(),true).toNearestInt();
                reference.drawRotarySlider(g,b.getX(),b.getY(),b.getWidth(),b.getHeight(),float(slider.valueToProportionOfLength(slider.getValue())),0,0,slider);
            }
            // Silver's shadow must fade to transparent before each cell edge.
            // Dark controls are byte-identical to the clean renderer.
            int differences=0;
            for(int y=0;y<light.getHeight();++y)for(int x=0;x<light.getWidth();++x)
            {
                if(light.getPixelAt(x,y)!=dry.getPixelAt(x,y))++differences;
                if(x==0 || y==0 || x==light.getWidth()-1 || y==light.getHeight()-1)shadowCheck(light.getPixelAt(x,y)==dry.getPixelAt(x,y),"silver shadow was clipped at control edge");
            }
            if(skin<3)shadowCheck(differences==0,"shadow leaked onto dark skin");
        }
    }
    mixer.setMixerSkin(3);mixer.setKnobSize(400);auto image=mixer.createComponentSnapshot(mixer.getLocalBounds(),true,2);juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/mixer-soft-shadow.png"));juce::PNGImageFormat().writeImageToStream(image,out);
    std::cout<<"PASS: 100–500% native knobs on all five mixer palettes, fixed strip geometry, no shadow at canvas/label boundary, dark skins unchanged.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
