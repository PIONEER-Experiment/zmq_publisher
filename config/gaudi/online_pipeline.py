"""Default online job selected and owned by zmq_publisher.

MIDAS events from all frontends may coexist in one buffer. PIMidasDecoder
dispatches each bank to the first tool whose CanDecode() accepts its prefix:
SAMPIC SD/SH/ST/SC/SG/SV, MUSIP Hddd, HDSoC HD/HT, and WFD5 CR.
"""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from online_common import PublicationStage, configure_online_pipeline
from online_dqm.PIONEER_TB_ONLINE_DQMConf import (
    PIAOnlineAtarSampicAmplitudeByChannel,
    PIAOnlineAtarSampicBaseline,
    PIAOnlineAtarSampicChargeDepth,
    PIAOnlineAtarSampicChargeVsAmplitude,
    PIAOnlineAtarSampicFeatures,
    PIAOnlineAtarSampicHitPositions,
    PIAOnlineAtarSampicMultiplicity,
    PIAOnlineAtarSampicNoise,
    PIAOnlineAtarSampicOccupancy,
    PIAOnlineAtarSampicPersistence,
    PIAOnlineAtarSampicWaveforms,
    PIAOnlineHDSocWaveforms,
    PIAOnlineMusipHitSeries,
    PIAOnlinePulseHistograms,
    PIAOnlineSampicWaveforms,
    PIAOnlineWFD5Waveforms,
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
        PerChannelHistograms=True,
    ),
    PIAOnlineAtarSampicFeatures(
        "AtarSampicFeatures",
        Channels=256,
        StripsPerLayer=32,
        Layers=8,
        TimeGroupGapNanoseconds=12.0,
    ),
]

# Each keyed entry declares one independently selectable publication algorithm
# and the transient-store path consumed by PIAOnlineOutput.
publication_stages = [
    PublicationStage(
        algorithm=PIAOnlineSampicWaveforms("SampicWaveforms"),
        output="/Event/OnlineDqmSampicWaveforms",
    ),
    PublicationStage(
        algorithm=PIAOnlineHDSocWaveforms("HDSocWaveforms"),
        output="/Event/OnlineDqmHDSocWaveforms",
    ),
    PublicationStage(
        algorithm=PIAOnlineWFD5Waveforms("WFD5Waveforms"),
        output="/Event/OnlineDqmWFD5Waveforms",
    ),
    PublicationStage(
        algorithm=PIAOnlineMusipHitSeries("MusipHitSeries"),
        output="/Event/OnlineDqmMusipHitSeries",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicOccupancy("AtarSampicOccupancy"),
        output="/Event/OnlineDqmAtarSampicOccupancy",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicMultiplicity(
            "AtarSampicMultiplicity",
            MaximumHits=96,
        ),
        output="/Event/OnlineDqmAtarSampicMultiplicity",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicBaseline(
            "AtarSampicBaseline",
            LongWindowSeconds=120.0,
            RecentWindowSeconds=30.0,
        ),
        output="/Event/OnlineDqmAtarSampicBaseline",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicNoise(
            "AtarSampicNoise",
            LongWindowSeconds=120.0,
            RecentWindowSeconds=30.0,
        ),
        output="/Event/OnlineDqmAtarSampicNoise",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicWaveforms("AtarSampicWaveforms"),
        output="/Event/OnlineDqmAtarSampicWaveforms",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicHitPositions("AtarSampicHitPositions"),
        output="/Event/OnlineDqmAtarSampicHitPositions",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicChargeDepth("AtarSampicChargeDepth"),
        output="/Event/OnlineDqmAtarSampicChargeDepth",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicPersistence(
            "AtarSampicPersistence",
            XBins=64,
            XMin=0.0,
            XMax=64.0,
            YBins=80,
            YMin=0.2,
            YMax=0.9,
            RollingEvents=200,
        ),
        output="/Event/OnlineDqmAtarSampicPersistence",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicChargeVsAmplitude(
            "AtarSampicChargeVsAmplitude",
            XBins=64,
            XMin=-0.5,
            XMax=0.0,
            YBins=64,
            YMin=0.0,
            YMax=15.0,
            RollingEvents=1000,
        ),
        output="/Event/OnlineDqmAtarSampicChargeVsAmplitude",
    ),
    PublicationStage(
        algorithm=PIAOnlineAtarSampicAmplitudeByChannel(
            "AtarSampicAmplitudeByChannel",
            XBins=256,
            XMin=0.0,
            XMax=256.0,
            YBins=64,
            YMin=-0.5,
            YMax=0.0,
            RollingEvents=1000,
        ),
        output="/Event/OnlineDqmAtarSampicAmplitudeByChannel",
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
    publication_stages=publication_stages,
)
