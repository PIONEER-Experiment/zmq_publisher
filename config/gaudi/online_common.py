import os

from Configurables import ApplicationMgr, AuditorSvc, EvtDataSvc, EvtPersistencySvc
from pi_midas.PIONEER_MIDAS_READERConf import PIMidasConversionSvc, PIMidasDecoder, PITMidasNull
from online_dqm.PIONEER_TB_ONLINE_DQMConf import (
    PIAOnlineOutput,
    PIMidasOnlineSelector,
    PIMidasOnlineSvc,
    PIMusipSimulationGeometrySvc,
    PIOnlineHistogramSvc,
    PIOnlineTimingAuditor,
)
from shared.PiGaudiSharedSvcConf import PIDataModelSvc, PIHeaderSvc


def _environment_flag(name, default):
    return os.environ.get(name, "1" if default else "0").lower() in {
        "1", "true", "yes", "on"
    }


def configure_online_pipeline(decoding_tools, analysis_stages=None, product_stages=None):
    """Configure the bank-dispatched online pipeline selected by the publisher."""
    if analysis_stages is None:
        analysis_stages = []
    if product_stages is None:
        product_stages = []

    online = PIMidasOnlineSvc("PIMidasOnlineSvc")
    online.SocketPath = os.environ.get("PIONEER_GAUDI_SOCKET", "/tmp/pioneer-gaudi.sock")
    online.MaximumEventBytes = int(
        os.environ.get("PIONEER_GAUDI_MAX_EVENT_BYTES", 16 * 1024 * 1024)
    )

    selector = PIMidasOnlineSelector("EventSelector")
    selector.OnlineService = online
    selector.OdbJson = os.environ.get("PIONEER_GAUDI_ODB_JSON", "{}")

    conversion = PIMidasConversionSvc("ConversionSvc")
    persistency = EvtPersistencySvc()
    persistency.CnvServices = ["PIMidasConversionSvc/ConversionSvc"]

    decoder = PIMidasDecoder("DecodeMidas")
    decoder.fallback = PITMidasNull("Null")
    decoder.decoders = decoding_tools

    epoch = os.environ.get("PIONEER_GAUDI_DQM_EPOCH", "online")

    maximum_products = int(os.environ.get("PIONEER_GAUDI_MAX_WAVEFORMS", "64"))
    maximum_samples = int(
        os.environ.get("PIONEER_GAUDI_MAX_SAMPLES_PER_WAVEFORM", "4096")
    )
    for product_stage in product_stages:
        product_stage.OnlineService = online
        product_stage.MaximumWaveforms = maximum_products
        product_stage.MaximumSamplesPerWaveform = maximum_samples
        product_stage.Epoch = epoch

    output = PIAOnlineOutput("OnlineOutput")
    output.OnlineService = online
    output.IncludeHistograms = _environment_flag(
        "PIONEER_GAUDI_INCLUDE_HISTOGRAMS", True
    )
    output.Epoch = epoch

    histograms = PIOnlineHistogramSvc("PIHistogramSvc")

    auditor_service = AuditorSvc()
    auditor_service.Auditors = [PIOnlineTimingAuditor("OnlineTiming")]

    headers = PIHeaderSvc()
    services = [headers]
    if _environment_flag("PIONEER_GAUDI_MUSIP_SIMULATION_GEOMETRY", False):
        services.append(PIMusipSimulationGeometrySvc("PIMusipSimulationGeometrySvc"))

    ApplicationMgr(
        TopAlg=[decoder, *analysis_stages, *product_stages],
        OutStream=[output],
        ExtSvc=[
            *services, PIDataModelSvc(), histograms, EvtDataSvc(), online,
            selector, conversion, persistency, auditor_service,
        ],
        EvtSel=selector,
        EvtMax=-1,
        AuditAlgorithms=True,
    )
