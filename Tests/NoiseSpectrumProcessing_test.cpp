#include "../Source/DSP/NoiseSpectrumProcessing.h"
#include "JuceHeader.h"

#if RUN_UNIT_TESTS == 1

struct NoiseSpectrumProcessingTests : public juce::UnitTest
{
    NoiseSpectrumProcessingTests()
        : juce::UnitTest ("NoiseSpectrumProcessing")
    {}
    
    void runTest() override
    {
        beginTest ("");
        {
            
        }
        
        // TODO: COMPARE STFT WITH scipy.signal.stft
    }
};

static NoiseSpectrumProcessingTests noiseSpectrumProcessingTests;

#endif
