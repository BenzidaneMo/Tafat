/*
 * RegisterFeaturePlugin.cpp - attendance register with the names of the students
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

#include <QTimer>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "MessageContext.h"
#include "PlatformCoreFunctions.h"
#include "RegisterFeaturePlugin.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


static const char* RegistrationVersionProperty = "registerVersion";
static constexpr int WorkerStopDelay = 1000;



RegisterFeaturePlugin::RegisterFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_registerFeature( QStringLiteral("StudentRegister"),
					   Feature::Flag::Action | Feature::Flag::AllComponents,
					   Feature::Uid( "c8356d73-0bd8-4c32-aa19-ecdda8a86493" ),
					   Feature::Uid(),
					   tr( "Register" ), {},
					   tr( "Ask the students for their names to take the attendance and to show "
						   "their names on the computers, also when they share one user account." ),
					   QStringLiteral(":/register/register.png") ),
	m_features( { m_registerFeature } )
{
}



RegisterFeaturePlugin::~RegisterFeaturePlugin()
{
	delete m_dialog;
}



bool RegisterFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											const QVariantMap& arguments,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( featureUid != m_registerFeature.uid() || operation != Operation::Start )
	{
		return false;
	}

	auto targets = computerControlInterfaces;
	targets.removeLocalHostInterfaces();

	sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::AskRegistration }, targets );

	return true;
}



bool RegisterFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_registerFeature.uid() )
	{
		return false;
	}

	m_lastComputers = computerControlInterfaces;
	m_lastComputers.removeLocalHostInterfaces();

	if( m_registerWindow == nullptr )
	{
		m_registerWindow = new RegisterWindow( master.mainWindow() );
		connect( m_registerWindow, &RegisterWindow::askAgainRequested, this, [this]() {
			controlFeature( m_registerFeature.uid(), Operation::Start, {}, m_lastComputers );
		} );
	}

	for( const auto& controlInterface : std::as_const( m_lastComputers ) )
	{
		const auto key = controlInterface->computer().hostName();
		m_registerWindow->addComputer( key, controlInterface->computerName() );
		if( m_registrations.contains( key ) )
		{
			m_registerWindow->setRegistration( key, m_registrations[key] );
		}
	}

	m_registerWindow->show();
	m_registerWindow->raise();

	return controlFeature( feature.uid(), Operation::Start, {}, m_lastComputers );
}



bool RegisterFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
												  const FeatureMessage& message )
{
	if( message.featureUid() != m_registerFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::Registered )
	{
		return false;
	}

	const auto key = computerControlInterface->computer().hostName();
	const Registration registration{ computerControlInterface->computerName(),
									 message.argument( Argument::StudentName ).toString(),
									 message.argument( Argument::Group ).toString(),
									 QDateTime::currentDateTime() };
	if( registration.studentName.isEmpty() )
	{
		return true;
	}

	m_registrations[key] = registration;

	if( m_watchedComputers.contains( key ) == false )
	{
		m_watchedComputers.insert( key );
		// keep showing the registered name when the monitoring updates the user information
		const auto weakInterface = computerControlInterface.toWeakRef();
		connect( computerControlInterface.data(), &ComputerControlInterface::userChanged, this, [this, weakInterface]() {
			if( const auto controlInterface = weakInterface.toStrongRef() )
			{
				applyStudentName( controlInterface );
			}
		} );
	}

	applyStudentName( computerControlInterface );

	if( m_registerWindow )
	{
		m_registerWindow->setRegistration( key, registration );
	}

	return true;
}



void RegisterFeaturePlugin::applyStudentName( const ComputerControlInterface::Pointer& computerControlInterface )
{
	const auto key = computerControlInterface->computer().hostName();
	if( m_registrations.contains( key ) == false )
	{
		return;
	}

	// the user logged off: the registration belonged to that session
	if( computerControlInterface->userLoginName().isEmpty() )
	{
		m_registrations.remove( key );
		return;
	}

	const auto& studentName = m_registrations[key].studentName;
	if( computerControlInterface->userFullName() != studentName )
	{
		computerControlInterface->setUserInformation( computerControlInterface->userLoginName(), studentName );
	}
}



bool RegisterFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
												  const MessageContext& messageContext,
												  const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_registerFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::AskRegistration )
	{
		return false;
	}

	++m_registrationRequests;
	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

	return true;
}



bool RegisterFeaturePlugin::handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message )
{
	if( message.featureUid() != m_registerFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::Registered )
	{
		return false;
	}

	// the dialog has closed: stop the worker so that the computer no longer reports the feature as active
	auto& workerManager = server.featureWorkerManager();
	QTimer::singleShot( WorkerStopDelay, this, [this, &workerManager, requests = m_registrationRequests]() {
		if( requests == m_registrationRequests )
		{
			workerManager.stopWorker( m_registerFeature.uid() );
		}
	} );

	{
		QMutexLocker locker( &m_serverMutex );
		m_serverStudentName = message.argument( Argument::StudentName ).toString().simplified();
		m_serverGroup = message.argument( Argument::Group ).toString().simplified();
	}
	m_registrationVersion.ref();

	return true;
}



void RegisterFeaturePlugin::sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext )
{
	const auto version = m_registrationVersion.loadAcquire();
	if( version == 0 || messageContext.ioDevice() == nullptr ||
		messageContext.ioDevice()->property( RegistrationVersionProperty ).toInt() == version )
	{
		return;
	}

	FeatureMessage reply{ m_registerFeature.uid(), FeatureCommand::Registered };
	{
		QMutexLocker locker( &m_serverMutex );
		reply.addArgument( Argument::StudentName, m_serverStudentName )
			.addArgument( Argument::Group, m_serverGroup );
	}

	server.sendFeatureMessageReply( messageContext, reply );
	messageContext.ioDevice()->setProperty( RegistrationVersionProperty, version );
}



bool RegisterFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if( message.featureUid() != m_registerFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::AskRegistration )
	{
		return false;
	}

	if( m_dialog == nullptr )
	{
		m_dialog = new RegistrationDialog;
		m_dialog->setAttribute( Qt::WA_DeleteOnClose );
		connect( m_dialog, &QDialog::accepted, this, [this, &worker]() {
			worker.sendFeatureMessageReply( FeatureMessage{ m_registerFeature.uid(), FeatureCommand::Registered }
												.addArgument( Argument::StudentName, m_dialog->studentName() )
												.addArgument( Argument::Group, m_dialog->group() ) );
		} );
		m_dialog->show();
	}

	VeyonCore::platform().coreFunctions().raiseWindow( m_dialog, true );

	return true;
}
