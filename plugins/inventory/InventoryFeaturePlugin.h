/*
 * InventoryFeaturePlugin.h - hardware and software inventory of the student computers
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

#include <QPointer>

#include "ComputerControlInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "InventoryWindow.h"

class InventoryFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.Inventory")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		Inventory
	};
	Q_ENUM(Argument)

	explicit InventoryFeaturePlugin( QObject* parent = nullptr );
	~InventoryFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("b7f1d3a8-62c4-4e9b-a05d-8c2e7f4b1d69") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Inventory");
	}

	QString description() const override
	{
		return tr( "Hardware and software inventory of the student computers" );
	}

	QString vendor() const override
	{
		return VeyonCore::productName();
	}

	QString copyright() const override
	{
		return tr( "%1 contributors" ).arg( VeyonCore::productName() );
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	enum class FeatureCommand
	{
		Query,
		Info
	};

	void addComputer( const ComputerControlInterface::Pointer& computerControlInterface );
	ComputerControlInterfaceList computers() const;

	const Feature m_inventoryFeature;
	const FeatureList m_features;

	QPointer<InventoryWindow> m_window;
	QMap<QString, QPointer<ComputerControlInterface>> m_computers;

};
