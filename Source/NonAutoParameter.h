/*
  ==============================================================================

    NonAutoParameter.h
    Created: 3 Dec 2021 2:00:17pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

template<typename T>
class NonAutoParameter
{
public:
    NonAutoParameter (const juce::String& parameterID, const juce::String& parameterName, T defaultValue)
        : mParameterID (parameterID),
          mParameterName (parameterName)
    {
        mParameter.setProperty (idTag, parameterID, nullptr);
        setValue (defaultValue);
    }
    
    ~NonAutoParameter() {}
    
    const juce::String& getParameterID()
    {
        return mParameterID;
    }
    
    const juce::String& getParameterName()
    {
        return mParameterName;
    }
    
    void setValue (const juce::var& newValue)
    {
        mParameter.setProperty (valueTag, newValue, nullptr);
        mAtomicValue.store (static_cast<T> (newValue));
    }
    
    const juce::var getValue()
    {
        return mAtomicValue.load();
    }
        
private:
    juce::ValueTree mParameter {"NON_AUTO_PARAM"};
    std::atomic<T> mAtomicValue {T (0)};
    const juce::String mParameterID;
    const juce::String mParameterName;
    
    const juce::Identifier idTag {"id"};
    const juce::Identifier valueTag {"value"};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutoParameter)
};

class NonAutoParameterChoice : public NonAutoParameter<int>
{
public:
    NonAutoParameterChoice (const juce::String& parameterID, const juce::String& parameterName, const StringArray& choices, int defaultItemIndex)
        : NonAutoParameter (parameterID, parameterName, defaultItemIndex),
          mChoices (choices)
    {}
    
    ~NonAutoParameterChoice() {}
    
    int getIndex()
    {
        return getValue();
    }
    
    const juce::StringArray mChoices;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutoParameterChoice)
};

class ComboBoxNonAutoParameterAttachment : private juce::ComboBox::Listener
{
public:
    ComboBoxNonAutoParameterAttachment (NonAutoParameter<int>& parameter,
                                        juce::ComboBox& comboBox)
          : mParameter (parameter),
            mComboBox (comboBox)
    {
        mComboBox.addListener (this);
    }
    
    ~ComboBoxNonAutoParameterAttachment()
    {
        mComboBox.removeListener (this);
    }
    
    void comboBoxChanged (juce::ComboBox*) override
    {
        mParameter.setValue (mComboBox.getSelectedItemIndex());
    }
        
private:
    NonAutoParameter<int>& mParameter;
    juce::ComboBox& mComboBox;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboBoxNonAutoParameterAttachment)
};
