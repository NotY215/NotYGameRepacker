# Reporting Guidelines

Use this guide when reporting bugs, documentation problems, security concerns, or other project issues.

## Before opening a report

Please check:

1. The issue is not already reported.
2. The problem is reproducible with the current project state when possible.
3. The report is about the project rather than unrelated software.
4. The report does not contain private information or secrets.

## Bug reports

Include:

- What you expected to happen.
- What actually happened.
- Steps to reproduce the problem.
- Windows version and architecture.
- Build configuration such as Debug, Release, or RelWithDebInfo.
- CMake/compiler version when relevant.
- Relevant hardware information when it affects the problem.
- Error messages or logs.
- The smallest useful example or test case.

Do not include passwords, private keys, access tokens, personal files, or other sensitive information.

## Documentation reports

For incorrect documentation, include:

- The affected file.
- The section or heading.
- What is incorrect or outdated.
- What the correct behavior or intended wording should be.

## Security reports

Security-sensitive issues should not be posted publicly when doing so would expose an exploitable weakness before a fix is available.

Describe:

- The affected component.
- The security impact.
- Reproduction information sufficient for maintainers to understand the issue.
- Whether the issue affects package integrity, installation safety, data handling, or another boundary.

Do not include real credentials, private keys, personal data, or harmful payloads.

The project does not currently advertise a dedicated private security mailbox. If no private reporting channel is available, contact the maintainer through the repository's available GitHub mechanisms and avoid publishing sensitive details in the initial public report.

## Feature requests

Explain:

- What problem the feature solves.
- Why it belongs in NotY Game Repacker.
- How it fits the existing architecture.
- Whether it can be implemented without adding an unnecessary dependency.

## Good reports

A useful report is specific, reproducible, and technically focused.

Avoid reports that only say that something "doesn't work" without describing what was attempted.
