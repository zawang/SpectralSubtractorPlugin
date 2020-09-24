/*
  ==============================================================================

    ParameterSlider.h
    Created: 15 Aug 2019 1:58:15pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

// All this class does is take JUCE’s default Slider and associate it with the AudioProcessorValueTreeState that exists in our processor.

class ParameterSlider
:   public Slider {
public:
    ParameterSlider(AudioProcessorValueTreeState& state,
                       const String& parameterID,
                       const String& parameterLabel);
    ~ParameterSlider();
    
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    
private:
    std::unique_ptr<SliderAttachment> mAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterSlider);
};
