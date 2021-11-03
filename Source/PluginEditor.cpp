/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectralSubtractorAudioProcessorEditor::SpectralSubtractorAudioProcessorEditor (SpectralSubtractorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      mMainPanel (&p)
{
    addAndMakeVisible (mMainPanel);
    
    setResizable (true, true);
    setResizeLimits (MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT, 3 * MAIN_PANEL_WIDTH, 3 * MAIN_PANEL_HEIGHT);
    getConstrainer()->setFixedAspectRatio (0.72);
    setSize (MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT);
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
}
