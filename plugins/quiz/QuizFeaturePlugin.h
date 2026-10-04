/*
 * QuizFeaturePlugin.h - quizzes and polls for students
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

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "Quiz.h"

class QuizResultsWindow;
class QuizWindow;
class VeyonWorkerInterface;

class QuizFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.Quiz")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		Quiz,
		QuizId,
		Answers,
		Finished
	};
	Q_ENUM(Argument)

	explicit QuizFeaturePlugin( QObject* parent = nullptr );
	~QuizFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("41453546-c3cf-4e28-a6f5-71d4eac55677") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Quiz");
	}

	QString description() const override
	{
		return tr( "Quizzes and polls for students" );
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

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message ) override;

	void sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	bool isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const override;

private:
	enum class FeatureCommand
	{
		StartQuiz,
		StopQuiz,
		ReportAnswers
	};

	static QByteArray toJsonData( const QJsonObject& object );
	static QJsonObject fromJsonData( const QVariant& json );

	const Feature m_quizFeature;
	const FeatureList m_features;

	// master side: the running quiz including its solutions
	Quiz m_masterQuiz;
	bool m_masterQuizActive{false};
	bool m_masterQuizCancelled{false};
	QPointer<QuizResultsWindow> m_resultsWindow;

	// server side: the latest answers reported by the student's quiz window
	mutable QMutex m_serverMutex;
	QString m_serverQuizId;
	bool m_serverQuizActive{false};
	QByteArray m_serverAnswers;
	bool m_serverFinished{false};
	QAtomicInt m_answersVersion{0};

	// worker side
	QPointer<QuizWindow> m_quizWindow;

};
