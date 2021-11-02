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
    kParameter_TotalNumParameters
};

static String ParameterID [kParameter_TotalNumParameters] =
{
    "subtractionStrength"
};

static String ParameterLabel [kParameter_TotalNumParameters] =
{
    "Subtraction Strength"
};
