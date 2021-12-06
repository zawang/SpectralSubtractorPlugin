#include "../Source/DSP/NoiseSpectrumProcessing.h"
#include "test_shared.cpp"

struct NoiseSpectrumProcessingTests : public juce::UnitTest
{
    NoiseSpectrumProcessingTests()
        : juce::UnitTest ("NoiseSpectrumProcessing")
    {}
    
    void runTest() override
    {
        int fftSize = 2048;
        mFFT.reset (new juce::dsp::FFT (std::log2 (fftSize)));
        size_t hopSize = mFFT->getSize() / 8;
        mWindow.reset (new juce::dsp::WindowingFunction<FloatType> (fftSize + 1, juce::dsp::WindowingFunction<FloatType>::hann, false));
        
        // TODO: come up with a cleaner way of finding the proper directory
        juce::File angelsFile {"/Users/zach/Audio Programming/JUCE Projects/Personal Projects/SpectralSubtractor/Test Data/aircomm.wav"};
        jassert (angelsFile.existsAsFile());
        
        juce::AudioBuffer<FloatType> angelsAudio;
        getAudioFile (angelsAudio, angelsFile);
        Spectrogram<FloatType> spectrogram;
        Spectrogram<FloatType> cancellationSpectrogram;
        HeapBlock<FloatType> magSpectrum;
        HeapBlock<FloatType> cancellationMagSpectrum;
        
        juce::Time time;
        
        {
            beginTest ("makeSpectrogram() cancellation test");
            
            auto start = time.getMillisecondCounterHiRes();
            makeSpectrogramPreOpt (cancellationSpectrogram, &angelsAudio, *(mFFT.get()), hopSize, *(mWindow.get()));
            auto stop = time.getMillisecondCounterHiRes();
            auto preOptTimeElapsed = (stop - start) / 1000.0;
            std::cout << "Time elapsed for makeSpectrogramPreOptimization: " << preOptTimeElapsed << std::endl;
            
            start = time.getMillisecondCounterHiRes();
            makeSpectrogram (spectrogram, &angelsAudio, *(mFFT.get()), hopSize, *(mWindow.get()));
            stop = time.getMillisecondCounterHiRes();
            auto postOptTimeElapsed = (stop - start) / 1000.0;
            std::cout << "Time elapsed for makeSpectrogram: " << postOptTimeElapsed << std::endl;
            
            for (int i = 0; i < spectrogram.size(); ++i)
            {
                for (int j = 0; j < fftSize; ++j)
                    expectEquals (spectrogram[i][j], cancellationSpectrogram[i][j]);
            }
            
            expectLessOrEqual (postOptTimeElapsed, preOptTimeElapsed);
            
            //==============================================================================
            //==============================================================================
            //==============================================================================
            
            beginTest ("computeAverageSpectrum() cancellation test");
        
            start = time.getMillisecondCounterHiRes();
            computeAverageSpectrumPreOpt (cancellationMagSpectrum, spectrogram, fftSize);
            stop = time.getMillisecondCounterHiRes();
            preOptTimeElapsed = (stop - start) / 1000.0;
            std::cout << "Time elapsed for computeAverageSpectrumPreOpt: " << preOptTimeElapsed << std::endl;
            
            start = time.getMillisecondCounterHiRes();
            computeAverageSpectrum (magSpectrum, spectrogram, fftSize);
            stop = time.getMillisecondCounterHiRes();
            postOptTimeElapsed = (stop - start) / 1000.0;
            std::cout << "Time elapsed for computeAverageSpectrum: " << postOptTimeElapsed << std::endl;
            
            for (int i = 0; i < fftSize; ++i)
                expectWithinAbsoluteError (magSpectrum[i], cancellationMagSpectrum[i], static_cast<FloatType> (0.00001));
            
            expectLessOrEqual (postOptTimeElapsed, preOptTimeElapsed);
        }
        
        // TODO: COMPARE STFT WITH scipy.signal.stft
        
        mFFT = nullptr;
        mWindow = nullptr;
    }
    
    // This is what the makeSpectrogram() function was before optimizations. Used for cancellation test.
    inline void makeSpectrogramPreOpt (Spectrogram<FloatType>& spectrogram,
                                       juce::AudioBuffer<FloatType>* signal,
                                       juce::dsp::FFT& fft,
                                       size_t hopSize,
                                       juce::dsp::WindowingFunction<FloatType>& window)
    {
        const size_t dataCount = signal->getNumSamples();
        
        ptrdiff_t fftSize = fft.getSize();
        
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / hopSize);
        
        spectrogram.resize (numHops + 1);
        for (int i = 0; i < spectrogram.size(); ++i)
        {
            spectrogram[i].realloc (fftSize);
            spectrogram[i].clear (fftSize);
        }
        
        size_t numRows = 1UL + (fftSize / 2UL);
        
        int numChannels = signal->getNumChannels();
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            std::vector<FloatType> fftBuffer (fftSize * 2UL);
        
            const FloatType* signalData = signal->getReadPointer (channel, 0);
            for (int i = 0; i < numHops; ++i)
            {
                std::memcpy (fftBuffer.data(), signalData, fftSize * sizeof (FloatType));
                
                window.multiplyWithWindowingTable (fftBuffer.data(), fftSize);
                
                fft.performFrequencyOnlyForwardTransform (fftBuffer.data());
                
                // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
                for (int j = 0; j < numRows; ++j)
                {
                    spectrogram[i][j] += (fftBuffer[j] / numChannels);      // Divide by numChannels because we're calculating the average spectrogram
                }
                
                signalData += hopSize;
            }
        }
    }
    
    // This is what the computeAverageSpectrum() function was before optimizations. Used for cancellation test.
    inline void computeAverageSpectrumPreOpt (HeapBlock<FloatType>& magSpectrum, Spectrogram<FloatType>& spectrogram, int fftSize)
    {
        magSpectrum.realloc (fftSize);
        magSpectrum.clear (fftSize);
        
        size_t numColumns = spectrogram.size();

        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
        {
            for (int freqBin = 0; freqBin < fftSize / 2 + 1; ++freqBin)
            {
                magSpectrum[freqBin] += spectrogram[freqColumn][freqBin] / numColumns;
            }
        }
    }
    
    std::unique_ptr<juce::dsp::FFT> mFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<FloatType>> mWindow;
};

static NoiseSpectrumProcessingTests noiseSpectrumProcessingTests;
