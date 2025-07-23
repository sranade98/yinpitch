#include "StringLabelManager.h"

StringLabelManager::StringLabelManager()
{
    for (auto& label : labels)
    {
        label.setFont(juce::FontOptions("Times New Roman", 28.0f, juce::Font::plain));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setColour(juce::Label::backgroundColourId, juce::Colours::black);
        label.setColour(juce::Label::outlineColourId, juce::Colours::darkgrey);
        label.setBorderSize(juce::BorderSize<int>(2));
    }
}

void StringLabelManager::addLabelsTo(juce::Component& parent)
{
    for (auto& label : labels)
        parent.addAndMakeVisible(label);
}

void StringLabelManager::updateTuningLabels(const juce::StringArray& stringNotes)
{
    for (int i = 0; i < labels.size(); ++i)
        labels[i].setText(stringNotes[i], juce::dontSendNotification);
}

void StringLabelManager::updateLabelHighlight(int index, bool inTune)
{
    if (index < 0 || index >= (int)labels.size())
        return;

    auto& label = labels[index];
    label.setColour(juce::Label::outlineColourId, inTune ? juce::Colours::green : juce::Colours::orange);
    label.setColour(juce::Label::backgroundColourId, inTune ? juce::Colours::orangered : juce::Colours::black);
}

void StringLabelManager::resetAllLabels()
{
    for (auto& label : labels)
    {
        label.setColour(juce::Label::outlineColourId, juce::Colours::darkgrey);
        label.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    }
}

void StringLabelManager::resizeLabels(int startX, int spacing, int labelY, int labelW, int labelH)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        int x = startX + i * spacing;
        labels[i].setBounds(x, labelY, labelW, labelH);
    }
}
