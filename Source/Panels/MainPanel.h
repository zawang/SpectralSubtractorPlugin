/*
  ==============================================================================

    MainPanel.h
    Created: 15 Aug 2019 11:12:20am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "TopPanel.h"
#include "BottomPanel.h"

// This class will be the parent component of all the other sub panels

class MainPanel
:   public PanelBase
{
public:
    MainPanel (SpectralSubtractorAudioProcessor* inProcessor);
    ~MainPanel();
    
    void resized() override;

private:
    TopPanel mTopPanel;
    BottomPanel mBottomPanel;
};
