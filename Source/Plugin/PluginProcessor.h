/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../DSP/SpectralSubtractor.h"
#include "../Helper/Parameters.h"
#include "../Helper/IDs.h"
#include "../Helper/NonAutoParameter.h"
#include "../Helper/AudioSpinMutex.h"

// One audio channel of FFT data over time, really 2-dimensional
template <typename FloatType>
using Spectrogram = std::vector<HeapBlock<FloatType>>;

// Compute the stft on each channel of signal and average the results to produce one spectrogram.
template <typename FloatType>
inline void makeSpectrogram (Spectrogram<FloatType>& spectrogram,
                             juce::AudioBuffer<FloatType>& signal,
                             juce::dsp::FFT& fft,
                             size_t hopSize,
                             juce::dsp::WindowingFunction<FloatType>& window)
{
    const size_t dataCount = signal.getNumSamples();
    
    // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
    ptrdiff_t fftSize = fft.getSize();
    
    // Calculate number of hops
    ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / hopSize);
    
    // Initialize spectrogram
    spectrogram.resize (numHops + 1);
    for (int i = 0; i < spectrogram.size(); ++i)
    {
        spectrogram[i].realloc (fftSize);
        spectrogram[i].clear (fftSize);
    }
    
    // We will discard the negative frequency bins, but leave the center bin.
    size_t numRows = 1UL + (fftSize / 2UL);
    
    int numChannels = signal.getNumChannels();
    FloatType inverseNumChannels = static_cast<FloatType> (1) / numChannels;
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        // fFft works on the data in place, and needs twice as much space as the input size.
        std::vector<FloatType> fftBuffer (fftSize * 2UL);
    
        // While data remains
        const FloatType* signalData = signal.getReadPointer (channel, 0);
        for (int i = 0; i < numHops; ++i)
        {
            // Prepare segment to perform FFT on.
            std::memcpy (fftBuffer.data(), signalData, fftSize * sizeof (FloatType));
            
            // Apply the windowing to the chunk of samples before passing it to the FFT.
            window.multiplyWithWindowingTable (fftBuffer.data(), fftSize);
            
            // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
            fft.performFrequencyOnlyForwardTransform (fftBuffer.data());
            
            // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
            juce::FloatVectorOperations::addWithMultiply (spectrogram[i].get(), fftBuffer.data(), inverseNumChannels, numRows);
            
            // Next chunk
            signalData += hopSize;
        }
    }
}

// Calculates the average spectrum from a given spectrogram
template <typename FloatType>
inline void computeAverageSpectrum (juce::HeapBlock<FloatType>& magSpectrum, Spectrogram<FloatType>& spectrogram, int fftSize)
{
    magSpectrum.realloc (fftSize);
    magSpectrum.clear (fftSize);
    
    size_t numColumns = spectrogram.size();
    FloatType inverseNumColumns = static_cast<FloatType> (1) / numColumns;

    // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
    int numBins = fftSize / 2 + 1;
    for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
    {
        juce::FloatVectorOperations::addWithMultiply (magSpectrum.get(), spectrogram[freqColumn].get(), inverseNumColumns, numBins);
    }
}

//==============================================================================
/**
*/
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
    NonAutoParameterChoice& getNonAutoParameterWithID (const String& parameterID);
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, juce::Identifier ("SpectralSubtractor"), createParameterLayout()};
    
    juce::AudioFormatManager* getFormatManager() { return mFormatManager.get(); }
    
    int getFFTSize() { return FFTSize[mFFTSizeParam->getIndex()]; }
    int getWindowOverlap() { return WindowOverlap[mWindowOverlapParam->getIndex()]; }

    void loadNoiseBuffer (const juce::File& noiseFile);
    
    void run() override;
    
    bool mRequiresUpdate {true};
    juce::CriticalSection mBackgroundMutex;

private:
    std::atomic<float>* mSubtractionStrengthParam = nullptr;
    std::unique_ptr<NonAutoParameterChoice> mFFTSizeParam = nullptr;
    std::unique_ptr<NonAutoParameterChoice> mWindowOverlapParam = nullptr;
    std::unique_ptr<NonAutoParameterChoice> mWindowParam = nullptr;
    std::map<juce::String, NonAutoParameterChoice*> mNonAutoParams;
    
    juce::ValueTree mAudioDataTree {IDs::AudioData};
    
    SpectralSubtractor<float> mSpectralSubtractor;
    juce::AudioBuffer<float> mNoiseBuffer;
    juce::HeapBlock<float> mTempNoiseSpectrum;
    std::unique_ptr<juce::AudioFormatReader> mReader;
    std::unique_ptr<juce::AudioFormatManager> mFormatManager;
    
    audio_spin_mutex mSpinMutex;
    
    juce::UnitTestRunner mUnitTestRunner;
    
    //==============================================================================
    // Variables and functions for the background thread

    // For creating spectrogram
    std::unique_ptr<juce::dsp::FFT> mBG_FFT;
    int mBG_overlap;
    size_t mBG_HopSize;
    int mBG_WindowIndex;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> mBG_Window;

    void checkIfSpectralSubtractorNeedsUpdate();
    void updateBackgroundThread();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSubtractorAudioProcessor)
};
