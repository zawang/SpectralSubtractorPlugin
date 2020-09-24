/*
  ==============================================================================

    BottomPanel.h
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "ParameterSlider.h"
#include "HelperFunctions.h"

class BottomPanel
:   public PanelBase {
public:
    BottomPanel(ExperimentalFilterAudioProcessor* inProcessor);
    ~BottomPanel();
    
    void paint(Graphics& g) override;
    
    void setParameterID(int inParameterID);

private:
    std::unique_ptr<ParameterSlider> mSlider;
};
