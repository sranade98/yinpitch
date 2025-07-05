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
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

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
    
    if (fundamentalFreq > 0.0f)
    {
        const float nyquist = 22050.0f; // or currentSampleRate * 0.5
        float minFreq = 50.0f;
        float maxFreq = 500.0f;
        float x = juce::jmap(juce::jlimit(minFreq, maxFreq, fundamentalFreq), minFreq, maxFreq, 0.0f, width);

        g.setColour(juce::Colours::red);
        g.drawLine(x, 0.0f, x, height, 2.0f);
    }

        
}


void FreqSpectrum::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::whitesmoke);
    drawFrame (g);
    
    if (fundamentalFreq > 0.0f)
    {
        auto width = (float) getLocalBounds().getWidth();
        const float nyquist = 22050.0f;
        float x = juce::jmap(fundamentalFreq, 0.0f, nyquist, 0.0f, width);

        juce::String label = juce::String(fundamentalFreq, 1) + " Hz";
        g.setFont(15.0f);
        g.setColour(juce::Colours::red);
        g.drawText(label, x + 5.0f, 10.0f, 100.0f, 20.0f, juce::Justification::left);
    }
}



void FreqSpectrum::updateSpectrumData (const float* newData, size_t size)
{
    jassert (size <= scopeSize);
    std::copy (newData, newData + size, scopeData);
}

void FreqSpectrum::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}
