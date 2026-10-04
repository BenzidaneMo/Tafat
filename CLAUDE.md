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
- Repo: `BenzidaneMo/Tafat` (formerly `opennetsupport`). Work happens
  directly on `main` (no PRs); the old work branch `ccr-b7df7899-2kzbe8` was merged
  with PR #2 (`121d68fd`). Never squash or rebase pushed history (that would drop
  Veyon's history); merge other branches with a **merge commit**. Cloud sessions
  that may only push their own branch restart it from `main` and open a PR (PR #3:
  CI fixes and translation drafts).

## 2. Golden rules ("thin rebrand", stay mergeable with upstream Veyon)

- Rebrand **only what users/admins see**. Internal identifiers stay `veyon`:
  `VeyonCore`, `VEYON_*` macros, CMake target names (`veyon-master` …), class
  and file names, feature UIDs, `translations/veyon_*.ts` file names.
- The product identity lives **only** in `cmake/modules/Branding.cmake`
  (`BRANDING_PRODUCT_NAME` "Tafat", `_SLUG` "tafat", `_ORGANIZATION`, `_DOMAIN`
  "benzidanemo.github.io", `_WEBSITE`, `_DEVELOPER` "BenzidaneMo" + `_DEVELOPER_URL`
  (About dialog "Developed by …", package maintainer), `_CONTACT`, `_APP_ID_PREFIX`
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
  build). Use escapes: `\u2066` (LRI), `\u2068` (FSI), `\u2069` (PDI). Isolate
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
cd ../tafat-build && QT_QPA_PLATFORM=offscreen ctest -LE integration --output-on-failure  # 14 suites
sudo ctest -L integration --output-on-failure   # server/client test, needs root + Xvfb

# Qt 5 check: add -DWITH_QT6=OFF (Linux CI builds Debian 11 Qt 5 too)
tools/check-branding.sh
```

- Test build traps (both broke the Debian 11 `unit-tests` job):
  - LibVNCServer's CMake options share names with ours. The bundled block in the root
    `CMakeLists.txt` saves and restores `WITH_TESTS` around its `set(WITH_TESTS OFF)`.
  - Test targets don't use `set_default_target_properties`. `tests/*/CMakeLists.txt`
    set `CMAKE_CXX_STANDARD 20`, because GCC 10 defaults to C++14.
  - To catch the second one with a newer GCC, configure with `-DCMAKE_CXX_FLAGS=-std=gnu++14`.
- With `WITH_TESTS=ON` the master's models get a `QAbstractItemModelTester`, which crashes the
  master when the directory reloads with new rooms (F5, "Add computers"). Release builds don't
  have it; to test such flows locally, build without tests or disable the testers temporarily.
- NSIS can be checked locally: `apt-get install nsis`, configure `nsis/veyon.nsi` (fill in
  `MIN_WINDOWS`/`IS_64BIT`), create placeholder files and run `makensis`; 32-bit Wine
  (`wine32:i386`) runs small test installers.

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

Traps found so far:
- A server is "active" for a feature while its worker runs (`QueryActiveFeatures`), and the tile
  then shows the feature's icon. `sendMessageToUnmanagedSessionWorker` *starts* a worker if none
  runs, so never forward a Stop to a worker that isn't running, and stop workers that are done
  (`stopWorker`, see quiz, classchat, rewards).
- Every Mode button first stops all features on all computers. A Mode feature that opens a dialog
  must call `ModeFeatureHelper::returnToMonitoringMode` on Cancel. With "Enforce selected mode"
  the master calls `startFeature` again for each reconnecting computer, so resend the last
  settings without a dialog while the mode is active (appcontrol, webcontrol, quiz).
- To test features with a second "student" in one container, run `tafat-server` in a network
  namespace (`ip netns`, veth 10.77.0.1/2); 127.0.0.1 and own addresses count as the teacher's
  computer and are skipped by blockers and quizzes. The build tree needs a `tafat-worker`
  symlink next to `tafat-server`, and the server needs `USER` set when there is no logind session.
  Without logind the server has no user session, so session features (lock, user names, Running
  apps history) stay off. To test them, run a private system D-Bus with a small fake
  `org.freedesktop.login1` (Session `Class` "user", `User` → `Name`), set
  `VEYON_SESSION_PATH=/org/freedesktop/login1/session/<id>` per server, and start Xvfb with
  `-noreset` (otherwise the root window is reset when its last client disconnects).
- `QT_USE_QSTRINGBUILDER` is on: never `auto x = a + b` with strings. It keeps a builder that
  refers to temporaries, which are already destroyed on the next line. Write `const QString x = …`.
- On the master, `handleFeatureMessage` gets `ComputerControlInterface::weakPointer()`: a
  QSharedPointer with a no-op deleter. Never keep a `QWeakPointer` from it (it expires at once).
  Store `QPointer<ComputerControlInterface>` and send with `->weakPointer()`.

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
  The upstream toolbar, bottom-bar, configurator page and access-rule icons are redrawn in the
  same style (`tools/generate-toolbar-icons.py` → `artwork/feature-*`, `panel-*`, `button-*`, `page-*`, `rule-*`,
  rendered to the upstream PNG names by `tools/render-artwork.sh`).
- About → Contributors shows the root `CONTRIBUTORS` file (BenzidaneMo and a
  credit line to the Veyon contributors) instead of the `git shortlog` list
  (root `CMakeLists.txt`; `-DCONTRIBUTORS=` still overrides it).
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
- Tamazight drafts (all *unfinished*):
  - `tafat_kab.ts`: 263/263; `veyon_kab.ts`: 815/1166, the same upstream texts that have Arabic.
  - Kabyle in Latin script, with the usual software terms (aselkim, asnas, asmel, afaylu,
    akaram, anelmad, aselmad, taxxamt for a room; Sewḥel and Sireg).
  - Qt has one plural form for `kab`, so plural texts use wording that fits any number
    (e.g. "Isteqsiyen: %n").
  - The `*_kab_Tfng.ts` catalogs are transliterated from the Latin drafts into Neo-Tifinagh.
    Placeholders, tags, file names, acronyms, product names and chunks copied from the
    English text (Ctrl+Q, C:\TMP) stay in Latin script, and `&` accelerators are dropped.
  - The scratch scripts were not committed: re-transliterate after editing the Latin drafts
    (map a ⴰ, b ⴱ, c ⵛ, č ⵞ, d ⴷ, ḍ ⴹ, e ⴻ, ɛ ⵄ, f ⴼ, g ⴳ, ǧ ⴵ, ɣ ⵖ, h ⵀ, ḥ ⵃ, i ⵉ,
    j ⵊ, k ⴽ, l ⵍ, m ⵎ, n ⵏ, q ⵇ, r ⵔ, ṛ ⵕ, s ⵙ, ṣ ⵚ, t ⵜ, ṭ ⵟ, u ⵓ, w ⵡ, x ⵅ,
    y ⵢ, z ⵣ, ẓ ⵥ).
- **Separate catalogs for Tafat's own plugins:** `translations/tafat_{ar,fr,kab,kab_Tfng}.ts`,
  generated by lupdate from `plugins/{appcontrol,classchat,inventory,labsetup,quiz,register,returnwork,rewards,webcontrol}` only
  (`translations/CMakeLists.txt`, those sources are excluded from the `veyon_*.ts`
  catalogs) and loaded after the `veyon` catalog (`VeyonCore::initLocaleAndTranslation`).
  Add new Tafat plugins to `tafat_plugins` there (now also `inventory`, `labsetup`, `rewards`). Arabic, French and Tamazight
  drafts for all 361 texts are in, marked *unfinished* (Qt still uses them) for native review.
  lupdate (Qt 5 and 6) refuses to update the `kab`/`kab_Tfng` catalogs ("target language is not recognized"):
  set `language="ja"` (one plural form) for the run, then put `kab`/`kab_Tfng` back.
- `veyon_ar.ts`: 816/1161, of which 598 are unfinished drafts.
  - First the visible texts: main window, toolbar, demo, lock, power, log in/off,
    tile states/tooltips, file transfer/collect, spotlight, slideshow, open
    website/start app, access messages.
  - Then the configurator pages, key management, access control tests, service
    control, login dialogs and the CLI headings.
  - Paths and names inside sentences are isolated with FSI/PDI.
  - Fixed along the way: a broken upstream placeholder ("1% 2%"), and unit suffixes
    without their leading space ("60ثواني"; also in `veyon_fr.ts`).
  - Still open: LDAP, WebAPI and CLI help.
  - Better to also contribute these to Veyon's Transifex, so they come back upstream.

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
  editor (single/multiple choice, text, and "True or false", saved as a single
  choice question so the wire format stays the same; points per question; `*` marks
  correct options; unmarked = survey), launcher, student window with countdown/auto-submit, live results
  window with per-question distribution, CSV export. Solutions are stripped
  before sending; grading happens on the teacher's PC. `m_serverQuizActive` is
  separate from the quiz ID so final answers still arrive after "end quiz".
- `plugins/register` — students enter name/class; attendance window + CSV;
  tiles show the student name via `setUserInformation(login, name)`, re-applied
  on `userChanged`, dropped on logoff (works with one shared lab account).
- `plugins/classchat` — "Hands and chat" (Action with sub-features, *not* a Mode,
  so it survives mode switches; `stopFeature` does nothing on purpose):
  *Show/Hide student toolbar* (always-on-top movable bar: raise hand + chat),
  *Open chat window* (teacher: list of students with hand icons, one
  conversation each, send to one/all, lower hand). The student toolbar has a chevron that
  shrinks it to the hand button (remembered in the student's QSettings "ClassChat"). Meta feature `HandRaised`
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
- `plugins/labsetup` — NetSupport-style setup without commands (install on the teacher PC only):
  - Master-only Actions: **Add computers** (`AddComputersDialog` + `ComputerScanner`: TCP connect
    to `veyonServerPort()` on the /24 of each private IPv4 address, ≤ 1024 hosts, 48 parallel, 600 ms,
    accepts only an `RFB 003.` greeting; a typed range (`ComputerScanner::rangeHosts`: `a.b.c.d/22-32`,
    `a.b.c.d-e`, `a.b.c.d-a.b.c.e`, private only, ≤ 1024) replaces the local networks; reverse DNS for names; **Add to the room** writes a temp CSV
    and runs `tafat-wcli networkobjects import <csv> location <room> format "%name%;%host%"` via
    `runProgramAsAdmin` (arguments quoted on Windows) or directly when elevated; then it calls
    `configuredDirectory()->update()` every 2 s and ticks the room in the master's
    `ComputerSelectPanel` tree (found by object name) so the tiles appear at once),
    **Student installer** (`StudentInstallerDialog`: installers from `<appdir>/setup` + added files,
    writes `Install Tafat - student (<Windows>).exe` per installer) and **Settings** (starts the
    configurator; the configurator now has OK | Apply | Reset, OK applies and closes).
  - `StudentPackage`: installer + compact JSON `{version:1,keyName,publicKey,config}` + 8-byte LE
    length + 24-byte marker `TAFAT-STUDENT-PACKAGE-v1` (config without `Core/InstallationID`).
    Checked under Wine: the NSIS CRC check still passes with appended data.
  - CLI `labsetup setupteacher [key]` (creates the key, `setaccessgroup` for the Users group
    resolved from SID S-1-5-32-545, key file auth), `createpackage <installer> <out> [key]`,
    `extractpackage <exe> <folder>`. Command names cannot contain `-` (slot dispatch).
  - NSIS (`nsis/veyon.nsi.in`): Master section runs `setupteacher` (skip with `/NoTeacherSetup`),
    restarts the service and copies itself to `$INSTDIR\setup`; `.onInit` reads the last 24 bytes of
    `$EXEPATH`, a student package unselects the Master and skips license/directory/components and
    imports via `extractpackage`; finish page "Start Tafat Master now" runs it through Explorer
    (not elevated). LangStrings in English, French and Arabic.
  - Service upgrade: `service unregister` only marks a busy service for deletion, so the
    installer waits until `sc query` fails before copying files (`WaitForServiceRemoval`) and
    retries `service register` until `sc qc` finds the service (`RegisterService`).
    `service register` can report success while the old service is still listed.
  - Still there (configurator page "Lab setup", Advanced): the student setup folder (`StudentSetup`):
    `<name>_public_key.pem`, `tafat-config.json` and `install-students.bat` (CRLF, ASCII; checks
    admin rights, picks `tafat-*-<win32|win64>[-legacy]-setup.exe` by `ver`/`PROCESSOR_ARCHITECTURE`,
    runs it with `/S /NoMaster /ApplyConfig=… /ImportPublicKey=… /PublicKeyName=…`).
- Tafat Master can hide its feature toolbar: a toggle at the right end of the bottom bar
  (`MainWindow::setToolBarHidden`, icons `master/resources/toolbar-{hide,show}[-dark].png`) and
  *Hide the toolbar* in the toolbar's context menu; saved as `UI/ToolBarHidden` in the master's
  user config (written when the master closes normally).
- Master window title is set from `VeyonCore::productName()` ("Tafat Master"); the `.ui` title is
  `notr`, so `BrandingTranslator` never saw it. "Hands & chat" became "Hands and chat" (the `&`
  was shown as a mnemonic, "Hands _chat").
- `plugins/inventory` — "Inventory" (Action): the server answers `Query` with
  `SystemInventory::collect()` (OS, kernel, CPU arch, build arch, CPU name from the
  registry / `/proc/cpuinfo`, cores, RAM via `GlobalMemoryStatusEx` / `/proc/meminfo`,
  system disk, IPv4 + MAC of running non-virtual interfaces, Tafat and Qt version;
  string keys are the wire format). `InventoryWindow`: sortable table, Qt 5 builds
  marked "legacy build", CSV export (UTF-8 BOM).
- `plugins/rewards` — "Rewards" (Action with sub-features *Give a star*, *Remove a star*,
  *Show stars*): the master keeps a `RewardBook` (student → stars, 0–999) per class
  (`RewardClasses`, ≤ 100 classes, natural sort, current class) in the teacher's QSettings
  ("Rewards/Classes"), so the next class on the same computers starts at 0; the key is the
  Register name, else the computer name (`RewardBook::key`). The Stars window has the class
  selector and opens with the first star of each master session. Each change sends `ShowReward` (new total + change) to the computer;
  the worker shows a `RewardPopup` (bottom right, bottom left in RTL, 6 s). The *Stars* window
  lists all students, removes a star, starts again and exports CSV (UTF-8 BOM).
- Master splash screen rebranded (`artwork/tafat-splash.svg` → `master/resources/splash.png`).
- NSIS installer refuses Windows < 10 for Qt 6 builds (points to the legacy
  installer), < 7 for legacy builds, and 64-bit installers on 32-bit Windows
  (`VEYON_INSTALLER_MIN_WINDOWS`, `VEYON_INSTALLER_64BIT` in root `CMakeLists.txt`).
- Teacher guides (drafts, need native review): `docs/guide-enseignant.md` (French),
  `docs/guide-enseignant-ar.md` (Arabic, `<div dir="rtl">`), `docs/guide-aselmad-kab.md`
  (Tamazight, Latin script; button names from `tafat_kab.ts`/`veyon_kab.ts`). Button names are taken
  from the current catalogs; update the guides when feature names change.
- `docs/install.html`: short installation page in French, English and Arabic. It is self-contained
  (works offline from a USB stick), with an installer chooser, copy buttons, brand colors,
  the light theme of the master (always light) and phone layout. Keep it in step with `DEPLOYMENT.md`.
- `docs/DEPLOYMENT.md`: lab installation guide (installers, keys, silent install
  options, room CSV import, ports, shared accounts).
- `plugins/filetransfer/FileTransferConfiguration.h`: collected files are grouped
  by full user name + computer name by default (one folder per registered student).
- Unit tests in `tests/unit/`: `ProcessControlTest`, `WebPolicyTest` (incl.
  `InternetBlocker` ranges/rules), `QuizTest`, `ClassChatTest`, `ClassListTest`,
  `ReturnWorkTest`, `HandInTest`, `LabSetupTest`, `InventoryTest`, `RewardsTest`,
  `FeatureMessageLimitsTest` (plus 3 upstream tests) → 14 suites.
  `ProcessControlTest` also covers `UsbStorageBlocker`, `PrintBlocker` and `AppHistory`.

**CI:**
- `.github/workflows/build.yml` has:
  - Linux builds (Debian 11 Qt 5, Fedora 44 Qt 6) and check-branding;
  - `unit-tests` (same two containers, `fail-fast: false`). It fails if fewer than 13 unit
    tests are found, then runs them offscreen and the integration test under Xvfb.
  - On Debian 11 (past its LTS), Xvfb comes from the main repository if the security
    mirror returns 404.
- `.github/workflows/windows.yml`: `fedora:44`, matrix arch {i686, x86_64} ×
  Qt {6, 5}, artifacts `tafat-windows-<arch>-qt<qt>`. A tag `vX.Y.Z…` (first one: `v1.0.0`)
  also runs the `release` job: it attaches the four `*-setup.exe` to a GitHub release
  (`softprops/action-gh-release`; tags with a `-suffix` become pre-releases), named after
  the tag by the root `CMakeLists.txt` (`tafat-1.0.0.0-win64-setup.exe` on the tagged commit). LDAP and WebAPI are **off**
  on Windows for now. A concurrency group cancels older runs of the same branch
  (so push in batches), and `build-fedora.sh` builds with `ninja -k 0` and prints
  all error lines under `==== build errors ====` at the end of the step.
- Reading CI logs: the last ~165 lines of a job log are checkout cleanup; fetch
  ~200–260 tail lines to see the build output.
- Qt 5 fixes for Windows-only code done so far: Windows platform plugin links
  `Qt${QT_MAJOR_VERSION}::GuiPrivate`; UltraVNC CMake falls back from
  `qt6_disable_unicode_defines` to `-UUNICODE -U_UNICODE`;
  `LogoffEventFilter::nativeEventFilter` uses `long*` on Qt 5.

## 7. CI status (checked 2026-10-04, release runs for v1.0.1–v1.0.3 green)

| Build | Status |
|---|---|
| Linux (Qt 5 + Qt 6), check-branding | green on every commit |
| unit-tests + integration (Debian 11 Qt 5, Fedora 44 Qt 6) | green; 14 unit suites + integration test |
| Windows Qt 6 i686 / x86_64 (Win 10/11) | green, installers ~27 MB |
| Windows Qt 5 legacy i686 / x86_64 (Win 7/8.1) | green, installers ~15 MB (the x86_64 legacy job is the slowest, ~13 min) |

Releases so far: v1.0.0 (first), v1.0.1 (teacher-only setup, student installer, Add computers),
v1.0.2 (service kept on upgrade), v1.0.3 (Tafat-style toolbar icons, Contributors), v1.0.4 (Rewards
with stars per class, quiz True/False, fixes for cancelled and enforced modes and leftover workers,
smaller student toolbar; the hideable teacher toolbar came after this tag). The `release`
job only runs after all four builds; until then a fresh release shows just the source archives.

First real Windows test (owner's PC, Windows 11, 2026-10-03): installing works, the master
shows the new buttons; upgrading 1.0.0 → 1.0.1 without a reboot lost the service (fixed in v1.0.2,
see §6 labsetup "Service upgrade").

The legacy jobs also run `tools/check-windows7-imports.sh` on the packaged files:
it prints `::warning::` lines for EXE/DLL files that import Windows 8+ DLLs or
functions (they would not load on Windows 7) and **fails the job**. It passes since
run 29 (`dd72b231`): Fedora's MinGW OpenSSL 3.2 imported
`api-ms-win-core-path-l1-1-0.dll`, so `fedora-deps.sh` now builds the OpenSSL 3.5
LTS libraries for the legacy builds and replaces Fedora's `libcrypto-3.dll` /
`libssl-3.dll` with them (same ABI). Nothing has been run on a real Windows PC yet.

## 8. Unfinished work (in order)

0. **CI tests:** the `unit-tests` job and `tests/integration/` work.
   - `tests/integration/` holds `FeatureRoundTripTest` and `run-integration-test.sh`: Xvfb,
     key file auth, `tafat-server`, and checks of the Inventory and Running apps replies.
     It has the label `integration`, needs root and is skipped otherwise; config and keys
     are restored afterwards.
   - Fixed in PR #3: WITH_TESTS shadowed by bundled libvnc; C++14 test builds on GCC 10;
     `QStringList( n, value )` on Qt 5.
   - Still open: confirm the Debian 11 Xvfb fallback in CI, then merge PR #3.
1. **Real-hardware testing** — checklist in `docs/HARDWARE-TESTS.md`, on Windows 7 SP1
   32-bit, 8.1 64-bit, 10 32-bit and 11, in this order:
   1. installer and service start, `install-students.bat` on a localized Windows
      (`ver` output), NSIS version/architecture checks;
   2. screen view, lock, demo, file transfer;
   3. each blocker — only checked by reading the code so far: internet
      (`netsh` rules, InternetBlocker), USB storage policy
      (`RemovableStorageDevices\Deny_All`, may only affect devices plugged in
      afterwards), print spooler stop/disable (PrintBlocker, SCM API), website
      policies in the browsers;
   4. running apps, quiz, register + class list, hands & chat;
   5. teacher and students on different builds (legacy and modern) together.

   Windows 7 notes: `SasEventListener` loads `sas.dll` with
   `LOAD_LIBRARY_SEARCH_SYSTEM32` (needs KB2533623; without it software SAS is
   unavailable); only Chrome ≤ 109 / Firefox ESR 115 support the policies. If the
   Windows 7 import check fails after a dependency update, fix the reported DLL.
2. **Translations:**
   - Native review of all *unfinished* drafts: Arabic/French (`tafat_*.ts`,
     `veyon_ar.ts`) and Tamazight. Kabyle speakers should check the Latin drafts first
     (`tafat_kab.ts`, `veyon_kab.ts`), then re-transliterate Tifinagh.
     Choices to confirm: Asmiḍen (inventory), Tilin (register), Asemɣer (spotlight),
     Amesbadu (configurator).
   - Still without Arabic: 157 upstream texts, all LDAP (off on Windows); the CLI help,
     WebAPI and Linux texts have drafts since 2026-10-04.
     Without Kabyle: 157, also only LDAP (CLI/WebAPI/Linux drafts added 2026-10-04, Tifinagh too).
   - Hosted Weblate; small `qtbase_kab*` overrides; RTL audit of `LockWidget`,
     `Toast` and other custom-painted widgets.
3. **Decisions:** do schools need LDAP/Active Directory or the WebAPI on Windows?
   Both are off for now; re-enable in `.ci/windows/build-fedora.sh` if so.
4. **Before any public release:** trademark check for "Tafat" (INAPI, WIPO, domain).
5. **Later:**
   - Website (URL) history for the teacher (app history is done; URLs need a
     browser extension or reading the browser history databases).
   - Then whiteboard/annotation, screen recording, audio, lesson plans/rewards,
     and a pilot in 1–2 schools (`docs/DEPLOYMENT.md` covers admins).
6. Known limits (documented in DEPLOYMENT.md):
   - allow-only app mode, internet, USB and print block are Windows only;
   - Firefox needs a restart for website policies.
7. Commit directly to `main` (no PRs); never rewrite pushed history.
8. Merge new upstream Veyon releases per `UPSTREAM.md` (current base v4.11.3).
