#include "MainComponent.h"
#include "FreqSpectrum.h"
#include <limits>
#include "StringLabelManager.h"




//==============================================================================
MainComponent::MainComponent() : forwardFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    setAudioChannels (2, 0); 
    startTimerHz (30);
    setSize (1500, 2000);
    
    // 1. Load and setup the sun image
    sonne = juce::ImageCache::getFromMemory(BinaryData::Sonne_png, BinaryData::Sonne_pngSize);
    glowingSonne.setImage(sonne);
    sonneGlow.setGlowProperties(50.0f, juce::Colours::orange.withAlpha(0.5f)); // Spread, Opacity
    glowingSonne.setComponentEffect(&sonneGlow);
    addAndMakeVisible(glowingSonne);
    // NEW: Create kanji label
    kanjiLabel.setText("Yin Pitch", juce::dontSendNotification);
    kanjiLabel.setFont(juce::FontOptions("Pauls Kanji Font", 150.0f, juce::Font::plain));
    kanjiLabel.setJustificationType(juce::Justification::centredTop);
    kanjiLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    kanjiLabel.setInterceptsMouseClicks(false, false); 

    addAndMakeVisible(kanjiLabel);
    kanjiLabel.toFront(false); 

    

    chooseTuning.setText("-", juce::dontSendNotification);
    
    tuningSelector.addItem("Select your tuning ", 1);
    
    tuningSelector.addItem("Standard Tuning (E A D G B E)", 2);
    tuningSelector.addItem("Drop D Tuning (D A D G B E)", 3);
    tuningSelector.addItem("Open D Tuning (D A D F# A D)", 4);
    tuningSelector.setSelectedId(1);
    tuningSelector.setLookAndFeel(&comboBoxLookAndFeel);
    chooseTuning.setFont(juce::FontOptions("Copperplate", 100.0f, juce::Font::plain));
    addAndMakeVisible(chooseTuning);
    
    border.setLookAndFeel(&comboBoxLookAndFeel);
       
    
    tuningSelector.onChange = [this]()
    {
        selectedTuning = tuningSelector.getSelectedId();

        switch (selectedTuning)
        {
            case 2: selectedTuningArray = standardTuning; break;
            case 3: selectedTuningArray = dropDTuning; break;
            case 4: selectedTuningArray = openDTuning; break;
            default: selectedTuningArray = nullptr; break;
        }

        juce::StringArray stringNotes;

        switch (selectedTuning)
        {
            case 2: stringNotes = { "E", "A", "D", "G", "B", "E" }; break;
            case 3: stringNotes = { "D", "A", "D", "G", "B", "E" }; break;
            case 4: stringNotes = { "D", "A", "D", "F#", "A", "D" }; break;
            default: stringNotes = { "", "", "", "", "", "" }; break;
        }

        stringLabelManager.updateTuningLabels(stringNotes);

        repaint();
    };


    selectedTuningArray = nullptr;
  
    addAndMakeVisible(tuningSelector);
    tuningSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
    tuningSelector.setColour(juce::ComboBox::textColourId, juce::Colours::orange);
    tuningSelector.setColour(juce::ComboBox::arrowColourId, juce::Colours::orange);
    tuningSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::orange);


    addAndMakeVisible(fspec);
    


    
    stringLabelManager.addLabelsTo(*this);
    
    leftFish = juce::ImageCache::getFromMemory(BinaryData::LeftFish_png, BinaryData::LeftFish_pngSize);
    rightFish = juce::ImageCache::getFromMemory(BinaryData::RightFish_png, BinaryData::RightFish_pngSize);
    sonne = juce::ImageCache::getFromMemory(BinaryData::Sonne_png, BinaryData::Sonne_pngSize);
    

    
    
}

MainComponent::~MainComponent()
{
    
    shutdownAudio(); // Shut down audio
    tuningSelector.setLookAndFeel(nullptr);
    

}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    currentSampleRate = sampleRate; // store sample rate for calculating tau_max, tau_min
    tau_max = currentSampleRate/f0_min; // 537
    tau_min = currentSampleRate/f0_max; //133
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    
    if (bufferToFill.buffer->getNumChannels()>0)
    {
        auto *channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        for (auto i = 0; i < bufferToFill.numSamples; ++i) // bufferToFill.numSamples = 512 samples
            pushNextSampleIntoFifo(channelData[i]);
            //differenceFunction(channelData[i]);
    }
}

void MainComponent::pushNextSampleIntoFifo (float sample) noexcept
{
    if (fifoIndex == fftSize) 
    {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));

            memcpy(fftData, fifo, sizeof(fifo));

            nextFFTBlockReady = true;

        }
        fifoIndex = 0; 
    }
    fifo[fifoIndex++] = sample; 
    
    if (yinIndex == fftSize) 
    {
        if (!YinBuffReady)
        {
            juce::zeromem(yinData, sizeof(yinData));
            memcpy(yinData, yinBuffer, sizeof(yinBuffer));
            YinBuffReady = true;
        }
        yinIndex = 0;
    }
    yinBuffer[yinIndex++] = sample;
    
    
    
}
// -------------------------------- Y I N Functions -----------------------------------------
void MainComponent::differenceFunction()
{
    
    auto audio_data = yinBuffer;
    int n = sizeof(yinBuffer)/sizeof(yinBuffer[0]); 
    auto lagRangeMax = std::min(tau_max, n); 
    

    std::vector<float> audio_cumsum;
    float sum = 0.0f;
    for (auto i = 0; i < n ; ++i) 
    {
        sum += audio_data[i] * audio_data[i]; 
        audio_cumsum.push_back(sum); 
    }
    audio_cumsum.insert(audio_cumsum.begin(), 0.0f); 
    

    
    
    int size = lagRangeMax + n; 
    int size_pad = juce::nextPowerOfTwo(size); 
    

    
    juce::dsp::FFT fft(log2(size_pad));  
    
 

    std::vector<float> fftBuffer(size_pad * 2, 0.0f); 


    std::copy(audio_data, audio_data + n, fftBuffer.begin());
    
    

    // Do FFT in-place:
    fft.performRealOnlyForwardTransform(fftBuffer.data());

    
    
    
    // 1) Compute power spectrum:
    for (int k = 0; k < size_pad / 2 + 1; ++k) 
    {
        float re = fftBuffer[2 * k]; 
        float im = fftBuffer[2 * k + 1]; 
        fftBuffer[2 * k] = re * re + im * im;  
        fftBuffer[2 * k + 1] = 0.0f;           
    }
    
    // fftBuffer now contains interleaved data of re and im parts. eg: [1, 0, 2, 0....]

   
    fft.performRealOnlyInverseTransform(fftBuffer.data()); // perform IFFT on the fft power spectrum
    // real values stored in first half [0....4095] (4096th element is the tau_max)

    
    
    std::vector<float> conv(fftBuffer.begin(), fftBuffer.begin() + lagRangeMax);
    // Initialize conv with the values of fftBuffer from beginning to end
    
    std::vector<float> dfResult(lagRangeMax);
    for (int tau = 0; tau < lagRangeMax; ++tau) // Start with 0 lag with tau being less than max lag (547 samples)
    {
        // Refer to eq 7 from the paper
        float term1 = audio_cumsum[n] - audio_cumsum[tau]; // energy term1 (r_t)
        float term2 = audio_cumsum[n] - audio_cumsum[n - tau]; // energy term2 (t_t+tau)
        dfResult[tau] = term1 + term2 - 2.0f * conv[tau];
    }
    
    size_t df_size = dfResult.size(); 
    
    cmndf = cumulativeMeanNormalizedDifferenceEquation(dfResult, df_size);

}


std::vector<float> MainComponent::cumulativeMeanNormalizedDifferenceEquation(const std::vector<float>& differenceFunction, size_t dfSize)
{
    std::vector<float> cmndf(dfSize);
    cmndf[0] = 1.0f; // Set the first element in df equation to 1, refer to Step: 3 in CMNDF

    float cumulativeSum = 0.0f;
    for (size_t tau = 1; tau < dfSize; ++tau) // cycle through every lag
    {
        cumulativeSum += differenceFunction[tau]; // Add that value to the cumulative sum
        cmndf[tau] = differenceFunction[tau] * tau / cumulativeSum; // ALWAYS multiply formulas first because of floating point precision
    
    }

    return cmndf; 
}

void MainComponent::getPitch()
{


    // Reset all string labels' appearance
    stringLabelManager.resetAllLabels();


    if (cmndf.empty() || selectedTuningArray == nullptr)
        return;

    constexpr float threshold = 0.1f;
    constexpr float matchToleranceHz = 30.0f;

    float bestF0 = -1.0f;
    float bestCMNDF = 1.0f;
    size_t bestTau = 0;

    
    for (size_t tau = tau_min; tau < cmndf.size(); ++tau)
    {
        float value = cmndf[tau];
        if (value < threshold && value < bestCMNDF)
        {
            float f0 = currentSampleRate / static_cast<float>(tau);

       
            for (int i = 0; i < 6; ++i)
            {
                if (std::abs(f0 - selectedTuningArray[i]) < matchToleranceHz)
                {
                    bestCMNDF = value;
                    bestF0 = f0;
                    bestTau = tau;
                    break;
                }
            }
        }
    }


    if (bestF0 > 0.0f)
    {
        juce::int64 now = juce::Time::getMillisecondCounter();
        const int updateIntervalMs = 100;

        if (now - lastPitchUpdateTime > updateIntervalMs)
        {
            lastPitchUpdateTime = now;

            const float smoothingFactor = 0.9f;
            if (currentF0 < 0.0f)
                currentF0 = bestF0; 
            else
                currentF0 = (1.0f - smoothingFactor) * bestF0 + smoothingFactor * currentF0;
        }


        int closestStringIndex = -1;
        float minDiff = std::numeric_limits<float>::max();

        for (int i = 0; i < 6; ++i)
        {
            float diff = std::abs(selectedTuningArray[i] - currentF0);
            if (diff < minDiff)
            {
                minDiff = diff;
                closestStringIndex = i;
            }
        }

        if (closestStringIndex != -1)
        {
            float idealFreq = selectedTuningArray[closestStringIndex];
            float minFreq = idealFreq - 50.0f;
            float maxFreq = idealFreq + 50.0f;
            fspec.setIdealFrequency(idealFreq, minFreq, maxFreq);

            stringLabelManager.updateLabelHighlight(closestStringIndex, minDiff <= 1.0f);
        }
    }
    else
    {
        currentF0 = -1.0f; 
    }

}



// -------------------------------------------------------------------------------------------
void MainComponent::drawNextFrameOfSpectrum() 
{
    
    window.multiplyWithWindowingTable (fftData, fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform (fftData);
    auto mindB = -55.0f;
    auto maxdB = 0.0f;
    for (int i = 0; i < scopeSize; ++i) 
    {
        auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) scopeSize) * 0.2f); 
        auto fftDataIndex = juce::jlimit (0, fftSize / 2, (int) (skewedProportionX * (float) fftSize * 0.5f));
        auto level = juce::jmap (juce::jlimit (mindB, maxdB, juce::Decibels::gainToDecibels (fftData[fftDataIndex]) - juce::Decibels::gainToDecibels ((float) fftSize)),
            mindB,
            maxdB,
            0.0f,
            1.0f);
        scopeData[i] = level; 
    }
    
}

void MainComponent::timerCallback()
{
    if (nextFFTBlockReady)
     {
         drawNextFrameOfSpectrum();
         nextFFTBlockReady = false;

         fspec.updateSpectrumData(scopeData, scopeSize);
         fspec.setFundamentalFrequency(currentF0);
         fspec.repaint();
     }

     if (YinBuffReady)
     {
         // --- Compute RMS ---
         float rms = 0.0f;
         for (int i = 0; i < fftSize; ++i)
             rms += yinBuffer[i] * yinBuffer[i];

         rms = std::sqrt(rms / (float)fftSize);

         const float volumeThreshold = 0.02f; // 0.01f works best

         if (rms > volumeThreshold)
         {
             differenceFunction();
             getPitch();
         }
         else
         {
             currentF0 = -1.0f; 
         }

         YinBuffReady = false;
     }
    
}

void MainComponent::drawFrame (juce::Graphics& g)
{
    for (int i = 1; i < scopeSize; ++i)
    {
        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();
        g.drawLine ({ (float) juce::jmap (i - 1, 0, scopeSize - 1, 0, width),
            juce::jmap (scopeData[i - 1], 0.0f, 1.0f, (float) height, 0.0f),
            (float) juce::jmap (i, 0, scopeSize - 1, 0, width),
            juce::jmap (scopeData[i], 0.0f, 1.0f, (float) height, 0.0f) });
    }
}

void FreqSpectrum::setFundamentalFrequency(float freq)
{
    fundamentalFreq = freq;

}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    constexpr float kBaseWidth = 1500.0f;
    float scale = getWidth() / kBaseWidth;

    g.fillAll (juce::Colours::black);


    addAndMakeVisible(border);
    border.setText("Frequency Spectrum");
    border.setColour(juce::GroupComponent::textColourId,juce::Colours::orange);
  


    

    g.setColour(juce::Colours::orange);
    
    

    
    
    
    g.setFont(juce::FontOptions("Times New Roman", 60.0f, juce::Font::plain));

    

    

    constexpr float relativeX = 0.167f;
    constexpr float relativeY = 0.9f;
    constexpr float relativeWidth = 0.666f;
    constexpr float relativeHeight = 0.05f;

    juce::Rectangle<int> bottomArea(
        juce::roundToInt(getWidth() * relativeX),
        juce::roundToInt(getHeight() * relativeY),
        juce::roundToInt(getWidth() * relativeWidth),
        juce::roundToInt(getHeight() * relativeHeight)
    );

    // Optional: scaled font size

    g.setFont(juce::FontOptions("Copperplate", 60.0f * scale, juce::Font::plain));
    
    // COMMENTED OUT REINSTATE IF ALT METHOD WORKS
    //    juce::String finalNoteDisplay;
//    for (int i = 0; i < notesDisplay.length(); ++i)
//    {
//        finalNoteDisplay += notesDisplay[i];
//
//        if(i+1 < notesDisplay.length() && notesDisplay[i+1] == '#')
//        {
//            finalNoteDisplay += '#';
//            ++i;
//        }
//
//        else
//        {
//            finalNoteDisplay += "    ";
//        }
//      
//    }

    // Draw text
    g.drawText(finalNoteDisplay, bottomArea, juce::Justification::horizontallyCentred, false);
    

    float x1 = 50;
    float y1 = 300;
    float x2 = 1100;
    float y2 = 325;
    float w = getWidth() * 0.20f;
    float h = getHeight() * 0.65f;

    


    juce::AffineTransform transform1 = juce::AffineTransform::scale(
        w / leftFish.getWidth(),
        h / leftFish.getHeight()
    );


    transform1 = transform1.translated(x1, y1);


    transform1 = transform1.rotated(juce::degreesToRadians(10.0f), x1 + w / 2.0f, y1 + h / 2.0f);


    g.drawImageTransformed(leftFish, transform1);
    
    
    

    juce::AffineTransform transform2 = juce::AffineTransform::scale(
        w / rightFish.getWidth(),
        h / rightFish.getHeight()
    );


    transform2 = transform2.translated(x2, y2);


    transform2 = transform2.rotated(juce::degreesToRadians(20.0f), x2 + w / 2.0f, y2 + h / 2.0f);

    g.drawImageTransformed(rightFish, transform2);



}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    //    auto area = getLocalBounds().reduced(getWidth() * 0.05f);
    //
    //    auto topbar = area.removeFromTop(proportionOfHeight(0.01f));
    //    chooseTuning.setBounds(topbar.withHeight(30).withSizeKeepingCentre(getWidth()*0.9f, 30));
    //
    //    // Take next 10% for dropdown
    //    auto dropdownArea = area.removeFromTop(proportionOfHeight(0.1f));
    //    tuningSelector.setBounds(dropdownArea.withHeight(30).withSizeKeepingCentre(getWidth() * 0.5f, 30));
    //
    //    // Centered box for spectrum
    //    auto spectrumArea = area.removeFromTop(proportionOfHeight(0.6f));
    //    fspec.setBounds(spectrumArea.withSizeKeepingCentre(getWidth() * 0.6f, getHeight() * 0.5f));
    
    
    float scaleFactor = 1.3f; // 120% of window size

    int sunWidth = getWidth() * scaleFactor;
    int sunHeight = getHeight() * scaleFactor;

    
    int sunX = (getWidth() - sunWidth) / 2 + 10;
    int sunY = (getHeight() - sunHeight) / 2 -50 ;

    glowingSonne.setBounds(sunX, sunY, sunWidth, sunHeight);
    

    constexpr float relativeX_kanji = 0.21f;
    constexpr float relativeY_kanji = 0.13f;
    constexpr float relativeWidth_kanji = 0.6f;
    constexpr float relativeHeight_kanji = 0.2f;

    kanjiLabel.setBounds(
        juce::roundToInt(getWidth() * relativeX_kanji),
        juce::roundToInt(getHeight() * relativeY_kanji),
        juce::roundToInt(getWidth() * relativeWidth_kanji),
        juce::roundToInt(getHeight() * relativeHeight_kanji));
    

    
    chooseTuning.setBounds(
                           getWidth() * 0.45f,  
                           getHeight() * 0.40f, 
                           getWidth() * 0.5f,    
                           getHeight() * 0.03f                   
                           );
    
    tuningSelector.setBounds(
                             getWidth() * 0.1875f,   
                             getHeight() * 0.33f,   
                             getWidth() * 0.620f,    
                             getHeight() * 0.035f                 
                             );
    
    fspec.setBounds(
                    getWidth() * 0.1870f,   
                    getHeight() * 0.40f,    
                    getWidth() * 0.625f,    
                    getHeight() * 0.4f      
                    );
    
    border.setBounds(
                     getWidth() * 0.1860f,
                     getHeight() * 0.389f,
                     getWidth() * 0.625f,
                     getHeight() * 0.420f);
    
    
    float labelY = getHeight() * 0.85f;
    float labelH = getHeight() * 0.05f;
    float labelW = getWidth() * 0.05f;

    float startX = 300.0f;
    float spacing = 150.0f;

    stringLabelManager.resizeLabels(startX, spacing, labelY, labelW, labelH);
    
    
}


