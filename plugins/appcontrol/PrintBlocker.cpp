/*
 * PrintBlocker.cpp - blocks printing by stopping the print spooler
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

#include <QElapsedTimer>
#include <QSettings>
#include <QThread>

#include "PrintBlocker.h"
#include "VeyonCore.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif


static constexpr int AutoStart = 2; // SERVICE_AUTO_START

#ifdef Q_OS_WIN
static constexpr int DisabledStart = 4; // SERVICE_DISABLED
static const auto SpoolerService = QStringLiteral("Spooler");

static QSettings printBlockStateStore()
{
	return QSettings( QSettings::SystemScope, VeyonCore::productName(), QStringLiteral("AppControl") );
}



class ServiceHandle
{
public:
	explicit ServiceHandle( SC_HANDLE handle ) : m_handle( handle ) { }
	~ServiceHandle()
	{
		if( m_handle )
		{
			CloseServiceHandle( m_handle );
		}
	}
	ServiceHandle( const ServiceHandle& ) = delete;
	ServiceHandle& operator=( const ServiceHandle& ) = delete;

	operator SC_HANDLE() const
	{
		return m_handle;
	}

private:
	SC_HANDLE m_handle;
};



static bool isRunning( SC_HANDLE service )
{
	SERVICE_STATUS status{};
	return QueryServiceStatus( service, &status ) && status.dwCurrentState != SERVICE_STOPPED;
}



static bool waitForState( SC_HANDLE service, DWORD state )
{
	QElapsedTimer timer;
	timer.start();
	SERVICE_STATUS status{};
	while( QueryServiceStatus( service, &status ) && status.dwCurrentState != state )
	{
		if( timer.elapsed() > 15000 )
		{
			return false;
		}
		QThread::msleep( 200 );
	}
	return status.dwCurrentState == state;
}



static bool stopService( SC_HANDLE manager, const QString& name )
{
	ServiceHandle service( OpenServiceW( manager, reinterpret_cast<LPCWSTR>( name.utf16() ),
										 SERVICE_STOP | SERVICE_QUERY_STATUS ) );
	if( service == nullptr )
	{
		return false;
	}
	if( isRunning( service ) == false )
	{
		return true;
	}
	SERVICE_STATUS status{};
	ControlService( service, SERVICE_CONTROL_STOP, &status );
	return waitForState( service, SERVICE_STOPPED );
}



static void startService( SC_HANDLE manager, const QString& name )
{
	ServiceHandle service( OpenServiceW( manager, reinterpret_cast<LPCWSTR>( name.utf16() ), SERVICE_START ) );
	if( service == nullptr || StartServiceW( service, 0, nullptr ) == false )
	{
		vWarning() << "could not start service" << name << GetLastError();
	}
}



// running services that depend on the spooler (e.g. the fax service)
static QStringList runningDependents( SC_HANDLE spooler )
{
	DWORD bytesNeeded = 0;
	DWORD count = 0;
	if( EnumDependentServicesW( spooler, SERVICE_ACTIVE, nullptr, 0, &bytesNeeded, &count ) ||
		GetLastError() != ERROR_MORE_DATA )
	{
		return {};
	}

	QByteArray buffer( int( bytesNeeded ), 0 );
	auto services = reinterpret_cast<LPENUM_SERVICE_STATUSW>( buffer.data() );
	if( EnumDependentServicesW( spooler, SERVICE_ACTIVE, services, bytesNeeded, &bytesNeeded, &count ) == false )
	{
		return {};
	}

	QStringList names;
	for( DWORD i = 0; i < count; ++i )
	{
		names.append( QString::fromWCharArray( services[i].lpServiceName ) );
	}
	return names;
}



static int startType( SC_HANDLE service )
{
	DWORD bytesNeeded = 0;
	QueryServiceConfigW( service, nullptr, 0, &bytesNeeded );
	if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
	{
		return -1;
	}
	QByteArray buffer( int( bytesNeeded ), 0 );
	auto config = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>( buffer.data() );
	if( QueryServiceConfigW( service, config, bytesNeeded, &bytesNeeded ) == false )
	{
		return -1;
	}
	return int( config->dwStartType );
}



static bool setStartType( SC_HANDLE service, int type )
{
	return ChangeServiceConfigW( service, SERVICE_NO_CHANGE, DWORD( type ), SERVICE_NO_CHANGE,
								 nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr );
}
#endif



bool PrintBlocker::isSupported()
{
#ifdef Q_OS_WIN
	return true;
#else
	return false;
#endif
}



int PrintBlocker::restoredStartType( const State& state )
{
	return state.previousStartType < 0 ? AutoStart : state.previousStartType;
}



QStringList PrintBlocker::servicesToRestart( const State& state )
{
	if( state.wasRunning == false )
	{
		return {};
	}
	return QStringList{ QStringLiteral("Spooler") } + state.stoppedDependents;
}



bool PrintBlocker::apply()
{
#ifdef Q_OS_WIN
	ServiceHandle manager( OpenSCManagerW( nullptr, nullptr, SC_MANAGER_CONNECT ) );
	if( manager == nullptr )
	{
		vWarning() << "could not open the service manager" << GetLastError();
		return false;
	}
	ServiceHandle spooler( OpenServiceW( manager, reinterpret_cast<LPCWSTR>( SpoolerService.utf16() ),
										 SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS |
											 SERVICE_ENUMERATE_DEPENDENTS ) );
	if( spooler == nullptr )
	{
		vWarning() << "could not open the print spooler service" << GetLastError();
		return false;
	}

	auto state = printBlockStateStore();
	const auto dependents = runningDependents( spooler );
	if( state.value( QStringLiteral("PrintBlock/Applied") ).toBool() == false )
	{
		state.setValue( QStringLiteral("PrintBlock/PreviousStartType"), startType( spooler ) );
		state.setValue( QStringLiteral("PrintBlock/WasRunning"), isRunning( spooler ) );
		state.setValue( QStringLiteral("PrintBlock/StoppedDependents"), dependents );
		state.setValue( QStringLiteral("PrintBlock/Applied"), true );
		state.sync();
	}

	setStartType( spooler, DisabledStart );

	for( const auto& dependent : dependents )
	{
		stopService( manager, dependent );
	}
	if( stopService( manager, SpoolerService ) == false )
	{
		vWarning() << "could not stop the print spooler";
		return false;
	}
	return true;
#else
	return false;
#endif
}



bool PrintBlocker::clear()
{
#ifdef Q_OS_WIN
	auto store = printBlockStateStore();
	if( store.value( QStringLiteral("PrintBlock/Applied") ).toBool() == false )
	{
		return true;
	}

	State state;
	state.applied = true;
	state.previousStartType = store.value( QStringLiteral("PrintBlock/PreviousStartType"), -1 ).toInt();
	state.wasRunning = store.value( QStringLiteral("PrintBlock/WasRunning") ).toBool();
	state.stoppedDependents = store.value( QStringLiteral("PrintBlock/StoppedDependents") ).toStringList();

	ServiceHandle manager( OpenSCManagerW( nullptr, nullptr, SC_MANAGER_CONNECT ) );
	if( manager == nullptr )
	{
		return false;
	}

	{
		ServiceHandle spooler( OpenServiceW( manager, reinterpret_cast<LPCWSTR>( SpoolerService.utf16() ),
											 SERVICE_CHANGE_CONFIG ) );
		if( spooler == nullptr || setStartType( spooler, restoredStartType( state ) ) == false )
		{
			vWarning() << "could not restore the start type of the print spooler" << GetLastError();
			return false;
		}
	}

	for( const auto& service : servicesToRestart( state ) )
	{
		startService( manager, service );
	}

	store.remove( QStringLiteral("PrintBlock") );
	store.sync();

	return true;
#else
	return true;
#endif
}
