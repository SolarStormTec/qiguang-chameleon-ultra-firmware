# QiGuang ChameleonUltra Firmware

A maintained Chameleon Ultra firmware fork for reliable mobile use, larger slot libraries, transactional badge storage, device events, power reporting, NFC recovery, and optional device attestation.

Current source release: **v2.49.0**

Upstream baseline: [RfidResearchGroup/ChameleonUltra](https://github.com/RfidResearchGroup/ChameleonUltra) commit `0c1d579`

## Scope

Release artifacts are built and validated for **Chameleon Ultra**. Chameleon Lite is not a supported release target unless a release explicitly says otherwise. The wire protocol remains compatible with upstream commands and adds opt-in extension commands for the QiGuang app.

The main changes are:

- BLE advertising that remains discoverable by iOS background scanning;
- 144 slots with paginated metadata and transactional read/write commands;
- durable FDS storage and explicit slot-health reporting;
- device events, idle sleep, button actions, and power-state reporting;
- NFC session recovery and diagnostics;
- P-256 device attestation with device-local private keys.

See [FORK-NOTICE.md](FORK-NOTICE.md) for provenance and the [upstream project](https://github.com/RfidResearchGroup/ChameleonUltra) for the original project overview.

## Build

Use the upstream Docker build environment:

```bash
docker build -t qiguang-chameleon-builder firmware
docker run --rm -v "$PWD:/workdir" -e CURRENT_DEVICE_TYPE=ultra qiguang-chameleon-builder firmware/build.sh
```

The output is written to `firmware/objects/`. This default path does not create a signed DFU package and does not require a private key. The repository also supports the Arm GNU toolchains documented in the [upstream development guide](https://github.com/RfidResearchGroup/ChameleonUltra/wiki/development).

To create a signed DFU package, set `BUILD_DFU_PACKAGE=1` and pass `DFU_SIGNING_KEY` as a path outside the repository. The key must match the public key embedded in the target bootloader. Never commit that key.

## Installation

Back up device data before changing firmware. For normal upgrades, use the application DFU package rather than a full image. Follow the [upstream firmware guide](https://github.com/RfidResearchGroup/ChameleonUltra/wiki/firmware) for DFU and recovery procedures.

## Security

Do not disclose vulnerabilities in a public issue. Follow [SECURITY.md](SECURITY.md). Device attestation identifies provisioned hardware; it does not encrypt the BLE transport or replace production readout protection.

## Contributing

Focused fixes are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md), include hardware and firmware details in bug reports, and describe the validation performed on a real device.

## License

This fork is distributed under the [GNU General Public License v3.0](LICENSE). Modified binaries must be accompanied by the corresponding source as required by that license.
