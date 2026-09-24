#!/usr/bin/env bash

if [[ $BASH_SOURCE = */* ]]; then
  cd -- "${BASH_SOURCE%/*}/" || exit
fi

set -euo pipefail

softdevice=s140
softdevice_version=7.2.0
softdevice_id=0x0100


release_version=${APP_FW_SEMVER:-}
if [[ -z $release_version ]]; then
  release_version=$(sed -nE 's/^APP_FW_SEMVER := ([0-9]+\.[0-9]+\.[0-9]+)$/\1/p' Makefile.defs)
fi
if [[ -z $release_version ]] && git rev-parse --git-dir >/dev/null 2>&1; then
  release_version=$(git describe --tags --abbrev=0 --match 'v*.*' | sed 's/^v//')
fi
if [[ ! $release_version =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
  echo "Set APP_FW_SEMVER to a semantic version such as 2.49.0." >&2
  exit 1
fi

version_major=${BASH_REMATCH[1]}
version_minor=${BASH_REMATCH[2]}
application_version=${APP_VERSION:-$((10#$version_major * 256 + 10#$version_minor))}
build_id=${GIT_VERSION:-v$release_version}
bootloader_version=1

device_type=${CURRENT_DEVICE_TYPE:-ultra}
case $device_type in
  "ultra") hw_version=0 ;;
  "lite")  hw_version=1 ;;
  *)       echo "Unknown CURRENT_DEVICE_TYPE $CURRENT_DEVICE_TYPE, aborting."; exit 1 ;;
esac

echo "Building firmware v$release_version for $device_type (hw_version=$hw_version, DFU version=$application_version)"

set -x

rm -rf "objects"

(
  cd bootloader
  make -j
)

(
  cd application
  make -j \
    APP_FW_VER_MAJOR="$version_major" \
    APP_FW_VER_MINOR="$version_minor" \
    GIT_VERSION="$build_id"
)

if [[ ${BUILD_DFU_PACKAGE:-0} != 1 ]]; then
  echo "Firmware binaries are available in firmware/objects/."
  exit 0
fi

signing_key=${DFU_SIGNING_KEY:-}
if [[ -z $signing_key || ! -f $signing_key ]]; then
  echo "Set DFU_SIGNING_KEY to a private key that matches the target bootloader." >&2
  exit 1
fi
signing_key=$(cd "$(dirname "$signing_key")" && pwd)/$(basename "$signing_key")

(
  cd objects

  cp ../nrf52_sdk/components/softdevice/${softdevice}/hex/${softdevice}_nrf52_${softdevice_version}_softdevice.hex softdevice.hex
  
  nrfutil nrf5sdk-tools pkg generate \
    --hw-version $hw_version \
    --bootloader  bootloader.hex   --bootloader-version  $bootloader_version  --key-file "$signing_key" \
    --application application.hex  --application-version $application_version\
    --softdevice  softdevice.hex \
    --sd-req ${softdevice_id} --sd-id ${softdevice_id} \
    ${device_type}-dfu-full.zip
	
  nrfutil nrf5sdk-tools pkg generate \
    --hw-version $hw_version --key-file "$signing_key" \
    --application application.hex  --application-version $application_version \
    --sd-req ${softdevice_id} \
    ${device_type}-dfu-app.zip

  nrfutil nrf5sdk-tools settings generate \
    --family NRF52840 \
    --application application.hex --application-version $application_version \
    --softdevice softdevice.hex \
    --bootloader-version $bootloader_version --bl-settings-version 2 \
    settings.hex
  mergehex \
    --merge \
    settings.hex \
    application.hex \
    --output application_merged.hex

  mergehex \
    --merge \
      bootloader.hex \
      application_merged.hex \
      softdevice.hex \
    --output fullimage.hex

  tmp_dir=$(mktemp -d -t cu_binaries_XXXXXXXXXX)
  cp *.hex "$tmp_dir"
  mv $tmp_dir/application_merged.hex $tmp_dir/application.hex
  rm $tmp_dir/settings.hex
  zip -j ${device_type}-binaries.zip $tmp_dir/*.hex
  rm -rf $tmp_dir
)
