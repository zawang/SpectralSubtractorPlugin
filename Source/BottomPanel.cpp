/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

BottomPanel::BottomPanel(ExperimentalFilterAudioProcessor* inProcessor)
:   PanelBase(inProcessor) {
    setSize(BOTTOM_PANEL_WIDTH, BOTTOM_PANEL_HEIGHT);
}

BottomPanel::~BottomPanel() {
    
}

void BottomPanel::paint(Graphics& g) {
    PanelBase::paint(g);

    if (mSlider) {
        paintComponentLabel(g, mSlider.get());
    }
}

void BottomPanel::setParameterID(int inParameterID) {
    mSlider = std::make_unique<ParameterSlider>(mProcessor->parameters,
                                                ParameterID[inParameterID],
                                                ParameterLabel[inParameterID]);

    const int slider_size = 54;

    mSlider->setBounds((getWidth() * 0.5) - (slider_size * 0.5),
                       (getHeight() * 0.25) - (slider_size * 0.5) - 10,
                       slider_size,
                       slider_size);

    addAndMakeVisible(*mSlider);
}
