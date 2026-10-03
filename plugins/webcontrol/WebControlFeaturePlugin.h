/*
 * WebControlFeaturePlugin.h - block or allow websites on student computers
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

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "WebPolicy.h"

class WebControlFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.WebControl")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		Mode,
		Sites,
		BlockInternet
	};
	Q_ENUM(Argument)

	explicit WebControlFeaturePlugin( QObject* parent = nullptr );
	~WebControlFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("f9be72bc-0b03-4382-a799-69e01911bd2c") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("WebControl");
	}

	QString description() const override
	{
		return tr( "Block or allow websites on student computers" );
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

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const override;

private:
	enum class FeatureCommand
	{
		Start,
		Stop
	};

	const Feature m_webControlFeature;
	const FeatureList m_features;

	bool m_active{false};

};
