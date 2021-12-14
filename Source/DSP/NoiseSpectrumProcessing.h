/*
  ==============================================================================

    ProcessingNoiseSpectrumThread.h
    Created: 4 Oct 2021 7:59:13am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "../Plugin/PluginProcessor.h"

// One audio channel of FFT data over time, really 2-dimensional
template <typename FloatType>
using Spectrogram = std::vector<HeapBlock<FloatType>>;

// Compute the stft on each channel of signal and average the results to produce one spectrogram.
template <typename FloatType>
inline void makeSpectrogram (Spectrogram<FloatType>& spectrogram,
                             juce::AudioBuffer<FloatType>* signal,
                             juce::dsp::FFT& fft,
                             size_t hopSize,
                             juce::dsp::WindowingFunction<FloatType>& window)
{
    const size_t dataCount = signal->getNumSamples();
    
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
    
    int numChannels = signal->getNumChannels();
    FloatType inverseNumChannels = static_cast<FloatType> (1) / numChannels;
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        // fFft works on the data in place, and needs twice as much space as the input size.
        std::vector<FloatType> fftBuffer (fftSize * 2UL);
    
        // While data remains
        const FloatType* signalData = signal->getReadPointer (channel, 0);
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
inline void computeAverageSpectrum (HeapBlock<FloatType>& magSpectrum, Spectrogram<FloatType>& spectrogram, int fftSize)
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

// Thread that calculates a noise file's average spectrum and replace's the audio processor's old noise spectrum with the one it just calculated.
template <typename FloatType>
class NoiseSpectrumProcessingThread : public juce::ThreadWithProgressWindow
{
public:
    NoiseSpectrumProcessingThread (SpectralSubtractorAudioProcessor* inProcessor, int fftSize, int overlap)
        : juce::ThreadWithProgressWindow ("Preparing noise spectrum...", true, true, 10000),
          mProcessor (inProcessor),
          mFFT (static_cast<int> (std::log2 (fftSize))),
          mWindow (static_cast<size_t> (fftSize + 1), juce::dsp::WindowingFunction<FloatType>::hann, false)
    {
        if (overlap != 0)
            mHopSize = mFFT.getSize() / overlap;
        
        jassert (mHopSize <= mFFT.getSize());
        
        DBG ("Background thread FFT size: " << mFFT.getSize());
        DBG ("Background thread hop size: " << mHopSize);
        setStatusMessage ("Getting ready...");
    }

    void run() override
    {
        mProcessor->suspendProcessing (true);
          
        setProgress (-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar...
        
        setStatusMessage ("Computing STFT of noise signal...");
        
        if (threadShouldExit()) return; // must check this as often as possible, because this is how we know if the user's pressed 'cancel'
        
        Spectrogram<FloatType> noiseSpectrogram;
        makeSpectrogram (noiseSpectrogram, mProcessor->mNoiseBuffer.get(), mFFT, mHopSize, mWindow);
        
        if (threadShouldExit()) return;

        setStatusMessage ("Computing noise spectrum...");
        
        computeAverageSpectrum (mTempNoiseSpectrum, noiseSpectrogram, mFFT.getSize());
        
        if (threadShouldExit()) return;
        
        setStatusMessage ("Almost finished...");
        
        mProcessor->loadNoiseSpectrum (mTempNoiseSpectrum);
        
        mProcessor->suspendProcessing (false);
    }

    // This method gets called on the message thread once our thread has finished..
    void threadComplete (bool userPressedCancel) override
    {
        mProcessor->suspendProcessing (false);   // in case it hasn't been done yet
        
        juce::String messageString;
        if (userPressedCancel)
            messageString = "Operation canceled!";
        else
            messageString = "Successfully loaded noise spectrum!";
        
        juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                           .withIconType (MessageBoxIconType::InfoIcon)
                                           .withTitle ("Progress window")
                                           .withMessage (messageString),
                                           nullptr);

        // ..and clean up by deleting our thread object..
        delete this;
    }

private:
    SpectralSubtractorAudioProcessor* mProcessor;
    HeapBlock<FloatType> mTempNoiseSpectrum;
    
    // For creating spectrogram
    juce::dsp::FFT mFFT;
    size_t mHopSize;
    juce::dsp::WindowingFunction<FloatType> mWindow;
};
