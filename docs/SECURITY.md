# Security and Trust

## Scope

NotY Game Repacker is designed for packaging files that the user legitimately owns or possesses.

The project does not implement:

- DRM bypass
- license bypass
- activation bypass
- launcher bypass
- ownership bypass
- destructive anti-debugging
- security-software bypassing

## Current security status

The current development stage does **not implement cryptography**.

There is therefore no claim that current packages provide encryption, authenticated encryption, or cryptographic confidentiality.

Do not describe an unfinished security mechanism as implemented.

## Integrity

Package integrity is a planned part of the architecture.

A future implementation should be able to detect problems such as:

- missing package data
- malformed package structure
- invalid metadata
- incomplete chunks
- modified payloads
- invalid installed files

The exact integrity algorithm is not declared final until implemented and tested.

## Future cryptography

If cryptographic protection is added later, the project should prefer a Windows-provided cryptographic implementation rather than inventing a weak custom primitive.

The implementation must document:

- algorithms
- key handling
- authentication behavior
- failure behavior
- version compatibility
- test coverage

## Safe failure

When validation fails, the application should:

1. stop the unsafe operation
2. report a clear error
3. close open resources safely
4. preserve unrelated user data
5. clean temporary state when safe

It must not delete unrelated files or damage the operating system.

## Package trust

Integrity and trust are different.

A package can pass its internal validation while still coming from an untrusted source.

Users should consider:

- where the package came from
- who created it
- expected package size
- published hashes when available
- package metadata
- whether the package is appropriate for the files being installed

Passing an integrity check is not proof that a package source is trustworthy.

## Reporting

Potential security problems should be reported according to [REPORTING_GUIDELINES.md](../REPORTING_GUIDELINES.md).
