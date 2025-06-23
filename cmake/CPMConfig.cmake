# cmake/CPMConfig.cmake

# ---------------------- CPM Package Registry ----------------------
# NOTE: Order matters! Dependencies should be declared in dependency order.

set(CPM_PACKAGE_LIST
  spdlog
  nlohmann_json
  fmt
  midas_receiver
  unpacker
  analysis_pipeline_stages
  analysis_pipeline
)

# ---------------------- spdlog ----------------------
set(spdlog_REPO   "gabime/spdlog")
set(spdlog_TAG    "v1.13.0")
set(spdlog_TARGET "spdlog::spdlog")

# ---------------------- nlohmann_json ----------------------
set(nlohmann_json_REPO   "nlohmann/json")
set(nlohmann_json_TAG    "v3.11.3")
set(nlohmann_json_TARGET "nlohmann_json::nlohmann_json")

# ---------------------- fmt ----------------------
set(fmt_REPO   "fmtlib/fmt")
set(fmt_TAG    "10.1.1")
set(fmt_TARGET "fmt::fmt")

# ---------------------- midas_receiver ----------------------
set(midas_receiver_REPO   "jaca230/midas_receiver")
set(midas_receiver_TAG    "main")
set(midas_receiver_TARGET "MidasReceiver::midas_receiver")

# ---------------------- analysis_pipeline_stages ----------------------
set(analysis_pipeline_stages_REPO   "jaca230/analysis_pipeline_stages")
set(analysis_pipeline_stages_TAG    "midas_with_unpackers")
set(analysis_pipeline_stages_TARGET "AnalysisPipelineStages::analysis_pipeline_stages")

# ---------------------- analysis_pipeline ----------------------
set(analysis_pipeline_REPO   "jaca230/analysis_pipeline")
set(analysis_pipeline_TAG    "midas_with_unpackers")
set(analysis_pipeline_TARGET "AnalysisPipeline::analysis_pipeline")

# ---------------------- unpacker ----------------------
set(unpacker_REPO   "jaca230/unpacker")
set(unpacker_TAG    "debug/One-CMakeLists.txt")
# Note: unpacker exports multiple targets, so we might list them separately if needed
set(unpacker_TARGETS
  unpacker::common_data_products
  unpacker::common_unpacking
  unpacker::nalu_data_products
  unpacker::nalu_unpacking
)
