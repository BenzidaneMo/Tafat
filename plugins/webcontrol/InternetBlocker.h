/*
 * InternetBlocker.h - blocks internet access of all programs
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

#include <QStringList>

// Blocks web traffic (HTTP, HTTPS and QUIC) of all programs to addresses
// outside the private network ranges with a Windows firewall rule, so that
// the school network and the connections to the teacher's computer keep
// working. Not supported on other platforms. Requires administrator privileges.
class InternetBlocker
{
public:
	static bool isSupported();

	static bool apply();
	static bool clear();

	// public internet addresses: everything except 0.0.0.0/8, 10.0.0.0/8,
	// 127.0.0.0/8, 169.254.0.0/16, 172.16.0.0/12, 192.168.0.0/16 and multicast
	static QString publicAddressRanges();

	// netsh arguments that add or delete the firewall rules
	static QList<QStringList> addRuleArguments();
	static QList<QStringList> deleteRuleArguments();

};
