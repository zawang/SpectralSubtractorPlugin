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
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    mMainPanel = std::make_unique<MainPanel>(&processor);
    addAndMakeVisible(*mMainPanel);
    
    setResizable (true, true);
    setResizeLimits(MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT, 2 * MAIN_PANEL_WIDTH, 2 * MAIN_PANEL_HEIGHT);
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
    mMainPanel->setBounds (getBounds());
}
