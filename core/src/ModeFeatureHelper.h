/*
 * ModeFeatureHelper.h - helpers for Mode features that open a dialog first
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

#include <QAbstractButton>
#include <QPointer>
#include <QTimer>

#include "BuiltinFeatures.h"
#include "MonitoringMode.h"
#include "VeyonMasterInterface.h"

namespace ModeFeatureHelper
{

// The master switches to a Mode feature before its plugin shows a dialog. When the
// teacher cancels the dialog, click the toolbar's monitoring mode button so that the
// master leaves the mode again (its buttons are named after their features).
inline void returnToMonitoringMode( VeyonMasterInterface& master )
{
	QPointer<QWidget> mainWindow( master.mainWindow() );
	if( mainWindow.isNull() )
	{
		return;
	}

	// the master sets the new mode after startFeature() returns, so wait for that
	QTimer::singleShot( 0, mainWindow, [mainWindow]() {
		if( mainWindow )
		{
			const auto button = mainWindow->findChild<QAbstractButton*>(
				VeyonCore::builtinFeatures().monitoringMode().feature().name() );
			if( button )
			{
				button->click();
			}
		}
	} );
}

}
