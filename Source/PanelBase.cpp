/*
  ==============================================================================

    PanelBase.cpp
    Created: 15 Aug 2019 11:12:10am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "PanelBase.h"

PanelBase::PanelBase(SpectralSubtractorAudioProcessor* inProcessor)
    : mProcessor (inProcessor) {}

PanelBase::~PanelBase() {}

void PanelBase::paint(Graphics& g)
{
    g.setColour (Colours::whitesmoke);
    g.fillAll();
    
    g.setColour (Colours::black);
    g.drawRect (0, 0, getWidth(), getHeight());
}
