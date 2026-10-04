/*
 * ComputerScanner.cpp - finds computers running the server in the local networks
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

#include <QNetworkInterface>
#include <QSet>
#include <QTimer>

#include "ComputerScanner.h"


ComputerScanner::ComputerScanner( int port, QObject* parent ) :
	QObject( parent ),
	m_port( port )
{
}



ComputerScanner::~ComputerScanner()
{
	cancel();
}



bool ComputerScanner::isPrivateAddress( const QHostAddress& address )
{
	if( address.protocol() != QAbstractSocket::IPv4Protocol )
	{
		return false;
	}

	const auto ip = address.toIPv4Address();
	return ( ip >> 24 ) == 10 ||				// 10.0.0.0/8
		   ( ip >> 20 ) == ( ( 172 << 4 ) | 1 ) ||	// 172.16.0.0/12
		   ( ip >> 16 ) == ( ( 192 << 8 ) | 168 );	// 192.168.0.0/16
}



bool ComputerScanner::isServerGreeting( const QByteArray& data )
{
	// "RFB 003.008\n"
	return data.startsWith( "RFB 003." );
}



QList<QHostAddress> ComputerScanner::candidateHosts( const QList<QNetworkAddressEntry>& entries,
													 const QList<QHostAddress>& ownAddresses,
													 int maximum )
{
	QSet<quint32> own;
	for( const auto& address : ownAddresses )
	{
		if( address.protocol() == QAbstractSocket::IPv4Protocol )
		{
			own.insert( address.toIPv4Address() );
		}
	}

	QList<QHostAddress> hosts;
	QSet<quint32> seen;

	for( const auto& entry : entries )
	{
		const auto address = entry.ip();
		if( isPrivateAddress( address ) == false )
		{
			continue;
		}

		// labs are usually one /24 network; larger ones are cut to the /24 around this computer
		const auto prefix = qBound( 24, entry.prefixLength() < 0 ? 24 : entry.prefixLength(), 30 );
		const auto mask = quint32( 0xffffffffu << ( 32 - prefix ) );
		const auto network = address.toIPv4Address() & mask;
		const auto broadcast = network | ~mask;

		for( auto ip = network + 1; ip < broadcast; ++ip )
		{
			if( own.contains( ip ) || seen.contains( ip ) )
			{
				continue;
			}
			if( hosts.size() >= maximum )
			{
				return hosts;
			}
			seen.insert( ip );
			hosts.append( QHostAddress( ip ) );
		}
	}

	return hosts;
}



QList<QHostAddress> ComputerScanner::rangeHosts( const QString& range, int maximum )
{
	const auto text = range.trimmed();
	quint32 first = 0;
	quint32 last = 0;

	const auto toIPv4 = []( const QString& value, quint32* ip ) {
		QHostAddress address;
		if( address.setAddress( value.trimmed() ) == false || address.protocol() != QAbstractSocket::IPv4Protocol )
		{
			return false;
		}
		*ip = address.toIPv4Address();
		return true;
	};

	if( text.contains( QLatin1Char('/') ) )
	{
		bool ok = false;
		const auto prefix = text.section( QLatin1Char('/'), 1 ).toInt( &ok );
		quint32 ip = 0;
		if( ok == false || prefix < 22 || prefix > 32 || toIPv4( text.section( QLatin1Char('/'), 0, 0 ), &ip ) == false )
		{
			return {};
		}
		const auto mask = prefix == 32 ? 0xffffffffu : quint32( 0xffffffffu << ( 32 - prefix ) );
		first = ip & mask;
		last = first | ~mask;
		if( prefix < 31 )
		{
			// without network and broadcast address
			++first;
			--last;
		}
		else
		{
			first = last = ip;
		}
	}
	else if( text.contains( QLatin1Char('-') ) )
	{
		const auto from = text.section( QLatin1Char('-'), 0, 0 ).trimmed();
		auto to = text.section( QLatin1Char('-'), 1 ).trimmed();
		if( to.contains( QLatin1Char('.') ) == false )
		{
			// "192.168.1.10-80": the end replaces the last part
			to = from.section( QLatin1Char('.'), 0, 2 ) + QLatin1Char('.') + to;
		}
		if( toIPv4( from, &first ) == false || toIPv4( to, &last ) == false || last < first )
		{
			return {};
		}
	}
	else if( toIPv4( text, &first ) )
	{
		last = first;
	}
	else
	{
		return {};
	}

	if( isPrivateAddress( QHostAddress( first ) ) == false || isPrivateAddress( QHostAddress( last ) ) == false ||
		quint64( last ) - first + 1 > quint64( maximum ) )
	{
		return {};
	}

	QList<QHostAddress> hosts;
	for( quint64 ip = first; ip <= last; ++ip )
	{
		hosts.append( QHostAddress( quint32( ip ) ) );
	}
	return hosts;
}



QList<QHostAddress> ComputerScanner::localCandidateHosts()
{
	QList<QNetworkAddressEntry> entries;
	const auto interfaces = QNetworkInterface::allInterfaces();
	for( const auto& networkInterface : interfaces )
	{
		const auto flags = networkInterface.flags();
		if( flags.testFlag( QNetworkInterface::IsUp ) == false ||
			flags.testFlag( QNetworkInterface::IsRunning ) == false ||
			flags.testFlag( QNetworkInterface::IsLoopBack ) )
		{
			continue;
		}
		entries.append( networkInterface.addressEntries() );
	}

	return candidateHosts( entries, QNetworkInterface::allAddresses() );
}



QByteArray ComputerScanner::importData( const QList<QPair<QString, QString>>& computers )
{
	QStringList lines;
	for( const auto& computer : computers )
	{
		// the separator must not appear in the name
		auto name = computer.first;
		name.replace( QLatin1Char(';'), QLatin1Char(' ') );
		lines.append( QStringLiteral("%1;%2").arg( name.trimmed(), computer.second.trimmed() ) );
	}
	return lines.join( QLatin1Char('\n') ).toUtf8();
}



void ComputerScanner::start( const QList<QHostAddress>& hosts )
{
	cancel();

	m_pending = hosts;
	m_total = int( hosts.size() );
	m_done = 0;
	m_running = true;

	if( m_pending.isEmpty() )
	{
		m_running = false;
		Q_EMIT finished();
		return;
	}

	for( int i = 0; i < ParallelConnections && m_pending.isEmpty() == false; ++i )
	{
		startNext();
	}
}



void ComputerScanner::cancel()
{
	m_pending.clear();
	for( const auto& socket : std::as_const( m_sockets ) )
	{
		if( socket )
		{
			socket->disconnect( this );
			socket->abort();
			socket->deleteLater();
		}
	}
	m_sockets.clear();
	m_running = false;
}



void ComputerScanner::startNext()
{
	if( m_pending.isEmpty() )
	{
		return;
	}

	const auto address = m_pending.takeFirst();
	auto socket = new QTcpSocket( this );
	m_sockets.append( socket );

	connect( socket, &QTcpSocket::readyRead, this, [this, socket]() {
		if( socket->bytesAvailable() >= 8 )
		{
			finishSocket( socket, isServerGreeting( socket->peek( 12 ) ) );
		}
	} );
	connect( socket, &QTcpSocket::disconnected, this, [this, socket]() { finishSocket( socket, false ); } );
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect( socket, &QTcpSocket::errorOccurred, this, [this, socket]() { finishSocket( socket, false ); } );
#endif
	// closed ports in a lab answer at once, switched off computers never: give each one a deadline
	QTimer::singleShot( TimeoutMs, socket, [this, socket]() { finishSocket( socket, false ); } );

	socket->connectToHost( address, quint16( m_port ) );
}



void ComputerScanner::finishSocket( QTcpSocket* socket, bool success )
{
	if( m_sockets.removeAll( socket ) == 0 )
	{
		return;
	}

	const auto address = socket->peerAddress().isNull() ? QHostAddress() : socket->peerAddress();
	socket->disconnect( this );
	socket->abort();
	socket->deleteLater();

	++m_done;
	if( success && address.isNull() == false )
	{
		Q_EMIT found( address );
	}
	Q_EMIT progress( m_done, m_total );

	if( m_pending.isEmpty() == false )
	{
		startNext();
	}
	else if( m_sockets.isEmpty() && m_running )
	{
		m_running = false;
		Q_EMIT finished();
	}
}
