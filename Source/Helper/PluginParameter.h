/*
  ==============================================================================

    PluginParameter.h
    Created: 1 Dec 2021 10:29:19am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class PluginParameterChoice : public juce::AudioParameterChoice
{
public:
    PluginParameterChoice (const juce::String& parameterID,
                           const juce::String& parameterName,
                           const juce::StringArray& choices,
                           int defaultItemIndex,
                           const juce::String& parameterLabel = juce::String(),
                           std::function<juce::String (int index, int maximumStringLength)> stringFromIndex = nullptr,
                           std::function<int (const juce::String& text)> indexFromString = nullptr,
                           const std::function<void (const int)> callback = nullptr)
        : juce::AudioParameterChoice (parameterID, parameterName, choices, defaultItemIndex, parameterLabel, stringFromIndex, indexFromString),
          onValueChange (callback)
    {}
    
private:
    void valueChanged (int newValue) override
    {
        onValueChange (newValue);
    }
    
    std::function<void (int)> onValueChange;
};
