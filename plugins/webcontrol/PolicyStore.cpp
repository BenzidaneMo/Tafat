/*
 * PolicyStore.cpp - applies and removes browser policies on this computer
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include "PolicyStore.h"
#include "VeyonCore.h"


static QSettings stateStore()
{
	return QSettings( QSettings::SystemScope, VeyonCore::productName(), QStringLiteral("WebControl") );
}



static QString stateKey( const QString& location )
{
	return QStringLiteral("Applied/") + QString::fromLatin1( location.toUtf8().toHex() );
}



QStringList PolicyStore::mergedList( const QStringList& existing, const QStringList& previouslyAdded,
									 const QStringList& added )
{
	QStringList result;

	auto toRemove = previouslyAdded;
	for( const auto& entry : existing )
	{
		if( toRemove.removeOne( entry ) == false )
		{
			result.append( entry );
		}
	}

	for( const auto& entry : added )
	{
		if( result.contains( entry ) == false )
		{
			result.append( entry );
		}
	}

	return result;
}



#ifdef Q_OS_WIN

static const QStringList& chromiumPolicyKeys()
{
	static const QStringList keys{
		QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Google\\Chrome"),
		QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Edge"),
		QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\BraveSoftware\\Brave"),
		QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Chromium")
	};
	return keys;
}

static const auto FirefoxFilterKey = QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Mozilla\\Firefox\\WebsiteFilter");



// list policies are stored as values named 1, 2, 3, ... below the policy key
static bool writeListPolicy( const QString& key, const QStringList& entries )
{
	auto state = stateStore();
	const auto previouslyAdded = state.value( stateKey( key ) ).toStringList();

	// the 64-bit registry view is used by the browsers, also when we are a 32-bit process
	QSettings registry( key, QSettings::Registry64Format );

	QStringList existing;
	for( int i = 1; registry.contains( QString::number( i ) ); ++i )
	{
		existing.append( registry.value( QString::number( i ) ).toString() );
	}

	const auto merged = PolicyStore::mergedList( existing, previouslyAdded, entries );

	for( int i = 1; i <= existing.size(); ++i )
	{
		registry.remove( QString::number( i ) );
	}
	for( int i = 0; i < merged.size(); ++i )
	{
		registry.setValue( QString::number( i + 1 ), merged[i] );
	}
	registry.sync();

	if( entries.isEmpty() )
	{
		state.remove( stateKey( key ) );
	}
	else
	{
		state.setValue( stateKey( key ), entries );
	}

	return registry.status() == QSettings::NoError;
}



bool PolicyStore::apply( const WebPolicy::ChromiumPolicies& chromium, const WebPolicy::FirefoxPolicies& firefox )
{
	bool success = true;

	for( const auto& key : chromiumPolicyKeys() )
	{
		success &= writeListPolicy( key + QStringLiteral("\\URLBlocklist"), chromium.blocklist );
		success &= writeListPolicy( key + QStringLiteral("\\URLAllowlist"), chromium.allowlist );
	}

	success &= writeListPolicy( FirefoxFilterKey + QStringLiteral("\\Block"), firefox.block );
	success &= writeListPolicy( FirefoxFilterKey + QStringLiteral("\\Exceptions"), firefox.exceptions );

	return success;
}



bool PolicyStore::clear()
{
	return apply( {}, {} );
}

#else

static const QStringList& chromiumPolicyDirectories()
{
	static const QStringList directories{
		QStringLiteral("/etc/opt/chrome/policies/managed"),
		QStringLiteral("/etc/chromium/policies/managed"),
		QStringLiteral("/etc/chromium-browser/policies/managed"),
		QStringLiteral("/etc/opt/edge/policies/managed"),
		QStringLiteral("/etc/brave/policies/managed")
	};
	return directories;
}

static const auto FirefoxPolicyFile = QStringLiteral("/etc/firefox/policies/policies.json");



static QString chromiumPolicyFileName()
{
	return VeyonCore::productSlug() + QStringLiteral("-webcontrol.json");
}



static bool writeFile( const QString& fileName, const QByteArray& data )
{
	QDir().mkpath( QFileInfo( fileName ).absolutePath() );

	QFile file( fileName );
	if( file.open( QFile::WriteOnly | QFile::Truncate ) == false )
	{
		vWarning() << "could not write" << fileName << file.errorString();
		return false;
	}

	file.write( data );
	return true;
}



bool PolicyStore::apply( const WebPolicy::ChromiumPolicies& chromium, const WebPolicy::FirefoxPolicies& firefox )
{
	bool success = true;

	for( const auto& directory : chromiumPolicyDirectories() )
	{
		success &= writeFile( directory + QDir::separator() + chromiumPolicyFileName(),
							  WebPolicy::chromiumPolicyFile( chromium ) );
	}

	// Firefox reads a single policy file, so never replace one we did not create
	auto state = stateStore();
	const auto createdFirefoxPolicy = state.value( stateKey( FirefoxPolicyFile ) ).toBool();
	if( QFile::exists( FirefoxPolicyFile ) && createdFirefoxPolicy == false )
	{
		vWarning() << "not changing existing Firefox policies in" << FirefoxPolicyFile;
	}
	else if( writeFile( FirefoxPolicyFile, WebPolicy::firefoxPolicyFile( firefox ) ) )
	{
		state.setValue( stateKey( FirefoxPolicyFile ), true );
	}
	else
	{
		success = false;
	}

	return success;
}



bool PolicyStore::clear()
{
	for( const auto& directory : chromiumPolicyDirectories() )
	{
		QFile::remove( directory + QDir::separator() + chromiumPolicyFileName() );
	}

	auto state = stateStore();
	if( state.value( stateKey( FirefoxPolicyFile ) ).toBool() )
	{
		QFile::remove( FirefoxPolicyFile );
		state.remove( stateKey( FirefoxPolicyFile ) );
	}

	return true;
}

#endif
