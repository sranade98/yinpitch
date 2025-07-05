/*
  ==============================================================================

    StringLamps.cpp
    Created: 10 Jun 2025 3:52:19pm
    Author:  Shaun Ranade

  ==============================================================================
*/

#include <JuceHeader.h>
#include "StringLamps.h"

//==============================================================================
StringLamp::StringLamp()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

}

StringLamp::~StringLamp()
{
}

void StringLamp::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(lampOn ? juce::Colours::green : juce::Colours::grey);
    g.fillEllipse(bounds);
    g.drawEllipse(bounds, 2.0f);
}

void StringLamp::setLampState(bool isOn)
{
    if (lampOn != isOn)
        {
            lampOn = isOn;
            repaint(); 
        }
}

void StringLamp::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}
