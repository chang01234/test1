#!/bin/bash

working_dir="$(pwd)"

echo "Working directory: ${working_dir}"
echo "::notice::Current working directory: ${working_dir}"

echo "Cleaning up previous build artifacts..."
rm -rf build/ bin/ sdkconfig*
# Add the current working directory and its submodules to the safe directory list
git config --global --add safe.directory "${working_dir}"
git submodule foreach --recursive 'git config --global --add safe.directory "`pwd`"'

if [ -n "${SECURE_PLATFORM}" ]; then
    # Validate the secure boot signing key
    secure_boot_signing_key_path="${working_dir}/secure_boot_signing_key.pem"
    if [ ! -f "${secure_boot_signing_key_path}" ]; then
        echo "::error::Secure boot signing key not found at path: ${secure_boot_signing_key_path}"
        exit 1
    fi

    # Validate the secure boot signing key
    openssl rsa -check -in "${secure_boot_signing_key_path}" -noout
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "::error::Invalid secure boot signing key at path: ${secure_boot_signing_key_path}"
        exit $ret
    fi
fi

# Build the project
idf.py all ${SECURE_PLATFORM} ${DICM_FWID} size dicm-version

# Check if the bootloader and application binaries are signed
if [ -n "${SECURE_PLATFORM}" ]; then
    fw_version=$(grep -oP 'version: \K.*' bin/version.yml)
    ota_image_bin="bin/${fw_version}-ota.bin"
    bootloader_bin="build/bootloader/bootloader.bin"

    # Verify the signature of the OTA image
    espsecure.py signature_info_v2 "${ota_image_bin}"
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "::error::Failed to verify signature of OTA image: ${ota_image_bin}"
        exit $ret
    fi
    echo "::notice::Verified signature of OTA image: ${ota_image_bin}"

    # Verify the signature of the bootloader binary
    espsecure.py signature_info_v2 ${bootloader_bin}
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "::error::Failed to verify signature of bootloader binary: ${bootloader_bin}"
        exit $ret
    fi
    echo "::notice::Verified signature of bootloader binary: ${bootloader_bin}"
    python -m pip install --ignore-installed esp-idf-sbom==0.21.0
    esp-idf-sbom create -o bin/${fw_version}.spdx build/project_description.json
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "::error::Not able to generate SBOM file bin/${fw_version}.spdx from build/project_description.json"
        exit $ret
    fi
    esp-idf-sbom check bin/${fw_version}.spdx -o bin/${fw_version}_sbom_report.txt
    esp-idf-sbom check bin/${fw_version}.spdx --format json -o bin/${fw_version}_sbom_report.json
    echo "::notice::Generated SBOM reports"
fi
