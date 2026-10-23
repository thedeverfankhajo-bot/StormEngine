# Licensing and attribution

## StormEngine code and documentation

StormEngine's original code and documentation are distributed under the **Apache License 2.0** (SPDX identifier: Apache-2.0). The repository-root LICENSE contains the complete license text and the project-level copyright notice: Copyright 2026 StormEngine contributors.

Apache-2.0 permits commercial and non-commercial use, modification, redistribution, and sublicensing when its conditions are met. Redistributors must include a copy of the license, retain applicable copyright, patent, trademark and attribution notices, and mark modified files with prominent notices. The license includes a contributor patent grant with a patent-litigation termination provision. It does not grant rights to StormEngine names, trademarks or logos, and the software is supplied without warranty as described in the license.

The project-level notice does not transfer ownership of later contributions. Contributors should preserve applicable file-level copyright/license headers and third-party notices. Unless a file or dependency notice says otherwise, the project's original source and documentation are covered by Apache-2.0.

## Third-party dependencies

Third-party components keep their own licenses. The optional SDL3 desktop bootstrap is separately distributed under SDL's zlib license; its pinned version, source archive, checksum, upstream license text, and intended use are documented in THIRD_PARTY_NOTICES.md and ADR 0001.

The default core build does not fetch SDL. Before adding or updating any dependency, record its exact version and source, verify the upstream license and transitive dependencies, document required notices, review supported platforms and maintenance status, and validate the build on the applicable CI matrix. Do not assume StormEngine's Apache-2.0 license overrides a third-party license.

## Installed SDKs and products

CMake installation copies LICENSE and THIRD_PARTY_NOTICES.md to the CMAKE_INSTALL_DATADIR/licenses/StormEngine directory (normally share/licenses/StormEngine). Keep these files with redistributed SDKs and follow each dependency's separate redistribution conditions when shipping an application or engine binary.

This document is a project maintenance summary; the text of the applicable license and third-party licenses controls.
