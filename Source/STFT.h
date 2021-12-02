/*
  ==============================================================================

    STFT.h
    Created: 22 Nov 2021 2:21:32pm
    Author:  Zachary Wang

  ==============================================================================
*/

// https://github.com/juandagilc/Audio-Effects

#pragma once

#include "JuceHeader.h"

//==============================================================================

template <typename FloatType>
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
    
    void processBlock (juce::AudioBuffer<FloatType>& block)
    {
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
    
protected:
    // Override this function to do something interesting!
    virtual void processMagAndPhase (int index, FloatType& magnitude, FloatType& phase) {}
    
private:
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
                    mFFTWindow[sample] = 1.0f - fabs (2.0f * (FloatType) sample / (FloatType) (mFFTSize - 1) - 1.0f);
                break;
            }
            case kWindowTypeHann:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (FloatType) sample / (FloatType) (mFFTSize - 1));
                break;
            }
            case kWindowTypeHamming:
            {
                for (int sample = 0; sample < mFFTSize; ++sample)
                    mFFTWindow[sample] = 0.54f - 0.46f * cosf (2.0f * M_PI * (FloatType) sample / (FloatType) (mFFTSize - 1));
                break;
            }
        }
        
        FloatType windowSum = static_cast<FloatType> (0);
        for (int sample = 0; sample < mFFTSize; ++sample)
            windowSum += mFFTWindow[sample];
        
        mWindowScaleFactor = static_cast<FloatType> (0);
        if (mOverlap != 0 && windowSum != static_cast<FloatType> (0))
            mWindowScaleFactor = 1.0f / (FloatType) mOverlap / windowSum * (FloatType) mFFTSize;
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

#if RUN_UNIT_TESTS == 1

struct STFTTests : public juce::UnitTest
{
    STFTTests()
        : juce::UnitTest ("STFT")
    {}
    
    void runTest() override
    {
        const int samplesPerBlock = 256;
        const int numChannels = 1;
        
        const int fftSize = 2048;
        const int hopSize = 512;
        const int window = STFT<float>::kWindowTypeHann;
        
        STFT<float> stft;
        stft.setup (numChannels);
        stft.updateParameters (fftSize,
                               fftSize / hopSize,
                               window);
        
        // TODO: come up with a cleaner way of finding the proper directory and doing this entire test in general
        juce::File aircommFile {"/Users/zach/Audio Programming/JUCE Projects/Personal Projects/SpectralSubtractor/Test Data/aircomm.wav"};
        jassert (aircommFile.existsAsFile());
        
        juce::AudioBuffer<float> aircomm;
        getAudioFile (aircomm, aircommFile);
        juce::AudioBuffer<float> aircommCopy;
        aircommCopy.makeCopyOf (aircomm);
        
        beginTest ("fft size: 2048, hop size: 512, window: hann");
        {
            process (stft, aircomm, samplesPerBlock, numChannels);
            
            const float* aircommData = aircomm.getReadPointer (0, fftSize);
            const float* aircommCopyData = aircommCopy.getReadPointer (0, 0);
            float maxAbsoluteError = 0.0001f;
            for (int i = 0; i < aircommCopy.getNumSamples() - fftSize; ++i)
                expectWithinAbsoluteError (aircommData[i], aircommCopyData[i], maxAbsoluteError);
        }
    }
    
    void getAudioFile (juce::AudioBuffer<float>& buffer, const juce::File& file)
    {
        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        std::unique_ptr<juce::AudioFormatReader> reader;
        reader.reset (formatManager.createReaderFor (file));
        if (reader.get() != nullptr)
        {
            buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), 0, buffer.getNumSamples());
        }
    }
    
    void process (STFT<float>& stft, juce::AudioBuffer<float>& audio, const int blockSize, const int numChannels)
    {
        auto totalNumSamples = audio.getNumSamples();
        int samplePtr = 0;

        while (totalNumSamples > 0)
        {
            auto curBlockSize = jmin (totalNumSamples, blockSize);
            totalNumSamples -= curBlockSize;

            AudioBuffer<float> curBuff (audio.getArrayOfWritePointers(), numChannels, samplePtr, curBlockSize);
            stft.processBlock (curBuff);

            samplePtr += curBlockSize;
        }
    }
};

static STFTTests stftTests;

#endif
