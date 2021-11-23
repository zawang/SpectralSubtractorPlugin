/*
  ==============================================================================

    AudioFunctions.h
    Created: 19 Aug 2019 12:19:50pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "STFT.h"

const static int globalFFTSize = 2048;
const static int globalHopSize = 512;
const static int globalWindow = STFT::kWindowTypeHann;
