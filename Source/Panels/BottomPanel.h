/*
  ==============================================================================

    BottomPanel.h
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "../Helper/ParameterComponent.h"
#include "../Helper/Parameters.h"
#include "../Plugin/PluginProcessor.h"

class BottomPanel
:   public PanelBase,
    public juce::Timer
{
public:
    BottomPanel (SpectralSubtractorAudioProcessor* inProcessor);
    ~BottomPanel();
    
    void resized() override;
    void paint (Graphics& g) override;
    
    void timerCallback() override;
    
private:
    ParameterSlider mSlider {mProcessor->apvts, ParameterID[kParameter_SubtractionStrength], ParameterLabel[kParameter_SubtractionStrength]};
    juce::Label mNoiseSpectrumStatus {"NoiseSpectrumStatus", ""};
};
