/*
 * RegisterFeaturePlugin.h - attendance register with the names of the students
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

#include <QAtomicInt>
#include <QMutex>
#include <QPointer>
#include <QSet>

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "RegisterWindows.h"

class RegisterFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.Register")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		StudentName,
		Group
	};
	Q_ENUM(Argument)

	explicit RegisterFeaturePlugin( QObject* parent = nullptr );
	~RegisterFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("a6e9972d-0ea9-4892-93b5-be07dbee2d16") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Register");
	}

	QString description() const override
	{
		return tr( "Attendance register with the names of the students" );
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

	bool handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message ) override;

	void sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	enum class FeatureCommand
	{
		AskRegistration,
		Registered
	};

	void applyStudentName( const ComputerControlInterface::Pointer& computerControlInterface );

	const Feature m_registerFeature;
	const FeatureList m_features;

	// master side: registrations by host name
	QMap<QString, Registration> m_registrations;
	QSet<QString> m_watchedComputers;
	ComputerControlInterfaceList m_lastComputers;
	QPointer<RegisterWindow> m_registerWindow;

	// server side: the registration of the current session
	mutable QMutex m_serverMutex;
	QString m_serverStudentName;
	QString m_serverGroup;
	QAtomicInt m_registrationVersion{0};
	int m_registrationRequests{0};

	// worker side
	QPointer<RegistrationDialog> m_dialog;

};
