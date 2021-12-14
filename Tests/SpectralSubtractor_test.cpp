#include "../Source/DSP/SpectralSubtractor.h"
#include "test_shared.cpp"

#if RUN_UNIT_TESTS == 1

struct SpectralSubtractorTests : public juce::UnitTest
{
    // Taken without modification (except the class name) from
    // https://github.com/juandagilc/Audio-Effects/blob/master/Template%20Frequency%20Domain/Source/STFT.h commit '226660d4ea2a3f888538b414030233dee95f14c3'.
    class STFT
    {
    public:
        enum windowTypeIndex {
            windowTypeRectangular = 0,
            windowTypeBartlett,
            windowTypeHann,
            windowTypeHamming,
        };

        //======================================

        STFT() : numChannels (1)
        {
        }

        virtual ~STFT()
        {
        }

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

        //======================================

        void processBlock (AudioSampleBuffer& block)
        {
            numSamples = block.getNumSamples();

            for (int channel = 0; channel < numChannels; ++channel) {
                float* channelData = block.getWritePointer (channel);

                currentInputBufferWritePosition = inputBufferWritePosition;
                currentOutputBufferWritePosition = outputBufferWritePosition;
                currentOutputBufferReadPosition = outputBufferReadPosition;
                currentSamplesSinceLastFFT = samplesSinceLastFFT;

                for (int sample = 0; sample < numSamples; ++sample) {
                    const float inputSample = channelData[sample];
                    inputBuffer.setSample (channel, currentInputBufferWritePosition, inputSample);
                    if (++currentInputBufferWritePosition >= inputBufferLength)
                        currentInputBufferWritePosition = 0;

                    channelData[sample] = outputBuffer.getSample (channel, currentOutputBufferReadPosition);
                    outputBuffer.setSample (channel, currentOutputBufferReadPosition, 0.0f);
                    if (++currentOutputBufferReadPosition >= outputBufferLength)
                        currentOutputBufferReadPosition = 0;

                    if (++currentSamplesSinceLastFFT >= hopSize) {
                        currentSamplesSinceLastFFT = 0;

                        analysis (channel);
                        modification();
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

            inputBufferWritePosition = 0;
            outputBufferWritePosition = 0;
            outputBufferReadPosition = 0;
            samplesSinceLastFFT = 0;
        }

        void updateHopSize (const int newOverlap)
        {
            overlap = newOverlap;
            if (overlap != 0) {
                hopSize = fftSize / overlap;
                outputBufferWritePosition = hopSize % outputBufferLength;
            }
        }

        void updateWindow (const int newWindowType)
        {
            switch (newWindowType) {
                case windowTypeRectangular: {
                    for (int sample = 0; sample < fftSize; ++sample)
                        fftWindow[sample] = 1.0f;
                    break;
                }
                case windowTypeBartlett: {
                    for (int sample = 0; sample < fftSize; ++sample)
                        fftWindow[sample] = 1.0f - fabs (2.0f * (float)sample / (float)(fftSize - 1) - 1.0f);
                    break;
                }
                case windowTypeHann: {
                    for (int sample = 0; sample < fftSize; ++sample)
                        fftWindow[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (float)sample / (float)(fftSize - 1));
                    break;
                }
                case windowTypeHamming: {
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
            for (int index = 0; index < fftSize; ++index) {
                timeDomainBuffer[index].real (fftWindow[index] * inputBuffer.getSample (channel, inputBufferIndex));
                timeDomainBuffer[index].imag (0.0f);

                if (++inputBufferIndex >= inputBufferLength)
                    inputBufferIndex = 0;
            }
        }

        virtual void modification()
        {
            fft->perform (timeDomainBuffer, frequencyDomainBuffer, false);

            for (int index = 0; index < fftSize / 2 + 1; ++index) {
                float magnitude = abs (frequencyDomainBuffer[index]);
                float phase = arg (frequencyDomainBuffer[index]);

                frequencyDomainBuffer[index].real (magnitude * cosf (phase));
                frequencyDomainBuffer[index].imag (magnitude * sinf (phase));
                if (index > 0 && index < fftSize / 2) {
                    frequencyDomainBuffer[fftSize - index].real (magnitude * cosf (phase));
                    frequencyDomainBuffer[fftSize - index].imag (magnitude * sinf (-phase));
                }
            }

            fft->perform (frequencyDomainBuffer, timeDomainBuffer, true);
        }

        void synthesis (const int channel)
        {
            int outputBufferIndex = currentOutputBufferWritePosition;
            for (int index = 0; index < fftSize; ++index) {
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
    
    SpectralSubtractorTests()
        : juce::UnitTest ("STFT")
    {}
    
    void runTest() override
    {
        const int samplesPerBlock = 256;
        const int numChannels = 1;
        
        const int fftSize = 2048;
        const int windowOverlap = 4;
        const int window = SpectralSubtractor<TestFloatType>::kWindowTypeHann;
        
        std::atomic<float>* dummySubtractionStrength = new std::atomic<float>;
        SpectralSubtractor<TestFloatType> spectralSubtractor;
        spectralSubtractor.setSubtractionStrength (dummySubtractionStrength);
        spectralSubtractor.reset (fftSize);
        spectralSubtractor.prepare (numChannels, fftSize, windowOverlap, window);
        
        STFT stft;
        stft.setup (numChannels);
        stft.updateParameters (fftSize, windowOverlap, window);
        
        // TODO: come up with a cleaner way of finding the proper directory and doing this entire test in general
        juce::File aircommFile {"/Users/zach/Audio Programming/JUCE Projects/Personal Projects/SpectralSubtractor/Test Data/aircomm.wav"};
        jassert (aircommFile.existsAsFile());
        
        juce::AudioBuffer<TestFloatType> aircommOG;
        getAudioFile (aircommOG, aircommFile);
        juce::AudioBuffer<TestFloatType> aircommCopy;
        aircommCopy.makeCopyOf (aircommOG);
        juce::AudioBuffer<TestFloatType> cancellationAircomm;
        cancellationAircomm.makeCopyOf (aircommOG);
        
        beginTest ("STFT cancellation equivalence test");
        {
            stftProcess (stft, cancellationAircomm, samplesPerBlock, numChannels);
            spectralSubtractorProcess (spectralSubtractor, aircommCopy, samplesPerBlock, numChannels);
            
            const TestFloatType* aircommCopyData = aircommCopy.getReadPointer (0, 0);
            const TestFloatType* cancellationAircommData = cancellationAircomm.getReadPointer (0, 0);
            for (int i = 0; i < aircommCopy.getNumSamples(); ++i)
                expectEquals (aircommCopyData[i], cancellationAircommData[i]);
        }
        
        beginTest ("STFT cancellation performance test");
        {
            int numRuns = 50;
            juce::File desktop = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);
            PerformanceProfiler ppSTFT ("STFT", numRuns, desktop.getChildFile ("STFT_log.txt"));
            PerformanceProfiler ppSpectralSubtractor ("Spectral Subtractor", numRuns, desktop.getChildFile ("SpectralSubtractor_log.txt"));
            
            cancellationAircomm.makeCopyOf (aircommOG);
            aircommCopy.makeCopyOf (aircommOG);
            
            for (int i = 0; i < numRuns; ++i)
            {
                ppSTFT.start();
                stftProcess (stft, cancellationAircomm, samplesPerBlock, numChannels);
                ppSTFT.stop();
            }
            
            for (int i = 0; i < numRuns; ++i)
            {
                ppSpectralSubtractor.start();
                spectralSubtractorProcess (spectralSubtractor, aircommCopy, samplesPerBlock, numChannels);
                ppSpectralSubtractor.stop();
            }
            
            expectLessOrEqual (ppSpectralSubtractor.getAverageSeconds(), ppSTFT.getAverageSeconds());
            
            ppSTFT.printStatisticsAndReset();
            ppSpectralSubtractor.printStatisticsAndReset();
        }
        
        aircommCopy.makeCopyOf (aircommOG);
        
        beginTest ("STFT self-cancellation test");
        {
            spectralSubtractorProcess (spectralSubtractor, aircommCopy, samplesPerBlock, numChannels);
            
            const TestFloatType* aircommOGData = aircommOG.getReadPointer (0, 0);
            const TestFloatType* aircommCopyData = aircommCopy.getReadPointer (0, fftSize);
            TestFloatType maxAbsoluteError = static_cast<TestFloatType> (0.0001);
            for (int i = 0; i < aircommOG.getNumSamples() - fftSize; ++i)
                expectWithinAbsoluteError (aircommCopyData[i], aircommOGData[i], maxAbsoluteError);
        }
        
        delete dummySubtractionStrength;
    }
    
    void spectralSubtractorProcess (SpectralSubtractor<TestFloatType>& spectralSubtractor, juce::AudioBuffer<TestFloatType>& audio, const int blockSize, const int numChannels)
    {
        auto totalNumSamples = audio.getNumSamples();
        int samplePtr = 0;

        while (totalNumSamples > 0)
        {
            auto curBlockSize = jmin (totalNumSamples, blockSize);
            totalNumSamples -= curBlockSize;

            AudioBuffer<TestFloatType> curBuff (audio.getArrayOfWritePointers(), numChannels, samplePtr, curBlockSize);
            spectralSubtractor.process (curBuff);

            samplePtr += curBlockSize;
        }
    }
    
    void stftProcess (STFT& stft, juce::AudioBuffer<TestFloatType>& audio, const int blockSize, const int numChannels)
    {
        auto totalNumSamples = audio.getNumSamples();
        int samplePtr = 0;

        while (totalNumSamples > 0)
        {
            auto curBlockSize = jmin (totalNumSamples, blockSize);
            totalNumSamples -= curBlockSize;

            AudioBuffer<TestFloatType> curBuff (audio.getArrayOfWritePointers(), numChannels, samplePtr, curBlockSize);
            stft.processBlock (curBuff);

            samplePtr += curBlockSize;
        }
    }
};

static SpectralSubtractorTests stftTests;

#endif
