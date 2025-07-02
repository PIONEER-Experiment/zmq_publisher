# cmake/CPMConfig.cmake

# ---------------------- CPM Package Registry ----------------------
# NOTE: Order matters! Dependencies should be declared in dependency order.
# NOTE: The name in package list matters! Dependencies use "Find{name}.cmake",
# so the name has to match the normal package export name for that package



set(CPM_PACKAGE_LIST
  cppzmq
  spdlog
  TBB
  nlohmann_json
  midas_receiver
  unpacker
  AnalysisPipelineStagesCore
  default_midas_unpacking_pipeline_plugin
  custom_midas_unpacking_pipeline_plugin
  hdsoc_analysis_stages
  AnalysisPipeline
)

# ---------------------- cppzmq (header-only) ----------------------
set(cppzmq_REPO   "zeromq/cppzmq")
set(cppzmq_TAG    "v4.8.1")
set(cppzmq_TARGET "")  # No target needed; header-only
set(cppzmq_DOWNLOAD_ONLY YES)
set(cppzmq_OPTIONS
  "BUILD_EXAMPLES OFF"
  "BUILD_TESTS OFF"
)

# ---------------------- spdlog ----------------------
set(spdlog_REPO   "gabime/spdlog")
set(spdlog_TAG    "v1.13.0")
set(spdlog_TARGET "")
set(spdlog_DOWNLOAD_ONLY YES)
set(spdlog_OPTIONS
  "SPDLOG_BUILD_TESTS OFF"
  "SPDLOG_BUILD_EXAMPLE OFF"
)

# ---------------------- oneTBB ----------------------
set(TBB_REPO   "oneapi-src/oneTBB")
set(TBB_TAG    "v2021.12.0")  # Or latest stable
set(TBB_TARGET "") # Public Dependecy of Analysis Pipeline, no need to link
set(TBB_OPTIONS
  "TBB_TEST OFF;TBB_STRICT OFF;TBB_EXAMPLES OFF;CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- nlohmann_json ----------------------
set(nlohmann_json_REPO   "nlohmann/json")
set(nlohmann_json_TAG    "v3.11.3")
set(nlohmann_json_TARGET "nlohmann_json::nlohmann_json")
set(nlohmann_json_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
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
set(unpacker_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- analysis_pipeline_stages ----------------------
set(AnalysisPipelineStagesCore_REPO   "jaca230/analysis_pipeline_stages_core")
set(AnalysisPipelineStagesCore_TAG    "main")
set(AnalysisPipelineStagesCore_TARGET "") # Public Dependecy of Analysis Pipeline, no need to link
set(AnalysisPipelineStagesCore_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- default_midas_unpacking_pipeline_plugin ----------------------
set(default_midas_unpacking_pipeline_plugin_REPO   "jaca230/default_midas_unpacking_pipeline_plugin")
set(default_midas_unpacking_pipeline_plugin_TAG    "main")
set(default_midas_unpacking_pipeline_plugin_TARGET "") # Dynamically linked
set(default_midas_unpacking_pipeline_plugin_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- custom_midas_unpacking_pipeline_plugin ----------------------
set(custom_midas_unpacking_pipeline_plugin_REPO   "jaca230/custom_midas_unpacking_pipeline_plugin")
set(custom_midas_unpacking_pipeline_plugin_TAG    "main")
set(custom_midas_unpacking_pipeline_plugin_TARGET "") # Dynamically linked
set(custom_midas_unpacking_pipeline_plugin_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- hdsoc_analysis_stages ----------------------
set(hdsoc_analysis_stages_REPO   "jaca230/hdsoc_analysis_stages")
set(hdsoc_analysis_stages_TAG    "main")
set(hdsoc_analysis_stages_TARGET "") # Dynamically linked
set(hdsoc_analysis_stages_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)


# ---------------------- analysis_pipeline ----------------------
set(AnalysisPipeline_REPO   "jaca230/analysis_pipeline")
set(AnalysisPipeline_TAG    "main")
set(AnalysisPipeline_TARGET "AnalysisPipeline::analysis_pipeline")
set(AnalysisPipeline_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "BUILD_EXAMPLE_PLUGIN OFF"
)