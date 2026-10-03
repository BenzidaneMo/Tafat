/*
 * AppControlFeaturePlugin.cpp - block or allow applications on student computers
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

#include <QDateTime>
#include <QMessageBox>

#include "AppControlFeaturePlugin.h"
#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformSessionFunctions.h"
#include "PlatformUserFunctions.h"
#include "ProcessControl.h"
#include "PrintBlocker.h"
#include "UsbStorageBlocker.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


AppControlFeaturePlugin::AppControlFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_appControlFeature( QStringLiteral("AppControl"),
						 Feature::Flag::Mode | Feature::Flag::AllComponents,
						 Feature::Uid( "4b4f6a4b-f9c0-494e-9dce-8de93a384e30" ),
						 Feature::Uid(),
						 tr( "Block apps" ), tr( "Unblock apps" ),
						 tr( "Block applications on the selected computers, or allow only "
							 "the applications needed for the lesson. Blocked applications "
							 "are closed automatically while this mode is active." ),
						 QStringLiteral(":/appcontrol/application-control.png") ),
	m_runningAppsFeature( QStringLiteral("RunningApps"),
						  Feature::Flag::Action | Feature::Flag::AllComponents,
						  Feature::Uid( "9e1c4b7a-3d2f-4a86-b5c9-0f7e2d6a1b38" ),
						  Feature::Uid(),
						  tr( "Running apps" ), {},
						  tr( "See which applications are open on the selected computers and close them." ),
						  QStringLiteral(":/appcontrol/running-apps.png") ),
	m_features( { m_appControlFeature, m_runningAppsFeature } )
{
	m_enforcementTimer.setInterval( EnforcementInterval );

	if( VeyonCore::component() == VeyonCore::Component::Server )
	{
		// restore the storage policy and the print service e.g. after a crash or power loss
		connect( VeyonCore::instance(), &VeyonCore::initialized, this, []() {
			UsbStorageBlocker::clear();
			PrintBlocker::clear();
		} );

		// remember which applications the student used during the session
		m_historyTimer.setInterval( HistoryInterval );
		connect( &m_historyTimer, &QTimer::timeout, this, &AppControlFeaturePlugin::updateHistory );
		m_historyTimer.start();
	}
}



bool AppControlFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid == m_runningAppsFeature.uid() )
	{
		if( operation != Operation::Start )
		{
			return false;
		}
		const auto application = arguments.value( argToString( Argument::Applications ) ).toStringList().value( 0 );
		auto targets = computerControlInterfaces;
		targets.removeLocalHostInterfaces();
		sendFeatureMessage( application.isEmpty()
								? FeatureMessage{ featureUid, FeatureCommand::QueryApplications }
								: FeatureMessage{ featureUid, FeatureCommand::CloseApplication }
									  .addArgument( Argument::Applications, QStringList{ application } ),
							targets );
		return true;
	}

	if( featureUid != m_appControlFeature.uid() )
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
								.addArgument( Argument::Applications, arguments.value( argToString( Argument::Applications ) ).toStringList() )
								.addArgument( Argument::BlockUsbStorage, arguments.value( argToString( Argument::BlockUsbStorage ) ).toBool() )
								.addArgument( Argument::BlockPrinting, arguments.value( argToString( Argument::BlockPrinting ) ).toBool() ),
							targets );
		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Stop }, computerControlInterfaces );
		return true;
	}

	return false;
}



bool AppControlFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() == m_runningAppsFeature.uid() )
	{
		if( m_runningAppsWindow == nullptr )
		{
			m_runningAppsWindow = new RunningAppsWindow( master.mainWindow() );
			connect( m_runningAppsWindow, &RunningAppsWindow::refreshRequested, this, [this]() {
				controlFeature( m_runningAppsFeature.uid(), Operation::Start, {}, runningAppsComputers() );
			} );
			connect( m_runningAppsWindow, &RunningAppsWindow::closeRequested, this,
					 [this]( const QString& key, const QString& application ) {
				if( const auto computer = m_runningAppsComputers.value( key ).toStrongRef() )
				{
					controlFeature( m_runningAppsFeature.uid(), Operation::Start,
									{ { argToString( Argument::Applications ), QStringList{ application } } }, { computer } );
				}
			} );
			connect( m_runningAppsWindow, &RunningAppsWindow::closeEverywhereRequested, this,
					 [this]( const QString& application ) {
				controlFeature( m_runningAppsFeature.uid(), Operation::Start,
								{ { argToString( Argument::Applications ), QStringList{ application } } },
								runningAppsComputers() );
			} );
		}

		m_runningAppsWindow->clear();
		m_runningAppsComputers.clear();
		for( const auto& computer : computerControlInterfaces )
		{
			addRunningAppsComputer( computer );
		}
		m_runningAppsWindow->show();
		m_runningAppsWindow->raise();

		return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	if( feature.uid() != m_appControlFeature.uid() )
	{
		return false;
	}

	AppControlDialog dialog( master.mainWindow() );
	if( dialog.exec() == QDialog::Accepted )
	{
		controlFeature( feature.uid(), Operation::Start,
						{ { argToString( Argument::Mode ), int( dialog.mode() ) },
						  { argToString( Argument::Applications ), dialog.applications() },
						  { argToString( Argument::BlockUsbStorage ), dialog.blockUsbStorage() },
						  { argToString( Argument::BlockPrinting ), dialog.blockPrinting() } },
						computerControlInterfaces );
	}

	return true;
}



bool AppControlFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
													const FeatureMessage& message )
{
	if( message.featureUid() != m_runningAppsFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::ApplicationList )
	{
		return false;
	}

	if( m_runningAppsWindow )
	{
		addRunningAppsComputer( computerControlInterface );
		m_runningAppsWindow->setApplications( computerControlInterface->computer().hostName(),
											   message.argument( Argument::Applications ).toStringList(),
											   AppHistory::fromVariant( message.argument( Argument::History ).toList() ) );
	}

	return true;
}



void AppControlFeaturePlugin::addRunningAppsComputer( const ComputerControlInterface::Pointer& computerControlInterface )
{
	const auto key = computerControlInterface->computer().hostName();
	if( key.isEmpty() || m_runningAppsWindow == nullptr )
	{
		return;
	}

	m_runningAppsComputers[key] = computerControlInterface.toWeakRef();

	const auto user = computerControlInterface->userFullName().isEmpty() ? computerControlInterface->userLoginName()
																		  : computerControlInterface->userFullName();
	m_runningAppsWindow->setComputer( key, user.isEmpty() ? computerControlInterface->computerName()
														  : QStringLiteral("\u2068%1\u2069 \u2013 \u2068%2\u2069").arg( VeyonCore::stripDomain( user ),
																							   computerControlInterface->computerName() ) );
}



ComputerControlInterfaceList AppControlFeaturePlugin::runningAppsComputers() const
{
	ComputerControlInterfaceList computers;
	for( const auto& weakComputer : m_runningAppsComputers )
	{
		if( const auto computer = weakComputer.toStrongRef() )
		{
			computers.append( computer );
		}
	}
	return computers;
}



bool AppControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message )
{
	if( message.featureUid() == m_runningAppsFeature.uid() )
	{
		const auto command = message.command<FeatureCommand>();
		if( command != FeatureCommand::QueryApplications && command != FeatureCommand::CloseApplication )
		{
			return false;
		}

		if( command == FeatureCommand::CloseApplication &&
			VeyonCore::platform().sessionFunctions().currentSessionHasUser() )
		{
			const auto application = ProcessControl::normalizedName(
				message.argument( Argument::Applications ).toStringList().value( 0 ) );
			for( const auto& process : ProcessControl::sessionProcesses() )
			{
				if( process.name == application && ProcessControl::isProtected( process.name ) == false )
				{
					ProcessControl::terminate( process.id );
				}
			}
			vInfo() << "closed application" << application << "on request of the teacher";
		}

		const auto applications = updateHistory();
		server.sendFeatureMessageReply( messageContext,
										FeatureMessage{ m_runningAppsFeature.uid(), FeatureCommand::ApplicationList }
											.addArgument( Argument::Applications, applications )
											.addArgument( Argument::History, AppHistory::toVariant( m_history.entries() ) ) );
		return true;
	}

	if( message.featureUid() != m_appControlFeature.uid() )
	{
		return false;
	}

	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::Start:
		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false )
		{
			vDebug() << "not controlling applications since not running in a user session";
			return true;
		}

		m_mode = AppControlDialog::Mode( message.argument( Argument::Mode ).toInt() );
		m_applications.clear();
		for( const auto& application : message.argument( Argument::Applications ).toStringList() )
		{
			m_applications.append( ProcessControl::normalizedName( application ) );
		}

		if( message.argument( Argument::BlockUsbStorage ).toBool() && UsbStorageBlocker::isSupported() )
		{
			UsbStorageBlocker::apply();
		}
		else
		{
			UsbStorageBlocker::clear();
		}

		if( message.argument( Argument::BlockPrinting ).toBool() && PrintBlocker::isSupported() )
		{
			PrintBlocker::apply();
		}
		else
		{
			PrintBlocker::clear();
		}

		m_enforcementTimer.disconnect( this );
		connect( &m_enforcementTimer, &QTimer::timeout, this, [this, &server]() { enforce( server ); } );
		m_enforcementTimer.start();
		enforce( server );

		vInfo() << "controlling applications, mode" << int(m_mode) << "applications" << m_applications;
		return true;

	case FeatureCommand::Stop:
		m_enforcementTimer.stop();
		m_applications.clear();
		UsbStorageBlocker::clear();
		PrintBlocker::clear();
		vInfo() << "stopped controlling applications";
		return true;

	default:
		break;
	}

	return false;
}



bool AppControlFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() != m_appControlFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::NotifyClosedApplications )
	{
		return false;
	}

	const auto text = tr( "Your teacher has blocked the following applications, so they were closed:\n\n%1" )
						  .arg( message.argument( Argument::Applications ).toStringList().join( QStringLiteral(", ") ) );

	if( m_notice )
	{
		m_notice->setText( text );
	}
	else
	{
		m_notice = new QMessageBox( QMessageBox::Information, tr( "Application blocked" ), text );
		m_notice->setAttribute( Qt::WA_DeleteOnClose );
		m_notice->show();
	}

	VeyonCore::platform().coreFunctions().raiseWindow( m_notice, true );

	return true;
}



bool AppControlFeaturePlugin::isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const
{
	Q_UNUSED(server)

	return featureUid == m_appControlFeature.uid() && m_enforcementTimer.isActive();
}



void AppControlFeaturePlugin::enforce( VeyonServerInterface& server )
{
	QStringList closedApplications;

	const auto processes = ProcessControl::processesToClose( ProcessControl::sessionProcesses(), m_mode, m_applications );
	for( const auto& process : processes )
	{
		if( ProcessControl::terminate( process.id ) && closedApplications.contains( process.name ) == false )
		{
			closedApplications.append( process.name );
		}
	}

	if( closedApplications.isEmpty() == false )
	{
		vInfo() << "closed applications" << closedApplications;
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(
			FeatureMessage{ m_appControlFeature.uid(), FeatureCommand::NotifyClosedApplications }
				.addArgument( Argument::Applications, closedApplications ) );
	}
}



QStringList AppControlFeaturePlugin::updateHistory()
{
	if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false )
	{
		m_history.clear();
		m_historyUser.clear();
		return {};
	}

	// a new user starts with an empty history
	const auto user = VeyonCore::platform().userFunctions().queryCurrentUserProperty( PlatformUserFunctions::UserProperty::LoginName );
	if( user != m_historyUser )
	{
		m_history.clear();
		m_historyUser = user;
	}

	const auto applications = ProcessControl::openApplications( ProcessControl::sessionProcesses() );
	m_history.update( applications, QDateTime::currentMSecsSinceEpoch() );
	return applications;
}
