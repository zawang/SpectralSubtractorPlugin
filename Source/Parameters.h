/*
  ==============================================================================

    Parameters.h
    Created: 22 Sep 2020 11:19:32am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

enum Parameter
{
    kParameter_SubtractionStrength = 0,
    kParameter_FFTSize,
    kParameter_TotalNumParameters
};

static juce::String ParameterID [kParameter_TotalNumParameters] =
{
    "subtractionStrength",
    "fftSize"
};

static juce::String ParameterLabel [kParameter_TotalNumParameters] =
{
    "Subtraction Strength",
    "FFT Size"
};

enum FFTSizeIndex
{
    kFFTSize32 = 0,
    kFFTSize64,
    kFFTSize128,
    kFFTSize256,
    kFFTSize512,
    kFFTSize1024,
    kFFTSize2048,
    kFFTSize4096,
    kFFTSize8192,
    kNumFFTSizes
};

static std::array<int, kNumFFTSizes> FFTSize =
{
    32,
    64,
    128,
    256,
    512,
    1024,
    2048,
    4096,
    8192
};

inline juce::StringArray getFFTSizeItems()
{
    juce::StringArray array;
    for (int i = 0; i < FFTSize.size(); ++i)
        array.add (juce::String (FFTSize[i]));
    return array;
}

static juce::StringArray FFTSizeItems = getFFTSizeItems();
