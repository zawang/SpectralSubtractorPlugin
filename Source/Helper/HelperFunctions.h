/*
  ==============================================================================

    HelperFunctions.h
    Created: 23 Sep 2020 2:46:34pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "InterfaceDefines.h"

inline void paintComponentLabel (juce::Graphics& g, juce::Component* inComponent)
{
    const int x = inComponent->getX() - (inComponent->getWidth() * 0.25f);
    const int y = inComponent->getY() + inComponent->getHeight();
    const int w = inComponent->getWidth() * 1.5f;
    const int h = 20;
    
    const float cornerSize = 3.0f;
    
    const juce::String label = inComponent->getName();
    
    g.setColour (Colour_3);
    g.fillRoundedRectangle (x, y, w, h, cornerSize);
    
    g.setColour (Colour_6);
    
    juce::Font temp_font_1 ("Helvetica Neue", 12.00f, juce::Font::bold);
    g.setFont(temp_font_1);
    
    // TODO: THE LINE BELOW RESULTS IN A LEAK ERROR WHEN THE HOST QUITS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    g.setFont(font_1);
    
    g.drawFittedText (label, x, y, w, h, juce::Justification::centred, 1);
}
