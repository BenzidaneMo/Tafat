/*
 * WebPolicyTest.cpp - tests for website control policies
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include "PolicyStore.h"
#include "WebPolicy.h"

using Mode = WebPolicy::Mode;

class WebPolicyTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void normalizedSite_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("expected");

		QTest::newRow("plain") << QStringLiteral("youtube.com") << QStringLiteral("youtube.com");
		QTest::newRow("url") << QStringLiteral("https://www.YouTube.com/watch?v=1") << QStringLiteral("youtube.com");
		QTest::newRow("subdomain") << QStringLiteral("m.facebook.com/") << QStringLiteral("m.facebook.com");
		QTest::newRow("port and credentials") << QStringLiteral("http://user@example.org:8080/x") << QStringLiteral("example.org");
		QTest::newRow("spaces") << QStringLiteral("  tiktok.com  ") << QStringLiteral("tiktok.com");
		QTest::newRow("no domain") << QStringLiteral("localhost") << QString();
		QTest::newRow("invalid") << QStringLiteral("not a site") << QString();
		QTest::newRow("empty") << QString() << QString();
	}

	void normalizedSite()
	{
		QFETCH(QString, input);
		QFETCH(QString, expected);

		QCOMPARE( WebPolicy::normalizedSite( input ), expected );
	}

	void chromiumBlockListed()
	{
		const auto policies = WebPolicy::chromiumPolicies( Mode::BlockListed, { QStringLiteral("youtube.com") } );
		QCOMPARE( policies.blocklist, QStringList{ QStringLiteral("youtube.com") } );
		QVERIFY( policies.allowlist.isEmpty() );
	}

	void chromiumAllowListedOnly()
	{
		const auto policies = WebPolicy::chromiumPolicies( Mode::AllowListedOnly, { QStringLiteral("wikipedia.org") } );
		QCOMPARE( policies.blocklist, QStringList{ QStringLiteral("*") } );
		QVERIFY( policies.allowlist.contains( QStringLiteral("wikipedia.org") ) );
		QVERIFY( policies.allowlist.contains( QStringLiteral("chrome://*") ) );
	}

	void firefoxPolicies()
	{
		const auto blocked = WebPolicy::firefoxPolicies( Mode::BlockListed, { QStringLiteral("youtube.com") } );
		QCOMPARE( blocked.block, QStringList( { QStringLiteral("*://youtube.com/*"), QStringLiteral("*://*.youtube.com/*") } ) );
		QVERIFY( blocked.exceptions.isEmpty() );

		const auto allowed = WebPolicy::firefoxPolicies( Mode::AllowListedOnly, { QStringLiteral("wikipedia.org") } );
		QCOMPARE( allowed.block, QStringList{ QStringLiteral("<all_urls>") } );
		QCOMPARE( allowed.exceptions.size(), 2 );
	}

	void policyFiles()
	{
		const auto chromium = QJsonDocument::fromJson( WebPolicy::chromiumPolicyFile(
			WebPolicy::chromiumPolicies( Mode::AllowListedOnly, { QStringLiteral("wikipedia.org") } ) ) ).object();
		QCOMPARE( chromium[QStringLiteral("URLBlocklist")].toArray().size(), 1 );
		QVERIFY( chromium.contains( QStringLiteral("URLAllowlist") ) );

		const auto firefox = QJsonDocument::fromJson( WebPolicy::firefoxPolicyFile(
			WebPolicy::firefoxPolicies( Mode::BlockListed, { QStringLiteral("youtube.com") } ) ) ).object();
		const auto filter = firefox[QStringLiteral("policies")].toObject()[QStringLiteral("WebsiteFilter")].toObject();
		QCOMPARE( filter[QStringLiteral("Block")].toArray().size(), 2 );
		QVERIFY( filter.contains( QStringLiteral("Exceptions") ) == false );
	}

	void mergedListKeepsAdministratorEntries()
	{
		const QStringList administrator{ QStringLiteral("admin-blocked.com") };

		// first application
		auto merged = PolicyStore::mergedList( administrator, {}, { QStringLiteral("youtube.com") } );
		QCOMPARE( merged, QStringList( { QStringLiteral("admin-blocked.com"), QStringLiteral("youtube.com") } ) );

		// changed list replaces our previous entries
		merged = PolicyStore::mergedList( merged, { QStringLiteral("youtube.com") }, { QStringLiteral("tiktok.com") } );
		QCOMPARE( merged, QStringList( { QStringLiteral("admin-blocked.com"), QStringLiteral("tiktok.com") } ) );

		// clearing removes only our entries
		merged = PolicyStore::mergedList( merged, { QStringLiteral("tiktok.com") }, {} );
		QCOMPARE( merged, administrator );
	}
};

QTEST_GUILESS_MAIN(WebPolicyTest)

#include "WebPolicyTest.moc"
