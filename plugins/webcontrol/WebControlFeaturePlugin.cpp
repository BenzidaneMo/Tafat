/*
 * WebControlFeaturePlugin.cpp - block or allow websites on student computers
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

#include "ComputerControlInterface.h"
#include "InternetBlocker.h"
#include "ModeFeatureHelper.h"
#include "PolicyStore.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "WebControlDialog.h"
#include "WebControlFeaturePlugin.h"


WebControlFeaturePlugin::WebControlFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_webControlFeature( QStringLiteral("WebControl"),
						 Feature::Flag::Mode | Feature::Flag::AllComponents,
						 Feature::Uid( "e89b4196-5150-4a43-aed3-a5140da25e6e" ),
						 Feature::Uid(),
						 tr( "Block websites" ), tr( "Unblock websites" ),
						 tr( "Block websites on the selected computers, or allow only the "
							 "websites needed for the lesson, in Chrome, Edge, Brave, "
							 "Chromium and Firefox." ),
						 QStringLiteral(":/webcontrol/website-control.png") ),
	m_features( { m_webControlFeature } )
{
	if( VeyonCore::component() == VeyonCore::Component::Server )
	{
		// remove policies left over e.g. after a crash or power loss
		connect( VeyonCore::instance(), &VeyonCore::initialized, this, []() {
			PolicyStore::clear();
			InternetBlocker::clear();
		} );
	}
}



bool WebControlFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid != m_webControlFeature.uid() )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		// never restrict the teacher's own computer
		auto targets = computerControlInterfaces;
		targets.removeLocalHostInterfaces();

		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Start }
								.addArgument( Argument::Mode, arguments.value( argToString( Argument::Mode ) ).toInt() )
								.addArgument( Argument::Sites, arguments.value( argToString( Argument::Sites ) ).toStringList() )
								.addArgument( Argument::BlockInternet, arguments.value( argToString( Argument::BlockInternet ) ).toBool() ),
							targets );
		return true;
	}

	if( operation == Operation::Stop )
	{
		m_masterModeActive = false;
		m_masterModeCancelled = false;
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Stop }, computerControlInterfaces );
		return true;
	}

	return false;
}



bool WebControlFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_webControlFeature.uid() )
	{
		return false;
	}

	if( m_masterModeActive )
	{
		// the master enforces the selected mode on a computer that (re)connected
		return controlFeature( feature.uid(), Operation::Start, m_masterStartArguments, computerControlInterfaces );
	}
	if( m_masterModeCancelled )
	{
		// the dialog was cancelled and the master is going back to monitoring mode
		return true;
	}

	WebControlDialog dialog( master.mainWindow() );
	if( dialog.exec() != QDialog::Accepted )
	{
		m_masterModeCancelled = true;
		ModeFeatureHelper::returnToMonitoringMode( master );
		return true;
	}

	m_masterStartArguments = { { argToString( Argument::Mode ), int( dialog.mode() ) },
							   { argToString( Argument::Sites ), dialog.sites() },
							   { argToString( Argument::BlockInternet ), dialog.blockInternet() } };
	m_masterModeActive = true;
	controlFeature( feature.uid(), Operation::Start, m_masterStartArguments, computerControlInterfaces );

	return true;
}



bool WebControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_webControlFeature.uid() )
	{
		return false;
	}

	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::Start:
	{
		const auto mode = WebPolicy::Mode( message.argument( Argument::Mode ).toInt() );
		QStringList sites;
		for( const auto& site : message.argument( Argument::Sites ).toStringList() )
		{
			const auto normalized = WebPolicy::normalizedSite( site );
			if( normalized.isEmpty() == false )
			{
				sites.append( normalized );
			}
		}

		if( PolicyStore::apply( WebPolicy::chromiumPolicies( mode, sites ), WebPolicy::firefoxPolicies( mode, sites ) ) == false )
		{
			vWarning() << "could not apply all website policies";
		}

		const auto blockInternet = message.argument( Argument::BlockInternet ).toBool();
		if( blockInternet && InternetBlocker::isSupported() )
		{
			InternetBlocker::apply();
		}
		else
		{
			InternetBlocker::clear();
		}

		m_active = true;
		vInfo() << "controlling websites, mode" << int(mode) << "sites" << sites << "block internet" << blockInternet;
		return true;
	}

	case FeatureCommand::Stop:
		PolicyStore::clear();
		InternetBlocker::clear();
		m_active = false;
		vInfo() << "stopped controlling websites";
		return true;
	}

	return false;
}



bool WebControlFeaturePlugin::isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const
{
	Q_UNUSED(server)

	return featureUid == m_webControlFeature.uid() && m_active;
}
