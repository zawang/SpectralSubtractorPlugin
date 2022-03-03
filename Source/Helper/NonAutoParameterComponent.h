/*
  ==============================================================================

    NonAutoParameterComponent.h
    Created: 3 Dec 2021 10:48:20pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "NonAutoParameter.h"

/**
    Each component wrapper class maintains a connection with a non-automatable parameter.
    This easily allows us to ensure that an attachment has the same lifetime as its corresponding component.
*/

class NonAutoParameterComboBox
    : public juce::ComboBox
{
public:
    NonAutoParameterComboBox (NonAutoParameterChoice& parameter)
          : juce::ComboBox (parameter.getParameterName())
    {
        addItemList (parameter.mChoices, 1);
        setSelectedItemIndex (parameter.getIndex());
        
        mAttachment = std::make_unique<ComboBoxNonAutoParameterAttachment> (parameter, *this);
    }
    
    ~NonAutoParameterComboBox() {}
        
private:
    std::unique_ptr<ComboBoxNonAutoParameterAttachment> mAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutoParameterComboBox)
};
