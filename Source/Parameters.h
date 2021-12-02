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
    kParameter_HopSize,
    kParameter_TotalNumParameters
};

static const juce::String ParameterID [kParameter_TotalNumParameters] =
{
    "subtractionStrength",
    "fftSize",
    "hopSize"
};

static const juce::String ParameterLabel [kParameter_TotalNumParameters] =
{
    "Subtraction Strength",
    "FFT Size",
    "Hop Size"
};

//==============================================================================
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

static const std::array<int, kNumFFTSizes> FFTSize =
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

static const juce::StringArray FFTSizeItemsUI = getFFTSizeItems();

//==============================================================================
enum HopSizeIndex
{
    kHopSize2 = 0,
    kHopSize4,
    kHopSize8,
    kNumHopSizes
};

static const std::array<int, kNumHopSizes> HopSize =
{
    2,
    4,
    8
};

inline juce::StringArray getHopSizeItems()
{
    juce::StringArray array;
    for (int i = 0; i < HopSize.size(); ++i)
        array.add (juce::String ("1/") + juce::String (HopSize[i]) + juce::String (" Window"));
    return array;
}

static const juce::StringArray HopSizeItemsUI = getHopSizeItems();
