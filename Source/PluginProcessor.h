/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpectralSubtractor.h"
#include "Parameters.h"
#include "IDs.h"
#include "NonAutoParameter.h"

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
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void setParams();
    NonAutoParameterChoice& getNonAutoParameterWithID (const String& parameterID);
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void prepareAndResetSpectralSubtractor();
    void loadNoiseSpectrum (HeapBlock<float>& tempNoiseSpectrum);
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, juce::Identifier("SpectralSubtractor"), createParameterLayout()};
    
    juce::AudioFormatManager* getFormatManager() { return mFormatManager.get(); }
    
    int getFFTSize() { return FFTSize[mFFTSizeParam->getIndex()]; }
    int getWindowOverlap() { return WindowOverlap[mWindowOverlapParam->getIndex()]; }

private:
    std::atomic<float>* mSubtractionStrengthParam = nullptr;
    std::unique_ptr<NonAutoParameterChoice> mFFTSizeParam;
    juce::AudioParameterChoice* mWindowOverlapParam = nullptr;
    juce::AudioParameterChoice* mWindowParam = nullptr;
    std::map<juce::String, NonAutoParameterChoice*> mNonAutoParams;
    
    SpectralSubtractor<float> mSpectralSubtractor;
    std::unique_ptr<juce::AudioFormatManager> mFormatManager;
    
    juce::UnitTestRunner mUnitTestRunner;

    void heapBlockToArray (HeapBlock<float>& heapBlock, Array<var>& array);
    void arrayToHeapBlock (Array<var>& array, HeapBlock<float>& heapBlock);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSubtractorAudioProcessor)
};
