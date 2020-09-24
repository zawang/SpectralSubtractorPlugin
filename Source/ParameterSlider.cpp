/*
  ==============================================================================

    ParameterSlider.cpp
    Created: 15 Aug 2019 1:58:15pm
    Author:  Zachary Wang

  ==============================================================================
*/

#include "ParameterSlider.h"

ParameterSlider::ParameterSlider(AudioProcessorValueTreeState& state,
                                       const String& parameterID,
                                       const String& parameterLabel)
:   juce::Slider(parameterLabel) {
    setSliderStyle(SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, false, 0, 0);
    setRange(0.f, 5.f, 0.001f);
    
    mAttachment =
    std::make_unique<SliderAttachment>(state,
                                       parameterID,
                                       *this);
}

ParameterSlider::~ParameterSlider() {
    
}
