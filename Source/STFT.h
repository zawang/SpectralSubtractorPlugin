/*
  ==============================================================================

    STFT.h
    Created: 22 Nov 2021 2:21:32pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

// https://github.com/juandagilc/Audio-Effects

#pragma once

#include "JuceHeader.h"
#include "AudioFunctions.h"

//==============================================================================

class STFT
{
public:
    enum windowTypeIndex
    {
        kWindowTypeRectangular = 0,
        kWindowTypeBartlett,
        kWindowTypeHann,
        kWindowTypeHamming,
    };
    
    //======================================
    
    STFT() {}
    
    virtual ~STFT() {}
    
    //======================================
    
    void setup (const int numInputChannels)
    {
        mNumChannels = (numInputChannels > 0) ? numInputChannels : 1;
    }
    
    void updateParameters (const int newFFTSize, const int newOverlap, const int newWindowType)
    {
        updateFFTSize (newFFTSize);
        updateHopSize (newOverlap);
        updateWindow (newWindowType);
    }
    
    //======================================
    
    void processBlock (juce::AudioBuffer<float>& block)
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
                    modification();
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
    
    void updateFFTSize (const int newFFTSize)
    {
        mFFTSize = newFFTSize;
        mFFT = std::make_unique<dsp::FFT>(log2 (mFFTSize));
        
        mInputBufferLength = mFFTSize;
        mInputBuffer.clear();
        mInputBuffer.setSize (mNumChannels, mInputBufferLength);
        
        mOutputBufferLength = mFFTSize;
        mOutputBuffer.clear();
        mOutputBuffer.setSize (mNumChannels, mOutputBufferLength);
        
        mFFTWindow.realloc (mFFTSize);
        mFFTWindow.clear (mFFTSize);
        
        mTimeDomainBuffer.realloc (mFFTSize);
        mTimeDomainBuffer.clear (mFFTSize);
        
        mFrequencyDomainBuffer.realloc (mFFTSize);
        mFrequencyDomainBuffer.clear (mFFTSize);
        
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
            mHopSize = mFFTSize / mOverlap;
            mOutputBufferWritePosition = mHopSize % mOutputBufferLength;
        }
    }
    
    void updateWindow (const int newWindowType)
    {
        switch (newWindowType)
        {
            case kWindowTypeRectangular:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 1.0f;
                break;
            }
            case kWindowTypeBartlett:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 1.0f - fabs (2.0f * (float) sample / (float) (mFFTSize - 1) - 1.0f);
                break;
            }
            case kWindowTypeHann:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (float) sample / (float) (mFFTSize - 1));
                break;
            }
            case kWindowTypeHamming:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 0.54f - 0.46f * cosf (2.0f * M_PI * (float) sample / (float) (mFFTSize - 1));
                break;
            }
        }
        
        float windowSum = 0.0f;
        for (int sample = 0; sample < mFFTSize; ++sample)
            windowSum += mFFTWindow[sample];
        
        mWindowScaleFactor = 0.0f;
        if (mOverlap != 0 && windowSum != 0.0f)
            mWindowScaleFactor = 1.0f / (float) mOverlap / windowSum * (float) mFFTSize;
    }
    
    //======================================
    
    void analysis (const int channel)
    {
        int inputBufferIndex = mCurrentInputBufferWritePosition;
        for (int index = 0; index < mFFTSize; ++index)
        {
            mTimeDomainBuffer[index].real (mFFTWindow[index] * mInputBuffer.getSample (channel, inputBufferIndex));
            mTimeDomainBuffer[index].imag (0.0f);
            
            if (++inputBufferIndex >= mInputBufferLength)
                inputBufferIndex = 0;
        }
    }
    
    // Where we do our time-frequency domain processing.
    void modification ()
    {
        // Forward FFT
        mFFT->perform (mTimeDomainBuffer, mFrequencyDomainBuffer, false);
        
        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int index = 0; index < mFFTSize / 2 + 1; ++index)
        {
            // Separate magnitude and phase
            float magnitude = abs (mFrequencyDomainBuffer[index]);
            float phase = arg (mFrequencyDomainBuffer[index]);
            
            processMagAndPhase (index, magnitude, phase);
            
            mFrequencyDomainBuffer[index].real (magnitude * cosf (phase));
            mFrequencyDomainBuffer[index].imag (magnitude * sinf (phase));
            if (index > 0 && index < mFFTSize / 2)
            {
                mFrequencyDomainBuffer[mFFTSize - index].real (magnitude * cosf (phase));
                mFrequencyDomainBuffer[mFFTSize - index].imag (magnitude * sinf (-phase));
            }
        }
        
        // Inverse FFT
        mFFT->perform (mFrequencyDomainBuffer, mTimeDomainBuffer, true);
    }
    
    void synthesis (const int channel)
    {
        int outputBufferIndex = mCurrentOutputBufferWritePosition;
        for (int index = 0; index < mFFTSize; ++index)
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
    
    // Override this function to do something interesting!
    virtual void processMagAndPhase (int index, float& magnitude, float& phase) {}
    
protected:
    //======================================
    int mNumChannels {1};
    int mNumSamples;
    
    int mFFTSize;
    std::unique_ptr<dsp::FFT> mFFT;
    
    int mInputBufferLength;
    juce::AudioBuffer<float> mInputBuffer;
    
    int mOutputBufferLength;
    juce::AudioBuffer<float> mOutputBuffer;
    
    HeapBlock<float> mFFTWindow;
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
