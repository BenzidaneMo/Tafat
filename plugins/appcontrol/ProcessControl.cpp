/*
 * ProcessControl.cpp - lists and terminates applications of the user session
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include "PlatformUserFunctions.h"
#include "ProcessControl.h"
#include "VeyonCore.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#endif


QString ProcessControl::normalizedName( const QString& executable )
{
	auto name = executable.trimmed();
	name = name.mid( qMax( name.lastIndexOf( QLatin1Char('/') ), name.lastIndexOf( QLatin1Char('\\') ) ) + 1 );
	if( name.endsWith( QLatin1String(".exe"), Qt::CaseInsensitive ) )
	{
		name.chop( 4 );
	}
	return name.toLower();
}



bool ProcessControl::isProtected( const QString& name )
{
	static const QStringList desktopComponents{
		QStringLiteral("explorer"), QStringLiteral("dwm"), QStringLiteral("sihost"), QStringLiteral("ctfmon"),
		QStringLiteral("winlogon"), QStringLiteral("logonui"), QStringLiteral("lockapp"), QStringLiteral("csrss"),
		QStringLiteral("searchui"), QStringLiteral("searchapp"), QStringLiteral("searchhost"),
		QStringLiteral("startmenuexperiencehost"), QStringLiteral("shellexperiencehost"),
		QStringLiteral("textinputhost"), QStringLiteral("applicationframehost")
	};

	return name.startsWith( VeyonCore::productSlug() + QLatin1Char('-') ) ||
		   desktopComponents.contains( name );
}



QList<ProcessControl::Process> ProcessControl::processesToClose( const QList<Process>& processes, Policy policy,
																 const QStringList& applications,
																 bool windowDetection )
{
	QList<Process> result;

	for( const auto& process : processes )
	{
		if( isProtected( process.name ) )
		{
			continue;
		}

		const auto listed = applications.contains( process.name );
		const auto close = policy == Policy::BlockListed
							   ? listed
							   : ( windowDetection && process.hasWindow && listed == false );
		if( close )
		{
			result.append( process );
		}
	}

	return result;
}



#ifdef Q_OS_WIN

static BOOL CALLBACK collectWindowProcess( HWND window, LPARAM param )
{
	if( IsWindowVisible( window ) && GetWindow( window, GW_OWNER ) == nullptr && GetWindowTextLengthW( window ) > 0 )
	{
		DWORD processId = 0;
		GetWindowThreadProcessId( window, &processId );
		reinterpret_cast<QSet<quint32> *>( param )->insert( processId );
	}
	return TRUE;
}



QList<ProcessControl::Process> ProcessControl::sessionProcesses()
{
	DWORD currentSession = 0;
	ProcessIdToSessionId( GetCurrentProcessId(), &currentSession );

	QSet<quint32> windowProcesses;
	EnumWindows( collectWindowProcess, reinterpret_cast<LPARAM>( &windowProcesses ) );

	QList<Process> processes;

	const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( snapshot == INVALID_HANDLE_VALUE )
	{
		return processes;
	}

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);

	if( Process32FirstW( snapshot, &entry ) )
	{
		do
		{
			DWORD session = 0;
			if( entry.th32ProcessID != 0 &&
				ProcessIdToSessionId( entry.th32ProcessID, &session ) &&
				session == currentSession )
			{
				processes.append( { entry.th32ProcessID,
									normalizedName( QString::fromWCharArray( entry.szExeFile ) ),
									windowProcesses.contains( entry.th32ProcessID ) } );
			}
		} while( Process32NextW( snapshot, &entry ) );
	}

	CloseHandle( snapshot );

	return processes;
}



bool ProcessControl::terminate( quint32 processId )
{
	const auto process = OpenProcess( PROCESS_TERMINATE, FALSE, processId );
	if( process == nullptr )
	{
		return false;
	}

	const auto result = TerminateProcess( process, 1 );
	CloseHandle( process );

	return result;
}



bool ProcessControl::supportsWindowDetection()
{
	return true;
}

#else

QList<ProcessControl::Process> ProcessControl::sessionProcesses()
{
	const auto user = VeyonCore::platform().userFunctions().queryCurrentUserProperty( PlatformUserFunctions::UserProperty::LoginName );
	const auto passwordEntry = user.isEmpty() ? nullptr : getpwnam( user.toUtf8().constData() );
	if( passwordEntry == nullptr )
	{
		return {};
	}

	const auto userId = passwordEntry->pw_uid;

	QList<Process> processes;

	const auto entries = QDir( QStringLiteral("/proc") ).entryList( QDir::Dirs | QDir::NoDotAndDotDot );
	for( const auto& entry : entries )
	{
		bool isProcess = false;
		const auto processId = entry.toUInt( &isProcess );
		const auto processPath = QStringLiteral("/proc/") + entry;

		// /proc/<pid> is owned by the effective user of the process
		if( isProcess == false || QFileInfo( processPath ).ownerId() != userId )
		{
			continue;
		}

		auto executable = QFileInfo( processPath + QStringLiteral("/exe") ).symLinkTarget();
		if( executable.isEmpty() )
		{
			QFile comm( processPath + QStringLiteral("/comm") );
			if( comm.open( QFile::ReadOnly ) )
			{
				executable = QString::fromUtf8( comm.readAll().trimmed() );
			}
		}

		if( executable.isEmpty() == false )
		{
			processes.append( { processId, normalizedName( executable ), false } );
		}
	}

	return processes;
}



bool ProcessControl::terminate( quint32 processId )
{
	return ::kill( pid_t(processId), SIGTERM ) == 0;
}



bool ProcessControl::supportsWindowDetection()
{
	return false;
}

#endif
