/*
 * LabSetupPlugin.h - plugin with the lab setup page of the configurator
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

#include "ConfigurationPagePluginInterface.h"

class LabSetupPlugin : public QObject, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.LabSetup")
	Q_INTERFACES(PluginInterface ConfigurationPagePluginInterface)
public:
	explicit LabSetupPlugin( QObject* parent = nullptr ) :
		QObject( parent )
	{
	}

	~LabSetupPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("5d2b9e64-7a1f-4c38-8e05-3f6a9c1d7b42") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("LabSetup");
	}

	QString description() const override
	{
		return tr( "Prepare the installation of the student computers" );
	}

	QString vendor() const override
	{
		return VeyonCore::productName();
	}

	QString copyright() const override
	{
		return tr( "%1 contributors" ).arg( VeyonCore::productName() );
	}

	ConfigurationPage* createConfigurationPage() override;

};
