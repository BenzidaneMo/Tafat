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
#include <QTimer>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "MessageContext.h"
#include "ModeFeatureHelper.h"
#include "PlatformCoreFunctions.h"
#include "QuizEditorDialogs.h"
#include "QuizFeaturePlugin.h"
#include "QuizResultsWindow.h"
#include "QuizWindow.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


static const char* AnswersVersionProperty = "quizAnswersVersion";
// time the worker gets to hand in the final answers before it is stopped
static constexpr int WorkerStopDelay = 3000;



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



QByteArray QuizFeaturePlugin::toJsonData( const QJsonObject& object )
{
	// sent as byte array: feature messages limit strings to 32768 characters
	return QJsonDocument( object ).toJson( QJsonDocument::Compact );
}



QJsonObject QuizFeaturePlugin::fromJsonData( const QVariant& json )
{
	// also accepts JSON sent as string
	return QJsonDocument::fromJson( json.toByteArray() ).object();
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

		const auto quiz = Quiz::fromJson( fromJsonData( arguments.value( argToString( Argument::Quiz ) ) ) );

		// solutions never leave the teacher's computer
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::StartQuiz }
								.addArgument( Argument::Quiz, toJsonData( quiz.withoutSolutions().toJson() ) ),
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

	if( m_masterQuizActive == false && m_masterQuizCancelled )
	{
		// the launcher was cancelled and the master is going back to monitoring mode
		return true;
	}

	const auto newQuiz = m_masterQuizActive == false;
	if( newQuiz )
	{
		QuizLauncherDialog launcher( master.mainWindow() );
		if( launcher.exec() != QDialog::Accepted || launcher.selectedQuiz().questions.isEmpty() )
		{
			m_masterQuizCancelled = true;
			ModeFeatureHelper::returnToMonitoringMode( master );
			return true;
		}

		m_masterQuiz = launcher.selectedQuiz();
		m_masterQuizActive = true;

		delete m_resultsWindow;
		m_resultsWindow = new QuizResultsWindow( m_masterQuiz, master.mainWindow() );
	}
	// else the master enforces the selected mode on a computer that (re)connected:
	// send it the running quiz again
	if( m_resultsWindow.isNull() )
	{
		m_resultsWindow = new QuizResultsWindow( m_masterQuiz, master.mainWindow() );
	}

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
	if( newQuiz )
	{
		m_resultsWindow->show();
	}

	controlFeature( feature.uid(), Operation::Start,
					{ { argToString( Argument::Quiz ), toJsonData( m_masterQuiz.toJson() ) } },
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

	m_masterQuizActive = false;
	m_masterQuizCancelled = false;
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
										Quiz::answersFromJson( fromJsonData( message.argument( Argument::Answers ) ) ),
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
		const auto quiz = Quiz::fromJson( fromJsonData( message.argument( Argument::Quiz ) ) );
		{
			QMutexLocker locker( &m_serverMutex );
			m_serverQuizId = quiz.id;
			m_serverQuizActive = true;
			m_serverAnswers = toJsonData( {} );
			m_serverFinished = false;
		}
		m_answersVersion.ref();
	}
	else if( message.command<FeatureCommand>() == FeatureCommand::StopQuiz )
	{
		{
			// keep the quiz ID so that the final answers handed in by the worker are still reported
			QMutexLocker locker( &m_serverMutex );
			m_serverQuizActive = false;
		}

		// the master stops all modes when switching modes: don't start a worker just to stop it
		auto& workerManager = server.featureWorkerManager();
		if( workerManager.isWorkerRunning( m_quizFeature.uid() ) )
		{
			workerManager.sendMessageToUnmanagedSessionWorker( message );
			// give the worker the time to hand in the final answers and close its window
			QTimer::singleShot( WorkerStopDelay, this, [this, &workerManager]() {
				QMutexLocker locker( &m_serverMutex );
				if( m_serverQuizActive == false )
				{
					workerManager.stopWorker( m_quizFeature.uid() );
				}
			} );
		}
		return true;
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
		m_serverAnswers = message.argument( Argument::Answers ).toByteArray();
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
		const auto quiz = Quiz::fromJson( fromJsonData( message.argument( Argument::Quiz ) ) );

		delete m_quizWindow;
		m_quizWindow = new QuizWindow( quiz );
		m_quizWindow->setAttribute( Qt::WA_DeleteOnClose );

		connect( m_quizWindow, &QuizWindow::answersChanged, this,
				 [&worker, quizId = quiz.id, featureUid = m_quizFeature.uid()]( const QuizAnswers& answers, bool finished ) {
			worker.sendFeatureMessageReply( FeatureMessage{ featureUid, FeatureCommand::ReportAnswers }
												.addArgument( Argument::QuizId, quizId )
												.addArgument( Argument::Answers, toJsonData( Quiz::answersToJson( answers ) ) )
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
