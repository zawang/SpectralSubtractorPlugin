#pragma once

#include "JuceHeader.h"
#define FloatType float

void getAudioFile (juce::AudioBuffer<FloatType>& buffer, const juce::File& file)
{
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader;
    reader.reset (formatManager.createReaderFor (file));
    if (reader.get() != nullptr)
    {
        buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), 0, buffer.getNumSamples());
    }
}
