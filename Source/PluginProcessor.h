/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioFunctions.h"
#include "Filter.h"
#include "Parameters.h"

//==============================================================================
/**
*/
class SpectralSubtractorAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    SpectralSubtractorAudioProcessor();
    ~SpectralSubtractorAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void loadNewNoiseSpectrum(HeapBlock<float>& tempNoiseSpectrum);
    
    // Contains a ValueTree that is used to manage the processor's entire state.
    // Adding parameters to an APVTS automatically adds them to the attached processor too.
    AudioProcessorValueTreeState parameters;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSubtractorAudioProcessor)
    
    void initializeDSP();
    void heapBlockToArray(HeapBlock<float>& heapBlock, Array<var>& array);
    void arrayToHeapBlock(Array<var>& array, HeapBlock<float>& heapBlock);
    
    // varArrayToDelimitedString and delimitedStringToVarArray are taken from juce_ValueWithDefault.h
    String varArrayToDelimitedString(const Array<var>& input);
    Array<var> delimitedStringToVarArray (StringRef input);
    
    Filter mFilter;
    HeapBlock<float> mNoiseSpectrum;                                    // Holds the average magnitude spectrum of the noise signal
    std::atomic<float>* mSubtractionStrengthParameter = nullptr;        // The amount of the noise spectrum to remove
};
