/*
  ==============================================================================

    BottomPanel.cpp
    Created: 15 Aug 2019 11:52:58am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "BottomPanel.h"

BottomPanel::BottomPanel(ExperimentalFilterAudioProcessor* inProcessor)
:   PanelBase(inProcessor) {
    setSize(BOTTOM_PANEL_WIDTH, BOTTOM_PANEL_HEIGHT);
}

BottomPanel::~BottomPanel() {
    
}
