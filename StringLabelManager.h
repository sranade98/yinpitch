#pragma once

#include <JuceHeader.h>

class StringLabelManager
{
public:
    StringLabelManager();
    void addLabelsTo(juce::Component& parent);
    void updateTuningLabels(const juce::StringArray& stringNotes);
    void updateLabelHighlight(int index, bool inTune);
    void resetAllLabels();
    void resizeLabels(int startX, int spacing, int labelY, int labelW, int labelH);

private:
    std::array<juce::Label, 6> labels;
};
