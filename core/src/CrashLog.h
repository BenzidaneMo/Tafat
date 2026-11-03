/*
 * CrashLog.h - crash report file for bug reports
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

#include "VeyonCore.h"

// On Windows, a program that crashes appends a report to %TEMP%\<product>-crash.log:
// program, command line, versions, the start-up stage reached, the exception, the
// faulting module + offset, a stack trace and the loaded modules. It is installed when
// the core library is loaded, so crashes before the configuration and the log file
// exist are covered too. On Linux, LinuxPlatformPlugin logs a backtrace instead.
class VEYON_CORE_EXPORT CrashLog
{
public:
	static void install();

	// name of what the program is doing, written to the report; must be a string literal
	static void setStage( const char* stage );

};
