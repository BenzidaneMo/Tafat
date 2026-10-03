<p align="center"><img src="artwork/tafat-logo.svg" alt="Tafat logo" width="160"></p>

# Tafat — ⵜⴰⴼⴰⵜ — تافات

Tafat ("light" in Tamazight) is a free and open source classroom management
solution for Algerian high schools — an open alternative to NetSupport School.

Tafat is based on [Veyon](https://veyon.io) 4.11.3 by Tobias Junghans /
Veyon Solutions and keeps its full history so upstream fixes can be merged
(see [UPSTREAM.md](UPSTREAM.md)).

> **Status:** early development, not yet tested in a real lab. Based on Veyon
> 4.11.3; Arabic/Tamazight translations and more features are in progress —
> see the [roadmap](docs/ROADMAP.md).

## Features

Inherited from Veyon:

  * Overview: monitor all computers in one or multiple classrooms
  * Remote access: view or control computers to watch and support students
  * Demo: broadcast the teacher's screen in realtime (fullscreen/window)
  * Screen lock: draw attention to what matters right now
  * Communication: send text messages to students
  * Start and end lessons: log in and log out users all at once
  * Screenshots: record learning progress and document infringements
  * Programs & websites: launch programs and open website URLs remotely
  * Teaching material: distribute and collect documents, images and videos
  * Administration: power on/off and reboot computers remotely

Added by Tafat:

  * Block apps: block listed programs or allow only the programs of the lesson,
    optionally also USB sticks and printing; see the open applications of each
    computer, the ones used before, and close them
  * Block websites: block listed sites or allow only some, in Chrome, Edge,
    Brave, Chromium and Firefox, optionally block the internet for all programs
  * Quiz: quizzes and polls with live results, scores and CSV export
  * Register: attendance with student names on the computers (also with a
    shared account), class list import and absent students
  * Hands & chat: students raise their hands, chat with the teacher and hand in
    their work
  * Return work: give each student back their own corrected files

More is planned, see the [roadmap](docs/ROADMAP.md). Installing in a lab:
[docs/DEPLOYMENT.md](docs/DEPLOYMENT.md).

## Languages

Arabic, French, Tamazight (Latin and Tifinagh scripts) and English are the
target languages for the user interface.

Tamazight in Tifinagh script uses the bundled Noto Sans Tifinagh font
(SIL Open Font License 1.1, see `core/resources/fonts/NotoSansTifinagh-OFL.txt`).

## Platforms

  * Windows 10/11, 32-bit and 64-bit
  * Windows 7/8.1, 32-bit and 64-bit (legacy build, in progress)
  * Linux

## Building

See [INSTALL](INSTALL) for build instructions.

## License

Copyright (C) 2026 Tafat contributors.
Copyright (C) 2004-2026 Tobias Junghans / Veyon Solutions.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See [LICENSE](LICENSE) for the license text and
[COPYING](COPYING) for the license as distributed with Veyon, including the
OpenSSL linking exception granted for Veyon's code.

SPDX-License-Identifier: GPL-2.0-or-later
