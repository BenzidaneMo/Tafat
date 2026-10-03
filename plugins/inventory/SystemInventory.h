/*
 * SystemInventory.h - hardware and software facts of a computer
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
#include <QVariantMap>

// Collects facts about the computer for the lab inventory: operating system,
// processor, memory, disk, network addresses and the installed version.
// The keys of the map are part of the protocol between student and teacher.
class SystemInventory
{
public:
	static constexpr auto ComputerName = "computer";
	static constexpr auto OperatingSystem = "os";
	static constexpr auto KernelVersion = "kernel";
	static constexpr auto Architecture = "arch";
	static constexpr auto BuildArchitecture = "buildArch";
	static constexpr auto Processor = "cpu";
	static constexpr auto Cores = "cores";
	static constexpr auto MemoryMB = "memoryMB";
	static constexpr auto DiskTotalMB = "diskTotalMB";
	static constexpr auto DiskFreeMB = "diskFreeMB";
	static constexpr auto IpAddresses = "ip";
	static constexpr auto MacAddresses = "mac";
	static constexpr auto ProductVersion = "version";
	static constexpr auto QtVersion = "qt";

	static QVariantMap collect();

	// e.g. "512 MB" or "3.9 GB"
	static QString formatMegabytes( qint64 megabytes );

	// "Windows 7 (32-bit)", also for a 32-bit program on 64-bit Windows
	static QString describeSystem( const QVariantMap& inventory );

	// one line of a CSV file: quoted fields separated by commas
	static QString csvLine( const QStringList& fields );

	// parses the memory size from the contents of /proc/meminfo
	static qint64 memoryFromMeminfo( const QByteArray& meminfo );

	// parses the processor name from the contents of /proc/cpuinfo
	static QString processorFromCpuinfo( const QByteArray& cpuinfo );

private:
	static qint64 totalMemoryMB();
	static QString processorName();

};
