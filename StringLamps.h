/*
  ==============================================================================

    StringLamps.h
    Created: 10 Jun 2025 3:52:19pm
    Author:  Shaun Ranade

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class StringLamp  : public juce::Component
{
public:
    StringLamp();
    ~StringLamp() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void setLampState(bool isOn);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StringLamp)
    
    bool lampOn = false;
};
