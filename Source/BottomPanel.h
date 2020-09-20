/*
  ==============================================================================

    BottomPanel.h
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"

class BottomPanel
:   public PanelBase {
public:
    BottomPanel(ExperimentalFilterAudioProcessor* inProcessor);
    ~BottomPanel();

private:
};
