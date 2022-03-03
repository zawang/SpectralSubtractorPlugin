/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

#include "../Helper/HelperFunctions.h"

BottomPanel::BottomPanel (SpectralSubtractorAudioProcessor* inProcessor)
    : PanelBase (inProcessor)
{
    addAndMakeVisible (mSlider);
}

BottomPanel::~BottomPanel() {}

void BottomPanel::resized()
{
    int width = getWidth();
    int height = getHeight();
    float sliderSize = (float) width / 3;
    
    mSlider.setBounds ((width * 0.5f) - (sliderSize * 0.5f), (height * 0.5f) - sliderSize, sliderSize, sliderSize);
}

void BottomPanel::paint (Graphics& g)
{
    // We override PanelBase's paint() function but can still call it within BottomPanel's paint() function.
    PanelBase::paint (g);

    paintComponentLabel (g, &mSlider);
}
