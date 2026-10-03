/*
 * ReturnWorkFeaturePlugin.h - returns corrected work to the students
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

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "ReturnWorkDialog.h"

class ReturnWorkFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.ReturnWork")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit ReturnWorkFeaturePlugin( QObject* parent = nullptr );
	~ReturnWorkFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("8c3e5a71-4b2d-4f6e-9a1c-7d5b3e2f9a04") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("ReturnWork");
	}

	QString description() const override
	{
		return tr( "Return collected and corrected work to each student" );
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
							   const FeatureMessage& message ) override
	{
		Q_UNUSED(computerControlInterface)
		Q_UNUSED(message)
		return false;
	}

private:
	const Feature m_returnWorkFeature;
	const FeatureList m_features;

	QPointer<ReturnWorkDialog> m_dialog;

};
