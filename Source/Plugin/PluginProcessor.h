/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../DSP/SpectralSubtractor.h"
#include "../Helper/IDs.h"

//==============================================================================
/**
*/

class NonAutoParameterChoice;

class SpectralSubtractorAudioProcessor  : public AudioProcessor, public juce::Thread
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

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void attachSubTrees();
    NonAutoParameterChoice* getNonAutoParameterWithID (const String& parameterID);
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, juce::Identifier ("SpectralSubtractor"), createParameterLayout()};
    
    const juce::AudioFormatManager* getFormatManager() const;
    
    const juce::String& getStatusMessage() const;
    
    void run() override;
    
    bool requiresUpdate {true};
    juce::CriticalSection backgroundMutex;
    juce::CriticalSection pathMutex;
    juce::CriticalSection statusMessageMutex;
    juce::String chosenPath;

private:
    std::atomic<float>* mSubtractionStrengthParam {nullptr};
    std::unique_ptr<NonAutoParameterChoice> mFFTSizeParam;
    std::unique_ptr<NonAutoParameterChoice> mWindowOverlapParam;
    std::unique_ptr<NonAutoParameterChoice> mWindowParam;
    std::map<juce::String, NonAutoParameterChoice*> mNonAutoParams;
    
    juce::ValueTree mAudioDataTree {IDs::AudioData};
    
    SpectralSubtractor<float> mSpectralSubtractor;
    juce::AudioBuffer<float> mNoiseBuffer;
    juce::HeapBlock<float> mTempNoiseSpectrum;
    std::unique_ptr<juce::AudioFormatReader> mReader;
    std::unique_ptr<juce::AudioFormatManager> mFormatManager;
    juce::String mNoiseFileName {""};
    juce::String mStatusMessage {""};
    
    juce::UnitTestRunner mUnitTestRunner;
    
    //==============================================================================
    // Variables and functions for the background thread

    // For creating spectrogram
    std::unique_ptr<juce::dsp::FFT> mBG_FFT;
    int mBG_overlap;
    size_t mBG_HopSize;
    int mBG_WindowIndex;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> mBG_Window;

    void checkForPathToOpen();
    void checkIfSpectralSubtractorNeedsUpdate();
    void updateBackgroundThread();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSubtractorAudioProcessor)
};
