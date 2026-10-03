/*
 * InventoryFeaturePlugin.cpp - hardware and software inventory of the student computers
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

#include "InventoryFeaturePlugin.h"
#include "SystemInventory.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


InventoryFeaturePlugin::InventoryFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_inventoryFeature( QStringLiteral("Inventory"),
						Feature::Flag::Action | Feature::Flag::AllComponents,
						Feature::Uid( "e3a95c21-0f4d-4b7e-9d68-5a1c3e7b2f80" ),
						Feature::Uid(),
						tr( "Inventory" ), {},
						tr( "Show the operating system, hardware, network addresses and installed version of "
							"the selected computers." ),
						QStringLiteral(":/inventory/inventory.png") ),
	m_features( { m_inventoryFeature } )
{
}



bool InventoryFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( featureUid != m_inventoryFeature.uid() || operation != Operation::Start )
	{
		return false;
	}

	sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Query }, computerControlInterfaces );
	return true;
}



bool InventoryFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_inventoryFeature.uid() )
	{
		return false;
	}

	if( m_window == nullptr )
	{
		m_window = new InventoryWindow( master.mainWindow() );
		connect( m_window, &InventoryWindow::refreshRequested, this, [this]() {
			controlFeature( m_inventoryFeature.uid(), Operation::Start, {}, computers() );
		} );
	}

	m_window->clear();
	m_computers.clear();
	for( const auto& computer : computerControlInterfaces )
	{
		addComputer( computer );
	}
	m_window->show();
	m_window->raise();

	return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
}



bool InventoryFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
												   const FeatureMessage& message )
{
	if( message.featureUid() != m_inventoryFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::Info )
	{
		return false;
	}

	if( m_window )
	{
		addComputer( computerControlInterface );
		m_window->setInventory( computerControlInterface->computer().hostName(),
								message.argument( Argument::Inventory ).toMap() );
	}

	return true;
}



bool InventoryFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
												   const MessageContext& messageContext,
												   const FeatureMessage& message )
{
	if( message.featureUid() != m_inventoryFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::Query )
	{
		return false;
	}

	server.sendFeatureMessageReply( messageContext,
									FeatureMessage{ m_inventoryFeature.uid(), FeatureCommand::Info }
										.addArgument( Argument::Inventory, SystemInventory::collect() ) );
	return true;
}



void InventoryFeaturePlugin::addComputer( const ComputerControlInterface::Pointer& computerControlInterface )
{
	const auto key = computerControlInterface->computer().hostName();
	if( key.isEmpty() || m_window == nullptr )
	{
		return;
	}

	m_computers[key] = computerControlInterface.toWeakRef();

	const auto user = computerControlInterface->userFullName().isEmpty() ? computerControlInterface->userLoginName()
																		  : computerControlInterface->userFullName();
	m_window->setComputer( key, computerControlInterface->computerName(), VeyonCore::stripDomain( user ) );
}



ComputerControlInterfaceList InventoryFeaturePlugin::computers() const
{
	ComputerControlInterfaceList computers;
	for( const auto& weakComputer : m_computers )
	{
		if( const auto computer = weakComputer.toStrongRef() )
		{
			computers.append( computer );
		}
	}
	return computers;
}
