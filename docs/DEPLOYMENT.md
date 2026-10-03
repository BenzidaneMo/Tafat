# Installing Tafat in a computer lab

This guide is for the person who sets up a lab: one teacher computer and the
student computers in the same network. All steps also work for several labs.
For the teachers: [guide de l'enseignant](guide-enseignant.md) (français),
[دليل الأستاذ](guide-enseignant-ar.md) (العربية). A short version of this guide in French,
English and Arabic, for printing or a USB stick: [install.html](install.html).

> Status: the installers are built by CI but have not been tested on real lab
> computers yet. Please report problems.

## 1. Choose the installer

| Windows on the computer | Installer |
|---|---|
| Windows 10 / 11, 64-bit | `tafat-<version>-win64-setup.exe` |
| Windows 10 / 11, 32-bit | `tafat-<version>-win32-setup.exe` |
| Windows 7 / 8.1, 64-bit | `tafat-<version>-win64-legacy-setup.exe` |
| Windows 7 / 8.1, 32-bit | `tafat-<version>-win32-legacy-setup.exe` |

All builds work together: a Windows 11 teacher computer can control Windows 7
student computers and the other way round. The installer refuses to run on a
Windows version or architecture it is not made for.

Download the installers from the [Releases page](https://github.com/BenzidaneMo/Tafat/releases/latest). Every version tag
(`vX.Y.Z`, e.g. `v1.0.0`; `v1.1.0-beta1` makes a pre-release) publishes the four
installers there. Newer
development builds are the artifacts of the "Windows builds" GitHub Actions
workflow (`tafat-windows-<arch>-qt<6|5>`; Qt 5 = legacy build). They are zip files,
need a GitHub login and expire after 90 days.

### Windows 7 and 8.1

- Windows 7 needs Service Pack 1. Update KB2533623 (included in most update
  rollups) is recommended; without it the "secure attention sequence"
  (Ctrl+Alt+Del) cannot be sent remotely.
- Website blocking works with the last browser versions for Windows 7:
  Chrome 109 and Firefox ESR 115.
- Windows 7 gets no security updates any more: keep these computers in the
  lab network, ideally without direct internet access.

## 2. Teacher computer

Run the installer with all components. With the teacher program selected, the
installer also:

- creates the key pair `teacher` (kept on updates) and switches to key file
  authentication (`tafat-wcli labsetup setupteacher`);
- lets the local *Users* group read the private key, so the teacher can work
  with a normal Windows account (the group is found by its well-known SID, so
  this also works on French or Arabic Windows);
- keeps a copy of itself in `C:\Program Files\Tafat\setup\` for the student
  installer.

The finish page offers *Start Tafat Master now* (started with the rights of the
logged-on user). `/NoTeacherSetup` skips the key and authentication setup.

The language is set in **Tafat Master** → **Settings** (the configurator) →
*General* → *Language*: Arabic, French, Tamazight (Latin or Tifinagh) or
English. *OK* applies and closes the configurator.

## 3. Student computers

### With the student installer (recommended)

1. In **Tafat Master**, click **Student installer**. The list shows the
   installer of the teacher computer; for other Windows versions in the lab
   (Windows 7/8.1, 32-bit) click *Add installer…* and choose them.
2. Click *Create on USB stick or folder…*. One file per installer is written,
   e.g. `Install Tafat - student (Windows 10-11 64-bit).exe`.
3. On each student computer, double-click it and answer *Yes* to the Windows
   prompt. Only Welcome, progress and Finish pages are shown; the teacher
   program is not installed.

A student installer is the normal installer with a block appended at its end
(`plugins/labsetup/StudentPackage.cpp`): the teacher's public key and the
current configuration without `Core/InstallationID`, as JSON, then an 8-byte
length and the marker `TAFAT-STUDENT-PACKAGE-v1`. The installer finds the
marker, runs `tafat-wcli labsetup extractpackage` and imports both files like
`/ApplyConfig` and `/ImportPublicKey`. NSIS only checks the data covered by its
own header, so the appended block does not break the integrity check. It also
works silently (`"Install Tafat - student (…).exe" /S`). Without the dialog:

```bat
"C:\Program Files\Tafat\tafat-cli.exe" labsetup createpackage tafat-<version>-win64-setup.exe E:\students.exe [key name]
```

### Adding the computers to a room

1. Switch the student computers on.
2. In **Tafat Master**, click **Add computers**, enter the room name and click
   **Search**. Tafat tries TCP port 11100 on every address of the private IPv4
   networks of the teacher computer (the /24 network around each address, at most
   1024 addresses, about 5 s for a /24) and lists the hosts that answer like a
   Tafat server. Names come from reverse DNS; computers on other networks can be
   added by name or IP address.
3. Tick the computers and click **Add to the room** (one administrator prompt).
   It runs `tafat-wcli networkobjects import <csv> location <room> format
   "%name%;%host%"`; the room is ticked in *Locations & computers*, so the
   computers show up at once.

### Advanced: student setup folder or a deployment tool

**Settings** → *Lab setup* → *Export student setup…* writes
`teacher_public_key.pem`, `tafat-config.json` and `install-students.bat` to a
folder. Copy the installers next to them and run `install-students.bat` as
administrator on each student computer: it picks the installer for the Windows
version and installs silently with these options:

```bat
tafat-<version>-win64-setup.exe /S /NoMaster /ApplyConfig=D:\tafat-config.json /ImportPublicKey=D:\teacher_public_key.pem
```

Installer options:

| Option | Effect |
|---|---|
| `/S` | silent installation |
| `/NoMaster` | do not install the teacher program |
| `/NoTeacherSetup` | with the teacher program: do not create the key or change authentication |
| `/NoInterception` | do not install the input device driver (used for locking the keyboard/mouse) |
| `/NoStartMenuFolder` | no start menu entries |
| `/ApplyConfig=<file>` | import a configuration exported from the configurator |
| `/ImportPublicKey=<file>` | import a public key (replaces a key of the same name) |
| `/PublicKeyName=<name>` | name for `/ImportPublicKey` (default `teacher`) |
| `/D=<folder>` | installation folder (must be the last option) |

Rooms can also be imported from a CSV file (`room;name;host-or-IP`):

```bat
"C:\Program Files\Tafat\tafat-cli.exe" networkobjects import computers.csv format "%location%;%name%;%host%"
```

> In **PowerShell**, a quoted program path needs `&` in front:
> `& "C:\Program Files\Tafat\tafat-cli.exe" …`. Otherwise PowerShell reports
> "Unexpected token". `cmd.exe` needs no `&`. Commands that change the
> configuration need an administrator prompt.

Uninstall: `"C:\Program Files\Tafat\uninstall.exe" /S`, add `/ClearConfig` to
remove the configuration as well.

After the installation, select all computers in Tafat Master and open
**Inventory** to check that every computer answers and runs the expected
version (legacy builds are marked); *Export CSV…* saves the list.

## 4. Network

- The installer adds a Windows firewall exception for the Tafat service. The
  teacher computer connects to TCP port 11100 of the students (plus 11400 on
  the teacher computer for demos). Other firewalls in the lab must allow them.
- The teacher and the students must reach each other by name or IP address.
  Fixed IP addresses or DHCP reservations make the lab list reliable.
- "Block websites" with "Also block the internet for all other programs" adds
  Windows firewall rules on the students that block web traffic to public
  internet addresses only; the lab network keeps working.
- "Block apps" can also block USB sticks and printing on Windows. Printing is
  blocked by stopping the *Print Spooler* service, so shared printers of the
  student computers are unavailable meanwhile; the service is restored when the
  mode ends or the Tafat service restarts.

## 5. Shared student accounts

Labs often use one Windows account for all students. Use **Register** at the
beginning of the lesson: the students type their names, the teacher's computer
tiles show them, attendance can be exported, collected files are filed per
student (students can also **hand in** files from their toolbar, and **Return
work** gives each student their corrected files back),
and the chat shows who wrote. Import the class list (CSV from the
school's spreadsheet) in the register window to see who is absent.

## 6. Linux

Tafat also builds for Linux (`.deb`/`.rpm` from the "build" workflow). The
service is named `tafat`. Allow-only application control and the internet
block for all programs are Windows only.

---

Tafat is developed by [BenzidaneMo](https://github.com/BenzidaneMo), based on [Veyon](https://veyon.io).
