/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

BottomPanel::BottomPanel(ExperimentalFilterAudioProcessor* inProcessor)
:   PanelBase(inProcessor)
{
    setSize(BOTTOM_PANEL_WIDTH, BOTTOM_PANEL_HEIGHT);
    
    mSlider = std::make_unique<ParameterSlider>(mProcessor->parameters,
                                                ParameterID[kParameter_SubtractionStrength],
                                                ParameterLabel[kParameter_SubtractionStrength]);
    
    const int slider_size = 80;
    
    mSlider->setBounds((getWidth() * 0.5) - (slider_size * 0.5),
                       (getHeight() * 0.5) - (slider_size) - 10,
                       slider_size,
                       slider_size);
    
    addAndMakeVisible(*mSlider);
}

BottomPanel::~BottomPanel() {}

void BottomPanel::paint(Graphics& g)
{
    // We override PanelBase's paint() function but can still call it within BottomPanel's paint() function.
    PanelBase::paint(g);

    if (mSlider != nullptr)
    {
        paintComponentLabel(g, mSlider.get());
    }
}
