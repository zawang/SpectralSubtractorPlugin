/*
  ==============================================================================

    PanelBase.h
    Created: 15 Aug 2019 11:12:10am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "InterfaceDefines.h"

// All other panels are derived from PanelBase
class PanelBase :
    public Component
{
public:
    PanelBase(ExperimentalFilterAudioProcessor* inProcessor);
    ~PanelBase();
        
    void paint(Graphics& g) override;
        
protected:
    ExperimentalFilterAudioProcessor* mProcessor;
};
