/*
 * UsbStorageBlocker.cpp - blocks access to USB sticks and other removable storage
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

#include <QSettings>

#include "UsbStorageBlocker.h"
#include "VeyonCore.h"


#ifdef Q_OS_WIN
static const auto PolicyKey = QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\RemovableStorageDevices");
static const auto PolicyValue = QStringLiteral("Deny_All");

static QSettings stateStore()
{
	return QSettings( QSettings::SystemScope, VeyonCore::productName(), QStringLiteral("AppControl") );
}
#endif



bool UsbStorageBlocker::isSupported()
{
#ifdef Q_OS_WIN
	return true;
#else
	return false;
#endif
}



QVariant UsbStorageBlocker::restoredValue( const State& state )
{
	return state.hadValue ? state.previousValue : QVariant{};
}



bool UsbStorageBlocker::apply()
{
#ifdef Q_OS_WIN
	auto state = stateStore();
	// the policy is read from the 64-bit registry view, also by 32-bit processes
	QSettings policy( PolicyKey, QSettings::Registry64Format );

	if( state.value( QStringLiteral("UsbBlock/Applied") ).toBool() == false )
	{
		state.setValue( QStringLiteral("UsbBlock/HadValue"), policy.contains( PolicyValue ) );
		state.setValue( QStringLiteral("UsbBlock/PreviousValue"), policy.value( PolicyValue ) );
		state.setValue( QStringLiteral("UsbBlock/Applied"), true );
		state.sync();
	}

	policy.setValue( PolicyValue, 1 );
	policy.sync();

	if( policy.status() != QSettings::NoError )
	{
		vWarning() << "could not write the removable storage policy";
		return false;
	}
	return true;
#else
	return false;
#endif
}



bool UsbStorageBlocker::clear()
{
#ifdef Q_OS_WIN
	auto state = stateStore();
	if( state.value( QStringLiteral("UsbBlock/Applied") ).toBool() == false )
	{
		return true;
	}

	const auto value = restoredValue( { true, state.value( QStringLiteral("UsbBlock/HadValue") ).toBool(),
										state.value( QStringLiteral("UsbBlock/PreviousValue") ) } );

	QSettings policy( PolicyKey, QSettings::Registry64Format );
	if( value.isValid() )
	{
		policy.setValue( PolicyValue, value );
	}
	else
	{
		policy.remove( PolicyValue );
	}
	policy.sync();

	state.remove( QStringLiteral("UsbBlock") );
	state.sync();

	return policy.status() == QSettings::NoError;
#else
	return true;
#endif
}
