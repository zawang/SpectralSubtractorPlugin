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

class MainPanel
:   public PanelBase {
public:
    MainPanel(ExperimentalFilterAudioProcessor* inProcessor);
    ~MainPanel();

private:
    std::unique_ptr<TopPanel> mTopPanel;
    std::unique_ptr<BottomPanel> mBottomPanel;
};
