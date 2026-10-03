# CLAUDE.md — Tafat

Guidance for Claude Code (and humans) working on this repository. Read this first,
then `docs/ROADMAP.md` (full plan) and `UPSTREAM.md` (merge procedure).

## 1. Project

**Tafat** ("light" in Tamazight; تافات / ⵜⴰⴼⴰⵜ) is a free classroom management
program for Algerian high school computer labs, meant to **replace NetSupport
School**. It is a fork of **Veyon v4.11.3** (C++/Qt) with Veyon's full git
history, licensed **GPL-2.0-or-later** (`LICENSE`, plus Veyon's `COPYING`).

Decisions already taken with the project owner (do not re-open them):

- **Windows-first** (Linux must keep building and working).
- **32-bit is required, including Windows 7/8.1** (old lab PCs) → legacy Qt 5 builds.
- Languages: **Arabic**, **Tamazight in two scripts** — Latin (`kab`) and
  Tifinagh (`kab_Tfng`) — French and English.
- Stay on C++/Qt (no rewrite in Rust).
- Feature priority: 1) app and website blocking, 2) quizzes and surveys,
  3) collect work + student register, then hand-raise/chat and the rest.
- Name "Tafat" is held in one file (`cmake/modules/Branding.cmake`). A trademark
  check (INAPI, WIPO Global Brand Database, domain) is still due before the
  first public release.
- Repo: `BenzidaneMo/Tafat` (formerly `opennetsupport`). Work branch:
  `ccr-b7df7899-2kzbe8`. `main` contains PR #1 only; everything after is on the
  work branch. Merge PRs with a **merge commit**, never squash/rebase (that would
  drop Veyon's history).

## 2. Golden rules ("thin rebrand", stay mergeable with upstream Veyon)

- Rebrand **only what users/admins see**. Internal identifiers stay `veyon`:
  `VeyonCore`, `VEYON_*` macros, CMake target names (`veyon-master` …), class
  and file names, feature UIDs, `translations/veyon_*.ts` file names.
- The product identity lives **only** in `cmake/modules/Branding.cmake`
  (`BRANDING_PRODUCT_NAME` "Tafat", `_SLUG` "tafat", `_ORGANIZATION`, `_DOMAIN`
  "benzidanemo.github.io", `_WEBSITE`, `_CONTACT`, `_APP_ID_PREFIX`
  "io.github.benzidanemo", `_SERVER_APP_ID`, `set_branded_output_name()`).
  It reaches C++ through `core/src/veyonconfig.h.in` (`VEYON_PRODUCT_NAME` …)
  and `VeyonCore::productName()`, `productSlug()`, `executableName("master")`.
- **Never edit upstream UI texts that say "Veyon".** `core/src/BrandingTranslator`
  replaces "Veyon" with the product name at runtime in all translations (it skips
  texts with veyon.io / veyon.readthedocs.io links, which are kept on purpose).
- In new code: never hard-code `"veyon-…"` program names or "Veyon" in
  `QStringLiteral`/`QLatin1String`; use the accessors above.
  `tools/check-branding.sh` (CI job "check-branding") enforces this — run it before pushing.
- **New features go into new plugins** under `plugins/`, not into core files.
- Keep all Veyon copyright headers and the "based on Veyon" credit (About dialog).

## 3. Code conventions

- Follow Veyon's style: tabs, spaces inside parentheses `foo( bar )`, `m_member`
  names, `Q_OBJECT` plugins via `cmake/modules/BuildVeyonPlugin.cmake`. New files
  get the header used in `core/src/BrandTheme.cpp` ("Copyright (c) 2026 Tafat
  contributors … This file is part of Tafat, which is based on Veyon" + GPLv2+ text).
- Must compile with **Qt 5.15 and Qt 6** (guard with `QT_VERSION_CHECK`). Known
  trap: don't pass a `QStringBuilder` (`a + b`) to `QVariant::fromValue`; wrap in `QString(...)`.
- Must not use **Windows 8+ APIs without a fallback** (legacy builds target
  `_WIN32_WINNT=0x0601`); load such functions with `GetProcAddress`.
- Never put raw bidi control characters in source (GCC `-Wbidi-chars` fails the
  build). Use escapes: `⁦` (LRI), `⁨` (FSI), `⁩` (PDI). Isolate
  numbers/names in RTL text so "3 / 4" is not shown reversed in Arabic.
- Colors only via `core/src/BrandTheme.h` tokens: cream `#faf4ea`, cream-2
  `#f4ebdd`, paper `#fffdf9`, brown `#3b271d`, brown-2 `#4b3427`, ink `#2b1f18`,
  muted `#7c6b5f`, line `#eadfd0`, orange `#f26b2d`, orange-soft `#fde6d7`,
  teal `#1f9d86`, teal-dark `#13705f`, teal-soft `#dff2ea`, yellow `#f6bf3f`,
  yellow-soft `#fdf0cc`, hot `#f5a524`, error `#b42318`, error-soft `#fde8e6`.
- CSV exports: UTF-8 **with BOM** (Excel needs it for Arabic/Tifinagh).
- **Feature message limits** (`core/src/VariantStream`, checked on receive; a message
  that breaks one is dropped *silently*): only bool, int, qint64, QString,
  QStringList, QByteArray, QUuid, QRect, QVariantList, QVariantMap (no double, uint,
  QDateTime …); strings ≤ 32768 characters; lists/maps ≤ 1024 entries; nesting ≤ 3
  levels below the argument map. Send JSON and other large data as **QByteArray**
  (≤ 16 MB) and read it with `toByteArray()` (see quiz/classchat `toJsonData`);
  `tests/unit/FeatureMessageLimitsTest` round-trips plugin payloads.
- Commits: small, descriptive English messages; one feature per commit.

## 4. Build and test

```sh
git submodule update --init --recursive   # incl. 3rdparty/qthttpserver/src/3rdparty/http-parser

# Linux (Qt 6) development build, as used so far
cmake -S . -B ../tafat-build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DWITH_TESTS=ON -DWITH_TRANSLATIONS=OFF -DWITH_LTO=OFF
ninja -C ../tafat-build
cd ../tafat-build && xvfb-run -a ctest --output-on-failure    # 13 suites, all pass

# Qt 5 check: add -DWITH_QT6=OFF (Linux CI builds Debian 11 Qt 5 too)
tools/check-branding.sh
```

- `WITH_TRANSLATIONS=ON` rewrites `translations/*.ts` in the source tree; undo
  with `git checkout -- translations/` unless you meant to update them.
- Useful options (root `CMakeLists.txt`): `WITH_QT6`, `WITH_LEGACY_WINDOWS`
  (Windows 7/8.1, requires Qt 5), `WITH_LDAP`, `WITH_WEBAPI`, `WITH_WERROR`
  (default ON; OFF on Windows CI because UltraVNC has warnings),
  `WITH_BUNDLED_LIBVNC`, `WITH_TESTS`.
- **Windows installers** are cross-compiled with Fedora's MinGW packages, as in CI:

  ```sh
  docker run --rm -v "$PWD":/src -w /src fedora:44 bash -c \
    'dnf -y install git && git config --global --add safe.directory /src &&
     .ci/windows/fedora-deps.sh i686 6 && .ci/windows/build-fedora.sh i686 6'
  # args: <i686|x86_64> <6|5>; Qt 5 = legacy Windows 7/8.1 build
  ```

  `fedora-deps.sh` installs MinGW Qt + NSIS and builds LZO, Interception and
  QCA (Qt 6). `build-fedora.sh` uses `cmake/modules/FedoraMinGW{32,64}Toolchain.cmake`
  and produces `*-setup.exe`; DLLs are deployed by `tools/windows-deploy-dlls.sh`
  (objdump-based). The installer is always a 32-bit NSIS program.
- Artwork: edit SVGs in `artwork/`, then run `tools/render-artwork.sh` (writes
  PNG/ICO/XPM/BMP under the upstream file names).

## 5. Architecture cheat-sheet

Components: **master** (teacher app), **service** (Windows service / systemd unit,
starts one **server** per user session), **worker** (helper in the user session
for UI such as dialogs), **cli**, **configurator**. Features are plugins
implementing `core/src/FeatureProviderInterface.h`
(model: `plugins/textmessage/TextMessageFeaturePlugin.cpp`).

Message flow:

1. master `startFeature`/`controlFeature` → `sendFeatureMessage`
2. server `handleFeatureMessage( VeyonServerInterface&, … )` →
   `featureWorkerManager().sendMessageToUnmanagedSessionWorker( … )`
3. worker `handleFeatureMessage( VeyonWorkerInterface&, … )`; replies with
   `worker.sendFeatureMessageReply` → server `handleFeatureMessageFromWorker`
4. server → master: `sendAsyncFeatureMessages` + `server.sendFeatureMessageReply(
   messageContext, … )`, with a per-connection version counter stored as an
   `ioDevice()->property` so each master only gets new data (see `plugins/quiz`)
5. master `handleFeatureMessage( ComputerControlInterface::Pointer, … )`;
   `isFeatureActive` drives the active-state marks on the computer tiles.

OS-specific code goes behind `Platform*Functions` in `plugins/platform/{windows,linux}`.

## 6. What is done

**Docs/licensing:** `LICENSE` (GPLv2), `README.md` (logo, features, credit),
`UPSTREAM.md`, `docs/ROADMAP.md` (incl. Progress section).

**Rebrand (Phase 1, done):**
- Executables `tafat-master`, `tafat-server`, … (`BuildVeyonApplication.cmake`);
  install dirs `lib/tafat`, `share/tafat`; lookups in `core/src/Filesystem.cpp`,
  `core/src/VeyonServiceControl.cpp`.
- Service `tafat` (Linux) / `TafatService` (Windows); firewall rule names
  (`ConfigurationManager`); log file names (`Logger`).
- Desktop/polkit/D-Bus/systemd templates (`XdgInstall.cmake`, takes `NAME`),
  CPack (`cmake/CPackDefinitions.cmake`), NSIS (`nsis/veyon.nsi.in`,
  `WindowsInstaller.cmake` windows-binaries target).
- UI texts via `BrandingTranslator`; About dialog "Tafat – based on Veyon" with
  both copyrights, donate button hidden (`core/src/AboutDialog.cpp`).
- Theme: `core/src/BrandTheme.{h,cpp}` light/dark palettes, tooltip palette,
  stylesheet; applied in `VeyonCore::initUi`.
- Logo (T with light rays over three laptops) and icons in `artwork/`
  (`tafat-logo.svg`, `-dark`, master/configurator icons, feature icons).
- Branding guard `tools/check-branding.sh` in `.github/workflows/build.yml`.

**Languages (Phase 2, started):**
- `core/src/TranslationLoader.cpp` tries the configured catalog name first
  (`veyon_kab_Tfng.qm`) because `QLocale::name()` drops the script.
- `configurator/src/GeneralConfigurationPage.cpp` lists
  "Tamazight - Tamaziɣt (kab_DZ)" and "Tamazight Tifinagh - ⵜⴰⵎⴰⵣⵉⵖⵜ (kab_Tfng)".
  The code is read from the **first** parenthesized part, so labels must not
  contain other parentheses.
- Noto Sans Tifinagh bundled (`core/resources/fonts/`, OFL, `core/resources/tafat.qrc`),
  used as UI font for `kab_Tfng` (`BrandTheme::initFonts`).
- `translations/veyon_kab.ts` and `veyon_kab_Tfng.ts` exist but are **untranslated**.
- **Separate catalogs for Tafat's own plugins:** `translations/tafat_{ar,fr,kab,kab_Tfng}.ts`,
  generated by lupdate from `plugins/{appcontrol,classchat,inventory,labsetup,quiz,register,returnwork,webcontrol}` only
  (`translations/CMakeLists.txt`, those sources are excluded from the `veyon_*.ts`
  catalogs) and loaded after the `veyon` catalog (`VeyonCore::initLocaleAndTranslation`).
  Add new Tafat plugins to `tafat_plugins` there (now also `inventory`, `labsetup`). Arabic and French drafts for all
  263 texts are in, marked *unfinished* (Qt still uses them) for native review.
- `veyon_ar.ts`: 211 visible upstream texts (main window, toolbar, demo, lock, power,
  log in/off, tile states/tooltips, file transfer/collect dialogs, spotlight,
  slideshow, open website/start app, access messages) added as unfinished drafts
  → 430/1161; a broken upstream placeholder ("1% 2%") was fixed. Better to
  also contribute them to Veyon's Transifex so they come back upstream.

**Features (Phase 3) — new plugins, vendor "Tafat", each with an icon:**
- `plugins/appcontrol` — "Block apps": block list or allow-only list;
  `ProcessControl` (name normalization, session processes, protected system
  processes, terminate); server checks every 2 s, worker shows a notice.
  Allow-only needs `Process::hasWindow` → **Windows only**.
  Action **"Running apps"** (`RunningAppsWindow`): lists the open
  applications per computer (`ProcessControl::openApplications`, windowed processes
  on Windows, all session processes on Linux), refreshes every 5 s, closes an app on
  one or all computers (commands `QueryApplications`/`ApplicationList`/`CloseApplication`).
  The server samples the open apps every 10 s into an `AppHistory` (first/last seen,
  max 100, reset when the login user changes) that is sent along (`Argument::History`);
  the window shows "since 10:02" for open and a grey time range for closed apps.
  Mode options (Windows only): *block USB storage* (`UsbStorageBlocker`, policy
  `RemovableStorageDevices\Deny_All`) and *block printing* (`PrintBlocker`: stops the
  `Spooler` service and its running dependents and sets it to disabled; previous
  start type and running state are kept in QSettings "AppControl" and restored on
  stop and at server start).
- `plugins/webcontrol` — "Block websites": `WebPolicy` builds Chrome/Edge/Brave/
  Chromium `URLBlocklist`/`URLAllowlist` and Firefox `WebsiteFilter` policies;
  `PolicyStore` writes them (Windows registry 64-bit view, Linux `/etc` policy
  files), merges with admin entries instead of overwriting, keeps state in
  QSettings (system scope, "WebControl") and clears leftovers on server start.
  Firefox needs a restart to apply.
- `plugins/quiz` — quizzes and polls: JSON library in the teacher's data dir,
  editor (single/multiple choice, text; `*` marks correct options; unmarked =
  survey), launcher, student window with countdown/auto-submit, live results
  window with per-question distribution, CSV export. Solutions are stripped
  before sending; grading happens on the teacher's PC. `m_serverQuizActive` is
  separate from the quiz ID so final answers still arrive after "end quiz".
- `plugins/register` — students enter name/class; attendance window + CSV;
  tiles show the student name via `setUserInformation(login, name)`, re-applied
  on `userChanged`, dropped on logoff (works with one shared lab account).
- `plugins/classchat` — "Hands & chat" (Action with sub-features, *not* a Mode,
  so it survives mode switches; `stopFeature` does nothing on purpose):
  *Show/Hide student toolbar* (always-on-top movable bar: raise hand + chat),
  *Open chat window* (teacher: list of students with hand icons, one
  conversation each, send to one/all, lower hand). Meta feature `HandRaised`
  (Master|Service, icon) is reported active while a hand is up → hand icon on
  the tile (`ComputerItemDelegate` draws icons of active Master features).
  Server keeps a `ChatLog` per session (IDs, last 200, 2000 chars) and pushes
  only new messages per connection (properties `classChatVersion`,
  `classChatMessageId`, `classChatSessionId`; a new session ID resets the
  master's log). Teacher messages pop up the student chat window even without
  the toolbar. Hiding the toolbar stops the worker after 1 s.
  **Hand in work** (student toolbar): up to 10 files, 20 MB each, 50 MB together;
  the worker sends them in 256 KB `HandInChunk`s to the server, which keeps them
  in a `HandInQueue` (≤ 64 MB, 10 min) and sends 4 chunks per connection every
  sync (500 ms, ≈ 2 MB/s) to each master. The master reassembles them
  (`HandInAssembler`) and saves them to `<CollectedFilesDestinationDirectory>/
  Handed-in work <date>/<student>_<computer>/` (`HandIn::safeFileName`,
  `uniqueFilePath`); the conversation shows "Handed in: …" and the window gets an
  "Open handed-in work" button. Enum values are on the wire: only append to
  `FeatureCommand`/`Argument`.
- `plugins/register` also imports a **class list** (CSV, `ClassList.{h,cpp}`:
  `,`/`;`/tab, quotes, BOM, header detection) stored in the teacher's QSettings;
  absent students are listed in red and exported. Name matching
  (`ClassListUtils::matchKey`) ignores case, word order, hyphens, Latin accents,
  Arabic tashkeel/tatweel and أ/إ/آ→ا, ة→ه, ى→ي.
- `plugins/quiz` results draw the answer distribution as bars
  (`AnswerBarDelegate`, teal = correct or poll, orange = wrong, RTL-aware).
- `plugins/webcontrol` option **"Also block the internet for all other programs
  (Windows only)"**: `InternetBlocker` adds two `netsh advfirewall` outbound
  block rules (TCP 80/443, UDP 443) for *public* IPv4 ranges only, so LAN,
  teacher connection and demos keep working; removed on stop and server start.
  An empty allow list already blocks all websites in the browsers.
- `plugins/returnwork` — "Return work" (Master-only Action): pick a folder of
  collected files; each selected computer is matched to its subfolder
  (`ReturnWorkMatcher`: "<student>_<computer>", then computer, then student) and
  gets only those files into `~/<"Returned work">`. It sends the upstream
  file transfer plugin's *Distribute* messages per computer
  (`ReturnWorkTransfer`, `ReturnWorkProtocol` mirrors the upstream enums and
  feature UID — `ReturnWorkTest` static_asserts them), so upstream code stays
  unchanged. Flat files only (no subfolders).
- `plugins/labsetup` — configurator page "Lab setup" (`ConfigurationPagePluginInterface`,
  no features): exports a student setup folder (`StudentSetup`): `<name>_public_key.pem`,
  `tafat-config.json` (current config, `Core/InstallationID` removed) and
  `install-students.bat` (CRLF, ASCII; checks admin rights, picks
  `tafat-*-<win32|win64>[-legacy]-setup.exe` by `ver`/`PROCESSOR_ARCHITECTURE`,
  falls back to legacy/32-bit, runs it with `/S /NoMaster /ApplyConfig=…
  /ImportPublicKey=… /PublicKeyName=…`). The NSIS options `/ImportPublicKey=` and
  `/PublicKeyName=` are new (delete + `authkeys import` via `tafat-wcli`).
- `plugins/inventory` — "Inventory" (Action): the server answers `Query` with
  `SystemInventory::collect()` (OS, kernel, CPU arch, build arch, CPU name from the
  registry / `/proc/cpuinfo`, cores, RAM via `GlobalMemoryStatusEx` / `/proc/meminfo`,
  system disk, IPv4 + MAC of running non-virtual interfaces, Tafat and Qt version;
  string keys are the wire format). `InventoryWindow`: sortable table, Qt 5 builds
  marked "legacy build", CSV export (UTF-8 BOM).
- Master splash screen rebranded (`artwork/tafat-splash.svg` → `master/resources/splash.png`).
- NSIS installer refuses Windows < 10 for Qt 6 builds (points to the legacy
  installer), < 7 for legacy builds, and 64-bit installers on 32-bit Windows
  (`VEYON_INSTALLER_MIN_WINDOWS`, `VEYON_INSTALLER_64BIT` in root `CMakeLists.txt`).
- Teacher guides (drafts, need native review): `docs/guide-enseignant.md` (French),
  `docs/guide-enseignant-ar.md` (Arabic, `<div dir="rtl">`). Button names are taken
  from the current catalogs; update the guides when feature names change.
- `docs/DEPLOYMENT.md`: lab installation guide (installers, keys, silent install
  options, room CSV import, ports, shared accounts).
- `plugins/filetransfer/FileTransferConfiguration.h`: collected files are grouped
  by full user name + computer name by default (one folder per registered student).
- Unit tests in `tests/unit/`: `ProcessControlTest`, `WebPolicyTest` (incl.
  `InternetBlocker` ranges/rules), `QuizTest`, `ClassChatTest`, `ClassListTest`,
  `ReturnWorkTest`, `HandInTest`, `LabSetupTest`, `InventoryTest`, `FeatureMessageLimitsTest`
  (plus 3 upstream tests) → 13 suites.
  `ProcessControlTest` also covers `UsbStorageBlocker`, `PrintBlocker` and `AppHistory`.

**CI:**
- `.github/workflows/build.yml`: Linux builds (Debian 11 Qt 5, Fedora 44 Qt 6) and check-branding.
- `.github/workflows/windows.yml`: `fedora:44`, matrix arch {i686, x86_64} ×
  Qt {6, 5}, artifacts `tafat-windows-<arch>-qt<qt>`. LDAP and WebAPI are **off**
  on Windows for now. A concurrency group cancels older runs of the same branch
  (so push in batches), and `build-fedora.sh` builds with `ninja -k 0` and prints
  all error lines under `==== build errors ====` at the end of the step.
- Reading CI logs: the last ~165 lines of a job log are checkout cleanup; fetch
  ~200–260 tail lines to see the build output.
- Qt 5 fixes for Windows-only code done so far: Windows platform plugin links
  `Qt${QT_MAJOR_VERSION}::GuiPrivate`; UltraVNC CMake falls back from
  `qt6_disable_unicode_defines` to `-UUNICODE -U_UNICODE`;
  `LogoffEventFilter::nativeEventFilter` uses `long*` on Qt 5.

## 7. CI status (checked 2026-10-03, Windows run 31 green incl. new plugins)

| Build | Status |
|---|---|
| Linux (Qt 5 + Qt 6), check-branding | green on every commit |
| Windows Qt 6 i686 / x86_64 (Win 10/11) | green, installers ~27 MB |
| Windows Qt 5 legacy i686 / x86_64 (Win 7/8.1) | **green since run 24 (`959a6331`)**, installers ~15 MB |

The legacy jobs also run `tools/check-windows7-imports.sh` on the packaged files:
it prints `::warning::` lines for EXE/DLL files that import Windows 8+ DLLs or
functions (they would not load on Windows 7) and **fails the job**. It passes since
run 29 (`dd72b231`): Fedora's MinGW OpenSSL 3.2 imported
`api-ms-win-core-path-l1-1-0.dll`, so `fedora-deps.sh` now builds the OpenSSL 3.5
LTS libraries for the legacy builds and replaces Fedora's `libcrypto-3.dll` /
`libssl-3.dll` with them (same ABI). Nothing has been run on a real Windows PC yet.

## 8. Unfinished work (in order)

0. **Next session, started but not pushed:**
   - CI does not build or run the unit tests (`.ci/common/linux-build.sh` has no
     `WITH_TESTS`). Add a separate `unit-tests` job to `build.yml` (containers
     `veyon/ci.linux.fedora.44` and `.debian.11` with `-DWITH_QT6=OFF
     -DWITH_BUNDLED_LIBVNC=ON`; `cmake -DWITH_TESTS=ON`, `ninja`,
     `QT_QPA_PLATFORM=offscreen ctest`). All 13 suites pass offscreen with Qt 6;
     check the Qt 5 build of the tests locally first (a reconfigure with
     `-DWITH_TESTS=ON` found no tests, so do a clean Qt 5 build dir).
   - End-to-end check that worked on Linux: `Xvfb :77`, then run `build/server/tafat-server`
     with `DISPLAY=:77` (as root, key `teacher` created with `tafat-cli authkeys create`).
     A small client (call `VeyonCore::setupApplicationParameters()` *before* creating
     the app, then `VeyonCore(app, Component::Master, …)`, `initAuthentication()`,
     `ComputerControlInterface::start(…, FeatureControlOnly)`, and send raw
     `FeatureMessage`s via `cci->sendFeatureMessage`, read `cci->connection()`
     `featureMessageReceived`) got correct Inventory and Running apps replies. Without
     logind there is no user session, so session-bound features answer empty.
     Turn this into a scripted integration test.

1. **Legacy Windows installers:** they build and pass the Windows 7 import check.
   If the check fails after a dependency update, fix the reported DLL (Qt 5,
   OpenSSL and the MinGW runtime are checked too). Windows 7 notes:
   - `SasEventListener` loads `sas.dll` with `LOAD_LIBRARY_SEARCH_SYSTEM32`
     (needs KB2533623; without it, software SAS is just unavailable).
   - Browsers on Windows 7: only Chrome ≤ 109 / Firefox ESR 115 support the policies.
2. **Real-hardware testing** (Windows 7 SP1 32-bit, 8.1 64-bit, 10 32-bit, 11):
   - install, service start, screen capture, lock, demo, file transfer;
   - each Tafat plugin end to end: block app/site/internet/USB/printing, running apps, quiz, register +
     class list, hands & chat;
   - teacher and students on different builds must interoperate.

   Things only checked by reading the code:
   - `netsh` firewall rules (InternetBlocker);
   - the `RemovableStorageDevices\Deny_All` policy (UsbStorageBlocker; may only
     affect devices plugged in afterwards);
   - stopping/disabling the print spooler (PrintBlocker, Windows SCM API);
   - the NSIS version/architecture checks.
3. Decide whether LDAP/AD and WebAPI are needed on Windows; re-enable if so.
4. **Translations:**
   - Native review of all *unfinished* Arabic/French drafts (`tafat_*.ts`,
     `veyon_ar.ts`).
   - Translate Tamazight Latin + Tifinagh (`veyon_kab*.ts`, `tafat_kab*.ts`; all empty).
   - Remaining Arabic upstream texts (731; mostly LDAP, configurator pages, CLI help).
   - Hosted Weblate; small `qtbase_kab*` overrides.
   - RTL audit of `LockWidget`, `Toast` and other custom-painted widgets.
5. **Next features:**
   - Website (URL) history for the teacher (app history is done; URLs need a
     browser extension or reading the browser history databases).
   - Then: whiteboard/annotation, screen recording, audio, lesson plans/rewards,
     mobile app (the inventory is done).
6. **Packaging:** the student setup export is done (`plugins/labsetup`); still open:
   test `install-students.bat` on real Windows 7/10 (incl. localized `ver` output),
   room import in the lab setup page (the configurator keeps its own copy of the
   config, so call the import in-process, not via the CLI), teacher guide in
   Tamazight, pilot in 1–2
   schools (`docs/DEPLOYMENT.md` covers admins).
7. Known limits (documented in DEPLOYMENT.md):
   - allow-only app mode, internet, USB and print block are Windows only;
   - Firefox needs a restart for website policies.
8. Open a PR `ccr-b7df7899-2kzbe8` → `main` when ready (merge commit).
9. Merge new upstream Veyon releases per `UPSTREAM.md` (current base v4.11.3).
