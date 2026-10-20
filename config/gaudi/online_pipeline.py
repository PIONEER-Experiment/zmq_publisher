"""Default online job selected and owned by zmq_publisher.

MIDAS events from all frontends may coexist in one buffer. PIMidasDecoder
dispatches each bank to the first tool whose CanDecode() accepts its prefix:
SAMPIC SD/SH/ST/SC/SG/SV, MUSIP Hddd, HDSoC HD/HT, and WFD5 CR.
"""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from online_common import configure_online_pipeline
from online_dqm.PIONEER_TB_ONLINE_DQMConf import (
    PIAOnlineHDSocWaveformBuilder,
    PIAOnlineAtarChannelMetric,
    PIAOnlineAtarEventView,
    PIAOnlineAtarFeatures,
    PIAOnlineAtarHistogram2D,
    PIAOnlineAtarMultiplicity,
    PIAOnlineAtarOccupancy,
    PIAOnlineMusipHitSeriesBuilder,
    PIAOnlinePulseHistograms,
    PIAOnlineSampicWaveformBuilder,
    PIAOnlineWFD5WaveformBuilder,
)
from pi_midas.PIONEER_MIDAS_READERConf import (
    PITMidasHDSoc,
    PITMidasMusip,
    PITMidasSampic,
    PITMidasWFD5,
)


# Add ordinary Gaudi reconstruction and monitoring algorithms here in their
# required execution order. Every event passes through these stages after the
# applicable banks have been unpacked.
analysis_stages = [
    PIAOnlinePulseHistograms(
        "PulseIntegrals",
        Bins=120,
        RangeCalibrationEvents=100,
        RangeSigmas=4.0,
    ),
    PIAOnlineAtarFeatures(
        "AtarFeatures",
        Channels=256,
        StripsPerLayer=32,
        Layers=8,
    ),
]

# Each publication product is an independent Gaudi stage. Add or remove a
# builder here without changing the unpacking, analysis, or output machinery.
product_stages = [
    PIAOnlineSampicWaveformBuilder("SampicWaveformProduct"),
    PIAOnlineHDSocWaveformBuilder("HDSocWaveformProduct"),
    PIAOnlineWFD5WaveformBuilder("WFD5WaveformProduct"),
    PIAOnlineMusipHitSeriesBuilder("MusipHitSeriesProduct"),
    PIAOnlineAtarOccupancy("AtarOccupancy"),
    PIAOnlineAtarMultiplicity(
        "AtarMultiplicity", MaximumHits=96,
    ),
    PIAOnlineAtarChannelMetric(
        "AtarBaseline", Metric="baseline",
        Output="/Event/OnlineDqmAtarBaseline",
        LongWindowSeconds=120.0, RecentWindowSeconds=30.0,
    ),
    PIAOnlineAtarChannelMetric(
        "AtarNoise", Metric="noise",
        Output="/Event/OnlineDqmAtarNoise",
        LongWindowSeconds=120.0, RecentWindowSeconds=30.0,
    ),
    PIAOnlineAtarEventView(
        "AtarWaveforms", View="waveforms",
        Output="/Event/OnlineDqmAtarWaveforms",
    ),
    PIAOnlineAtarEventView(
        "AtarHitPositions", View="hit-positions",
        Output="/Event/OnlineDqmAtarHitPositions",
    ),
    PIAOnlineAtarEventView(
        "AtarChargeDepth", View="charge-depth",
        Output="/Event/OnlineDqmAtarChargeDepth",
    ),
    PIAOnlineAtarHistogram2D(
        "AtarPersistence", View="persistence",
        Output="/Event/OnlineDqmAtarPersistence",
        XBins=64, XMin=0.0, XMax=64.0,
        YBins=80, YMin=0.2, YMax=0.9, RollingEvents=200,
    ),
    PIAOnlineAtarHistogram2D(
        "AtarChargeVsAmplitude", View="charge-vs-amplitude",
        Output="/Event/OnlineDqmAtarChargeVsAmplitude",
        XBins=64, XMin=-0.5, XMax=0.0,
        YBins=64, YMin=0.0, YMax=15.0, RollingEvents=1000,
    ),
    PIAOnlineAtarHistogram2D(
        "AtarAmplitudeByChannel", View="amplitude-by-channel",
        Output="/Event/OnlineDqmAtarAmplitudeByChannel",
        XBins=256, XMin=0.0, XMax=256.0,
        YBins=64, YMin=-0.5, YMax=0.0, RollingEvents=1000,
    ),
]

configure_online_pipeline(
    decoding_tools=[
        PITMidasSampic("Sampic"),
        PITMidasMusip("Musip"),
        PITMidasHDSoc("HDSoc"),
        PITMidasWFD5("WFD5"),
    ],
    analysis_stages=analysis_stages,
    product_stages=product_stages,
)
