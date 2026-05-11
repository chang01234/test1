#!/bin/bash
# This script builds the DICM firmware in a Docker container.
#
# Usage: ./build-in-docker.sh [-fwid <fwid>] [-idf <idf_version>] [-target <idf_target>]
# -fwid: The firmware ID to build
# -idf: The Espressif IDF version to use (default: v5.3.2)
# -target: The Espressif IDF target to build for (default: esp32)

# Uncomment the following lines if you are using Podman instead of Docker
# shopt -s expand_aliases
# alias docker=podman

# Set default values for the fwid, Espressif IDF version and target
dicm_fw_id=""
esp_idf_version="v5.3.2"
esp_idf_target="esp32"
secure_platform=""

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -fwid) dicm_fw_id="$2"; shift ;;
        -idf) esp_idf_version="$2"; shift ;;
        -target) esp_idf_target="$2"; shift ;;
        -secure-dev) secure_platform="secure-dev";;
        -secure-prod) secure_platform="secure-prod";;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

dicm_fw_id_option=""
if [ -n "$dicm_fw_id" ]; then
    dicm_fw_id_option="-DBUILD_PRODUCT_ID=${dicm_fw_id}"
fi

dicm_secure_platform=""
if [ "$secure_platform" == "secure-dev" ]; then
    dicm_secure_platform="-DCONFIG_SECURE_PLATFORM_DEVELOPMENT=1"
elif [ "$secure_platform" == "secure-prod" ]; then
    dicm_secure_platform="-DCONFIG_SECURE_PLATFORM_PRODUCTION=1"
fi

esp_idf_work_dir="$(pwd)"
esp_idf_docker_work_dir=$(basename "$esp_idf_work_dir")

echo "::group::Building in Docker"
echo "::notice::IDF Version: ${esp_idf_version}"
echo "::notice::IDF Target: ${esp_idf_target}"
echo "::notice::Firmware ID: ${dicm_fw_id}"
echo "::notice::Secure Platform: ${dicm_secure_platform}"
echo "::endgroup::"

build_log=dicm_build.log

docker run --rm \
    -v "${esp_idf_work_dir}":"/${esp_idf_docker_work_dir}" \
    -w "/${esp_idf_docker_work_dir}" \
    -e IDF_TARGET=${esp_idf_target} \
    -e SECURE_PLATFORM="${dicm_secure_platform}" \
    -e DICM_FWID="${dicm_fw_id_option}" \
    docker.io/espressif/idf:${esp_idf_version} \
    ./DICMFramework/tools/scripts/run-build.sh 2>&1 | tee "${build_log}"

status=$?
if [ $status -ne 0 ]; then
    echo "::error::Build failed"
else
    echo "::notice::Build succeeded"
fi

exit $status
