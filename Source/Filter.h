/*
  ==============================================================================

    Filter.h
    Created: 13 May 2020 9:32:59am
    Author:  Zachary Wang

  ==============================================================================
*/

// https://github.com/juandagilc/Audio-Effects

#pragma once

#include "JuceHeader.h"
#include "AudioFunctions.h"

//==============================================================================

class Filter
{
public:
    //======================================
    
    Filter() : mNumChannels (1) {}
    
    virtual ~Filter() {}
    
    //======================================
    
    void setup (const int numInputChannels)
    {
        mNumChannels = (numInputChannels > 0) ? numInputChannels : 1;
    }
    
    void updateParameters (const int newFftSize, const int newOverlap, const int newWindowType)
    {
        updateFftSize (newFftSize);
        updateHopSize (newOverlap);
        updateWindow (newWindowType);
    }
    
    //======================================
    
    void processBlock (juce::AudioBuffer<float>& block, HeapBlock<float>& noiseSpectrum, float subtractionStrength)
    {
        mNumSamples = block.getNumSamples();
        
        for (int channel = 0; channel < mNumChannels; ++channel)
        {
            float* channelData = block.getWritePointer (channel);
            
            mCurrentInputBufferWritePosition = mInputBufferWritePosition;
            mCurrentOutputBufferWritePosition = mOutputBufferWritePosition;
            mCurrentOutputBufferReadPosition = mOutputBufferReadPosition;
            mCurrentSamplesSinceLastFFT = mSamplesSinceLastFFT;
            
            for (int sample = 0; sample < mNumSamples; ++sample)
            {
                const float inputSample = channelData[sample];
                mInputBuffer.setSample (channel, mCurrentInputBufferWritePosition, inputSample);
                if (++mCurrentInputBufferWritePosition >= mInputBufferLength)
                    mCurrentInputBufferWritePosition = 0;
                
                channelData[sample] = mOutputBuffer.getSample (channel, mCurrentOutputBufferReadPosition);
                mOutputBuffer.setSample (channel, mCurrentOutputBufferReadPosition, 0.0f);
                if (++mCurrentOutputBufferReadPosition >= mOutputBufferLength)
                    mCurrentOutputBufferReadPosition = 0;
                
                if (++mCurrentSamplesSinceLastFFT >= mHopSize)
                {
                    mCurrentSamplesSinceLastFFT = 0;
                    
                    analysis (channel);
                    modification(noiseSpectrum, subtractionStrength);
                    synthesis (channel);
                }
            }
        }
        
        mInputBufferWritePosition = mCurrentInputBufferWritePosition;
        mOutputBufferWritePosition = mCurrentOutputBufferWritePosition;
        mOutputBufferReadPosition = mCurrentOutputBufferReadPosition;
        mSamplesSinceLastFFT = mCurrentSamplesSinceLastFFT;
    }
    
private:
    //======================================
    
    void updateFftSize (const int newFftSize)
    {
        mFftSize = newFftSize;
        mFft = std::make_unique<dsp::FFT>(log2 (mFftSize));
        
        mInputBufferLength = mFftSize;
        mInputBuffer.clear();
        mInputBuffer.setSize (mNumChannels, mInputBufferLength);
        
        mOutputBufferLength = mFftSize;
        mOutputBuffer.clear();
        mOutputBuffer.setSize (mNumChannels, mOutputBufferLength);
        
        mFftWindow.realloc (mFftSize);
        mFftWindow.clear (mFftSize);
        
        mTimeDomainBuffer.realloc (mFftSize);
        mTimeDomainBuffer.clear (mFftSize);
        
        mFrequencyDomainBuffer.realloc (mFftSize);
        mFrequencyDomainBuffer.clear (mFftSize);
        
        mInputBufferWritePosition = 0;
        mOutputBufferWritePosition = 0;
        mOutputBufferReadPosition = 0;
        mSamplesSinceLastFFT = 0;
    }
    
    void updateHopSize (const int newOverlap)
    {
        mOverlap = newOverlap;
        if (mOverlap != 0)
        {
            mHopSize = mFftSize / mOverlap;
            mOutputBufferWritePosition = mHopSize % mOutputBufferLength;
        }
    }
    
    void updateWindow (const int newWindowType)
    {
        switch (newWindowType)
        {
            case kWindowTypeRectangular:
            {
                for (int sample = 0; sample < mFftSize; ++sample)
                    mFftWindow[sample] = 1.0f;
                break;
            }
            case kWindowTypeBartlett:
            {
                for (int sample = 0; sample < mFftSize; ++sample)
                    mFftWindow[sample] = 1.0f - fabs (2.0f * (float) sample / (float) (mFftSize - 1) - 1.0f);
                break;
            }
            case kWindowTypeHann:
            {
                for (int sample = 0; sample < mFftSize; ++sample)
                    mFftWindow[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (float) sample / (float) (mFftSize - 1));
                break;
            }
            case kWindowTypeHamming:
            {
                for (int sample = 0; sample < mFftSize; ++sample)
                    mFftWindow[sample] = 0.54f - 0.46f * cosf (2.0f * M_PI * (float) sample / (float) (mFftSize - 1));
                break;
            }
        }
        
        float windowSum = 0.0f;
        for (int sample = 0; sample < mFftSize; ++sample)
            windowSum += mFftWindow[sample];
        
        mWindowScaleFactor = 0.0f;
        if (mOverlap != 0 && windowSum != 0.0f)
            mWindowScaleFactor = 1.0f / (float) mOverlap / windowSum * (float) mFftSize;
    }
    
    //======================================
    
    void analysis (const int channel)
    {
        int inputBufferIndex = mCurrentInputBufferWritePosition;
        for (int index = 0; index < mFftSize; ++index)
        {
            mTimeDomainBuffer[index].real (mFftWindow[index] * mInputBuffer.getSample (channel, inputBufferIndex));
            mTimeDomainBuffer[index].imag (0.0f);
            
            if (++inputBufferIndex >= mInputBufferLength)
                inputBufferIndex = 0;
        }
    }
    
    // Where we do our time-frequency domain processing.
    void modification (HeapBlock<float>& noiseSpectrum, float subtractionStrength)
    {
        // Forward FFT
        mFft->perform (mTimeDomainBuffer, mFrequencyDomainBuffer, false);
        
        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int index = 0; index < mFftSize / 2 + 1; ++index)
        {
            // Separate magnitude and phase
            float magnitude = abs (mFrequencyDomainBuffer[index]);
            float phase = arg (mFrequencyDomainBuffer[index]);
            
            float newMagnitude = magnitude - subtractionStrength * noiseSpectrum[index];
            newMagnitude = (newMagnitude < 0.0) ? 0.0 : newMagnitude;
            
            mFrequencyDomainBuffer[index].real (newMagnitude * cosf (phase));
            mFrequencyDomainBuffer[index].imag (newMagnitude * sinf (phase));
            if (index > 0 && index < mFftSize / 2)
            {
                mFrequencyDomainBuffer[mFftSize - index].real (newMagnitude * cosf (phase));
                mFrequencyDomainBuffer[mFftSize - index].imag (newMagnitude * sinf (-phase));
            }
        }
        
        // Inverse FFT
        mFft->perform (mFrequencyDomainBuffer, mTimeDomainBuffer, true);
    }
    
    void synthesis (const int channel)
    {
        int outputBufferIndex = mCurrentOutputBufferWritePosition;
        for (int index = 0; index < mFftSize; ++index)
        {
            float outputSample = mOutputBuffer.getSample (channel, outputBufferIndex);
            outputSample += mTimeDomainBuffer[index].real() * mWindowScaleFactor;
            mOutputBuffer.setSample (channel, outputBufferIndex, outputSample);
            
            if (++outputBufferIndex >= mOutputBufferLength)
                outputBufferIndex = 0;
        }
        
        mCurrentOutputBufferWritePosition += mHopSize;
        if (mCurrentOutputBufferWritePosition >= mOutputBufferLength)
            mCurrentOutputBufferWritePosition = 0;
    }
    
protected:
    //======================================
    int mNumChannels;
    int mNumSamples;
    
    int mFftSize;
    std::unique_ptr<dsp::FFT> mFft;
    
    int mInputBufferLength;
    juce::AudioBuffer<float> mInputBuffer;
    
    int mOutputBufferLength;
    juce::AudioBuffer<float> mOutputBuffer;
    
    HeapBlock<float> mFftWindow;
    HeapBlock<dsp::Complex<float>> mTimeDomainBuffer;
    HeapBlock<dsp::Complex<float>> mFrequencyDomainBuffer;
    
    int mOverlap;
    int mHopSize;
    float mWindowScaleFactor;
    
    int mInputBufferWritePosition;
    int mOutputBufferWritePosition;
    int mOutputBufferReadPosition;
    int mSamplesSinceLastFFT;
    
    int mCurrentInputBufferWritePosition;
    int mCurrentOutputBufferWritePosition;
    int mCurrentOutputBufferReadPosition;
    int mCurrentSamplesSinceLastFFT;
};
