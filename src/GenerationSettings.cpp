#include "GenerationSettings.h"

#include <QSettings>

GenerationSettings::GenerationSettings()
    : maxResponse(256),
      temperature(0.75),
      topK(100),
      topP(0.92),
      minP(0.0),
      typicalP(1.0),
      tfs(1.0),
      repeatPenalty(1.0),
      repeatPenaltyRange(320),
      repeatPenaltySlope(1.0),
      seed(-1)
{
}

void GenerationSettings::load()
{
    QSettings settings(
        "SillyChat",
        "SillyChat"
    );

    maxResponse =
        settings.value(
            "generation/maxResponse",
            256
        ).toInt();

    temperature =
        settings.value(
            "generation/temperature",
            0.75
        ).toDouble();

    topK =
        settings.value(
            "generation/topK",
            100
        ).toInt();

    topP =
        settings.value(
            "generation/topP",
            0.92
        ).toDouble();

    minP =
        settings.value(
            "generation/minP",
            0.0
        ).toDouble();

    typicalP =
        settings.value(
            "generation/typicalP",
            1.0
        ).toDouble();

    tfs =
        settings.value(
            "generation/tfs",
            1.0
        ).toDouble();

    repeatPenalty =
        settings.value(
            "generation/repeatPenalty",
            1.0
        ).toDouble();

    repeatPenaltyRange =
        settings.value(
            "generation/repeatPenaltyRange",
            320
        ).toInt();

    repeatPenaltySlope =
        settings.value(
            "generation/repeatPenaltySlope",
            1.0
        ).toDouble();

    seed =
        settings.value(
            "generation/seed",
            -1
        ).toInt();
}

void GenerationSettings::save() const
{
    QSettings settings(
        "SillyChat",
        "SillyChat"
    );

    settings.setValue(
        "generation/maxResponse",
        maxResponse
    );

    settings.setValue(
        "generation/temperature",
        temperature
    );

    settings.setValue(
        "generation/topK",
        topK
    );

    settings.setValue(
        "generation/topP",
        topP
    );

    settings.setValue(
        "generation/minP",
        minP
    );

    settings.setValue(
        "generation/typicalP",
        typicalP
    );

    settings.setValue(
        "generation/tfs",
        tfs
    );

    settings.setValue(
        "generation/repeatPenalty",
        repeatPenalty
    );

    settings.setValue(
        "generation/repeatPenaltyRange",
        repeatPenaltyRange
    );

    settings.setValue(
        "generation/repeatPenaltySlope",
        repeatPenaltySlope
    );

    settings.setValue(
        "generation/seed",
        seed
    );

    settings.sync();
}

QJsonObject GenerationSettings::toJson() const
{
    QJsonObject json;

    json["temperature"] =
        temperature;

    json["top_k"] =
        topK;

    json["top_p"] =
        topP;

    json["min_p"] =
        minP;

    json["typical"] =
        typicalP;

    json["tfs"] =
        tfs;

    json["rep_pen"] =
        repeatPenalty;

    json["rep_pen_range"] =
        repeatPenaltyRange;

    json["rep_pen_slope"] =
        repeatPenaltySlope;

    /*
     * KoboldCpp expects sampler_seed to be
     * omitted when using its normal/random seed.
     */

    if (seed >= 1)
    {
        json["sampler_seed"] =
            seed;
    }

    json["max_length"] =
        maxResponse;

    return json;
}

