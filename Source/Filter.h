/*
  ==============================================================================

    Filter.h
    Created: 13 May 2020 9:32:59am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "AudioFunctions.h"

//==============================================================================

class Filter
{
public:
    //======================================
    
    Filter() : numChannels (1) {}
    
    virtual ~Filter() {}
    
    //======================================
    
    void setup (const int numInputChannels)
    {
        numChannels = (numInputChannels > 0) ? numInputChannels : 1;
    }
    
    void updateParameters (const int newFftSize, const int newOverlap, const int newWindowType)
    {
        updateFftSize (newFftSize);
        updateHopSize (newOverlap);
        updateWindow (newWindowType);
    }
    
//    HeapBlock<float>& getNoiseSpectrum()
//    {
//        return mNoiseSpectrum;
//    }
    
    //======================================
    
    void processBlock (AudioSampleBuffer& block, HeapBlock<float>& noiseSpectrum, float subtractionStrength)
    {
        numSamples = block.getNumSamples();
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* channelData = block.getWritePointer (channel);
            
            currentInputBufferWritePosition = inputBufferWritePosition;
            currentOutputBufferWritePosition = outputBufferWritePosition;
            currentOutputBufferReadPosition = outputBufferReadPosition;
            currentSamplesSinceLastFFT = samplesSinceLastFFT;
            
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float inputSample = channelData[sample];
                inputBuffer.setSample (channel, currentInputBufferWritePosition, inputSample);
                if (++currentInputBufferWritePosition >= inputBufferLength)
                    currentInputBufferWritePosition = 0;
                
                channelData[sample] = outputBuffer.getSample (channel, currentOutputBufferReadPosition);
                outputBuffer.setSample (channel, currentOutputBufferReadPosition, 0.0f);
                if (++currentOutputBufferReadPosition >= outputBufferLength)
                    currentOutputBufferReadPosition = 0;
                
                if (++currentSamplesSinceLastFFT >= hopSize)
                {
                    currentSamplesSinceLastFFT = 0;
                    
                    analysis (channel);
                    modification(noiseSpectrum, subtractionStrength);
                    synthesis (channel);
                }
            }
        }
        
        inputBufferWritePosition = currentInputBufferWritePosition;
        outputBufferWritePosition = currentOutputBufferWritePosition;
        outputBufferReadPosition = currentOutputBufferReadPosition;
        samplesSinceLastFFT = currentSamplesSinceLastFFT;
    }
    
private:
    //======================================
    
    void updateFftSize (const int newFftSize)
    {
        fftSize = newFftSize;
        fft = std::make_unique<dsp::FFT>(log2 (fftSize));
        
        inputBufferLength = fftSize;
        inputBuffer.clear();
        inputBuffer.setSize (numChannels, inputBufferLength);
        
        outputBufferLength = fftSize;
        outputBuffer.clear();
        outputBuffer.setSize (numChannels, outputBufferLength);
        
        fftWindow.realloc (fftSize);
        fftWindow.clear (fftSize);
        
        timeDomainBuffer.realloc (fftSize);
        timeDomainBuffer.clear (fftSize);
        
        frequencyDomainBuffer.realloc (fftSize);
        frequencyDomainBuffer.clear (fftSize);
        
//        mNoiseSpectrum.realloc (fftSize);
//        mNoiseSpectrum.clear (fftSize);
        
        inputBufferWritePosition = 0;
        outputBufferWritePosition = 0;
        outputBufferReadPosition = 0;
        samplesSinceLastFFT = 0;
    }
    
    void updateHopSize (const int newOverlap)
    {
        overlap = newOverlap;
        if (overlap != 0)
        {
            hopSize = fftSize / overlap;
            outputBufferWritePosition = hopSize % outputBufferLength;
        }
    }
    
    void updateWindow (const int newWindowType)
    {
        switch (newWindowType)
        {
            case kWindowTypeRectangular:
            {
                for (int sample = 0; sample < fftSize; ++sample)
                    fftWindow[sample] = 1.0f;
                break;
            }
            case kWindowTypeBartlett:
            {
                for (int sample = 0; sample < fftSize; ++sample)
                    fftWindow[sample] = 1.0f - fabs (2.0f * (float)sample / (float)(fftSize - 1) - 1.0f);
                break;
            }
            case kWindowTypeHann:
            {
                for (int sample = 0; sample < fftSize; ++sample)
                    fftWindow[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (float)sample / (float)(fftSize - 1));
                break;
            }
            case kWindowTypeHamming:
            {
                for (int sample = 0; sample < fftSize; ++sample)
                    fftWindow[sample] = 0.54f - 0.46f * cosf (2.0f * M_PI * (float)sample / (float)(fftSize - 1));
                break;
            }
        }
        
        float windowSum = 0.0f;
        for (int sample = 0; sample < fftSize; ++sample)
            windowSum += fftWindow[sample];
        
        windowScaleFactor = 0.0f;
        if (overlap != 0 && windowSum != 0.0f)
            windowScaleFactor = 1.0f / (float)overlap / windowSum * (float)fftSize;
    }
    
    //======================================
    
    void analysis (const int channel)
    {
        int inputBufferIndex = currentInputBufferWritePosition;
        for (int index = 0; index < fftSize; ++index)
        {
            timeDomainBuffer[index].real (fftWindow[index] * inputBuffer.getSample (channel, inputBufferIndex));
            timeDomainBuffer[index].imag (0.0f);
            
            if (++inputBufferIndex >= inputBufferLength)
                inputBufferIndex = 0;
        }
    }
    
    // Where we do our time-frequency domain processing.
    void modification(HeapBlock<float>& noiseSpectrum, float subtractionStrength)
    {
        // Forward FFT
        fft->perform (timeDomainBuffer, frequencyDomainBuffer, false);
        
        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int index = 0; index < fftSize / 2 + 1; ++index)
        {
            // Separate magnitude and phase
            float magnitude = abs (frequencyDomainBuffer[index]);
            float phase = arg (frequencyDomainBuffer[index]);
            
            float newMagnitude = magnitude - subtractionStrength*noiseSpectrum[index];
            newMagnitude = (newMagnitude < 0.0) ? 0.0 : newMagnitude;
            
            frequencyDomainBuffer[index].real (newMagnitude * cosf (phase));
            frequencyDomainBuffer[index].imag (newMagnitude * sinf (phase));
            if (index > 0 && index < fftSize / 2)
            {
                frequencyDomainBuffer[fftSize - index].real (newMagnitude * cosf (phase));
                frequencyDomainBuffer[fftSize - index].imag (newMagnitude * sinf (-phase));
            }
        }
        
        // Inverse FFT
        fft->perform (frequencyDomainBuffer, timeDomainBuffer, true);
    }
    
    void synthesis (const int channel)
    {
        int outputBufferIndex = currentOutputBufferWritePosition;
        for (int index = 0; index < fftSize; ++index)
        {
            float outputSample = outputBuffer.getSample (channel, outputBufferIndex);
            outputSample += timeDomainBuffer[index].real() * windowScaleFactor;
            outputBuffer.setSample (channel, outputBufferIndex, outputSample);
            
            if (++outputBufferIndex >= outputBufferLength)
                outputBufferIndex = 0;
        }
        
        currentOutputBufferWritePosition += hopSize;
        if (currentOutputBufferWritePosition >= outputBufferLength)
            currentOutputBufferWritePosition = 0;
    }
    
protected:
    //======================================
    int numChannels;
    int numSamples;
    
    int fftSize;
    std::unique_ptr<dsp::FFT> fft;
    
    int inputBufferLength;
    AudioSampleBuffer inputBuffer;
    
    int outputBufferLength;
    AudioSampleBuffer outputBuffer;
    
    HeapBlock<float> fftWindow;
    HeapBlock<dsp::Complex<float>> timeDomainBuffer;
    HeapBlock<dsp::Complex<float>> frequencyDomainBuffer;
//    HeapBlock<float> mNoiseSpectrum;
    
    int overlap;
    int hopSize;
    float windowScaleFactor;
    
    int inputBufferWritePosition;
    int outputBufferWritePosition;
    int outputBufferReadPosition;
    int samplesSinceLastFFT;
    
    int currentInputBufferWritePosition;
    int currentOutputBufferWritePosition;
    int currentOutputBufferReadPosition;
    int currentSamplesSinceLastFFT;
};

//==============================================================================
