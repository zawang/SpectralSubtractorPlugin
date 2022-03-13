/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

#include "../Helper/HelperFunctions.h"
#include "../Helper/InterfaceConstants.h"

BottomPanel::BottomPanel (SpectralSubtractorAudioProcessor* inProcessor)
    : PanelBase (inProcessor)
{
    addAndMakeVisible (mSlider);
    addAndMakeVisible (mNoiseSpectrumStatus);
    
    mNoiseSpectrumStatus.getFont().setSizeAndStyle (SpectrumStatusFontSize, juce::Font::plain, 1.f, 0.f);
    mNoiseSpectrumStatus.setColour (juce::Label::textColourId, juce::Colours::black);
    mNoiseSpectrumStatus.setEditable (false, false, false);
    
    startTimer(250);
}

BottomPanel::~BottomPanel() {
    stopTimer();
}

void BottomPanel::resized()
{
    int width = getWidth();
    int height = getHeight();
    float sliderSize = (float) width / 3;
    
    mSlider.setBounds ((width * 0.5f) - (sliderSize * 0.5f), (height * 0.5f) - sliderSize, sliderSize, sliderSize);
    mNoiseSpectrumStatus.setBounds (0.f,
                                    height - mNoiseSpectrumStatus.getFont().getHeight(),
                                    width,
                                    mNoiseSpectrumStatus.getFont().getHeight());
}

void BottomPanel::paint (Graphics& g)
{
    // We override PanelBase's paint() function but can still call it within BottomPanel's paint() function.
    PanelBase::paint (g);

    paintComponentLabel (g, &mSlider);
}

void BottomPanel::timerCallback()
{
    const juce::ScopedLock lock (mProcessor->statusMessageMutex);
    mNoiseSpectrumStatus.setText (mProcessor->getStatusMessage(), juce::dontSendNotification);
}
