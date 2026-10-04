/*
 * RewardsTest.cpp - tests of the stars given to the students
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

#include "RewardBook.h"

class RewardsTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void key()
	{
		// the name from "Register" wins over the computer name
		QCOMPARE( RewardBook::key( QStringLiteral("  Amina   Benali "), QStringLiteral("PC-01") ), QStringLiteral("Amina Benali") );
		QCOMPARE( RewardBook::key( QString(), QStringLiteral("PC-01") ), QStringLiteral("PC-01") );
		QVERIFY( RewardBook::key( QString(), QString() ).isEmpty() );
	}

	void addRemove()
	{
		RewardBook book;
		QCOMPARE( book.add( QStringLiteral("Yanis") ), 1 );
		QCOMPARE( book.add( QStringLiteral("Yanis"), 2 ), 3 );
		QCOMPARE( book.add( QString() ), 0 );
		QCOMPARE( book.remove( QStringLiteral("Yanis") ), 2 );
		QCOMPARE( book.remove( QStringLiteral("Lina") ), 0 );	// unknown student
		QVERIFY( book.students().contains( QStringLiteral("Lina") ) == false );

		// removing the last star removes the student
		book.remove( QStringLiteral("Yanis"), 5 );
		QVERIFY( book.isEmpty() );

		QCOMPARE( book.add( QStringLiteral("Idir"), 5000 ), RewardBook::MaximumStars );
		book.reset();
		QVERIFY( book.isEmpty() );
	}

	void storeAndCsv()
	{
		RewardBook book;
		book.add( QStringLiteral("zahra") );
		book.add( QString::fromUtf8( "أمين" ), 2 );
		book.add( QStringLiteral("Amina \"Mina\""), 3 );

		const auto restored = RewardBook::fromVariantMap( book.toVariantMap() );
		QCOMPARE( restored.students(), book.students() );
		QCOMPARE( restored.stars( QString::fromUtf8( "أمين" ) ), 2 );

		// invalid stored values are dropped
		QVERIFY( RewardBook::fromVariantMap( { { QStringLiteral("x"), -4 } } ).isEmpty() );

		const auto csv = book.toCsv( QStringLiteral("Student"), QStringLiteral("Stars") );
		const auto lines = csv.split( QLatin1Char('\n'), Qt::SkipEmptyParts );
		QCOMPARE( lines.size(), 4 );
		QCOMPARE( lines[0], QStringLiteral("\"Student\",\"Stars\"") );
		QCOMPARE( lines[1], QStringLiteral("\"Amina \"\"Mina\"\"\",3") );	// sorted, case-insensitive
		QCOMPARE( lines[2], QStringLiteral("\"zahra\",1") );
	}
};

QTEST_GUILESS_MAIN(RewardsTest)
#include "RewardsTest.moc"
