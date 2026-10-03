/*
 * ClassListTest.cpp - tests for the class list of the register
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

#include "ClassList.h"

class ClassListTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void parsesCommaSeparatedListWithHeader()
	{
		const auto list = ClassListUtils::parseCsv( "Name,Class\nAmina Benali,2AS2\n\"Kaci, Yacine\",2AS2\n\n" );
		QCOMPARE( list.size(), 2 );
		QCOMPARE( list[0], ( ClassListEntry{ QStringLiteral("Amina Benali"), QStringLiteral("2AS2") } ) );
		QCOMPARE( list[1].name, QStringLiteral("Kaci, Yacine") );
	}

	void parsesSemicolonsBomAndCrLf()
	{
		const auto list = ClassListUtils::parseCsv( QByteArray( "\xEF\xBB\xBF" "Nom et prénom;Classe\r\nLina Meziane;1AS1\r\n" ) );
		QCOMPARE( list.size(), 1 );
		QCOMPARE( list[0].name, QStringLiteral("Lina Meziane") );
		QCOMPARE( list[0].group, QStringLiteral("1AS1") );
	}

	void parsesSingleColumnWithoutHeader()
	{
		const auto list = ClassListUtils::parseCsv( QStringLiteral("أمينة بن علي\nياسين قاسي\n").toUtf8() );
		QCOMPARE( list.size(), 2 );
		QCOMPARE( list[1].name, QStringLiteral("ياسين قاسي") );
		QVERIFY( list[1].group.isEmpty() );
	}

	void parsesTabsAndQuotedQuotes()
	{
		const auto list = ClassListUtils::parseCsv( "\"Ali \"\"Le Grand\"\" Haddad\"\t3AS1\n" );
		QCOMPARE( list.size(), 1 );
		QCOMPARE( list[0].name, QStringLiteral("Ali \"Le Grand\" Haddad") );
		QCOMPARE( list[0].group, QStringLiteral("3AS1") );
	}

	void skipsDuplicatesAndEmptyNames()
	{
		const auto list = ClassListUtils::parseCsv( "Amina Benali\n,2AS2\namina  BENALI\n" );
		QCOMPARE( list.size(), 1 );
	}

	void matchesNameVariants()
	{
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("Amina Benali"), QStringLiteral("benali amina") ) );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("Hélène Saïdi"), QStringLiteral("Helene Saidi") ) );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("Ben-Ali Amina"), QStringLiteral("Ben Ali Amina") ) );
		// hamza on alef, teh marbuta, tashkeel
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("أمينة"), QStringLiteral("امينه") ) );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("مُحَمَّد"), QStringLiteral("محمد") ) );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("ⵜⴰⴼⴰⵜ"), QStringLiteral("ⵜⴰⴼⴰⵜ") ) );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral("Amina"), QStringLiteral("Amira") ) == false );
		QVERIFY( ClassListUtils::isSameStudent( QStringLiteral(" "), QStringLiteral(" ") ) == false );
	}

	void findsAbsentStudents()
	{
		const ClassList classList{ { QStringLiteral("Amina Benali"), {} },
								   { QStringLiteral("Yacine Kaci"), {} },
								   { QStringLiteral("Lina Meziane"), {} } };

		const auto absent = ClassListUtils::absentStudents( classList, { QStringLiteral("kaci yacine"), QStringLiteral("Someone Else") } );
		QCOMPARE( absent.size(), 2 );
		QCOMPARE( absent[0].name, QStringLiteral("Amina Benali") );
		QCOMPARE( absent[1].name, QStringLiteral("Lina Meziane") );
		QVERIFY( ClassListUtils::contains( classList, QStringLiteral("MEZIANE Lina") ) );
		QVERIFY( ClassListUtils::contains( classList, QStringLiteral("Someone Else") ) == false );
	}

	void stringListRoundTrip()
	{
		const ClassList classList{ { QStringLiteral("Amina Benali"), QStringLiteral("2AS2") },
								   { QStringLiteral("ياسين"), {} } };
		QCOMPARE( ClassListUtils::fromStringList( ClassListUtils::toStringList( classList ) ), classList );
	}
};

QTEST_GUILESS_MAIN(ClassListTest)
#include "ClassListTest.moc"
