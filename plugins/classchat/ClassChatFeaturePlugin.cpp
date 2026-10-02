/*
 * ClassChatFeaturePlugin.cpp - hand raise and chat between teacher and students
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

#include <QJsonDocument>
#include <QTimer>

#include "ClassChatFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "MessageContext.h"
#include "PlatformCoreFunctions.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


static const char* VersionProperty = "classChatVersion";
static const char* MessageIdProperty = "classChatMessageId";
static const char* SessionIdProperty = "classChatSessionId";



ClassChatFeaturePlugin::ClassChatFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_chatFeature( QStringLiteral("ClassChat"),
				   Feature::Flag::Action | Feature::Flag::AllComponents,
				   Feature::Uid( "0b6c7e4e-5d55-4f3a-b5a4-1c2d9e5f7a10" ),
				   Feature::Uid(),
				   tr( "Hands & chat" ), {},
				   tr( "Let the students raise their hands and chat with you. Raised hands are "
					   "shown on the computers." ),
				   QStringLiteral(":/classchat/classchat.png") ),
	m_showToolbarFeature( QStringLiteral("ShowStudentToolbar"),
						  Feature::Flag::Action | Feature::Flag::Master,
						  Feature::Uid( "6a2a9c1e-61c6-4f7e-8f53-3a7f0b1e4c21" ),
						  m_chatFeature.uid(),
						  tr( "Show student toolbar" ), {},
						  tr( "Show a small toolbar on the selected computers so that the students can "
							  "raise their hands and write to you." ) ),
	m_hideToolbarFeature( QStringLiteral("HideStudentToolbar"),
						  Feature::Flag::Action | Feature::Flag::Master,
						  Feature::Uid( "b1d3f6a8-0e2c-4a57-9d1b-7c4e2f8a6b32" ),
						  m_chatFeature.uid(),
						  tr( "Hide student toolbar" ), {},
						  tr( "Remove the student toolbar and the chat from the selected computers." ) ),
	m_openChatFeature( QStringLiteral("OpenChatWindow"),
					   Feature::Flag::Action | Feature::Flag::Master,
					   Feature::Uid( "e7c5a2b9-3f1d-4b68-a0e4-9d2c6b1f8e43" ),
					   m_chatFeature.uid(),
					   tr( "Open chat window" ), {},
					   tr( "Open the window with the raised hands and the conversations with the "
						   "selected students." ) ),
	m_handRaisedFeature( QStringLiteral("HandRaised"),
						 Feature::Flag::Meta | Feature::Flag::Master | Feature::Flag::Service,
						 Feature::Uid( "4c8e1f2a-7b3d-4e9a-b6c5-2d1f0e9a8b54" ),
						 Feature::Uid(),
						 tr( "Hand raised" ), {},
						 tr( "The student raised the hand." ),
						 QStringLiteral(":/classchat/hand-raised.png") ),
	m_features( { m_chatFeature, m_showToolbarFeature, m_hideToolbarFeature, m_openChatFeature, m_handRaisedFeature } )
{
}



ClassChatFeaturePlugin::~ClassChatFeaturePlugin()
{
	delete m_toolbar;
	delete m_studentChatWindow;
}



QString ClassChatFeaturePlugin::toJsonString( const QJsonArray& array )
{
	return QString::fromUtf8( QJsonDocument( array ).toJson( QJsonDocument::Compact ) );
}



QString ClassChatFeaturePlugin::toJsonString( const QJsonObject& object )
{
	return QString::fromUtf8( QJsonDocument( object ).toJson( QJsonDocument::Compact ) );
}



bool ClassChatFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	auto targets = computerControlInterfaces;
	targets.removeLocalHostInterfaces();

	const auto text = arguments.value( argToString( Argument::Text ) ).toString();

	if( featureUid == m_chatFeature.uid() || featureUid == m_showToolbarFeature.uid() )
	{
		if( operation == Operation::Start )
		{
			if( featureUid == m_chatFeature.uid() && text.isEmpty() == false )
			{
				sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::TeacherMessage }
										.addArgument( Argument::Text, ChatLog::cleanText( text ) ), targets );
			}
			else
			{
				sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::ShowToolbar }, targets );
			}
			return true;
		}
		if( operation == Operation::Stop )
		{
			sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::HideToolbar }, targets );
			return true;
		}
		return false;
	}

	if( featureUid == m_hideToolbarFeature.uid() && operation == Operation::Start )
	{
		sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::HideToolbar }, targets );
		return true;
	}

	if( featureUid == m_handRaisedFeature.uid() && operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::LowerHand }, targets );
		return true;
	}

	return false;
}



bool ClassChatFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces )
{
	m_mainWindow = master.mainWindow();

	if( feature.uid() == m_showToolbarFeature.uid() || feature.uid() == m_chatFeature.uid() )
	{
		chatWindow( &master );
		addComputers( computerControlInterfaces );
		return controlFeature( m_showToolbarFeature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	if( feature.uid() == m_hideToolbarFeature.uid() )
	{
		return controlFeature( m_hideToolbarFeature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	if( feature.uid() == m_openChatFeature.uid() )
	{
		auto window = chatWindow( &master );
		addComputers( computerControlInterfaces );
		window->show();
		window->raise();
		window->activateWindow();
		return true;
	}

	return false;
}



bool ClassChatFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
												   const FeatureMessage& message )
{
	if( message.featureUid() != m_chatFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::Update )
	{
		return false;
	}

	const auto key = computerControlInterface->computer().hostName();
	auto window = chatWindow( nullptr );
	addComputer( computerControlInterface );

	const auto sessionId = message.argument( Argument::SessionId ).toString();
	if( m_masterSessionIds.value( key ) != sessionId )
	{
		// the student's computer restarted the session: start a new conversation
		m_masterSessionIds[key] = sessionId;
		m_masterLogs[key] = {};
		window->clearConversation( key );
	}

	auto& log = m_masterLogs[key];
	ChatMessageList newMessages;
	const auto messages = ChatLog::fromJson( QJsonDocument::fromJson( message.argument( Argument::Messages ).toString().toUtf8() ).array() );
	for( const auto& chatMessage : messages )
	{
		if( log.add( chatMessage ) )
		{
			newMessages.append( chatMessage );
		}
	}

	window->addMessages( key, newMessages );
	window->setHandRaised( key, message.argument( Argument::HandRaised ).toBool() );

	return true;
}



bool ClassChatFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
												   const MessageContext& messageContext,
												   const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	auto& workerManager = server.featureWorkerManager();

	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::ShowToolbar:
	{
		QMutexLocker locker( &m_serverMutex );
		m_serverToolbarActive = true;
		break;
	}
	case FeatureCommand::HideToolbar:
	{
		{
			QMutexLocker locker( &m_serverMutex );
			m_serverToolbarActive = false;
			m_serverHandRaised = false;
		}
		m_serverVersion.ref();

		if( workerManager.isWorkerRunning( m_chatFeature.uid() ) )
		{
			workerManager.sendMessageToUnmanagedSessionWorker( message );
			// give the worker the time to close its windows
			QTimer::singleShot( 1000, this, [this, &workerManager]() {
				QMutexLocker locker( &m_serverMutex );
				if( m_serverToolbarActive == false )
				{
					workerManager.stopWorker( m_chatFeature.uid() );
				}
			} );
		}
		return true;
	}
	case FeatureCommand::TeacherMessage:
	{
		ChatMessage chatMessage;
		{
			QMutexLocker locker( &m_serverMutex );
			chatMessage = m_serverLog.append( true, message.argument( Argument::Text ).toString() );
		}
		if( chatMessage.id == 0 )
		{
			return true;
		}
		m_serverVersion.ref();

		workerManager.sendMessageToUnmanagedSessionWorker(
			FeatureMessage{ m_chatFeature.uid(), FeatureCommand::TeacherMessage }
				.addArgument( Argument::Message, toJsonString( chatMessage.toJson() ) ) );
		return true;
	}
	case FeatureCommand::LowerHand:
	{
		{
			QMutexLocker locker( &m_serverMutex );
			m_serverHandRaised = false;
		}
		m_serverVersion.ref();
		if( workerManager.isWorkerRunning( m_chatFeature.uid() ) == false )
		{
			return true;
		}
		break;
	}
	default:
		return false;
	}

	workerManager.sendMessageToUnmanagedSessionWorker( message );

	return true;
}



bool ClassChatFeaturePlugin::handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message )
{
	Q_UNUSED(server)

	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	if( message.command<FeatureCommand>() == FeatureCommand::HandState )
	{
		QMutexLocker locker( &m_serverMutex );
		m_serverHandRaised = message.argument( Argument::HandRaised ).toBool();
	}
	else if( message.command<FeatureCommand>() == FeatureCommand::StudentMessage )
	{
		QMutexLocker locker( &m_serverMutex );
		if( m_serverLog.append( false, message.argument( Argument::Text ).toString() ).id == 0 )
		{
			return true;
		}
	}
	else
	{
		return false;
	}

	m_serverVersion.ref();

	return true;
}



void ClassChatFeaturePlugin::sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext )
{
	const auto version = m_serverVersion.loadAcquire();
	auto ioDevice = messageContext.ioDevice();
	if( version == 0 || ioDevice == nullptr ||
		ioDevice->property( VersionProperty ).toInt() == version )
	{
		return;
	}

	// a connection only gets the messages it did not get yet
	auto lastMessageId = ioDevice->property( MessageIdProperty ).toLongLong();
	if( ioDevice->property( SessionIdProperty ).toString() != m_serverSessionId )
	{
		lastMessageId = 0;
	}

	FeatureMessage reply{ m_chatFeature.uid(), FeatureCommand::Update };
	{
		QMutexLocker locker( &m_serverMutex );
		reply.addArgument( Argument::SessionId, m_serverSessionId )
			.addArgument( Argument::HandRaised, m_serverHandRaised )
			.addArgument( Argument::Messages, toJsonString( ChatLog::toJson( m_serverLog.messagesAfter( lastMessageId ) ) ) );
		lastMessageId = m_serverLog.lastId();
	}

	server.sendFeatureMessageReply( messageContext, reply );

	ioDevice->setProperty( VersionProperty, version );
	ioDevice->setProperty( MessageIdProperty, lastMessageId );
	ioDevice->setProperty( SessionIdProperty, m_serverSessionId );
}



bool ClassChatFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::ShowToolbar:
		if( m_toolbar == nullptr )
		{
			m_toolbar = new StudentToolbar;
			connect( m_toolbar, &StudentToolbar::handRaisedChanged, this, [this, &worker]( bool raised ) {
				worker.sendFeatureMessageReply( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::HandState }
													.addArgument( Argument::HandRaised, raised ) );
			} );
			connect( m_toolbar, &StudentToolbar::chatRequested, this, [this, &worker]() {
				auto window = studentChatWindow( worker );
				window->show();
				VeyonCore::platform().coreFunctions().raiseWindow( window, true );
			} );
		}
		m_toolbar->show();
		VeyonCore::platform().coreFunctions().raiseWindow( m_toolbar, true );
		return true;

	case FeatureCommand::HideToolbar:
		delete m_toolbar;
		delete m_studentChatWindow;
		return true;

	case FeatureCommand::LowerHand:
		if( m_toolbar )
		{
			m_toolbar->setHandRaised( false );
		}
		return true;

	case FeatureCommand::TeacherMessage:
	{
		const auto chatMessage = ChatMessage::fromJson(
			QJsonDocument::fromJson( message.argument( Argument::Message ).toString().toUtf8() ).object() );
		if( chatMessage.text.isEmpty() == false )
		{
			auto window = studentChatWindow( worker );
			window->addMessage( chatMessage );
			window->show();
			VeyonCore::platform().coreFunctions().raiseWindow( window, true );
		}
		return true;
	}

	default:
		break;
	}

	return false;
}



bool ClassChatFeaturePlugin::isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const
{
	Q_UNUSED(server)

	QMutexLocker locker( &m_serverMutex );

	if( featureUid == m_chatFeature.uid() )
	{
		return m_serverToolbarActive;
	}

	if( featureUid == m_handRaisedFeature.uid() )
	{
		return m_serverHandRaised;
	}

	return false;
}



TeacherChatWindow* ClassChatFeaturePlugin::chatWindow( VeyonMasterInterface* master )
{
	if( m_chatWindow == nullptr )
	{
		m_chatWindow = new TeacherChatWindow( master ? master->mainWindow() : m_mainWindow.data() );

		connect( m_chatWindow, &TeacherChatWindow::sendRequested, this, [this]( const QStringList& keys, const QString& text ) {
			controlFeature( m_chatFeature.uid(), Operation::Start, { { argToString( Argument::Text ), text } },
							computersForKeys( keys ) );
		} );
		connect( m_chatWindow, &TeacherChatWindow::lowerHandRequested, this, [this]( const QString& key ) {
			controlFeature( m_handRaisedFeature.uid(), Operation::Stop, {}, computersForKeys( { key } ) );
		} );
	}

	return m_chatWindow;
}



void ClassChatFeaturePlugin::addComputers( const ComputerControlInterfaceList& computerControlInterfaces )
{
	for( const auto& controlInterface : computerControlInterfaces )
	{
		addComputer( controlInterface );
	}
}



void ClassChatFeaturePlugin::addComputer( const ComputerControlInterface::Pointer& computerControlInterface )
{
	const auto key = computerControlInterface->computer().hostName();
	if( key.isEmpty() || m_chatWindow == nullptr )
	{
		return;
	}

	m_computers[key] = computerControlInterface.toWeakRef();
	m_chatWindow->addComputer( key, computerControlInterface->computerName(),
							   computerControlInterface->userFullName().isEmpty() ? computerControlInterface->userLoginName()
																				   : computerControlInterface->userFullName() );
}



ComputerControlInterfaceList ClassChatFeaturePlugin::computersForKeys( const QStringList& keys ) const
{
	ComputerControlInterfaceList computers;
	for( const auto& key : keys )
	{
		if( const auto controlInterface = m_computers.value( key ).toStrongRef() )
		{
			computers.append( controlInterface );
		}
	}
	return computers;
}



StudentChatWindow* ClassChatFeaturePlugin::studentChatWindow( VeyonWorkerInterface& worker )
{
	if( m_studentChatWindow == nullptr )
	{
		m_studentChatWindow = new StudentChatWindow;
		connect( m_studentChatWindow, &StudentChatWindow::messageSent, this, [this, &worker]( const QString& text ) {
			worker.sendFeatureMessageReply( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::StudentMessage }
												.addArgument( Argument::Text, text ) );
		} );
	}

	return m_studentChatWindow;
}
