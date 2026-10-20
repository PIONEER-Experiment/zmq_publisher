# Build accommodations owned by the publisher's Gaudi sidecar integration.
#
# The selected shared revision consumes OpenSSL::Crypto and includes conditions
# headers by leaf name, but does not establish either requirement itself. Keep
# those transient requirements here instead of modifying the shared submodule.
find_package(OpenSSL REQUIRED)
include_directories("${CMAKE_SOURCE_DIR}/shared/gaudi/conditions")
