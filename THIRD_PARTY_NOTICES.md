# Third-party notices

StormEngine's original code and documentation are licensed under the Apache License 2.0 (SPDX: `Apache-2.0`); see [`LICENSE`](LICENSE). The following notice applies to the optional SDL3 desktop prototype when `STORM_BUILD_SDL3_DEMO=ON`. SDL remains separately licensed under zlib; this notice does not modify or replace either license.

## SDL — Simple DirectMedia Layer

- **Version:** 3.4.18
- **Upstream project:** https://github.com/libsdl-org/SDL
- **Release tag:** `release-3.4.18`
- **Tag commit:** `829a65d769d935c4852f8159e964312c0957260a`
- **Source archive:** https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
- **SHA-256:** `9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3`
- **License:** zlib (per upstream `LICENSE.txt`)
- **Use in StormEngine:** optional static dependency for the first desktop window/input/2D bootstrap demo; not used by the core build unless the option is enabled.

### Upstream license text

Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

### Maintenance

Do not update the version, release URL or checksum independently. Review the upstream release page and license, update this notice and the architecture decision together, and validate the CMake and desktop CI builds. SDL's permissive license covers SDL only; it does not resolve StormEngine's project-license decision.
