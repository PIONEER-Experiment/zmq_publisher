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
    PIAOnlineAtarMonitor,
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
    PIAOnlineAtarMonitor(
        "AtarMonitor",
        Channels=256,
        StripsPerLayer=32,
        Layers=8,
        LongWindowSeconds=120.0,
        RecentWindowSeconds=30.0,
        RollingEvents=1000,
    ),
]

# Each publication product is an independent Gaudi stage. Add or remove a
# builder here without changing the unpacking, analysis, or output machinery.
product_stages = [
    PIAOnlineSampicWaveformBuilder("SampicWaveformProduct"),
    PIAOnlineHDSocWaveformBuilder("HDSocWaveformProduct"),
    PIAOnlineWFD5WaveformBuilder("WFD5WaveformProduct"),
    PIAOnlineMusipHitSeriesBuilder("MusipHitSeriesProduct"),
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
