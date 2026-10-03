/*
 * SystemInventory.cpp - hardware and software facts of a computer
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

#include <QFile>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QThread>

#include "SystemInventory.h"
#include "VeyonCore.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif


QVariantMap SystemInventory::collect()
{
	QVariantMap inventory;

	inventory[QLatin1String(ComputerName)] = QSysInfo::machineHostName();
	inventory[QLatin1String(OperatingSystem)] = QSysInfo::prettyProductName();
	inventory[QLatin1String(KernelVersion)] = QSysInfo::kernelVersion();
	inventory[QLatin1String(Architecture)] = QSysInfo::currentCpuArchitecture();
	inventory[QLatin1String(BuildArchitecture)] = QSysInfo::buildCpuArchitecture();
	inventory[QLatin1String(Processor)] = processorName();
	inventory[QLatin1String(Cores)] = QThread::idealThreadCount();
	inventory[QLatin1String(MemoryMB)] = totalMemoryMB();

	const auto root = QStorageInfo::root();
	if( root.isValid() )
	{
		inventory[QLatin1String(DiskTotalMB)] = root.bytesTotal() / ( 1024 * 1024 );
		inventory[QLatin1String(DiskFreeMB)] = root.bytesAvailable() / ( 1024 * 1024 );
	}

	QStringList ipAddresses;
	QStringList macAddresses;
	for( const auto& networkInterface : QNetworkInterface::allInterfaces() )
	{
		const auto flags = networkInterface.flags();
		if( flags.testFlag( QNetworkInterface::IsUp ) == false ||
			flags.testFlag( QNetworkInterface::IsRunning ) == false ||
			flags.testFlag( QNetworkInterface::IsLoopBack ) ||
			networkInterface.type() == QNetworkInterface::Virtual )
		{
			continue;
		}

		bool hasIPv4 = false;
		for( const auto& entry : networkInterface.addressEntries() )
		{
			if( entry.ip().protocol() == QAbstractSocket::IPv4Protocol )
			{
				ipAddresses.append( entry.ip().toString() );
				hasIPv4 = true;
			}
		}
		if( hasIPv4 && networkInterface.hardwareAddress().isEmpty() == false )
		{
			macAddresses.append( networkInterface.hardwareAddress() );
		}
	}
	inventory[QLatin1String(IpAddresses)] = ipAddresses.join( QStringLiteral(", ") );
	inventory[QLatin1String(MacAddresses)] = macAddresses.join( QStringLiteral(", ") );

	inventory[QLatin1String(ProductVersion)] = VeyonCore::versionString();
	inventory[QLatin1String(QtVersion)] = QString::fromLatin1( qVersion() );

	return inventory;
}



QString SystemInventory::formatMegabytes( qint64 megabytes )
{
	if( megabytes <= 0 )
	{
		return {};
	}
	if( megabytes < 1024 )
	{
		return QStringLiteral("%1 MB").arg( megabytes );
	}
	return QStringLiteral("%1 GB").arg( double( megabytes ) / 1024, 0, 'f', 1 );
}



QString SystemInventory::describeSystem( const QVariantMap& inventory )
{
	const auto os = inventory.value( QLatin1String(OperatingSystem) ).toString();
	const auto arch = inventory.value( QLatin1String(Architecture) ).toString();
	if( os.isEmpty() )
	{
		return {};
	}

	const auto is64 = arch.contains( QLatin1String("64") );
	return QStringLiteral("%1 (%2)").arg( os, is64 ? QStringLiteral("64-bit") : QStringLiteral("32-bit") );
}



QString SystemInventory::csvLine( const QStringList& fields )
{
	QStringList quoted;
	quoted.reserve( fields.size() );
	for( auto field : fields )
	{
		quoted.append( QLatin1Char('"') + field.replace( QLatin1Char('"'), QStringLiteral("\"\"") ) + QLatin1Char('"') );
	}
	return quoted.join( QLatin1Char(',') );
}



qint64 SystemInventory::memoryFromMeminfo( const QByteArray& meminfo )
{
	static const QRegularExpression memTotalRX{ QStringLiteral("^MemTotal:\\s+(\\d+)\\s+kB"),
												QRegularExpression::MultilineOption };
	const auto match = memTotalRX.match( QString::fromLatin1( meminfo ) );
	return match.hasMatch() ? match.captured( 1 ).toLongLong() / 1024 : 0;
}



QString SystemInventory::processorFromCpuinfo( const QByteArray& cpuinfo )
{
	static const QRegularExpression modelNameRX{ QStringLiteral("^model name\\s*:\\s*(.+)$"),
												 QRegularExpression::MultilineOption };
	const auto match = modelNameRX.match( QString::fromUtf8( cpuinfo ) );
	return match.hasMatch() ? match.captured( 1 ).simplified() : QString{};
}



qint64 SystemInventory::totalMemoryMB()
{
#ifdef Q_OS_WIN
	MEMORYSTATUSEX status{};
	status.dwLength = sizeof( status );
	if( GlobalMemoryStatusEx( &status ) )
	{
		return qint64( status.ullTotalPhys / ( 1024 * 1024 ) );
	}
	return 0;
#else
	QFile meminfo( QStringLiteral("/proc/meminfo") );
	return meminfo.open( QFile::ReadOnly ) ? memoryFromMeminfo( meminfo.readAll() ) : 0;
#endif
}



QString SystemInventory::processorName()
{
#ifdef Q_OS_WIN
	const QSettings cpu( QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
						 QSettings::NativeFormat );
	return cpu.value( QStringLiteral("ProcessorNameString") ).toString().simplified();
#else
	QFile cpuinfo( QStringLiteral("/proc/cpuinfo") );
	return cpuinfo.open( QFile::ReadOnly ) ? processorFromCpuinfo( cpuinfo.readAll() ) : QString{};
#endif
}
