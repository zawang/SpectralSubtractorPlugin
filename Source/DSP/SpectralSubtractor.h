/*
  ==============================================================================

    Filter.h
    Created: 13 May 2020 9:32:59am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "../Helper/AudioSpinMutex.h"

//==============================================================================

// Based off of https://github.com/juandagilc/Audio-Effects/tree/master/Template%20Frequency%20Domain

template <typename FloatType>
class SpectralSubtractor
{
public:
    enum windowTypeIndex
    {
        kWindowTypeRectangular = 0,
        kWindowTypeBartlett,
        kWindowTypeHann,
        kWindowTypeHamming,
    };
    
    SpectralSubtractor () {}
    
    ~SpectralSubtractor() {}
    
    void setNumChannels (const int numInputChannels)
    {
        std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
        mNumChannels = (numInputChannels > 0) ? numInputChannels : 1;
    }
    
    void updateParameters (const int newFFTSize, const int newOverlap, const int newWindowType)
    {
        std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
        updateFFTSize (newFFTSize);
        updateHopSize (newOverlap);
        updateWindow (newWindowType);
    }
    
    void reset (const int newFFTSize)
    {
        std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
        mNoiseSpectrum.calloc (newFFTSize);
    }
    
    void setSubtractionStrength (std::atomic<float>* subtractionStrength)
    {
        mSubtractionStrength = subtractionStrength;
        jassert (mSubtractionStrength);
    }
    
    // Replaces the old noise spectrum with a new noise spectrum.
    void loadNoiseSpectrum (const juce::HeapBlock<FloatType>& newNoiseSpectrum)
    {
        std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
        // TODO: add a safety check to ensure mNoiseSpectrum and newNoiseSpectrum have the same float type?
        mNoiseSpectrum.realloc (mFFTSize);
        std::memcpy (mNoiseSpectrum, newNoiseSpectrum, mFFTSize * sizeof (FloatType));
    }
    
    juce::HeapBlock<FloatType>& getNoiseSpectrum()
    {
        return mNoiseSpectrum;
    }
    
    //==============================================================================
    
    void process (juce::AudioBuffer<FloatType>& block)
    {
        // Note that we use a try lock here, so that the audio thread doesn't get stuck waiting in the case that another thread is currently modifying parameters.
        std::unique_lock lock (mSpinMutex, std::try_to_lock);
        if (!lock.owns_lock())
            return;
        
        mNumSamples = block.getNumSamples();
                
        for (int channel = 0; channel < mNumChannels; ++channel)
        {
            FloatType* channelData = block.getWritePointer (channel);
            
            mCurrentInputBufferWritePosition = mInputBufferWritePosition;
            mCurrentOutputBufferWritePosition = mOutputBufferWritePosition;
            mCurrentOutputBufferReadPosition = mOutputBufferReadPosition;
            mCurrentSamplesSinceLastFFT = mSamplesSinceLastFFT;
            
            int relativeCurrentOutputBufferReadPosition = 0;
            int relativeCurrentInputBufferWritePosition = 0;
            FloatType* inputBufferData = mInputBuffer.getWritePointer (channel, mCurrentInputBufferWritePosition);
            FloatType* outputBufferData = mOutputBuffer.getWritePointer (channel, mCurrentOutputBufferReadPosition);
            for (int sample = 0; sample < mNumSamples; ++sample)
            {
                const FloatType inputSample = channelData[sample];
                inputBufferData[relativeCurrentInputBufferWritePosition] = inputSample;
                ++relativeCurrentInputBufferWritePosition;
                if (++mCurrentInputBufferWritePosition >= mInputBufferLength)
                {
                    mCurrentInputBufferWritePosition = 0;
                    inputBufferData = mInputBuffer.getWritePointer (channel, mCurrentInputBufferWritePosition);
                    relativeCurrentInputBufferWritePosition = 0;
                }
                
                channelData[sample] = outputBufferData[relativeCurrentOutputBufferReadPosition];
                outputBufferData[relativeCurrentOutputBufferReadPosition] = static_cast<FloatType> (0);
                ++relativeCurrentOutputBufferReadPosition;
                if (++mCurrentOutputBufferReadPosition >= mOutputBufferLength)
                {
                    mCurrentOutputBufferReadPosition = 0;
                    outputBufferData = mOutputBuffer.getWritePointer (channel, mCurrentOutputBufferReadPosition);
                    relativeCurrentOutputBufferReadPosition = 0;
                }
                
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
    juce::HeapBlock<FloatType> mNoiseSpectrum;             // Holds the average magnitude spectrum of the noise signal
    std::atomic<float>* mSubtractionStrength = nullptr;
    
    audio_spin_mutex mSpinMutex;
    
    //==============================================================================
    
    int mNumChannels {1};
    int mNumSamples;
    
    int mFFTSize;
    std::unique_ptr<dsp::FFT> mFFT;
    
    int mInputBufferLength;
    juce::AudioBuffer<FloatType> mInputBuffer;
    
    int mOutputBufferLength;
    juce::AudioBuffer<FloatType> mOutputBuffer;
    
    HeapBlock<FloatType> mFFTWindow;
    HeapBlock<dsp::Complex<FloatType>> mTimeDomainBuffer;
    HeapBlock<dsp::Complex<FloatType>> mFrequencyDomainBuffer;
    
    int mOverlap;
    int mHopSize;
    FloatType mWindowScaleFactor;
    
    int mInputBufferWritePosition;
    int mOutputBufferWritePosition;
    int mOutputBufferReadPosition;
    int mSamplesSinceLastFFT;
    
    int mCurrentInputBufferWritePosition;
    int mCurrentOutputBufferWritePosition;
    int mCurrentOutputBufferReadPosition;
    int mCurrentSamplesSinceLastFFT;
    
    void updateFFTSize (const int newFFTSize)
    {
        mFFTSize = newFFTSize;
        mFFT = std::make_unique<dsp::FFT> (log2 (mFFTSize));
        
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
        else
            jassertfalse;
    }
    
    void updateWindow (const int newWindowType)
    {
        switch (newWindowType)
        {
            case kWindowTypeRectangular:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = static_cast<FloatType> (1);
                break;
            }
            case kWindowTypeBartlett:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = static_cast<FloatType> (1) - fabs (static_cast<FloatType> (2) * (FloatType) sample / (FloatType) (mFFTSize - 1) - static_cast<FloatType> (1));
                break;
            }
            case kWindowTypeHann:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = static_cast<FloatType> (0.5) - static_cast<FloatType> (0.5) * cosf (static_cast<FloatType> (2) * M_PI * (FloatType) sample / (FloatType) (mFFTSize - 1));
                break;
            }
            case kWindowTypeHamming:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = static_cast<FloatType> (0.54) - static_cast<FloatType> (0.46) * cosf (static_cast<FloatType> (2) * M_PI * (FloatType) sample / (FloatType) (mFFTSize - 1));
                break;
            }
        }
        
        FloatType windowSum = static_cast<FloatType> (0);
        for (int sample = 0; sample < mFFTSize; ++sample)
            windowSum += mFFTWindow[sample];
        
        mWindowScaleFactor = static_cast<FloatType> (0);
        if (mOverlap != 0 && windowSum != static_cast<FloatType> (0))
            mWindowScaleFactor = static_cast<FloatType> (1) / (FloatType) mOverlap / windowSum * (FloatType) mFFTSize;
    }
    
    //======================================
    
    void analysis (const int channel)
    {
        int inputBufferIndex = mCurrentInputBufferWritePosition;
        int relativeInputBufferIndex = 0;
        const FloatType* inputBufferData = mInputBuffer.getReadPointer (channel, inputBufferIndex);
        for (int index = 0; index < mFFTSize; ++index)
        {
            mTimeDomainBuffer[index].real (mFFTWindow[index] * inputBufferData[relativeInputBufferIndex]);
            mTimeDomainBuffer[index].imag (static_cast<FloatType> (0));
            
            ++relativeInputBufferIndex;
            if (++inputBufferIndex >= mInputBufferLength)
            {
                inputBufferIndex = 0;
                inputBufferData = mInputBuffer.getReadPointer (channel, inputBufferIndex);
                relativeInputBufferIndex = 0;
            }
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
            FloatType magnitude = abs (mFrequencyDomainBuffer[index]);
            FloatType phase = arg (mFrequencyDomainBuffer[index]);
            
            magnitude -= (*mSubtractionStrength) * mNoiseSpectrum[index];
            magnitude = (magnitude < 0.0f) ? 0.0f : magnitude;
            
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
        int relativeOutputBufferIndex = 0;
        FloatType* outputBufferData = mOutputBuffer.getWritePointer (channel, outputBufferIndex);
        for (int index = 0; index < mFFTSize; ++index)
        {
            outputBufferData[relativeOutputBufferIndex] = outputBufferData[relativeOutputBufferIndex] + (mTimeDomainBuffer[index].real() * mWindowScaleFactor);
            
            ++relativeOutputBufferIndex;
            if (++outputBufferIndex >= mOutputBufferLength)
            {
                outputBufferIndex = 0;
                outputBufferData = mOutputBuffer.getWritePointer (channel, outputBufferIndex);
                relativeOutputBufferIndex = 0;
            }
        }
        
        mCurrentOutputBufferWritePosition += mHopSize;
        if (mCurrentOutputBufferWritePosition >= mOutputBufferLength)
            mCurrentOutputBufferWritePosition = 0;
    }
};
