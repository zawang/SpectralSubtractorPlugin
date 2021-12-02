/*
  ==============================================================================

    BottomPanel.h
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "ParameterComponent.h"
#include "HelperFunctions.h"

class BottomPanel
:   public PanelBase
{
public:
    BottomPanel (SpectralSubtractorAudioProcessor* inProcessor);
    ~BottomPanel();
    
    void resized() override;
    void paint (Graphics& g) override;
    
private:
    ParameterSlider mSlider {mProcessor->apvts, ParameterID[kParameter_SubtractionStrength], ParameterLabel[kParameter_SubtractionStrength]};
};
