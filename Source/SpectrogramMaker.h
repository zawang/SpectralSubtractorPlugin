/*
  ==============================================================================

    SpectrogramMaker.h
    Created: 13 May 2020 4:00:20pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "AudioFunctions.h"

class SpectrogramMaker
{
public:
    SpectrogramMaker(int fftBins, size_t hopSize)
        : fFft(std::log2(fftBins)),
          fWindow(fFft.getSize() + 1, dsp::WindowingFunction<float>::hann, false)
    {
        hop = hopSize;
    }
    
    ~SpectrogramMaker() {}
    
    // This perform function makes a Spectrogram by averaging the Spectrograms made from each channel of signal.
    void perform(const AudioSampleBuffer& signal, Spectrogram& spectrogram)
    {
        const size_t dataCount = signal.getNumSamples();
        
        // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
        ptrdiff_t fftSize = fFft.getSize();
        
        // Calculate number of hops
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / hop);
        
        // Initialize spectrogram
        spectrogram.resize(numHops+1);
        for (auto i = 0; i <= numHops; i++)
        {
            spectrogram[i].realloc(fftSize);
            spectrogram[i].clear(fftSize);
        }
        
        // We will discard the negative frequency bins, but leave the center bin.
        size_t numRows = 1UL + (fftSize / 2UL);
        
        int numChannels = signal.getNumChannels();
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float* data = signal.getReadPointer(channel);
            
            // fFft works on the data in place, and needs twice as much space as the input size.
            std::vector<float> fftBuffer(fftSize * 2UL);
        
            // While data remains
            for (int i = 0; i <= numHops; ++i)
            {
                // Prepare segment to perform FFT on.
                std::memcpy(fftBuffer.data(), data, fftSize * sizeof(float));
                
                // Apply the windowing to the chunk of samples before passing it to the FFT.
                fWindow.multiplyWithWindowingTable(fftBuffer.data(), fftSize);
                
                // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
                fFft.performFrequencyOnlyForwardTransform(fftBuffer.data());
                
                // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
                for (int j = 0; j < numRows; ++j)
                {
                    spectrogram[i][j] += (fftBuffer[j] / numChannels);      // Divide by numChannels because we're calculating the average spectrogram
                }
                
                // Unsure if I have to flip the data vertically??????????????????????????????????????????????????????????????????????????????????????????????????????
                
                // Next chunk
                data += hop;
            }
        }
    }
    
    /**
    // This perform function makes a Spectrogram from only the first channel of signal.
    void perform(const AudioSampleBuffer& signal, Spectrogram& spectrogram)
    {
        const float* data = signal.getReadPointer(0);       // Read from channel 0
        const size_t dataCount = signal.getNumSamples();
        
        // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
        ptrdiff_t fftSize = fFft.getSize();
        
        // Calculate number of hops
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / hop);
        
        // Initialize spectrogram
        spectrogram.resize(numHops+1);
        for (auto i = 0; i <= numHops; i++)
        {
            spectrogram[i].realloc(fftSize);
            spectrogram[i].clear(fftSize);
        }
        
        // We will discard the negative frequency bins, but leave the center bin.
        size_t numRows = 1UL + (fftSize / 2UL);
        
        // fFft works on the data in place, and needs twice as much space as the input size.
        std::vector<float> fftBuffer(fftSize * 2UL);
        
        // While data remains
        for (int i = 0; i <= numHops; ++i)
        {
            // Prepare segment to perform FFT on.
            std::memcpy(fftBuffer.data(), data, fftSize * sizeof(float));
            
            // Apply the windowing to the chunk of samples before passing it to the FFT.
            fWindow.multiplyWithWindowingTable(fftBuffer.data(), fftSize);
            
            // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
            fFft.performFrequencyOnlyForwardTransform(fftBuffer.data());
            
            // Copy only the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
            std::memcpy(spectrogram[i], fftBuffer.data(), numRows * sizeof(float));
            
            // Unsure if I have to flip the data vertically??????????????????????????????????????????????????????????????????????????????????????????????????????
            
            // Next chunk
            data += hop;
        }
    }*/

private:
    size_t hop;
    dsp::FFT fFft;
    dsp::WindowingFunction<float> fWindow;
};
