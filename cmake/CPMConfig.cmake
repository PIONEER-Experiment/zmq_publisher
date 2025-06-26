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
  default_midas_unpacking_pipeline_plugin
  custom_midas_unpacking_pipeline_plugin
  hdsoc_analysis_stages
  analysis_pipeline
)

# ---------------------- spdlog ----------------------
set(spdlog_REPO   "gabime/spdlog")
set(spdlog_TAG    "v1.13.0")
set(spdlog_TARGET "spdlog::spdlog")
set(spdlog_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "SPDLOG_BUILD_TESTS OFF"
  "SPDLOG_BUILD_EXAMPLE OFF"
)

# ---------------------- nlohmann_json ----------------------
set(nlohmann_json_REPO   "nlohmann/json")
set(nlohmann_json_TAG    "v3.11.3")
set(nlohmann_json_TARGET "nlohmann_json::nlohmann_json")
set(nlohmann_json_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- fmt ----------------------
set(fmt_REPO   "fmtlib/fmt")
set(fmt_TAG    "10.1.1")
set(fmt_TARGET "fmt::fmt")
set(fmt_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "FMT_TEST OFF"
)

# ---------------------- midas_receiver ----------------------
set(midas_receiver_REPO   "jaca230/midas_receiver")
set(midas_receiver_TAG    "main")
set(midas_receiver_TARGET "MidasReceiver::midas_receiver")
set(midas_receiver_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- unpacker ----------------------
set(unpacker_REPO   "jaca230/unpacker")
set(unpacker_TAG    "debug/One-CMakeLists.txt")
# unpacker exports multiple targets
set(unpacker_TARGETS
  unpacker::common_data_products
  unpacker::common_unpacking
  unpacker::nalu_data_products
  unpacker::nalu_unpacking
)
set(unpacker_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- analysis_pipeline_stages ----------------------
set(analysis_pipeline_stages_REPO   "jaca230/analysis_pipeline_stages_core")
set(analysis_pipeline_stages_TAG    "main")
set(analysis_pipeline_stages_TARGET "AnalysisPipelineStagesCore::analysis_pipeline_stages_core")
set(analysis_pipeline_stages_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- default_midas_unpacking_pipeline_plugin ----------------------
set(default_midas_unpacking_pipeline_plugin_REPO   "jaca230/default_midas_unpacking_pipeline_plugin")
set(default_midas_unpacking_pipeline_plugin_TAG    "main")
set(default_midas_unpacking_pipeline_plugin_TARGET "default_midas_unpacking_pipeline_plugin::default_midas_unpacking_pipeline_plugin")
set(default_midas_unpacking_pipeline_plugin_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- custom_midas_unpacking_pipeline_plugin ----------------------
set(custom_midas_unpacking_pipeline_plugin_REPO   "jaca230/custom_midas_unpacking_pipeline_plugin")
set(custom_midas_unpacking_pipeline_plugin_TAG    "main")
set(custom_midas_unpacking_pipeline_plugin_TARGET "custom_midas_unpacking_pipeline_plugin::custom_midas_unpacking_pipeline_plugin")
set(custom_midas_unpacking_pipeline_plugin_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- hdsoc_analysis_stages ----------------------
set(hdsoc_analysis_stages_REPO   "jaca230/hdsoc_analysis_stages")
set(hdsoc_analysis_stages_TAG    "main")
set(hdsoc_analysis_stages_TARGET "hdsoc_analysis_stages::hdsoc_analysis_stages")
set(hdsoc_analysis_stages_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- analysis_pipeline ----------------------
set(analysis_pipeline_REPO   "jaca230/analysis_pipeline")
set(analysis_pipeline_TAG    "main")
set(analysis_pipeline_TARGET "AnalysisPipeline::analysis_pipeline")
set(analysis_pipeline_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "BUILD_EXAMPLE_PLUGIN OFF"
)