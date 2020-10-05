/*
  ==============================================================================

    MainPanel.cpp
    Created: 15 Aug 2019 11:12:20am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "MainPanel.h"

MainPanel::MainPanel(ExperimentalFilterAudioProcessor* inProcessor)
:   PanelBase(inProcessor)
{
    setSize(MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT);
    
    mTopPanel = std::make_unique<TopPanel>(inProcessor);
    mTopPanel->setTopLeftPosition(0, 0);
    addAndMakeVisible(*mTopPanel);
    
    mBottomPanel = std::make_unique<BottomPanel>(inProcessor);
    mBottomPanel->setTopLeftPosition(0, TOP_PANEL_HEIGHT);
    mBottomPanel->setParameterID(kParameter_SubtractionStrength);
    addAndMakeVisible(*mBottomPanel);
}

MainPanel::~MainPanel() {}
