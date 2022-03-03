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
    kParameter_WindowOverlap,
    kParameter_Window,
    kParameter_TotalNumParameters
};

static const juce::String ParameterID [kParameter_TotalNumParameters] =
{
    "subtractionStrength",
    "fftSize",
    "windowOverlap",
    "window"
};

static const juce::String ParameterLabel [kParameter_TotalNumParameters] =
{
    "Subtraction Strength",
    "FFT Size",
    "Window Overlap",
    "Window"
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

static constexpr std::array<int, kNumFFTSizes> FFTSize =
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
enum WindowOverlapIndex
{
    kWindowOverlap2 = 0,
    kWindowOverlap4,
    kWindowOverlap8,
    kNumWindowOverlaps
};

static constexpr std::array<int, kNumWindowOverlaps> WindowOverlap =
{
    2,
    4,
    8
};

inline juce::StringArray getWindowOverlapItems()
{
    juce::StringArray array;
    for (int i = 0; i < WindowOverlap.size(); ++i)
        array.add (juce::String ("1/") + juce::String (WindowOverlap[i]) + juce::String (" Window"));
    return array;
}

static const juce::StringArray WindowOverlapItemsUI = getWindowOverlapItems();

//==============================================================================
static const StringArray WindowTypeItemsUI =
{
    "Rectangular",
    "Bartlett",
    "Hann",
    "Hamming"
};
