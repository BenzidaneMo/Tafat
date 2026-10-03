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

1. Run the installer with all components.
2. Start **Tafat Configurator** → *Authentication*: choose *Key file
   authentication* and create a key pair (for example named `teacher`).
   From the command line:

   ```bat
   "C:\Program Files\Tafat\tafat-cli.exe" authkeys create teacher
   "C:\Program Files\Tafat\tafat-cli.exe" authkeys export teacher/public teacher_public_key.pem
   ```

3. *Locations & computers*: add the lab and its computers, or import them
   from a CSV file (`room;name;host-or-IP`):

   ```bat
   "C:\Program Files\Tafat\tafat-cli.exe" networkobjects import computers.csv format "%location%;%name%;%host%"
   ```

4. *General* → *Language*: Arabic, French, Tamazight (Latin or Tifinagh) or
   English.
5. Export the configuration for the students (*File* → *Save settings to
   file*, e.g. `lab.json`). It contains the authentication method and the
   access settings, not the private key.

## 3. Student computers

### With the student setup folder (recommended)

1. In **Tafat Configurator** → *Lab setup*, choose the teacher's key pair and
   click *Export student setup…*. Choose an empty folder, e.g. on a USB stick.
   It receives `teacher_public_key.pem` (only the public key),
   `tafat-config.json` (these settings) and `install-students.bat`.
2. Copy the installers into the same folder (all four, or only the ones the
   lab needs).
3. On each student computer, right click `install-students.bat` →
   *Run as administrator*. The script picks the installer for the Windows
   version (legacy installer on Windows 7/8.1, 32 or 64 bit) and installs
   silently without the teacher program, applies the settings and imports the
   key.

### By hand or with a deployment tool

Install without the teacher program, import the teacher's public key and the
configuration:

```bat
tafat-<version>-win64-setup.exe /S /NoMaster /ApplyConfig=D:\lab.json
"C:\Program Files\Tafat\tafat-cli.exe" authkeys import teacher/public D:\teacher_public_key.pem
"C:\Program Files\Tafat\tafat-cli.exe" service restart
```

Installer options:

| Option | Effect |
|---|---|
| `/S` | silent installation |
| `/NoMaster` | do not install the teacher program |
| `/NoInterception` | do not install the input device driver (used for locking the keyboard/mouse) |
| `/NoStartMenuFolder` | no start menu entries |
| `/ApplyConfig=<file>` | import a configuration exported from the configurator |
| `/ImportPublicKey=<file>` | import a public key (replaces a key of the same name) |
| `/PublicKeyName=<name>` | name for `/ImportPublicKey` (default `teacher`) |
| `/D=<folder>` | installation folder (must be the last option) |

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
