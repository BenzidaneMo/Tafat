# Tafat roadmap: Veyon fork → NetSupport School replacement for Algerian high schools

## Context
Tafat (repo `benzidanemo/opennetsupport`, GPL-2.0-or-later) should replace NetSupport School
(NSS) in Algerian high school computer labs. Building on
**Veyon** (GPL-2.0-or-later, C++/Qt, ~83k LOC, actively maintained, latest stable tag `v4.11.3`)
gives us monitoring, remote control, demo, lock, messages, launch apps/URLs, file distribute
**and collect**, screenshots, power, logon/logoff on Windows + Linux. We add: full rebrand,
complete Arabic, Tamazight (Latin + Tifinagh), and the NSS features Veyon lacks.

Decisions:
- Tamazight: **two** translations — Latin (Kabyle, `kab`) and Tifinagh.
- Lab OS: **Windows-first** (Linux kept building/working).
- **32-bit is required, including Windows 7/8.1** (old lab PCs). See Phase 0b.
- Name: **Tafat** (Kabyle "light"; تافات / ⵜⴰⴼⴰⵜ). Replaces the "OpenNetsupport" placeholder
  (which contained the NetSupport Ltd trademark). Still held in ONE branding file; do an
  INAPI + WIPO Global Brand Database + domain check before the first public release.
- Feature priority: **1) App & web blocking, 2) Quizzes & surveys, 3) Collect work + register**,
  then hand-raise/chat and the rest.

## Guiding principle: "thin rebrand", stay mergeable with upstream
- Keep Veyon's git history and an `upstream` remote; merge upstream releases regularly
  (security fixes, Qt updates).
- Rebrand **only what users/admins see** (names, binaries, icons, installer, service, paths,
  registry/config locations, URLs, About). Keep internal identifiers (`VeyonCore`,
  `VEYON_*` macros, class/file names, feature UIDs, `veyon_*.ts` file names) unchanged to
  minimise merge conflicts.
- New features live in **new plugins** under `plugins/`, not in modified core files.
- Keep all Veyon copyright headers, `COPYING` (with Veyon's OpenSSL exception for Veyon code)
  and a "Based on Veyon by Tobias Junghans / Veyon Solutions" credit in the About dialog.

---

## Phase 0 — Fork & baseline build (week 1)
1. In our repo: `git remote add upstream https://github.com/veyon/veyon.git`, fetch tag
   `v4.11.3`, `git merge --allow-unrelated-histories v4.11.3`. Resolve: keep our `README.md`
   (rewrite), keep `LICENSE` (plain GPLv2) **and** Veyon's `COPYING`.
2. `git submodule update --init --recursive` (libvncserver, x11vnc, ultravnc, kldap,
   qthttpserver, libfakekey — all upstream deps, keep as-is).
3. Get a clean Linux build (Qt6, QCA, LDAP, PAM, LZO…) and run `tests/`.
4. CI: adapt `.github/workflows/build.yml` + `release.yml` (they use `veyon/ci.*` docker
   images); add the Windows MinGW cross-build (`cmake/modules/MinGWCrossCompile.cmake`,
   `Win64Toolchain.cmake`, `WindowsInstaller.cmake`) so every push produces a Windows installer.
5. Add `UPSTREAM.md` documenting the merge procedure.

## Phase 0b — 32-bit + Windows 7/8.1 support (weeks 1–3)
Findings: Veyon already has the pieces — `VEYON_BUILD_WIN32` (`CMakeLists.txt:95-104`),
`cmake/modules/Win32Toolchain.cmake` (i686-w64-mingw32), `.ci/windows/build.sh <arch>`,
`option(WITH_QT6 … ON)` with a Qt 5 path (`CMakeLists.txt:148-165`, `3rdparty/kldap-qt5`,
26 files with `QT_VERSION_CHECK` guards, min Qt 5.14 via `SetDefaultTargetProperties.cmake:7`).
Blockers: Qt 6 needs Windows 10+, and Veyon targets Windows 8 APIs
(`_WIN32_WINNT=0x0602` in `core/CMakeLists.txt:74` and `cmake/modules/PchHelpers.cmake:16`).

**Release matrix (Windows):**
| Build | Qt | Arch | Runs on |
|---|---|---|---|
| Modern | Qt 6 | x64 | Windows 10/11 64-bit |
| Legacy | Qt 5.15 (`-DWITH_QT6=OFF`) | x86 | Windows 7/8.1/10 32-bit |
| Legacy | Qt 5.15 | x64 | Windows 7/8.1 64-bit |
(Linux: Qt 6 on current distros as Veyon does; i386 Linux not planned.)
Teacher and student builds interoperate — same protocol across the matrix.

Steps:
1. New CMake option `WITH_LEGACY_WINDOWS` (implies `WITH_QT6=OFF`) that sets
   `_WIN32_WINNT=0x0601` in both places above.
2. Compile the legacy build and fix every Win8+ API it hits: load such functions
   dynamically (`GetProcAddress`) with a Windows 7 fallback, in
   `plugins/platform/windows/*` and `plugins/vncserver/ultravnc-builtin` (UltraVNC already
   falls back from Desktop Duplication to GDI capture on Windows 7).
3. CI: add i686 + x86_64 Qt 5.15 MinGW jobs (Fedora `mingw32/64-qt5-*` cross packages, same as
   the existing MinGW approach) producing `…-win32-legacy-setup.exe` / `…-win64-legacy-setup.exe`;
   NSIS installer picks/warns on wrong OS version.
4. **Rule for all new code**: must build with Qt 5.15 + Qt 6 and avoid Win8+ APIs without a
   fallback; CI enforces by building all three Windows targets on every push.
5. Test VMs: Windows 7 SP1 32-bit, Windows 8.1 64-bit, Windows 10 32-bit, Windows 11.
6. Known Win7 limits to document: browser policies only on Chrome ≤109 / Firefox ESR 115
   (last Win7 versions); no updates from Microsoft, so recommend isolated lab networks.

## Phase 1 — Full rebranding (weeks 2–3)
Footprint: ~4,200 "veyon" occurrences in 593 files, but only a small set is user-visible.

1. **Single source of truth**: new `cmake/Branding.cmake` (PRODUCT_NAME "Tafat",
   PRODUCT_SLUG "tafat", VENDOR "Tafat contributors", DOMAIN/WEBSITE/DOCS_URL placeholders
   until a domain is registered, PRODUCT_VERSION with our own scheme, icon paths) +
   `configure_file` → generated `core/src/BrandingConfig.h`. Renaming later = edit one file.
2. **Runtime identity**: `core/src/VeyonCore.cpp:226-228` (org name/domain/app name) from branding.
3. **Binary names**: set `OUTPUT_NAME` `${PRODUCT_SLUG}-master/-server/-service/-worker/-cli/
   -configurator` in `cmake/modules/BuildVeyonApplication.cmake` (keep CMake target names);
   update hard-coded lookups `core/src/Filesystem.cpp:180,188` and
   `core/src/VeyonServiceControl.cpp:60`.
4. **Install/data paths**: `CMakeLists.txt:122-142` (`lib/veyon`, `share/veyon`, translations
   dir) → slug-based.
5. **System integration**: service name (`VeyonServiceControl::name()`), Windows registry keys
   and firewall rule names (`plugins/platform/windows/`), Linux systemd unit/desktop files
   (`cmake/modules/XdgInstall.cmake`), CPack (`cmake/CPackDefinitions.cmake`), NSIS
   (`nsis/veyon.nsi.in`, `.ico`/`.bmp`), Android (`android/`).
6. **UI strings**: 23 `tr()` strings contain "Veyon" — change them to `tr("… %1 …").arg(productName)`
   and update the matching entries in `translations/*.ts` with a script
   (`tools/rebrand-ts.py`) so existing translations are not lost.
7. **Artwork**: new logo/icon set (`master/data/veyon-master.*`, `configurator/data/*`,
   `nsis/*`, About dialog, tray icons, Android `res/`).
8. **Links**: replace `veyon.io` help/docs URLs with project URLs.
9. **Guard**: `tools/check-branding.sh` (CI) fails if a user-visible "Veyon" string reappears
   after an upstream merge (allow-list for copyright headers/credits).

## Phase 2 — Localisation: Arabic, Tamazight (Latin + Tifinagh), French (weeks 2–6, parallel)
Existing infra: `core/src/TranslationLoader.cpp`, language list built from `veyon*.qm` in
`configurator/src/GeneralConfigurationPage.cpp:53-71`, RTL already applied at
`core/src/VeyonCore.cpp:597` (`setLayoutDirection(QLocale{}.textDirection())`).
Status at v4.11.3: French 998/1161 (upstream master has it complete), Arabic 218/1161
(943 to do), Tamazight none.

1. **Script-aware locales**: Qt's `QLocale::name()` drops the script, so Kabyle Latin and
   Tifinagh would collide. Extend `TranslationLoader::load()` and the language list to use
   script-qualified file names (`veyon_kab.qm` = Latin, `veyon_kab_Tfng.qm` = Tifinagh;
   confirm exact codes with Algerian Tamazight teachers / HCA conventions) and show
   proper native names in the selector.
2. **Fonts**: bundle Noto Sans Tifinagh (and Noto Sans Arabic as fallback), both SIL OFL
   (GPL-compatible to ship), register with `QFontDatabase::addApplicationFont` at startup.
3. **Translations**: create `translations/veyon_kab.ts`, `veyon_kab_Tfng.ts`; finish
   `veyon_ar.ts`; also translate every new plugin's strings. Set up **Hosted Weblate**
   (free for libre projects) for ar / kab / kab-Tfng / fr and recruit teachers as translators.
4. **Qt's own strings** (OK/Cancel, file dialogs): `qtbase_ar` exists; for kab/Tfng add small
   `qtbase_kab*.ts` overrides for common dialog buttons.
5. **RTL audit**: check custom-painted widgets (`master/src/ComputerItemDelegate.cpp`,
   `ComputerMonitoringView`, toolbars, `LockWidget`, `Toast`, new plugin UIs) in Arabic;
   mirror directional icons.
6. Default UI language = Windows system language, with Arabic/French/Tamazight selectable
   per teacher in master settings.

## Phase 3 — NSS feature parity (priority order)
**Pattern for every feature** — a new plugin (`plugins/<name>/`, built with
`cmake/modules/BuildVeyonPlugin.cmake`) implementing `core/src/FeatureProviderInterface.h`,
modelled on `plugins/textmessage/TextMessageFeaturePlugin.cpp`:
master `startFeature()` → `controlFeature()` → `sendFeatureMessage()` → student server
`handleFeatureMessage(VeyonServerInterface&…)` → user-session UI via the worker
(`handleFeatureMessage(VeyonWorkerInterface&…)`). Student → teacher data flows through
`sendAsyncFeatureMessages()` + master-side `handleFeatureMessage(ComputerControlInterface::Pointer…)`
(see `core/src/MonitoringMode.cpp`). OS-specific code goes behind the `Platform*Functions`
interfaces in `plugins/platform/{windows,linux}`.

### 3.1 App & web control (first)
- `plugins/appcontrol`: allow-list / block-list modes per class; server (SYSTEM service)
  watches processes (Windows: Toolhelp32/WMI process-start events; Linux: `/proc`) and kills
  blocked ones; reports foreground app + history to master; master panel/overlay showing
  current app per student (reuse `master/src/NetworkObjectOverlayDataModel`).
- `plugins/webcontrol`: enforce via browser enterprise policies (Edge/Chrome/Firefox
  `URLBlocklist`/`URLAllowlist` registry policies — works with HTTPS), "block all internet"
  via Windows firewall (reuse firewall code in `WindowsNetworkFunctions` / `netfw.h`);
  block unmanaged browsers via appcontrol; URL/title history to master.
- Later in this track: print control and USB-storage blocking (Windows device policies).

### 3.2 Quizzes & surveys
- `plugins/quiz`: master-side test designer (single/multiple choice, true/false, short
  answer; images; per-question points; timer), library stored as JSON in the teacher's data
  dir; worker shows a full-screen, RTL-aware test window; answers return via async messages;
  auto-grading, live progress grid, results per student/question, CSV/PDF export.
- `plugins/survey`: instant poll (yes/no, custom options), live bar chart in master.

### 3.3 Collect work + student register
- Collect/distribute already exist (`plugins/filetransfer/FileCollectController.*`,
  `FileTransferController.*`) — extend with per-lesson folders named by student, hand-in
  button on the student side, and "return marked work".
- `plugins/register`: Algerian labs often use one shared student account, so at lesson start
  the worker asks name/class; master shows the name on each tile (instead of the OS user from
  `ComputerControlInterface::userFullName()`), saves attendance CSV; class lists importable
  from CSV.

### 3.4 Then (rough order)
Hand-raise/help requests + two-way chat → whiteboard/annotation during demo → screen
recording & replay → audio mute/broadcast → lesson plans & reward points → hardware/software
inventory → teacher mobile app (Android build already in `android/`).

## Phase 4 — Packaging & deployment for schools
- Rebranded Windows installer (NSIS) with silent install flags for mass deployment, plus a
  "lab setup wizard" (auth-key generation/distribution, room import from CSV via existing
  `builtindirectory` CLI).
- Admin + teacher guides in Arabic, French, Tamazight; quick-start video.
- Pilot in 1–2 schools before the wider rollout; collect feedback per release.

## Verification
- Every phase: CI green on Linux build + all three Windows builds (Qt 6 x64, Qt 5.15 x86,
  Qt 5.15 x64); `tests/` pass; `ctest`.
- 32-bit/legacy: install the legacy x86 build on a Windows 7 SP1 32-bit VM, control it from a
  Windows 11 teacher (and vice versa); check screen capture, lock, file transfer, service start.
- Rebrand: `tools/check-branding.sh` passes; installed files/service/registry contain no
  "veyon" user-facing names; About dialog shows upstream credit.
- Localisation: launch master/configurator under `xvfb`/offscreen with `ar`, `kab`,
  `kab_Tfng`, `fr` and screenshot — check RTL mirroring and Tifinagh glyphs render (no tofu).
- Features: end-to-end on a test network (1 teacher + 2 student Windows VMs, 1 Linux VM):
  block an app/site and confirm it's killed/blocked and logged; run a quiz and verify grades;
  register names appear on tiles; collect files land in per-student folders.
- Upstream: do a trial merge of the next Veyon release to confirm the thin-rebrand keeps
  conflicts small.

## Progress
- Phase 0 done (pushed to `ccr-b7df7899-2kzbe8`): Veyon v4.11.3 merged with history,
  `README.md`, `UPSTREAM.md`, `docs/ROADMAP.md`; Linux build + 3/3 tests pass (under Xvfb).
- Open: Windows MinGW CI image. Veyon's image (`veyon/ci-mingw-w64`) is private, so we
  build our own from Fedora's `mingw32-*`/`mingw64-*` packages in GitHub Actions.
- Phase 1 in progress: name set to **Tafat**.

