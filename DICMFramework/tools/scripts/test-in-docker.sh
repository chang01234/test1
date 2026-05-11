#!/bin/bash

# Uncomment the following lines if you are using Podman instead of Docker
# shopt -s expand_aliases
# alias docker=podman

esp_idf_version="v5.3.4"
esp_idf_work_dir="$(pwd)"
esp_idf_docker_work_dir=$(basename "$esp_idf_work_dir")

echo "Testing in Docker: Using Espressif IDF ${esp_idf_version}"

docker run --rm \
    -v "${esp_idf_work_dir}":"/${esp_idf_docker_work_dir}" \
    -w "/${esp_idf_docker_work_dir}" \
    docker.io/espressif/idf:${esp_idf_version} \
    ./DICMFramework/tools/scripts/run-tests.sh

status=$?
if [ $status -ne 0 ]; then
    echo "::error::Test run failed"
else
    echo "::notice::Test run succeeded"
fi

exit $status
