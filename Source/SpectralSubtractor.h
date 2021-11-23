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

//==============================================================================

template <typename FloatType>
class SpectralSubtractor : public STFT<FloatType>
{
public:
    //======================================
    
    SpectralSubtractor (juce::HeapBlock<FloatType>& noiseSpectrum)
        : mNoiseSpectrum (noiseSpectrum)
    {}
    
    ~SpectralSubtractor() {}
    
    void setSubtractionStrength (std::atomic<float>* subtractionStrength)
    {
        mSubtractionStrength = subtractionStrength;
        jassert (mSubtractionStrength);
    }
    
private:
    juce::HeapBlock<FloatType>& mNoiseSpectrum;
    std::atomic<float>* mSubtractionStrength = nullptr;
    
    void processMagAndPhase (int index, FloatType& magnitude, FloatType& phase) override
    {
        magnitude -= (*mSubtractionStrength) * mNoiseSpectrum[index];
        magnitude = (magnitude < 0.0f) ? 0.0f : magnitude;
    }
};
