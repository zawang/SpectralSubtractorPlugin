/*
  ==============================================================================

    IDs.h
    Created: 30 Sep 2020 10:21:30am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

namespace IDs
{
    #define DECLARE_ID(name) static const juce::Identifier name (#name);

    DECLARE_ID (NoiseSpectrum)
    DECLARE_ID (AudioData)

    #undef DECLARE_ID
}
