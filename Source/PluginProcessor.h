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
#include "SpectrogramMaker.h"

constexpr auto MINBLOCKSPERWINDOW = 16;

//==============================================================================
/**
*/
class ExperimentalFilterAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    ExperimentalFilterAudioProcessor();
    ~ExperimentalFilterAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill);

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
    
    // Given a noise signal, calculate its spectrogram, then calculate the average spectrum from the spectrogram and store it in mNoiseSpectrum
    void storeNoiseSpectrum(const AudioSampleBuffer& noiseSignal);
    
    AudioFormatManager* getFormatManager() {
        return mFormatManager.get();
    }
    
    AudioSampleBuffer* getNoiseBuffer() {
        return mNoiseBuffer.get();
    }
    
    int* getPosition() {
        return mPosition.get();
    }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExperimentalFilterAudioProcessor)
    // internal
    void initializeDSP();
    
    Filter mFilter;
    SpectrogramMaker mSpectrogramMaker;
//    HeapBlock<dsp::Complex<float>> mNoiseSpectrum;
    
    std::unique_ptr<AudioFormatManager> mFormatManager;    // manages what audio formats are allowed
    std::unique_ptr<AudioSampleBuffer> mNoiseBuffer;       // buffer that holds the noise signal
    
    // mPosition is only useful for the purposes of getNextAudioBlock (not used in the final plugin)
    std::unique_ptr<int> mPosition;
    
//    juce::dsp::FFT mFFT;
//    std::unique_ptr<float> mFileBufferFifo[2][kFFTSize];
//    std::unique_ptr<float> mFileBufferFFTData[2][2 * kFFTSize];
//    std::unique_ptr<int> mFileBufferFifoIndex;
    
//    std::unique_ptr<Filter> mFilter[2];
    
    double phase;
    double phaseDelta;
    double freq;
};
