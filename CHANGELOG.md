# Changelog

All notable changes to this firmware fork are documented here. Earlier history is available in the [upstream repository](https://github.com/RfidResearchGroup/ChameleonUltra).

## 2.49.0 - 2026-09-24

### Added

- 144-slot storage with paginated metadata and transactional read/write commands.
- Opt-in device events, power reporting, NFC diagnostics, and configurable BLE idle sleep.
- Device-local P-256 attestation and certificate provisioning commands.

### Changed

- iOS-compatible BLE service advertising and non-blocking slot feedback.
- FDS durability, slot-health reporting, and NFC session recovery.
- Portable firmware build entry point and a minimal read-only GitHub Actions build.

### Security

- Explicitly clears temporary attestation private-key material and failed unsaved keys from RAM.
