# ------------------------------------------------------------------------------
# CPM Package Configuration
# Define GitHub repositories and version tags used by CPM.cmake
# This file is included by the top-level CMakeLists.txt
# ------------------------------------------------------------------------------

# Logging (spdlog)
set(SPDLOG_REPO               "gabime/spdlog"                  CACHE STRING "SPDLog GitHub repo")
set(SPDLOG_TAG                "v1.13.0"                       CACHE STRING "SPDLog version tag")

# JSON Parsing (nlohmann/json)
set(JSON_REPO                 "nlohmann/json"                 CACHE STRING "JSON GitHub repo")
set(JSON_TAG                  "v3.11.3"                      CACHE STRING "JSON version tag")

# Formatting library (fmt)
set(FMT_REPO                  "fmtlib/fmt"                   CACHE STRING "FMT GitHub repo")
set(FMT_TAG                   "10.1.1"                       CACHE STRING "FMT version tag")

# Project-Specific Dependencies
set(MIDAS_RECEIVER_REPO       "jaca230/midas_receiver"       CACHE STRING "MIDAS receiver repo")
set(MIDAS_RECEIVER_TAG        "main"                         CACHE STRING "MIDAS receiver tag")

set(ANALYSIS_PIPELINE_REPO    "jaca230/analysis_pipeline"    CACHE STRING "Analysis pipeline repo")
set(ANALYSIS_PIPELINE_TAG     "midas"                        CACHE STRING "Analysis pipeline tag")

set(ANALYSIS_PIPELINE_STAGES_REPO "jaca230/analysis_pipeline_stages" CACHE STRING "Analysis pipeline stages repo")
set(ANALYSIS_PIPELINE_STAGES_TAG  "midas"                       CACHE STRING "Analysis pipeline stages tag")
