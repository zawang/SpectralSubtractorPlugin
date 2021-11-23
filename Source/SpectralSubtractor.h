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

class SpectralSubtractor : public STFT
{
public:
    //======================================
    
    SpectralSubtractor (juce::HeapBlock<float>& noiseSpectrum)
        : mNoiseSpectrum (noiseSpectrum)
    {}
    
    ~SpectralSubtractor() {}
    
    void setSubtractionStrength (std::atomic<float>* subtractionStrength)
    {
        mSubtractionStrength = subtractionStrength;
        jassert (mSubtractionStrength);
    }
    
private:
    juce::HeapBlock<float>& mNoiseSpectrum;
    std::atomic<float>* mSubtractionStrength = nullptr;
    
    void processMagAndPhase (int index, float& magnitude, float& phase) override
    {
        magnitude -= (*mSubtractionStrength) * mNoiseSpectrum[index];
        magnitude = (magnitude < 0.0f) ? 0.0f : magnitude;
    }
};
