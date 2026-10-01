/*
 * WebPolicy.cpp - browser policies for blocking or allowing websites
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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "WebPolicy.h"


QString WebPolicy::normalizedSite( const QString& input )
{
	auto site = input.trimmed().toLower();

	// strip scheme, path, query, port and credentials
	const auto schemeEnd = site.indexOf( QLatin1String("://") );
	if( schemeEnd >= 0 )
	{
		site = site.mid( schemeEnd + 3 );
	}
	site = site.section( QLatin1Char('/'), 0, 0 ).section( QLatin1Char('?'), 0, 0 ).section( QLatin1Char('#'), 0, 0 );
	site = site.section( QLatin1Char('@'), -1 ).section( QLatin1Char(':'), 0, 0 );

	if( site.startsWith( QLatin1String("www.") ) )
	{
		site = site.mid( 4 );
	}
	while( site.endsWith( QLatin1Char('.') ) )
	{
		site.chop( 1 );
	}

	static const QRegularExpression validSite{ QStringLiteral("^[a-z0-9]([a-z0-9-]*[a-z0-9])?(\\.[a-z0-9]([a-z0-9-]*[a-z0-9])?)+$") };
	if( validSite.match( site ).hasMatch() == false )
	{
		return {};
	}

	return site;
}



WebPolicy::ChromiumPolicies WebPolicy::chromiumPolicies( Mode mode, const QStringList& sites )
{
	// a host name in the URL filter format matches the domain and all its subdomains
	if( mode == Mode::BlockListed )
	{
		return { sites, {} };
	}

	auto allowlist = sites;
	// keep the browsers' own pages (new tab, settings) working
	allowlist += { QStringLiteral("chrome://*"), QStringLiteral("edge://*"), QStringLiteral("brave://*") };

	return { { QStringLiteral("*") }, allowlist };
}



WebPolicy::FirefoxPolicies WebPolicy::firefoxPolicies( Mode mode, const QStringList& sites )
{
	QStringList patterns;
	for( const auto& site : sites )
	{
		patterns += { QStringLiteral("*://%1/*").arg( site ), QStringLiteral("*://*.%1/*").arg( site ) };
	}

	if( mode == Mode::BlockListed )
	{
		return { patterns, {} };
	}

	return { { QStringLiteral("<all_urls>") }, patterns };
}



QByteArray WebPolicy::chromiumPolicyFile( const ChromiumPolicies& policies )
{
	QJsonObject root;
	root[QStringLiteral("URLBlocklist")] = QJsonArray::fromStringList( policies.blocklist );
	if( policies.allowlist.isEmpty() == false )
	{
		root[QStringLiteral("URLAllowlist")] = QJsonArray::fromStringList( policies.allowlist );
	}
	return QJsonDocument( root ).toJson();
}



QByteArray WebPolicy::firefoxPolicyFile( const FirefoxPolicies& policies )
{
	QJsonObject filter;
	filter[QStringLiteral("Block")] = QJsonArray::fromStringList( policies.block );
	if( policies.exceptions.isEmpty() == false )
	{
		filter[QStringLiteral("Exceptions")] = QJsonArray::fromStringList( policies.exceptions );
	}

	QJsonObject policyObject;
	policyObject[QStringLiteral("WebsiteFilter")] = filter;

	QJsonObject root;
	root[QStringLiteral("policies")] = policyObject;

	return QJsonDocument( root ).toJson();
}
