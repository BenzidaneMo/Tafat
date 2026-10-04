/*
 * ComputerScanner.h - finds computers running the server in the local networks
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

#include <QHostAddress>
#include <QList>
#include <QNetworkAddressEntry>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>

// Looks for student computers like NetSupport's "Browse": tries the server
// port of every address in the local private networks and keeps the hosts that
// answer with the greeting of a VNC (RFB) server.
class ComputerScanner : public QObject
{
	Q_OBJECT
public:
	static constexpr int MaximumHosts = 1024;
	static constexpr int ParallelConnections = 48;
	static constexpr int TimeoutMs = 600;

	explicit ComputerScanner( int port, QObject* parent = nullptr );
	~ComputerScanner() override;

	// addresses to try: the /24 network (or the smaller configured one) around
	// each private IPv4 address, without the own addresses
	static QList<QHostAddress> candidateHosts( const QList<QNetworkAddressEntry>& entries,
											   const QList<QHostAddress>& ownAddresses,
											   int maximum = MaximumHosts );
	static QList<QHostAddress> localCandidateHosts();

	// hosts of a typed range: "10.0.5.0/24" (prefix 22-32), "192.168.1.10-80" (last part)
	// or "192.168.1.10-192.168.2.20"; private IPv4 only, at most "maximum" hosts.
	// Returns an empty list for anything else.
	static QList<QHostAddress> rangeHosts( const QString& range, int maximum = MaximumHosts );

	static bool isPrivateAddress( const QHostAddress& address );
	static bool isServerGreeting( const QByteArray& data );

	// "name;host" lines for "networkobjects import … format %name%;%host%"
	static QByteArray importData( const QList<QPair<QString, QString>>& computers );

	void start( const QList<QHostAddress>& hosts );
	void cancel();

	bool isRunning() const
	{
		return m_running;
	}

Q_SIGNALS:
	void found( const QHostAddress& address );
	void progress( int done, int total );
	void finished();

private:
	void startNext();
	void finishSocket( QTcpSocket* socket, bool success );

	const int m_port;
	QList<QHostAddress> m_pending;
	QList<QPointer<QTcpSocket>> m_sockets;
	int m_total{0};
	int m_done{0};
	bool m_running{false};

};
