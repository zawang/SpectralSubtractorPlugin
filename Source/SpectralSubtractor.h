/*
  ==============================================================================

    Filter.h
    Created: 13 May 2020 9:32:59am
    Author:  Zachary Wang

  ==============================================================================
*/

// https://github.com/juandagilc/Audio-Effects

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
    
    void setSubtractionStrength (std::atomic<float>* subtractionStrength)
    {
        mSubtractionStrength = subtractionStrength;
        jassert (mSubtractionStrength);
    }
    
    // Replaces the old noise spectrum with a new noise spectrum.
    void loadNoiseSpectrum (HeapBlock<float>& tempNoiseSpectrum)
    {
        // TODO: add a saftey check to ensure mNoiseSpectrum and tempNoiseSpectrum have the same element type?
        
        std::memcpy (mNoiseSpectrum.get(), tempNoiseSpectrum, mNoiseSpectrum.size() * sizeof (float));
    }

    HeapBlockWrapper<FloatType> mNoiseSpectrum;             // Holds the average magnitude spectrum of the noise signal
    
private:
    std::atomic<float>* mSubtractionStrength = nullptr;
    
    void processMagAndPhase (int index, FloatType& magnitude, FloatType& phase) override
    {
        magnitude -= (*mSubtractionStrength) * (mNoiseSpectrum.get())[index];
        magnitude = (magnitude < 0.0f) ? 0.0f : magnitude;
    }
};
