/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "../Panels/MainPanel.h"

//==============================================================================
/**
*/
class SpectralSubtractorAudioProcessorEditor : public AudioProcessorEditor
{
public:
    SpectralSubtractorAudioProcessorEditor (SpectralSubtractorAudioProcessor&);
    ~SpectralSubtractorAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    SpectralSubtractorAudioProcessor& mProcessor;
    
    MainPanel mMainPanel;      // mMainPanel will contain all the other sub panels
    
    const juce::File mSpectralSubtractorDirectory { juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support/Zach Wang/Spectral Subtractor") };
    const juce::File mSettingsFile { mSpectralSubtractorDirectory.getChildFile ("settings.xml") };
    juce::XmlElement mSettingsXml {"SpectralSubtractor"};
    const juce::Identifier mWindowSizeAttrName {"windowHeight"};
    const int mDefaultWindowHeight {375};
    
    void saveWindowHeight (int windowHeight);
    int getSavedWindowHeight();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSubtractorAudioProcessorEditor)
};
