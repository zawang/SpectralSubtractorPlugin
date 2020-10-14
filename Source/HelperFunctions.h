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

inline void paintComponentLabel(Graphics& g, Component* inComponent)
{
    const int x = inComponent->getX() - (inComponent->getWidth() * 0.25f);
    const int y = inComponent->getY() + inComponent->getHeight();
    const int w = inComponent->getWidth() * 1.5f;
    const int h = 20;
    
    const float cornerSize = 3.0f;
    
    const String label = inComponent->getName();
    
    g.setColour(Colour_3);
    g.fillRoundedRectangle(x, y, w, h, cornerSize);
    
    g.setColour(Colour_6);
    
    Font temp_font_1 ("Helvetica Neue", 12.00f, Font::bold);
    g.setFont(temp_font_1);
    
    // TODO: FIGURE OUT WHY THE LINE BELOW RESULTS IN A LEAK ERROR WHEN THE HOST QUITS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    g.setFont(font_1);
    
    g.drawFittedText(label, x, y, w, h, Justification::centred, 1);
}
