/*
  ==============================================================================

    ParameterSlider.cpp
    Created: 15 Aug 2019 1:58:15pm
    Author:  Zachary Wang

  ==============================================================================
*/

#include "ParameterSlider.h"

ParameterSlider::ParameterSlider (AudioProcessorValueTreeState& state,
                                  const String& parameterID,
                                  const String& parameterLabel)
    : juce::Slider (parameterLabel)
{
    setSliderStyle (SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, false, 0, 0);
    
    
    // We don't even need to set up the slider's value range. This is done automatically by the SliderAttachment class.
    // All we need to do is pass the attachment constructor the APVTS, the parameter ID and the Slider object that it should attach to.
    mAttachment =
    std::make_unique<SliderAttachment>(state,
                                       parameterID,
                                       *this);
}

ParameterSlider::~ParameterSlider() {}
