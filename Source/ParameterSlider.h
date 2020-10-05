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
// This easily allow us to ensure that the attachment class has the same lifetime as the Slider object.

class ParameterSlider
:   public Slider
{
public:
    ParameterSlider(AudioProcessorValueTreeState& state,
                       const String& parameterID,
                       const String& parameterLabel);
    ~ParameterSlider();
    
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    
private:
    // ValueTree objects can have listeners attached to them.
    // This means that the APVTS class can almost automatically connect to sliders and buttons to keep the state of the UI and the processor up-to-date in a thread safe manner.
    // Therefore, there's no need for the Slider::Listener class in order to respond to slider interaction.
    std::unique_ptr<SliderAttachment> mAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterSlider);
};
