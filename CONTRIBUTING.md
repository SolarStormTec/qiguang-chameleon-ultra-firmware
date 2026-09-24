# Contributing

Thank you for improving this firmware. Keep pull requests small, focused, and reviewable.

## Before opening a pull request

1. Explain the device behavior or failure being changed.
2. Keep protocol changes backward compatible, or clearly document the compatibility break.
3. Build the Ultra firmware from a clean checkout.
4. Test hardware-facing changes on a real device and record the model, previous version, test path, and result.
5. Update public documentation when commands, payloads, storage formats, or user-visible behavior change.

Do not include device certificates, private keys, service credentials, production data, or generated build outputs. Use clear commit messages and avoid unrelated formatting changes.

Security reports must follow [SECURITY.md](SECURITY.md), not the public issue tracker.
