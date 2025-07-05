/*
  ==============================================================================

    FreqSpectrum.h
    Created: 10 Jun 2025 2:56:12pm
    Author:  Shaun Ranade

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/*
*/
class FreqSpectrum  : public juce::Component
{
public:
    void setFundamentalFrequency(float freq);
    FreqSpectrum();
    
    ~FreqSpectrum() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;
    void drawFrame (juce::Graphics& g);
    void updateSpectrumData (const float* newData, size_t size);
    


private:
    static constexpr size_t scopeSize = 512;  // Same value as MainComponent
    float scopeData[scopeSize] = { 0.0f };
    float fundamentalFreq = -1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqSpectrum)
};
