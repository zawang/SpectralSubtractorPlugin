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
    kParameter_TotalNumParameters, // If we ever need to iterate through our parameter list, we can use a for loop using kParameter_TotalNumParameters.
};

// The parameter ID should be a unique identifier for this parameter.
// It's like a variable name; it can contain alphanumeric characters and underscores, but no spaces.
static String ParameterID [kParameter_TotalNumParameters] =
{
    "subtractionStrength"
};

static String ParameterLabel [kParameter_TotalNumParameters] =
{
    "Subtraction Strength"
};
