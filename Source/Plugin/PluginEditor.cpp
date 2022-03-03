/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
SpectralSubtractorAudioProcessorEditor::SpectralSubtractorAudioProcessorEditor (SpectralSubtractorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      mProcessor (p),
      mMainPanel (&p)
{
    addAndMakeVisible (mMainPanel);
    
    getConstrainer()->setFixedAspectRatio (0.72);
    setSize (getConstrainer()->getFixedAspectRatio() * getSavedWindowHeight(), getSavedWindowHeight());
    setResizable (true, true);
    setResizeLimits (mDefaultWidth, mDefaultHeight, 3 * mDefaultWidth, 3 * mDefaultHeight);
}

SpectralSubtractorAudioProcessorEditor::~SpectralSubtractorAudioProcessorEditor()
{
}

//==============================================================================
void SpectralSubtractorAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void SpectralSubtractorAudioProcessorEditor::resized()
{
    mMainPanel.setBounds (getBounds());
    saveWindowHeight (getHeight());
}

void SpectralSubtractorAudioProcessorEditor::saveWindowHeight (int windowHeight)
{
    mSettingsXml.setAttribute (mWindowSizeAttrName, windowHeight);
    mSettingsFile.create();
    mSettingsXml.writeTo (mSettingsFile);
}

int SpectralSubtractorAudioProcessorEditor::getSavedWindowHeight()
{
    std::unique_ptr<juce::XmlElement> parsedSettings {juce::XmlDocument::parse (mSettingsFile)};
    if (parsedSettings)
        return parsedSettings->getIntAttribute (mWindowSizeAttrName, mDefaultHeight);
    else
        return mDefaultHeight;
}
