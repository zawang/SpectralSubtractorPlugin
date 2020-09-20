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

inline void averageSpectrum(Spectrogram& spectrogram, HeapBlock<float>& magSpectrum, int fftSize) {
    magSpectrum.realloc(fftSize);
    magSpectrum.clear(fftSize);
    
    auto numSamples = spectrogram.size();
    
    for (int i = 0; i < numSamples; i++) {
        float sum = 0.0;
        
        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        for (int j = 0; j < fftSize / 2 + 1; j++) {
            sum += spectrogram[i][j];
        }
        magSpectrum[i] = sum / numSamples;
    }
}




//template<class T>
//inline constexpr const T& clamp( const T& v, const T& lo, const T& hi )
//{
//    assert( !(hi < lo) );
//    return (v < lo) ? lo : (hi < v) ? hi : v;
//}






//inline float kap_linear_interpolation(float v0, float v1, float t) {
//    return (1 - t) * v0 + t * v1;
//}



//enum FFTConstants {
//
//};
