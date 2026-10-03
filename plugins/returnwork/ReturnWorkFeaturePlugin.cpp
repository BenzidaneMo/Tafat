/*
 * ReturnWorkFeaturePlugin.cpp - returns corrected work to the students
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

#include "ReturnWorkFeaturePlugin.h"
#include "VeyonMasterInterface.h"


ReturnWorkFeaturePlugin::ReturnWorkFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_returnWorkFeature( QStringLiteral("ReturnWork"),
						 Feature::Flag::Action | Feature::Flag::Master,
						 Feature::Uid( "2f6d9b4e-8a13-4c7f-b5e2-1a9c3d7e6f85" ),
						 Feature::Uid(),
						 tr( "Return work" ), {},
						 tr( "Give each student back the files of their own folder of collected files, "
							 "for example after you corrected them." ),
						 QStringLiteral(":/returnwork/return-work.png") ),
	m_features( { m_returnWorkFeature } )
{
}



ReturnWorkFeaturePlugin::~ReturnWorkFeaturePlugin()
{
	delete m_dialog;
}



bool ReturnWorkFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(featureUid)
	Q_UNUSED(operation)
	Q_UNUSED(arguments)
	Q_UNUSED(computerControlInterfaces)

	// interactive only: the files of each student are chosen in the dialog
	return false;
}



bool ReturnWorkFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_returnWorkFeature.uid() )
	{
		return false;
	}

	if( m_dialog == nullptr )
	{
		m_dialog = new ReturnWorkDialog( master.mainWindow() );
	}

	m_dialog->setComputers( computerControlInterfaces );
	m_dialog->show();
	m_dialog->raise();
	m_dialog->activateWindow();

	return true;
}
