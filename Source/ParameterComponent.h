/*
  ==============================================================================

    ParameterSlider.h
    Created: 15 Aug 2019 1:58:15pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

/**
    These classes take JUCE components (Slider, ComboBox) and associate them with an APVTS.
    This easily allows us to ensure that a JUCE parameter attachment has the same lifetime as its corresponding component.
    The parmater attachment classes allow the APVTS class to automatically connect to sliders and buttons to keep the state of the UI and the processor up-to-date in a thread safe manner.
*/

class ParameterSlider
:   public juce::Slider
{
public:
    ParameterSlider (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& parameterID,
                     const juce::String& parameterLabel)
        : juce::Slider (parameterLabel)
    {
        setSliderStyle (SliderStyle::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, false, 0, 0);
        
        mAttachment = std::make_unique<SliderAttachment> (apvts, parameterID, *this);
    }
    
    ~ParameterSlider() {}
    
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
private:
    std::unique_ptr<SliderAttachment> mAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

class ParameterComboBox
    : public juce::ComboBox
{
public:
    ParameterComboBox (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& parameterID,
                       const juce::String& parameterLabel)
          : juce::ComboBox (parameterLabel)
    {
        auto parameter = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (parameterID));
        jassert (parameter);
        addItemList (parameter->choices, 1);
        
        mAttachment = std::make_unique<ComboBoxAttachment> (apvts, parameterID, *this);
    }
    
    ~ParameterComboBox() {}
    
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    
private:
    std::unique_ptr<ComboBoxAttachment> mAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterComboBox)
};
