/*
  ==============================================================================

    NonAutoParameter.h
    Created: 3 Dec 2021 2:00:17pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

namespace {
    const juce::Identifier NonAutoParamIDTag {"id"};
    const juce::Identifier NonAutoParamValueTag {"value"};
}

/**
    NonAutoParameter is somewhat like a non-automatable version of juce::RangedAudioParameter, though it's much simpler.
    Call getValue() to read the value of the parameter in a thread-safe manner.
    The only scenario in which the ValueTree member should be directly modified by the client is when the plugin state is restored.
    All other scenarios that seek to modify the ValueTree member should call setValue(), not juce::ValueTree::setProperty().
 
 How to properly serialize a NonAutoParameter:
    To store a NonAutoParameter as part of the plugin state, call getValueTree() and append the returned ValueTree to an apvts.
    
    Restoring a NonAutoParameter from a saved plugin state is bit more tricky.
    Because ValueTrees are reference counted, we can't simply call apvts.replaceState().
    If we call apvts.replaceState(), then the apvts loses its connection to a NonAutoParameter's ValueTree member.
    To maintain the connection between an apvts and a NonAutoParameter's ValueTree member,
    copy properties over from the corresponding ValueTree in the saved state.
    Example: apvtsNonAutoParamValueTree.copyPropertiesFrom (savedStateNonAutoParamValueTree, nullptr);
*/
template<typename Type>
class NonAutoParameter : private juce::ValueTree::Listener
{
public:
    NonAutoParameter (const juce::String& parameterID, const juce::String& parameterName, Type defaultValue)
        : mParameterID (parameterID),
          mParameterName (parameterName)
    {
        mParameter.addListener (this);
        mParameter.setProperty (NonAutoParamIDTag, parameterID, nullptr);
        setValue (defaultValue);
    }
    
    ~NonAutoParameter()
    {
        mParameter.removeListener (this);
        
        #if __cpp_lib_atomic_is_always_lock_free
         static_assert (std::atomic<Type>::is_always_lock_free,
                        "NonAutoParameter can only be used for lock-free types");
        #endif
    }
    
    const juce::ValueTree getValueTree()
    {
        return mParameter;
    }
    
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
        mParameter.setProperty (NonAutoParamValueTag, newValue, nullptr);
    }
    
    const juce::var getValue()
    {
        return mAtomicValue.load();
    }
        
private:
    juce::ValueTree mParameter {"NON_AUTO_PARAM"};
    std::atomic<Type> mAtomicValue {Type (0)};
    const juce::String mParameterID;
    const juce::String mParameterName;
    
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override
    {
        mAtomicValue.store (static_cast<Type> (mParameter.getProperty (NonAutoParamValueTag)));
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutoParameter)
};

/**
    NonAutoParameterChoice is like a non-automatable version of juce::AudioParamterChoice.
*/
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

/**
    ComboBoxNonAutoParameterAttachment is like juce::ComboBoxParameterAttachment but for non-automatable parameters.
*/
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
