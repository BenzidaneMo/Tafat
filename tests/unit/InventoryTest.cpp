/*
 * InventoryTest.cpp - inventory of the student computers
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

#include <QtTest>

#include "SystemInventory.h"

class InventoryTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void formatMegabytes()
	{
		QCOMPARE( SystemInventory::formatMegabytes( 512 ), QStringLiteral("512 MB") );
		QCOMPARE( SystemInventory::formatMegabytes( 4000 ), QStringLiteral("3.9 GB") );
		QCOMPARE( SystemInventory::formatMegabytes( 16384 ), QStringLiteral("16.0 GB") );
		QVERIFY( SystemInventory::formatMegabytes( 0 ).isEmpty() );
	}

	void describeSystem()
	{
		QCOMPARE( SystemInventory::describeSystem( { { QLatin1String(SystemInventory::OperatingSystem), QStringLiteral("Windows 7") },
													 { QLatin1String(SystemInventory::Architecture), QStringLiteral("i386") } } ),
				  QStringLiteral("Windows 7 (32-bit)") );
		QCOMPARE( SystemInventory::describeSystem( { { QLatin1String(SystemInventory::OperatingSystem), QStringLiteral("Windows 10") },
													 { QLatin1String(SystemInventory::Architecture), QStringLiteral("x86_64") } } ),
				  QStringLiteral("Windows 10 (64-bit)") );
		QCOMPARE( SystemInventory::describeSystem( { { QLatin1String(SystemInventory::OperatingSystem), QStringLiteral("Windows 11") },
													 { QLatin1String(SystemInventory::Architecture), QStringLiteral("arm64") } } ),
				  QStringLiteral("Windows 11 (64-bit)") );
		QVERIFY( SystemInventory::describeSystem( {} ).isEmpty() );
	}

	void csvLine()
	{
		QCOMPARE( SystemInventory::csvLine( { QStringLiteral("PC-01"), QStringLiteral("say \"hi\""), QString() } ),
				  QStringLiteral("\"PC-01\",\"say \"\"hi\"\"\",\"\"") );
	}

	void procFiles()
	{
		QCOMPARE( SystemInventory::memoryFromMeminfo( "MemTotal:        8041236 kB\nMemFree:         1234 kB\n" ), 7852 );
		QCOMPARE( SystemInventory::memoryFromMeminfo( "garbage" ), 0 );
		QCOMPARE( SystemInventory::processorFromCpuinfo( "processor\t: 0\nmodel name\t: Intel(R) Core(TM)  i3-2100 CPU @ 3.10GHz\nflags\t: fpu\n" ),
				  QStringLiteral("Intel(R) Core(TM) i3-2100 CPU @ 3.10GHz") );
		QVERIFY( SystemInventory::processorFromCpuinfo( "" ).isEmpty() );
	}

	void collect()
	{
		const auto inventory = SystemInventory::collect();
		QVERIFY( inventory.value( QLatin1String(SystemInventory::OperatingSystem) ).toString().isEmpty() == false );
		QVERIFY( inventory.value( QLatin1String(SystemInventory::Cores) ).toInt() > 0 );
		QVERIFY( inventory.value( QLatin1String(SystemInventory::MemoryMB) ).toLongLong() > 0 );
		QCOMPARE( inventory.value( QLatin1String(SystemInventory::QtVersion) ).toString(), QString::fromLatin1( qVersion() ) );
	}
};

QTEST_GUILESS_MAIN(InventoryTest)
#include "InventoryTest.moc"
