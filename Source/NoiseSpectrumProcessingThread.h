/*
  ==============================================================================

    ProcessingNoiseSpectrumThread.h
    Created: 4 Oct 2021 7:59:13am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

// Thread that calculates a noise file's average spectrum and replace's the audio processor's old noise spectrum with the one it just calculated.
template <typename FloatType>
class NoiseSpectrumProcessingThread : public juce::ThreadWithProgressWindow
{
public:
    // One audio channel of FFT data over time, really 2-dimensional
    using Spectrogram = std::vector<HeapBlock<FloatType>>;
    
    NoiseSpectrumProcessingThread (SpectralSubtractorAudioProcessor* inProcessor, juce::File inFile, int fftSize, int hopSize)
        : juce::ThreadWithProgressWindow ("Preparing noise spectrum...", true, true, 10000),
          mProcessor (inProcessor),
          mNoiseFile (inFile),
          mFFT (static_cast<int> (std::log2 (fftSize))),
          mHop (hopSize),
          mWindow (static_cast<size_t> (fftSize + 1), juce::dsp::WindowingFunction<FloatType>::hann, false)
    {
        jassert (mNoiseFile != juce::File{});
        jassert (hopSize <= fftSize);
        
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
            auto noiseBuffer = std::make_unique<juce::AudioBuffer<FloatType>> ((int) mReader->numChannels, (int) mReader->lengthInSamples);
            
            if (threadShouldExit()) return; // must check this as often as possible, because this is how we know if the user's pressed 'cancel'
            
            mReader->read (noiseBuffer.get(),
                           0,
                           (int) mReader->lengthInSamples,
                           0,
                           true,
                           true);
            
            if (threadShouldExit()) return;
            
            setStatusMessage ("Computing STFT of noise signal...");
            
            Spectrogram noiseSpectrogram;
            stft (noiseSpectrogram, noiseBuffer.get());
            
            if (threadShouldExit()) return;
            
            setProgress (-1.0);
            setStatusMessage ("Computing noise spectrum...");
            
            computeAverageSpectrum (mTempNoiseSpectrum, noiseSpectrogram, mFFT.getSize());
            
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
    
    // Compute the stft on each channel of signal and average the results to produce one spectrogram.
    void stft (Spectrogram& spectrogram, juce::AudioBuffer<FloatType>* signal)
    {
        const size_t dataCount = signal->getNumSamples();
        
        // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
        ptrdiff_t fftSize = mFFT.getSize();
        
        // Calculate number of hops
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / mHop);
        
        // Initialize spectrogram
        spectrogram.resize (numHops + 1);
        for (int i = 0; i < spectrogram.size(); ++i)
        {
            if (threadShouldExit()) return;
            spectrogram[i].realloc (fftSize);
            spectrogram[i].clear (fftSize);
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
            std::vector<FloatType> fftBuffer (fftSize * 2UL);
        
            // While data remains
            const FloatType* signalData = signal->getReadPointer (channel, 0);
            for (int i = 0; i < numHops; ++i)
            {
                if (threadShouldExit()) return;
                
                setProgress (progress / thingsToDo);
                
                // Prepare segment to perform FFT on.
                std::memcpy (fftBuffer.data(), signalData, fftSize * sizeof (FloatType));
                
                // Apply the windowing to the chunk of samples before passing it to the FFT.
                mWindow.multiplyWithWindowingTable (fftBuffer.data(), fftSize);
                
                // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
                mFFT.performFrequencyOnlyForwardTransform (fftBuffer.data());
                
                // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
                for (int j = 0; j < numRows; ++j)
                {
                    spectrogram[i][j] += (fftBuffer[j] / numChannels);      // Divide by numChannels because we're calculating the average spectrogram
                }
                
                // Next chunk
                signalData += mHop;
                
                ++progress;
            }
        }
    }
    
    // Calculates the average spectrum from a given spectrogram
    void computeAverageSpectrum (HeapBlock<FloatType>& magSpectrum, Spectrogram& spectrogram, int fftSize)
    {
        magSpectrum.realloc (fftSize);
        magSpectrum.clear (fftSize);
        
        size_t numColumns = spectrogram.size();

        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int freqBin = 0; freqBin < fftSize / 2 + 1; ++freqBin)
        {
            if (threadShouldExit()) return;
            
            FloatType sum = 0.f;
            
            for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
                sum += spectrogram[freqColumn][freqBin];
                
            magSpectrum[freqBin] = sum / numColumns;
        }
    }

private:
    SpectralSubtractorAudioProcessor* mProcessor;
    HeapBlock<FloatType> mTempNoiseSpectrum;
    juce::File mNoiseFile;
    std::unique_ptr<juce::AudioFormatReader> mReader;
    bool mErrorLoadingFile {false};
    
    // For creating spectrogram
    juce::dsp::FFT mFFT;
    size_t mHop;
    juce::dsp::WindowingFunction<FloatType> mWindow;
};

#if RUN_UNIT_TESTS == 1

// TODO: COMPARE STFT WITH scipy.signal.stft
/*struct NoiseSpectrumProcessingThreadTests : public juce::UnitTest
{
    NoiseSpectrumProcessingThreadTests()
        : juce::UnitTest ("NoiseSpectrumProcessingThread")
    {}
    
    void runTest() override
    {
        beginTest ("");
        {
        }
    }
    
};
static NoiseSpectrumProcessingThreadTests noiseSpectrumProcessingThreadTests;*/

#endif
