/*
  ==============================================================================

    FreqSpectrum.cpp
    Created: 10 Jun 2025 2:56:12pm
    Author:  Shaun Ranade

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FreqSpectrum.h"
#include "MainComponent.h"



//==============================================================================
FreqSpectrum::FreqSpectrum()
{

    smoothedX.reset(60.0, 0.15); 


}

FreqSpectrum::~FreqSpectrum()
{
}



void FreqSpectrum::drawFrame (juce::Graphics& g)
{
    auto width  = (float) getLocalBounds().getWidth();
    auto height = (float) getLocalBounds().getHeight();

    for (int i = 1; i < scopeSize; ++i)
        {
            g.drawLine({
                juce::jmap ((float)(i - 1), 0.0f, (float)(scopeSize - 1), 0.0f, width),
                juce::jmap (scopeData[i - 1], 0.0f, 1.0f, height, 0.0f),
                juce::jmap ((float)i, 0.0f, (float)(scopeSize - 1), 0.0f, width),
                juce::jmap (scopeData[i], 0.0f, 1.0f, height, 0.0f)
            });
        }
        
}

void FreqSpectrum::setIdealFrequency(float freq, float min, float max)
{
    idealFreq = freq;
    displayMinFreq = min;
    displayMaxFreq = max;
}


void FreqSpectrum::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    

    
    g.setColour (juce::Colours::orange);
    //drawFrame (g);

    auto width = (float) getLocalBounds().getWidth();
    auto height = (float) getLocalBounds().getHeight();

    if (fundamentalFreq > 0.0f)
    {
        float targetX = juce::jmap(
            juce::jlimit(displayMinFreq, displayMaxFreq, fundamentalFreq),
            displayMinFreq, displayMaxFreq, 0.0f, (float)width);

        smoothedX.setTargetValue(targetX);
    }


    float x = smoothedX.getNextValue();


    trail.push_back({ x, 1.0f }); // New head of the trail


    for (auto& p : trail)
        p.alpha *= 0.80f; 

    while (!trail.empty() && trail.front().alpha < 0.02f)
        trail.pop_front();


    for (const auto& p : trail)
    {
        juce::Path path;
        path.startNewSubPath(p.x, 0.0f);
        path.lineTo(p.x, height);

        g.setColour(juce::Colours::orange.withAlpha(p.alpha));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }



    if (idealFreq > 0.0f)
    {
        float idealX = juce::jmap(
        juce::jlimit(displayMinFreq, displayMaxFreq, idealFreq),
        displayMinFreq, displayMaxFreq, 0.0f, width);

        g.setColour(juce::Colours::orangered);
        g.drawLine(idealX, 0.0f, idealX, height, 3.0f);

        juce::String label = "Target: " + juce::String(idealFreq, 1) + " Hz";
        g.setFont(15.0f);
        g.drawText(label, idealX + 5.0f, 30.0f, 150.0f, 20.0f, juce::Justification::left);
    }
}



void FreqSpectrum::updateSpectrumData (const float* newData, size_t size)
{
    jassert (size <= scopeSize);
    std::copy (newData, newData + size, scopeData);
}

void FreqSpectrum::resized()
{
  

}
