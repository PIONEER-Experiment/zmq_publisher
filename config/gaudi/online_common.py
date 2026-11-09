import os
from dataclasses import dataclass
from typing import Any

from Configurables import ApplicationMgr, AuditorSvc, EvtDataSvc, EvtPersistencySvc
from pi_midas.PIONEER_MIDAS_READERConf import PIMidasConversionSvc, PIMidasDecoder, PITMidasNull
from online_dqm.PIONEER_TB_ONLINE_DQMConf import (
    PIAOnlineOutput,
    PIMidasOnlineSelector,
    PIMidasOnlineSvc,
    PIMusipDemoGeometrySvc,
    PIOnlineHistogramSvc,
    PIOnlineTimingAuditor,
)
from shared.PiGaudiSharedSvcConf import PIDataModelSvc, PIHeaderSvc


@dataclass(frozen=True)
class PublicationStage:
    """A compact-product algorithm and the TES path it publishes."""

    algorithm: Any
    output: str

    def __post_init__(self):
        if not self.output.startswith("/Event/"):
            raise ValueError(
                "Online publication outputs must be absolute /Event/ TES paths"
            )


def _environment_flag(name, default):
    return os.environ.get(name, "1" if default else "0").lower() in {
        "1", "true", "yes", "on"
    }


def configure_online_pipeline(
    decoding_tools, analysis_stages=None, publication_stages=None
):
    """Assemble the ordered MIDAS-to-compact-product Gaudi pipeline.

    Event flow is:
      socket selector -> MIDAS decoder -> analysis -> publication -> output

    ``analysis_stages`` run for every event. ``publication_stages`` update
    cached compact products and expose them through their configured TES paths.
    The output stream serializes those paths only when publication is requested.
    """
    if analysis_stages is None:
        analysis_stages = []
    if publication_stages is None:
        publication_stages = []

    # Online input: receive one framed MIDAS event per local socket request.
    online = PIMidasOnlineSvc("PIMidasOnlineSvc")
    online.SocketPath = os.environ.get("PIONEER_GAUDI_SOCKET", "/tmp/pioneer-gaudi.sock")
    online.MaximumEventBytes = int(
        os.environ.get("PIONEER_GAUDI_MAX_EVENT_BYTES", 16 * 1024 * 1024)
    )

    selector = PIMidasOnlineSelector("EventSelector")
    selector.OnlineService = online
    selector.OdbJson = os.environ.get("PIONEER_GAUDI_ODB_JSON", "{}")

    # TES input bridge: let Gaudi turn the selected MIDAS event into an event
    # address, then dispatch each bank to the first matching decoding tool.
    conversion = PIMidasConversionSvc("ConversionSvc")
    persistency = EvtPersistencySvc()
    persistency.CnvServices = ["PIMidasConversionSvc/ConversionSvc"]

    decoder = PIMidasDecoder("DecodeMidas")
    decoder.fallback = PITMidasNull("Null")
    decoder.decoders = decoding_tools

    # Publication algorithms share transport metadata. Their declared outputs
    # also become the complete input list for OnlineOutput.
    epoch = os.environ.get("PIONEER_GAUDI_DQM_EPOCH", "online")
    publication_algorithms = []
    publication_inputs = []
    for stage in publication_stages:
        stage.algorithm.Output = stage.output
        stage.algorithm.OnlineService = online
        stage.algorithm.Epoch = epoch
        publication_algorithms.append(stage.algorithm)
        publication_inputs.append(stage.output)

    # Final publication boundary: serialize configured compact products and
    # the optional histogram snapshot, then reply over the online service.
    output = PIAOnlineOutput("OnlineOutput")
    output.OnlineService = online
    output.Inputs = publication_inputs
    output.IncludeHistograms = _environment_flag(
        "PIONEER_GAUDI_INCLUDE_HISTOGRAMS", True
    )
    output.Epoch = epoch

    # Supporting services used by reconstruction, monitoring, and timing.
    histograms = PIOnlineHistogramSvc("PIHistogramSvc")

    auditor_service = AuditorSvc()
    auditor_service.Auditors = [PIOnlineTimingAuditor("OnlineTiming")]

    headers = PIHeaderSvc()
    services = [headers]
    if _environment_flag("PIONEER_GAUDI_MUSIP_DEMO_GEOMETRY", False):
        services.append(PIMusipDemoGeometrySvc("PIMusipDemoGeometrySvc"))

    # Gaudi executes TopAlg in this order for each selected event. OutStream
    # runs afterward and completes the publisher's synchronous request.
    ApplicationMgr(
        TopAlg=[decoder, *analysis_stages, *publication_algorithms],
        OutStream=[output],
        ExtSvc=[
            *services, PIDataModelSvc(), histograms, EvtDataSvc(), online,
            selector, conversion, persistency, auditor_service,
        ],
        EvtSel=selector,
        EvtMax=-1,
        AuditAlgorithms=True,
    )
