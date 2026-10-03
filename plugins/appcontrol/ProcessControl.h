/*
 * ProcessControl.h - lists and terminates applications of the user session
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

#include <QList>
#include <QString>
#include <QStringList>

// Lists and terminates the applications running in the user session the
// server process belongs to.
class ProcessControl
{
public:
	enum class Policy
	{
		BlockListed,
		AllowListedOnly
	};

	struct Process
	{
		quint32 id{0};
		QString name; // normalized executable name, e.g. "chrome"
		bool hasWindow{false};
	};

	// "C:\Program Files\App\App.EXE" -> "app"
	static QString normalizedName( const QString& executable );

	static QList<Process> sessionProcesses();

	static bool terminate( quint32 processId );

	// whether Process::hasWindow is available (required for allow lists)
	static bool supportsWindowDetection();

	// the product's own programs and essential parts of the desktop are never closed
	static bool isProtected( const QString& name );

	// names of the applications the user works with: processes with a window
	// (all processes without window detection), without protected ones, sorted
	static QStringList openApplications( const QList<Process>& processes,
										 bool windowDetection = supportsWindowDetection() );

	// with Policy::AllowListedOnly only processes with a window are closed
	static QList<Process> processesToClose( const QList<Process>& processes, Policy policy,
											const QStringList& applications,
											bool windowDetection = supportsWindowDetection() );

};
