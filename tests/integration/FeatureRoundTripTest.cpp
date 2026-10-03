/*
 * FeatureRoundTripTest.cpp - feature messages between a master and a running server
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

#include <QApplication>
#include <QElapsedTimer>
#include <QtTest>

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "VeyonConnection.h"
#include "VeyonCore.h"

// wire values of the plugins (enums are private there; only appended to, never reordered)
namespace Inventory
{
static const Feature::Uid FeatureUid{ QStringLiteral("e3a95c21-0f4d-4b7e-9d68-5a1c3e7b2f80") };
enum class Command { Query, Info };
enum class Argument { Inventory };
}

namespace RunningApps
{
static const Feature::Uid FeatureUid{ QStringLiteral("9e1c4b7a-3d2f-4a86-b5c9-0f7e2d6a1b38") };
enum class Command { Start, Stop, NotifyClosedApplications, QueryApplications, ApplicationList, CloseApplication };
enum class Argument { Mode, Applications, BlockUsbStorage, BlockPrinting, History };
}

// started by tests/integration/run-integration-test.sh with a tafat-server on localhost
class FeatureRoundTripTest : public QObject
{
	Q_OBJECT
private:
	static constexpr int ConnectTimeout = 30000;
	static constexpr int ReplyTimeout = 15000;

	ComputerControlInterface::Pointer m_computer;

	template<typename Command>
	QVariantMap request( const FeatureMessage& message, Command replyCommand )
	{
		bool received = false;
		QVariantMap arguments;

		const auto connection = connect( m_computer->connection(), &VeyonConnection::featureMessageReceived, this,
										 [&]( const FeatureMessage& reply ) {
			if( reply.featureUid() == message.featureUid() && reply.command<Command>() == replyCommand )
			{
				arguments = reply.arguments();
				received = true;
			}
		} );

		m_computer->sendFeatureMessage( message );

		QElapsedTimer timer;
		timer.start();
		while( received == false && timer.elapsed() < ReplyTimeout )
		{
			QTest::qWait( 50 );
		}

		disconnect( connection );

		if( received == false )
		{
			qWarning() << "no reply to" << message;
		}
		return arguments;
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY2( VeyonCore::instance()->initAuthentication(), "no usable authentication key" );

		Computer computer;
		computer.setHostAddress( QStringLiteral("127.0.0.1") );

		m_computer = ComputerControlInterface::Pointer::create( computer );
		m_computer->start( {}, ComputerControlInterface::UpdateMode::FeatureControlOnly );

		QTRY_VERIFY_WITH_TIMEOUT( m_computer->state() == ComputerControlInterface::State::Connected, ConnectTimeout );
		QVERIFY( m_computer->connection() );
	}

	void cleanupTestCase()
	{
		if( m_computer )
		{
			m_computer->stop();
			m_computer.clear();
		}
	}

	void inventory()
	{
		const auto arguments = request( FeatureMessage{ Inventory::FeatureUid, Inventory::Command::Query },
										Inventory::Command::Info );
		QVERIFY2( arguments.isEmpty() == false, "no inventory reply" );

		const auto inventory = arguments.value( QString::number( int( Inventory::Argument::Inventory ) ) ).toMap();
		QVERIFY( inventory.value( QStringLiteral("os") ).toString().isEmpty() == false );
		QVERIFY( inventory.value( QStringLiteral("arch") ).toString().isEmpty() == false );
		QVERIFY( inventory.value( QStringLiteral("cores") ).toInt() > 0 );
		QCOMPARE( inventory.value( QStringLiteral("version") ).toString(), VeyonCore::versionString() );
	}

	void runningApps()
	{
		const auto arguments = request( FeatureMessage{ RunningApps::FeatureUid, RunningApps::Command::QueryApplications },
										RunningApps::Command::ApplicationList );
		QVERIFY2( arguments.isEmpty() == false, "no application list reply" );

		// without a logged-in user session the list may be empty, but it is always sent
		QVERIFY( arguments.contains( QString::number( int( RunningApps::Argument::Applications ) ) ) );
		QVERIFY( arguments.contains( QString::number( int( RunningApps::Argument::History ) ) ) );
	}
};



int main( int argc, char** argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	VeyonCore core( &app, VeyonCore::Component::Master, QStringLiteral("IntegrationTest") );

	FeatureRoundTripTest test;
	return QTest::qExec( &test, argc, argv );
}

#include "FeatureRoundTripTest.moc"
