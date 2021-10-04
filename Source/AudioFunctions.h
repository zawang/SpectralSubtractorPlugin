/*
  ==============================================================================

    AudioFunctions.h
    Created: 19 Aug 2019 12:19:50pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include <numeric>
#include "JuceHeader.h"

// One audio channel of FFT data over time, really 2-dimensional
using Spectrogram = std::vector<HeapBlock<float>>;

enum windowTypeIndex        // Used in Filter.h
{
    kWindowTypeRectangular = 0,
    kWindowTypeBartlett,
    kWindowTypeHann,
    kWindowTypeHamming,
};

const static int globalFFTSize = 2048;
const static int globalHopSize = 512;
const static int globalWindow = kWindowTypeHann;
