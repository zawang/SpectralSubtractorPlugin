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
        
        juce::Time time;
        
        beginTest ("makeSpectrogram() cancellation test");
        {
            auto start = time.getMillisecondCounterHiRes();
            makeSpectrogramPreOptimization (cancellationSpectrogram, &angelsAudio, *(mFFT.get()), hopSize, *(mWindow.get()));
            auto stop = time.getMillisecondCounterHiRes();
            std::cout << "Time elapsed for makeSpectrogramPreOptimization: " << (stop - start) / 1000.0 << std::endl;
            
            start = time.getMillisecondCounterHiRes();
            makeSpectrogram (spectrogram, &angelsAudio, *(mFFT.get()), hopSize, *(mWindow.get()));
            stop = time.getMillisecondCounterHiRes();
            std::cout << "Time elapsed for makeSpectrogram: " << (stop - start) / 1000.0 << std::endl;
            
            for (int i = 0; i < spectrogram.size(); ++i)
            {
                for (int j = 0; j < fftSize; ++j)
                    expectEquals (spectrogram[i][j], cancellationSpectrogram[i][j]);
            }
        }
        
        // TODO: COMPARE STFT WITH scipy.signal.stft
        
        mFFT = nullptr;
        mWindow = nullptr;
    }
    
    // This is what the makeSpectrogram function was before optimizations. Used for cancellation test.
    inline void makeSpectrogramPreOptimization (Spectrogram<FloatType>& spectrogram,
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
                
                for (int j = 0; j < numRows; ++j)
                {
                    spectrogram[i][j] += (fftBuffer[j] / numChannels);
                }
                
                signalData += hopSize;
            }
        }
    }
    
    std::unique_ptr<juce::dsp::FFT> mFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<FloatType>> mWindow;
};

static NoiseSpectrumProcessingTests noiseSpectrumProcessingTests;
