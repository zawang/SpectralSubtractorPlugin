/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

BottomPanel::BottomPanel(SpectralSubtractorAudioProcessor* inProcessor)
:   PanelBase(inProcessor)
{
    mSlider = std::make_unique<ParameterSlider>(mProcessor->parameters,
                                                ParameterID[kParameter_SubtractionStrength],
                                                ParameterLabel[kParameter_SubtractionStrength]);
    addAndMakeVisible(*mSlider);
}

BottomPanel::~BottomPanel() {}

void BottomPanel::resized()
{
    auto width = getWidth();
    auto height = getHeight();
    auto sliderSize = width / 3;
    
    mSlider->setBounds ((width * 0.5) - (sliderSize * 0.5), (height * 0.5) - sliderSize, sliderSize, sliderSize);
}

void BottomPanel::paint(Graphics& g)
{
    // We override PanelBase's paint() function but can still call it within BottomPanel's paint() function.
    PanelBase::paint(g);

    if (mSlider != nullptr)
    {
        paintComponentLabel(g, mSlider.get());
    }
}
