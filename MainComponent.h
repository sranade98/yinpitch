#pragma once

#include <JuceHeader.h>
#include "FreqSpectrum.h"
#include "FreqSpectrum.h"
#include "StringLabelManager.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
// Define LookAndFeel BEFORE MainComponent
class CustomComboBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::FontOptions("Copperplate", 24.0f, juce::Font::plain);
    }
};





class MainComponent  : public juce::AudioAppComponent,
                       private juce::Timer
{
    
    juce::Label chooseTuning;
    juce::ComboBox tuningSelector;
    juce::ComboBox styleMenu;
    juce::Label textLabel {{}, "Selec t tuning"};
    juce::FontOptions textFont {20.0f};
 

    

    
public:
    //==============================================================================
    MainComponent();
    
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void pushNextSampleIntoFifo (float sample) noexcept;
    void drawNextFrameOfSpectrum();
    void timerCallback() override;
    void releaseResources() override;
    
    void differenceFunction();
    std::vector<float> cumulativeMeanNormalizedDifferenceEquation(const std::vector<float>& differenceFunction, size_t dfSize);

    void getPitch();
    
    void drawFrame (juce::Graphics& g);

    

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    enum {
        fftOrder = 11,
        fftSize = 1 << fftOrder,
        scopeSize = 512,
        f0_min = 82,
        f0_max = 330,
    };
    

private:
    //==============================================================================
    // Your private member variables go here...
    FreqSpectrum fspec;
    
    


    
    juce::GroupComponent border;
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    float fifo[fftSize];
    float fftData[fftSize * 2];
    float yinData[fftSize * 2];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float scopeData[scopeSize];
    double currentSampleRate = 0.0;
    
    std::vector<float> cmndf;
    float currentF0 = -1.0f;
    float yinBuffer[fftSize];
    int yinIndex = 0;
    bool YinBuffReady = false;
    std::vector<float> differenceFunctionResult;
    
    int selectedTuning = 0;
    float* selectedTuningArray;
    float standardTuning[6] = {82.41, 110.00, 146.83, 196.00, 246.94, 329.63};
    float dropDTuning[6]    = {73.42, 110.00, 146.83, 196.00, 246.94, 329.63};
    float openDTuning[6]    = {73.42, 110.00, 146.83, 185.00, 220.00, 293.66};
    
    
    int tau_min = 0;
    int tau_max = 0;
    
    juce::String kanjiYin;
    juce::Label kanjiLabel;

    juce::String finalNoteDisplay;
    //juce::String notesDisplay;
    
    const float kBaseWidth = 1500.0f;
    const float kBaseHeight = 2000.0f;

    juce::int64 lastPitchUpdateTime = 0;

    StringLabelManager stringLabelManager;
    
    juce::Image leftFish;
    juce::Image rightFish;
    juce::Image sonne;
    juce::ImageComponent glowingSonne;
    juce::GlowEffect sonneGlow;
    CustomComboBoxLookAndFeel comboBoxLookAndFeel;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


