/*
  ==============================================================================

    AudioFunctions.h
    Created: 19 Aug 2019 12:19:50pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include <numeric>

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

inline void averageSpectrum(Spectrogram& spectrogram, HeapBlock<float>& magSpectrum, int fftSize)
{
    magSpectrum.realloc(fftSize);
    magSpectrum.clear(fftSize);
    
    auto numColumns = spectrogram.size();
    
    // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
    for (int freqBin = 0; freqBin < fftSize / 2 + 1; ++freqBin)
    {
        float sum = 0.f;
        
        for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
            sum += spectrogram[freqColumn][freqBin];
            
        magSpectrum[freqBin] = sum / numColumns;
    }
}
