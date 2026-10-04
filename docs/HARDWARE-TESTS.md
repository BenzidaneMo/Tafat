# Tafat – tests on real Windows PCs

Nothing has run on a real Windows machine yet. Work through the sections **in order**
on each PC and tick the boxes in a copy of this file (one copy per PC). Write down
anything that fails with the Windows version, the installer file name and, if
possible, the log files `Tafat*.log` from `C:\Windows\Temp\` (service, server) and
from `%TEMP%` of the logged-in user (teacher app, student helper).

| PC | Windows | Installer to use |
|---|---|---|
| A | 7 SP1, 32-bit | `tafat-*-win32-legacy-setup.exe` |
| B | 8.1, 64-bit | `tafat-*-win64-legacy-setup.exe` |
| C | 10, 32-bit | `tafat-*-win32-setup.exe` |
| D | 11 (64-bit) | `tafat-*-win64-setup.exe` |

Use one PC as the teacher and the others as students; in section 6 swap the roles.
On Windows 7, install KB2533623 first (needed for Ctrl+Alt+Del handling) and use
Chrome ≤ 109 or Firefox ESR 115 (newer browsers don't run there).

## 1. Installer and service

- [ ] Installer runs and finishes (as administrator).
- [ ] Wrong installer is refused with a clear message: modern installer on Windows 7/8.1,
      64-bit installer on 32-bit Windows.
- [ ] **Upgrade** over the previous version without rebooting: `sc query TafatService`
      still finds the service afterwards (v1.0.1 lost it: the old service was still
      being deleted when the new one was registered; fixed in the installer by waiting).
- [ ] Service "TafatService" is running after install and after a reboot
      (`sc query TafatService`).
- [ ] Teacher install (normal Windows account for the teacher, installer run as
      administrator): `tafat-cli authkeys list` shows `teacher` public and private,
      Authentication/Method is key file, `C:\Program Files\Tafat\setup\` holds a copy
      of the installer. On a **French or Arabic Windows**, the private key is readable
      by the local Users group (`setaccessgroup` with the localized group name).
- [ ] Finish page "Start Tafat Master now" starts the master **not** elevated; the title
      reads "Tafat Master".
- [ ] Master → **Student installer** → *Create on USB stick or folder…* writes
      `Install Tafat - student (…).exe` (also with an added legacy installer).
- [ ] Student: double-click the student installer → one UAC prompt, only Welcome /
      progress / Finish pages, no teacher program; afterwards service running, key
      `teacher` imported, configuration applied, finish page has no "Start" checkbox.
      Also silent: `"Install Tafat - student (…).exe" /S`. Windows does **not** report
      a damaged installer (data appended after the NSIS installer) and SmartScreen
      behaves as for the plain installer.
- [ ] Master → **Add computers** → **Search** finds the student PCs within ~5 s
      (Windows firewall rule for port 11100 active), names from reverse DNS;
      **Add to the room** shows one UAC prompt and the room appears ticked with its
      computers. Repeat with the master started as administrator (no prompt).
- [ ] Master → **Settings** opens the configurator (UAC prompt); *OK* applies and closes.
- [ ] Advanced: configurator Lab setup → export the student setup folder, copy the
      installers next to it and run `install-students.bat` as administrator, also on a
      French or Arabic Windows (the script reads the `ver` output); it picks the right
      installer (see the table above); service running, key imported, config applied.

## 2. Basic functions (upstream Veyon)

- [ ] Teacher sees the student screens (thumbnails update).
- [ ] Lock / unlock screens.
- [ ] Demo: full screen and window, teacher screen to students.
- [ ] File transfer: send a file to students; collect files from students.

## 3. Blockers

Start each blocker from the teacher, check on the student, then stop it and check
that everything is back to normal. Repeat once after rebooting the student PC while
the blocker is on (it must clean up at service start).

- [ ] **Block apps**: block list closes the app; allow-only list closes everything else
      but keeps Explorer and system programs; the student sees a notice.
- [ ] **Internet block** (option in "Block websites"):
      `netsh advfirewall firewall show rule name="Tafat - block internet (TCP)"`
      (and `(UDP)`) exists while active and is gone after stop. Websites don't load,
      but the teacher connection, demo and LAN shares still work.
- [ ] **USB storage**: while active,
      `HKLM\SOFTWARE\Policies\Microsoft\Windows\RemovableStorageDevices` has
      `Deny_All = 1`. Check a stick plugged in **before** and one plugged in **after**
      starting (only the second may be blocked). After stop the old value is back.
- [ ] **Printing**: while active, `sc query Spooler` is stopped and `sc qc Spooler`
      shows DISABLED; printing fails. After stop, the start type and running state are
      as before.
- [ ] **Website blocking**: Chrome/Edge/Brave show the list in `chrome://policy` /
      `edge://policy`; Firefox in `about:policies` (after a restart). Blocked sites are
      blocked; with an allow list only those sites open. After stop the policies are gone
      and policies set by the admin before are still there.

## 4. Tafat features

- [ ] **Running apps**: list per computer, refreshes, "close" works on one and on all
      computers; history shows "since …" and closed apps in grey.
- [ ] **Quiz**: create (single, multiple, text), launch, students answer, countdown
      auto-submits, live results and bars, CSV opens correctly in Excel (Arabic text).
- [ ] **Register + class list**: import a CSV class list (with Arabic names), students
      register, tiles show their names, absent students in red, CSV export.
- [ ] **Hands and chat**: show toolbar, raise hand (icon on tile), chat one/all,
      lower hand, hand in work (several files), "Open handed-in work".
- [ ] **Return work**: files come back into each student's "Returned work" folder.
- [ ] **Inventory**: all PCs listed, legacy builds marked.
- [ ] **Rewards**: "Give a star" on one and on several computers shows the popup on each
      student screen (also over a full-screen program), "Show stars" lists the names from
      the register, the stars stay after restarting Tafat Master, CSV opens in Excel.
      A second class (*New class*) starts without stars on the same computers.
- [ ] **Teacher toolbar**: the button at the right end of the bottom bar hides and shows
      the toolbar in Tafat Master; it stays hidden after closing and reopening the master.
- [ ] **Student toolbar**: the arrow makes the bar smaller (only the hand) and bigger
      again; raising the hand works in both; the choice stays after hiding and showing it.

## 5. Languages on Windows

- [ ] Arabic UI is right-to-left, numbers like "3 / 4" are not reversed.
- [ ] Tifinagh text is shown (not boxes) on Windows 7 and 10.

## 6. Mixed builds

- [ ] Teacher on a modern build (C or D) with students on legacy builds (A, B):
      sections 2–4 work.
- [ ] Teacher on a legacy build (A or B) with students on modern builds.

## Results

| Section | A (7, 32) | B (8.1, 64) | C (10, 32) | D (11) |
|---|---|---|---|---|
| 1 Installer | | | | |
| 2 Basic | | | | |
| 3 Blockers | | | | |
| 4 Features | | | | |
| 5 Languages | | | | |
| 6 Mixed | | | | |
