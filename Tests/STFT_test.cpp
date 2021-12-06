#include "../Source/DSP/STFT.h"
#include "test_shared.cpp"

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
        stft.prepare (numChannels, fftSize, fftSize / hopSize, window);
        
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
    
    void process (STFT<float>& stft, juce::AudioBuffer<float>& audio, const int blockSize, const int numChannels)
    {
        auto totalNumSamples = audio.getNumSamples();
        int samplePtr = 0;

        while (totalNumSamples > 0)
        {
            auto curBlockSize = jmin (totalNumSamples, blockSize);
            totalNumSamples -= curBlockSize;

            AudioBuffer<float> curBuff (audio.getArrayOfWritePointers(), numChannels, samplePtr, curBlockSize);
            stft.process (curBuff);

            samplePtr += curBlockSize;
        }
    }
};

static STFTTests stftTests;

#endif
