/*
 * PrintBlocker.h - blocks printing by stopping the print spooler
 *
 * Copyright (c) 2026 Tafat contributors
 *
 * This file is part of Tafat, which is based on Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QStringList>

// Stops the Windows print spooler and prevents it from being started again
// while the block is active. The previous start type of the service, whether
// it was running and the services depending on it that were running are
// restored afterwards. Not supported on other platforms. Requires
// administrator privileges.
class PrintBlocker
{
public:
	static bool isSupported();

	static bool apply();
	static bool clear();

	struct State
	{
		bool applied{false};
		int previousStartType{-1};
		bool wasRunning{false};
		QStringList stoppedDependents;
	};

	// start type to write back; the Windows default (automatic) if unknown
	static int restoredStartType( const State& state );

	// services to start again, the spooler first
	static QStringList servicesToRestart( const State& state );

};
