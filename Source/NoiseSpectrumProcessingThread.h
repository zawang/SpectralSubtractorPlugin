/*
  ==============================================================================

    ProcessingNoiseSpectrumThread.h
    Created: 4 Oct 2021 7:59:13am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"

// Thread that calculates a noise file's average spectrum and replace's the audio processor's old noise spectrum with the one it just calculated.
class NoiseSpectrumProcessingThread : public juce::ThreadWithProgressWindow
{
public:
    NoiseSpectrumProcessingThread(SpectralSubtractorAudioProcessor* inProcessor, juce::File inFile)
        : juce::ThreadWithProgressWindow ("Preparing noise spectrum...", true, true, 10000),
          mProcessor (inProcessor),
          mNoiseFile (inFile)
    {
        jassert (mNoiseFile != juce::File{});
        
        setStatusMessage ("Getting ready...");
    }

    void run() override
    {
        mProcessor->suspendProcessing (true);
          
        setProgress (-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar...
        setStatusMessage ("Reading noise file...");
        
        // Read the file
        mReader.reset (mProcessor->getFormatManager()->createReaderFor (mNoiseFile));
        if (mReader.get() != nullptr)
        {
            auto noiseBuffer = std::make_unique<juce::AudioBuffer<float>> ((int) mReader->numChannels, (int) mReader->lengthInSamples);
            
            if (threadShouldExit()) return; // must check this as often as possible, because this is how we know if the user's pressed 'cancel'
            
            mReader->read (noiseBuffer.get(),
                           0,
                           (int) mReader->lengthInSamples,
                           0,
                           true,
                           true);
            
            if (threadShouldExit()) return;
            
            setStatusMessage ("Calculating noise spectrogram...");
            
            Spectrogram noiseSpectrogram;
            makeSpectrogram (noiseSpectrogram, noiseBuffer.get());
            
            if (threadShouldExit()) return;
            
            setProgress (-1.0);
            setStatusMessage ("Calculating noise spectrum...");
            
            computeAverageSpectrum (mTempNoiseSpectrum, noiseSpectrogram, globalFFTSize);
            
            if (threadShouldExit()) return;
            
            setStatusMessage ("Almost finished...");
            
            mProcessor->loadNewNoiseSpectrum (mTempNoiseSpectrum);
        }
        else
        {
            mErrorLoadingFile = true;
            return;
        }
        
        mProcessor->suspendProcessing (false);
    }

    // This method gets called on the message thread once our thread has finished..
    void threadComplete (bool userPressedCancel) override
    {
        mProcessor->suspendProcessing (false);   // in case it hasn't been done yet
        
        juce::String messageString;
        if (userPressedCancel)
            messageString = "Operation canceled!";
        else if (mErrorLoadingFile)
            messageString = juce::String("Unable to load ") + mNoiseFile.getFileName();
        else
            messageString = "Successfully loaded noise spectrum!";
        
        juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                           .withIconType (MessageBoxIconType::InfoIcon)
                                           .withTitle ("Progress window")
                                           .withMessage (messageString),
                                           nullptr);

        // ..and clean up by deleting our thread object..
        delete this;
    }
    
    // Makes a Spectrogram by averaging the Spectrograms made from each channel of signal.
    void makeSpectrogram(Spectrogram& spectrogram, juce::AudioBuffer<float>* signal)
    {
        const size_t dataCount = signal->getNumSamples();
        
        // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
        ptrdiff_t fftSize = mFft.getSize();
        
        // Calculate number of hops
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / mHop);
        
        // Initialize spectrogram
        spectrogram.resize(numHops+1);      // WHY NUMHOPS + 1????
        for (auto i = 0; i < spectrogram.size(); ++i)
        {
            if (threadShouldExit()) return;
            spectrogram[i].realloc(fftSize);
            spectrogram[i].clear(fftSize);
        }
        
        // We will discard the negative frequency bins, but leave the center bin.
        size_t numRows = 1UL + (fftSize / 2UL);
        
        int numChannels = signal->getNumChannels();
        
        size_t thingsToDo = numChannels * numHops;
        double progress = 0.0;
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (threadShouldExit()) return;
            
            // fFft works on the data in place, and needs twice as much space as the input size.
            std::vector<float> fftBuffer(fftSize * 2UL);
        
            int signalIndex = 0;
            // While data remains
            for (int i = 0; i < numHops; ++i)
            {
                if (threadShouldExit()) return;
                
                setProgress ( progress / thingsToDo );
                
                jassert(signalIndex < signal->getNumSamples());
                
                // Prepare segment to perform FFT on.
                for (int j = 0; j < fftSize; ++j)
                {
                    fftBuffer[j] = signal->getSample(channel, signalIndex + j);
                }
//                std::memcpy(fftBuffer.data(), data, fftSize * sizeof(float));
                
                // Apply the windowing to the chunk of samples before passing it to the FFT.
                mWindow.multiplyWithWindowingTable(fftBuffer.data(), fftSize);
                
                // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
                mFft.performFrequencyOnlyForwardTransform(fftBuffer.data());
                
                // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
                for (int j = 0; j < numRows; ++j)
                {
                    spectrogram[i][j] += (fftBuffer[j] / numChannels);      // Divide by numChannels because we're calculating the average spectrogram
                }
                
                // Next chunk
                signalIndex += mHop;
                
                ++progress;
            }
        }
    }
    
    // Calculates the average spectrum from a given spectrogram
    void computeAverageSpectrum(HeapBlock<float>& magSpectrum, Spectrogram& spectrogram, int fftSize)
    {
        magSpectrum.realloc (fftSize);
        magSpectrum.clear (fftSize);
        
        auto numColumns = spectrogram.size();

        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int freqBin = 0; freqBin < fftSize / 2 + 1; ++freqBin)
        {
            if (threadShouldExit()) return;
            
            float sum = 0.f;
            
            for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
                sum += spectrogram[freqColumn][freqBin];
                
            magSpectrum[freqBin] = sum / numColumns;
        }
    }

private:
    SpectralSubtractorAudioProcessor* mProcessor;
    HeapBlock<float> mTempNoiseSpectrum;
    juce::File mNoiseFile;
    std::unique_ptr<juce::AudioFormatReader> mReader;
    bool mErrorLoadingFile { false };
    
    // For creating spectrogram
    size_t mHop { globalHopSize };
    juce::dsp::FFT mFft { static_cast<int>(std::log2 (globalFFTSize)) };
    juce::dsp::WindowingFunction<float> mWindow { static_cast<size_t>(mFft.getSize() + 1),
                                                  juce::dsp::WindowingFunction<float>::hann,
                                                  false };
};
