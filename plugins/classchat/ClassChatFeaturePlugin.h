/*
 * ClassChatFeaturePlugin.h - hand raise and chat between teacher and students
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
#include <QUuid>

#include "ChatLog.h"
#include "ClassChatWindows.h"
#include "ComputerControlInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "HandIn.h"

class ClassChatFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.ClassChat")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		Text,
		Message,
		Messages,
		HandRaised,
		SessionId,
		FileId,
		FileName,
		ChunkIndex,
		ChunkCount,
		Data
	};
	Q_ENUM(Argument)

	explicit ClassChatFeaturePlugin( QObject* parent = nullptr );
	~ClassChatFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("5f0b8f39-2a44-4e0c-9a3b-6f1f9f0d2c51") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("ClassChat");
	}

	QString description() const override
	{
		return tr( "Hand raise and chat between the teacher and the students" );
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

	bool isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const override;

private:
	enum class FeatureCommand
	{
		ShowToolbar,
		HideToolbar,
		TeacherMessage,
		LowerHand,
		HandState,
		StudentMessage,
		Update,
		HandInChunk
	};

	static QByteArray toJsonData( const QJsonArray& array );
	static QByteArray toJsonData( const QJsonObject& object );

	// master side
	TeacherChatWindow* chatWindow( VeyonMasterInterface* master );
	void addComputers( const ComputerControlInterfaceList& computerControlInterfaces );
	void addComputer( const ComputerControlInterface::Pointer& computerControlInterface );
	ComputerControlInterfaceList computersForKeys( const QStringList& keys ) const;

	void saveHandIn( const ComputerControlInterface::Pointer& computerControlInterface, const HandInAssembler::File& file );
	static QString handInFolder();

	// worker side
	StudentChatWindow* studentChatWindow( VeyonWorkerInterface& worker );
	void handIn( VeyonWorkerInterface& worker );

	static FeatureMessage& addChunk( FeatureMessage& message, const HandInChunk& chunk );
	static HandInChunk chunkFromMessage( const FeatureMessage& message );

	const Feature m_chatFeature;
	const Feature m_showToolbarFeature;
	const Feature m_hideToolbarFeature;
	const Feature m_openChatFeature;
	const Feature m_handRaisedFeature;
	const FeatureList m_features;

	// master side: conversations by host name
	QPointer<QWidget> m_mainWindow;
	QPointer<TeacherChatWindow> m_chatWindow;
	QMap<QString, QWeakPointer<ComputerControlInterface>> m_computers;
	QMap<QString, ChatLog> m_masterLogs;
	QMap<QString, QString> m_masterSessionIds;
	HandInAssembler m_handInAssembler;

	// server side: state of the current session
	mutable QMutex m_serverMutex;
	const QString m_serverSessionId{ QUuid::createUuid().toString( QUuid::WithoutBraces ) };
	ChatLog m_serverLog;
	bool m_serverToolbarActive{false};
	bool m_serverHandRaised{false};
	QAtomicInt m_serverVersion{0};
	HandInQueue m_serverHandIns;

	// worker side
	QPointer<StudentToolbar> m_toolbar;
	QPointer<StudentChatWindow> m_studentChatWindow;

};
