/*
 * LabSetupPlugin.h - lab setup page, teacher buttons and command line module
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

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"

class AddComputersDialog;
class StudentInstallerDialog;

// Everything for setting up a lab without commands: the "Lab setup" page of the
// configurator, the teacher's buttons "Create student installer", "Add
// computers" and "Settings", and the command line module used by the installer.
class LabSetupPlugin : public QObject,
		PluginInterface,
		ConfigurationPagePluginInterface,
		FeatureProviderInterface,
		CommandLinePluginInterface,
		CommandLineIO
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.LabSetup")
	Q_INTERFACES(PluginInterface ConfigurationPagePluginInterface FeatureProviderInterface CommandLinePluginInterface)
public:
	explicit LabSetupPlugin( QObject* parent = nullptr );
	~LabSetupPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("5d2b9e64-7a1f-4c38-8e05-3f6a9c1d7b42") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
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

	// configurator page
	ConfigurationPage* createConfigurationPage() override;

	// teacher's buttons
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

	// command line
	QString commandLineModuleName() const override
	{
		return QStringLiteral("labsetup");
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for setting up the teacher and student computers" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_setupteacher( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_createpackage( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_extractpackage( const QStringList& arguments );

private:
	static QString localUsersGroupName();
	int runCommandLine( const QStringList& arguments );

	const Feature m_studentInstallerFeature;
	const Feature m_addComputersFeature;
	const Feature m_settingsFeature;
	const FeatureList m_features;
	QMap<QString, QString> m_commands;

	QPointer<StudentInstallerDialog> m_studentInstallerDialog;
	QPointer<AddComputersDialog> m_addComputersDialog;

};
