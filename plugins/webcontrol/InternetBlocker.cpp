/*
 * InternetBlocker.cpp - blocks internet access of all programs
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
#include <QProcess>

#include "InternetBlocker.h"
#include "VeyonCore.h"


static QString ruleName( const QString& protocol )
{
	return QStringLiteral("%1 - block internet (%2)").arg( VeyonCore::productName(), protocol );
}



bool InternetBlocker::isSupported()
{
#ifdef Q_OS_WIN
	return true;
#else
	return false;
#endif
}



QString InternetBlocker::publicAddressRanges()
{
	return QStringLiteral("1.0.0.0-9.255.255.255,"
						  "11.0.0.0-126.255.255.255,"
						  "128.0.0.0-169.253.255.255,"
						  "169.255.0.0-172.15.255.255,"
						  "172.32.0.0-192.167.255.255,"
						  "192.169.0.0-223.255.255.255");
}



QList<QStringList> InternetBlocker::addRuleArguments()
{
	QList<QStringList> commands;

	for( const auto& protocol : { QStringLiteral("TCP"), QStringLiteral("UDP") } )
	{
		commands.append( {
			QStringLiteral("advfirewall"), QStringLiteral("firewall"), QStringLiteral("add"), QStringLiteral("rule"),
			QStringLiteral("name=%1").arg( ruleName( protocol ) ),
			QStringLiteral("dir=out"), QStringLiteral("action=block"), QStringLiteral("enable=yes"),
			QStringLiteral("profile=any"),
			QStringLiteral("protocol=%1").arg( protocol ),
			// UDP 443: QUIC (HTTP/3)
			QStringLiteral("remoteport=%1").arg( protocol == QLatin1String("TCP") ? QStringLiteral("80,443") : QStringLiteral("443") ),
			QStringLiteral("remoteip=%1").arg( publicAddressRanges() )
		} );
	}

	return commands;
}



QList<QStringList> InternetBlocker::deleteRuleArguments()
{
	QList<QStringList> commands;

	for( const auto& protocol : { QStringLiteral("TCP"), QStringLiteral("UDP") } )
	{
		commands.append( { QStringLiteral("advfirewall"), QStringLiteral("firewall"), QStringLiteral("delete"),
						   QStringLiteral("rule"), QStringLiteral("name=%1").arg( ruleName( protocol ) ) } );
	}

	return commands;
}



#ifdef Q_OS_WIN
static bool runNetsh( const QStringList& arguments )
{
	const auto netsh = QDir( QString::fromLocal8Bit( qgetenv( "SystemRoot" ) ) ).filePath( QStringLiteral("System32/netsh.exe") );

	QProcess process;
	process.start( QDir::toNativeSeparators( netsh ), arguments );
	if( process.waitForFinished( 15000 ) == false )
	{
		process.kill();
		return false;
	}

	return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
#endif



bool InternetBlocker::apply()
{
#ifdef Q_OS_WIN
	// never add the rules twice
	clear();

	bool success = true;
	for( const auto& arguments : addRuleArguments() )
	{
		success = runNetsh( arguments ) && success;
	}
	if( success == false )
	{
		vWarning() << "could not add the firewall rules that block internet access";
	}
	return success;
#else
	return false;
#endif
}



bool InternetBlocker::clear()
{
#ifdef Q_OS_WIN
	// deleting fails when there is no such rule, which is fine
	for( const auto& arguments : deleteRuleArguments() )
	{
		runNetsh( arguments );
	}
#endif
	return true;
}
