#pragma once

#include <QJsonObject>

class GenerationSettings
{
public:
    GenerationSettings();

    void load();
    void save() const;

    QJsonObject toJson() const;

    int maxResponse;

    double temperature;
    int topK;
    double topP;
    double minP;
    double typicalP;
    double tfs;

    double repeatPenalty;
    int repeatPenaltyRange;
    double repeatPenaltySlope;

    int seed;
};

