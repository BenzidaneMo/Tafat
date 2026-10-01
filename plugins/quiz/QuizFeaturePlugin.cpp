/*
 * QuizFeaturePlugin.cpp - quizzes and polls for students
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
#include <QMessageBox>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "MessageContext.h"
#include "PlatformCoreFunctions.h"
#include "QuizEditorDialogs.h"
#include "QuizFeaturePlugin.h"
#include "QuizResultsWindow.h"
#include "QuizWindow.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


static const char* AnswersVersionProperty = "quizAnswersVersion";



QuizFeaturePlugin::QuizFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_quizFeature( QStringLiteral("Quiz"),
				   Feature::Flag::Mode | Feature::Flag::AllComponents,
				   Feature::Uid( "da59e94c-3bf1-45e0-983e-0dab49226c00" ),
				   Feature::Uid(),
				   tr( "Quiz" ), tr( "End quiz" ),
				   tr( "Start a quiz or a poll on the selected computers and follow the "
					   "answers and scores live." ),
				   QStringLiteral(":/quiz/quiz.png") ),
	m_features( { m_quizFeature } )
{
}



QuizFeaturePlugin::~QuizFeaturePlugin()
{
	delete m_quizWindow;
}



QString QuizFeaturePlugin::toJsonString( const QJsonObject& object )
{
	return QString::fromUtf8( QJsonDocument( object ).toJson( QJsonDocument::Compact ) );
}



QJsonObject QuizFeaturePlugin::fromJsonString( const QString& json )
{
	return QJsonDocument::fromJson( json.toUtf8() ).object();
}



bool QuizFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
										const QVariantMap& arguments,
										const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid != m_quizFeature.uid() )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		auto targets = computerControlInterfaces;
		targets.removeLocalHostInterfaces();

		const auto quiz = Quiz::fromJson( fromJsonString( arguments.value( argToString( Argument::Quiz ) ).toString() ) );

		// solutions never leave the teacher's computer
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::StartQuiz }
								.addArgument( Argument::Quiz, toJsonString( quiz.withoutSolutions().toJson() ) ),
							targets );
		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::StopQuiz }, computerControlInterfaces );
		return true;
	}

	return false;
}



bool QuizFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_quizFeature.uid() )
	{
		return false;
	}

	QuizLauncherDialog launcher( master.mainWindow() );
	if( launcher.exec() != QDialog::Accepted || launcher.selectedQuiz().questions.isEmpty() )
	{
		return true;
	}

	m_masterQuiz = launcher.selectedQuiz();

	delete m_resultsWindow;
	m_resultsWindow = new QuizResultsWindow( m_masterQuiz, master.mainWindow() );
	for( const auto& controlInterface : computerControlInterfaces )
	{
		if( controlInterface->computer().hostName().isEmpty() == false )
		{
			m_resultsWindow->addParticipant( controlInterface->computer().hostName(),
											 controlInterface->computerName(),
											 controlInterface->userFullName().isEmpty() ? controlInterface->userLoginName()
																						 : controlInterface->userFullName() );
		}
	}
	m_resultsWindow->show();

	controlFeature( feature.uid(), Operation::Start,
					{ { argToString( Argument::Quiz ), toJsonString( m_masterQuiz.toJson() ) } },
					computerControlInterfaces );

	return true;
}



bool QuizFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
									 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)

	if( feature.uid() != m_quizFeature.uid() )
	{
		return false;
	}

	controlFeature( feature.uid(), Operation::Stop, {}, computerControlInterfaces );

	if( m_resultsWindow )
	{
		m_resultsWindow->setEnded();
		m_resultsWindow->show();
		m_resultsWindow->raise();
	}

	return true;
}



bool QuizFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
											  const FeatureMessage& message )
{
	if( message.featureUid() != m_quizFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::ReportAnswers )
	{
		return false;
	}

	if( m_resultsWindow && message.argument( Argument::QuizId ).toString() == m_masterQuiz.id )
	{
		const auto key = computerControlInterface->computer().hostName();
		m_resultsWindow->addParticipant( key, computerControlInterface->computerName(),
										 computerControlInterface->userFullName().isEmpty() ? computerControlInterface->userLoginName()
																							 : computerControlInterface->userFullName() );
		m_resultsWindow->updateAnswers( key,
										Quiz::answersFromJson( fromJsonString( message.argument( Argument::Answers ).toString() ) ),
										message.argument( Argument::Finished ).toBool() );
	}

	return true;
}



bool QuizFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
											  const MessageContext& messageContext,
											  const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_quizFeature.uid() )
	{
		return false;
	}

	if( message.command<FeatureCommand>() == FeatureCommand::StartQuiz )
	{
		const auto quiz = Quiz::fromJson( fromJsonString( message.argument( Argument::Quiz ).toString() ) );
		{
			QMutexLocker locker( &m_serverMutex );
			m_serverQuizId = quiz.id;
			m_serverQuizActive = true;
			m_serverAnswers = toJsonString( {} );
			m_serverFinished = false;
		}
		m_answersVersion.ref();
	}
	else if( message.command<FeatureCommand>() == FeatureCommand::StopQuiz )
	{
		// keep the quiz ID so that the final answers handed in by the worker are still reported
		QMutexLocker locker( &m_serverMutex );
		m_serverQuizActive = false;
	}
	else
	{
		return false;
	}

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

	return true;
}



bool QuizFeaturePlugin::handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message )
{
	Q_UNUSED(server)

	if( message.featureUid() != m_quizFeature.uid() ||
		message.command<FeatureCommand>() != FeatureCommand::ReportAnswers )
	{
		return false;
	}

	QMutexLocker locker( &m_serverMutex );
	if( message.argument( Argument::QuizId ).toString() == m_serverQuizId )
	{
		m_serverAnswers = message.argument( Argument::Answers ).toString();
		m_serverFinished = message.argument( Argument::Finished ).toBool();
		m_answersVersion.ref();
	}

	return true;
}



void QuizFeaturePlugin::sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext )
{
	const auto version = m_answersVersion.loadAcquire();
	if( messageContext.ioDevice() == nullptr ||
		messageContext.ioDevice()->property( AnswersVersionProperty ).toInt() == version )
	{
		return;
	}

	FeatureMessage reply{ m_quizFeature.uid(), FeatureCommand::ReportAnswers };
	{
		QMutexLocker locker( &m_serverMutex );
		if( m_serverQuizId.isEmpty() )
		{
			return;
		}
		reply.addArgument( Argument::QuizId, m_serverQuizId )
			.addArgument( Argument::Answers, m_serverAnswers )
			.addArgument( Argument::Finished, m_serverFinished );
	}

	server.sendFeatureMessageReply( messageContext, reply );
	messageContext.ioDevice()->setProperty( AnswersVersionProperty, version );
}



bool QuizFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if( message.featureUid() != m_quizFeature.uid() )
	{
		return false;
	}

	if( message.command<FeatureCommand>() == FeatureCommand::StartQuiz )
	{
		const auto quiz = Quiz::fromJson( fromJsonString( message.argument( Argument::Quiz ).toString() ) );

		delete m_quizWindow;
		m_quizWindow = new QuizWindow( quiz );
		m_quizWindow->setAttribute( Qt::WA_DeleteOnClose );

		connect( m_quizWindow, &QuizWindow::answersChanged, this,
				 [&worker, quizId = quiz.id, featureUid = m_quizFeature.uid()]( const QuizAnswers& answers, bool finished ) {
			worker.sendFeatureMessageReply( FeatureMessage{ featureUid, FeatureCommand::ReportAnswers }
												.addArgument( Argument::QuizId, quizId )
												.addArgument( Argument::Answers, toJsonString( Quiz::answersToJson( answers ) ) )
												.addArgument( Argument::Finished, finished ) );
		} );

		m_quizWindow->show();
		VeyonCore::platform().coreFunctions().raiseWindow( m_quizWindow, true );
		return true;
	}

	if( message.command<FeatureCommand>() == FeatureCommand::StopQuiz )
	{
		if( m_quizWindow )
		{
			// the teacher ended the quiz: hand in the current answers, then close
			Q_EMIT m_quizWindow->answersChanged( m_quizWindow->answers(), true );
			m_quizWindow->hide();
			m_quizWindow->deleteLater();
		}
		return true;
	}

	return false;
}



bool QuizFeaturePlugin::isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const
{
	Q_UNUSED(server)

	QMutexLocker locker( &m_serverMutex );
	return featureUid == m_quizFeature.uid() && m_serverQuizActive;
}
