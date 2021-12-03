/*
  ==============================================================================

    Filter.h
    Created: 13 May 2020 9:32:59am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "STFT.h"
#include "HeapBlockWrapper.h"

//==============================================================================

template <typename FloatType>
class SpectralSubtractor : public STFT<FloatType>
{
public:
    //======================================
    
    SpectralSubtractor ()
    {}
    
    ~SpectralSubtractor() {}
    
    // Do not call this from the audio callback thread!
    void reset (const int newFFTSize)
    {
        std::lock_guard<audio_spin_mutex> lock (STFT<FloatType>::mSpinMutex);
        mNoiseSpectrum.calloc (newFFTSize);
    }
    
    void setSubtractionStrength (std::atomic<float>* subtractionStrength)
    {
        mSubtractionStrength = subtractionStrength;
        jassert (mSubtractionStrength);
    }
    
    const juce::String getNoiseSpectrumAsString()
    {
        return mNoiseSpectrum.toString();
    }
    
    // Replaces the old noise spectrum with a new noise spectrum.
    void loadNoiseSpectrum (const HeapBlock<float>& newNoiseSpectrum)
    {
        // TODO: add a saftey check to ensure mNoiseSpectrum and newNoiseSpectrum have the same element type?
        
        std::memcpy (mNoiseSpectrum.get(), newNoiseSpectrum, mNoiseSpectrum.size() * sizeof (float));
    }
    
    void loadNoiseSpectrumFromString (const juce::String& newNoiseSpectrumAsString)
    {
        mNoiseSpectrum.allocateFromString (newNoiseSpectrumAsString);
    }
    
private:
    HeapBlockWrapper<FloatType> mNoiseSpectrum;             // Holds the average magnitude spectrum of the noise signal
    std::atomic<float>* mSubtractionStrength = nullptr;
    
    void processMagAndPhase (int index, FloatType& magnitude, FloatType& phase) override
    {
        magnitude -= (*mSubtractionStrength) * (mNoiseSpectrum.get())[index];
        magnitude = (magnitude < 0.0f) ? 0.0f : magnitude;
    }
};
