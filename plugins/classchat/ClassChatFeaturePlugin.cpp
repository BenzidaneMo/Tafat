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

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

#include "ClassChatFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "Filesystem.h"
#include "MessageContext.h"
#include "PlatformCoreFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


static const char* VersionProperty = "classChatVersion";
static const char* MessageIdProperty = "classChatMessageId";
static const char* SessionIdProperty = "classChatSessionId";
static const char* HandInSequenceProperty = "classChatHandInSequence";

// parts of handed-in files sent to a teacher computer per update
static constexpr int HandInChunksPerUpdate = 4;



ClassChatFeaturePlugin::ClassChatFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_chatFeature( QStringLiteral("ClassChat"),
				   Feature::Flag::Action | Feature::Flag::AllComponents,
				   Feature::Uid( "0b6c7e4e-5d55-4f3a-b5a4-1c2d9e5f7a10" ),
				   Feature::Uid(),
				   tr( "Hands and chat" ), {},
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



// JSON is sent as byte array: feature messages limit strings to 32768 characters
QByteArray ClassChatFeaturePlugin::toJsonData( const QJsonArray& array )
{
	return QJsonDocument( array ).toJson( QJsonDocument::Compact );
}



QByteArray ClassChatFeaturePlugin::toJsonData( const QJsonObject& object )
{
	return QJsonDocument( object ).toJson( QJsonDocument::Compact );
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
	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	if( message.command<FeatureCommand>() == FeatureCommand::HandInChunk )
	{
		const auto file = m_handInAssembler.add( computerControlInterface->computer().hostName(), chunkFromMessage( message ) );
		if( file.has_value() )
		{
			saveHandIn( computerControlInterface, *file );
		}
		return true;
	}

	if( message.command<FeatureCommand>() != FeatureCommand::Update )
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
	const auto messages = ChatLog::fromJson( QJsonDocument::fromJson( message.argument( Argument::Messages ).toByteArray() ).array() );
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
				.addArgument( Argument::Message, toJsonData( chatMessage.toJson() ) ) );
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
	else if( message.command<FeatureCommand>() == FeatureCommand::HandInChunk )
	{
		QMutexLocker locker( &m_serverMutex );
		m_serverHandIns.append( chunkFromMessage( message ) );
		return true;
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
	auto ioDevice = messageContext.ioDevice();
	if( ioDevice == nullptr )
	{
		return;
	}

	// handed-in files, a few parts at a time so that the connection stays responsive
	QList<QPair<qint64, HandInChunk>> handInChunks;
	{
		QMutexLocker locker( &m_serverMutex );
		handInChunks = m_serverHandIns.chunksAfter( ioDevice->property( HandInSequenceProperty ).toLongLong(),
													HandInChunksPerUpdate );
	}
	for( const auto& [sequence, chunk] : std::as_const( handInChunks ) )
	{
		FeatureMessage chunkMessage{ m_chatFeature.uid(), FeatureCommand::HandInChunk };
		server.sendFeatureMessageReply( messageContext, addChunk( chunkMessage, chunk ) );
		ioDevice->setProperty( HandInSequenceProperty, sequence );
	}

	const auto version = m_serverVersion.loadAcquire();
	if( version == 0 ||
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
			.addArgument( Argument::Messages, toJsonData( ChatLog::toJson( m_serverLog.messagesAfter( lastMessageId ) ) ) );
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
			connect( m_toolbar, &StudentToolbar::handInRequested, this, [this, &worker]() { handIn( worker ); } );
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
			QJsonDocument::fromJson( message.argument( Argument::Message ).toByteArray() ).object() );
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



FeatureMessage& ClassChatFeaturePlugin::addChunk( FeatureMessage& message, const HandInChunk& chunk )
{
	return message.addArgument( Argument::FileId, chunk.fileId )
		.addArgument( Argument::FileName, chunk.fileName )
		.addArgument( Argument::ChunkIndex, chunk.index )
		.addArgument( Argument::ChunkCount, chunk.count )
		.addArgument( Argument::Data, chunk.data );
}



HandInChunk ClassChatFeaturePlugin::chunkFromMessage( const FeatureMessage& message )
{
	return { message.argument( Argument::FileId ).toUuid(),
			 message.argument( Argument::FileName ).toString(),
			 message.argument( Argument::ChunkIndex ).toInt(),
			 message.argument( Argument::ChunkCount ).toInt(),
			 message.argument( Argument::Data ).toByteArray() };
}



QString ClassChatFeaturePlugin::handInFolder()
{
	// next to the files collected with "Collect"
	const auto base = VeyonCore::filesystem().expandPath(
		VeyonCore::config().value( QStringLiteral("CollectedFilesDestinationDirectory"), QStringLiteral("FileTransfer"),
								   QStringLiteral("%HOME%") ).toString() );
	return QDir( base ).filePath( tr( "Handed-in work" ) + QLatin1Char(' ') + QDate::currentDate().toString( Qt::ISODate ) );
}



void ClassChatFeaturePlugin::saveHandIn( const ComputerControlInterface::Pointer& computerControlInterface,
										 const HandInAssembler::File& file )
{
	const auto key = computerControlInterface->computer().hostName();
	auto student = VeyonCore::stripDomain( computerControlInterface->userFullName().isEmpty()
											   ? computerControlInterface->userLoginName()
											   : computerControlInterface->userFullName() );

	// one folder per student and computer, like the file collection
	const auto folderName = HandIn::safeFileName( student.isEmpty() ? computerControlInterface->computerName()
																	 : student + QLatin1Char('_') + computerControlInterface->computerName() );
	const auto folder = QDir( handInFolder() ).filePath( folderName );

	QFile output( HandIn::uniqueFilePath( folder, HandIn::safeFileName( file.fileName ) ) );
	const auto saved = QDir().mkpath( folder ) && output.open( QFile::WriteOnly ) &&
					   output.write( file.data ) == file.data.size();
	output.close();

	auto window = chatWindow( nullptr );
	addComputer( computerControlInterface );

	ChatMessage note;
	note.time = QDateTime::currentDateTime();
	note.text = saved ? tr( "Handed in: %1" ).arg( QFileInfo( output.fileName() ).fileName() )
					  : tr( "A file could not be saved: %1" ).arg( output.fileName() );
	window->addMessages( key, { note } );
	window->setHandInFolder( handInFolder() );

	if( saved == false )
	{
		vWarning() << "could not save handed-in file" << output.fileName();
	}
}



void ClassChatFeaturePlugin::handIn( VeyonWorkerInterface& worker )
{
	const auto files = QFileDialog::getOpenFileNames( m_toolbar, tr( "Hand in work" ),
													  QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) );
	if( files.isEmpty() )
	{
		return;
	}

	if( files.size() > HandIn::MaxFiles )
	{
		QMessageBox::warning( m_toolbar, tr( "Hand in work" ), tr( "Please choose at most %1 files." ).arg( HandIn::MaxFiles ) );
		return;
	}

	qint64 totalSize = 0;
	for( const auto& fileName : files )
	{
		totalSize += QFileInfo( fileName ).size();
	}
	if( totalSize > HandIn::MaxTotalSize )
	{
		QMessageBox::warning( m_toolbar, tr( "Hand in work" ),
							  tr( "These files are too large together. You can hand in at most %1 MB at once." )
								  .arg( HandIn::MaxTotalSize / 1024 / 1024 ) );
		return;
	}

	for( const auto& fileName : files )
	{
		if( QFileInfo( fileName ).size() > HandIn::MaxFileSize )
		{
			QMessageBox::warning( m_toolbar, tr( "Hand in work" ),
								  tr( "%1 is too large. Files can have at most %2 MB." )
									  .arg( QFileInfo( fileName ).fileName() ).arg( HandIn::MaxFileSize / 1024 / 1024 ) );
			return;
		}
	}

	QStringList handedIn;
	for( const auto& fileName : files )
	{
		QFile file( fileName );
		if( file.open( QFile::ReadOnly ) == false )
		{
			QMessageBox::warning( m_toolbar, tr( "Hand in work" ), tr( "Could not read %1." ).arg( fileName ) );
			continue;
		}

		for( const auto& chunk : HandIn::split( QFileInfo( fileName ).fileName(), file.readAll() ) )
		{
			FeatureMessage message{ m_chatFeature.uid(), FeatureCommand::HandInChunk };
			worker.sendFeatureMessageReply( addChunk( message, chunk ) );
		}
		handedIn.append( QFileInfo( fileName ).fileName() );
	}

	if( handedIn.isEmpty() == false )
	{
		QMessageBox::information( m_toolbar, tr( "Hand in work" ),
								  tr( "Your teacher receives these files:\n\n%1" ).arg( handedIn.join( QLatin1Char('\n') ) ) );
	}
}
