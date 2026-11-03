/*
 * HandIn.cpp - files students hand in to the teacher
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
#include <QFileInfo>
#include <QRegularExpression>

#include "HandIn.h"


QList<HandInChunk> HandIn::split( const QString& fileName, const QByteArray& data, int chunkSize )
{
	const auto fileId = QUuid::createUuid();
	const auto count = qMax( 1, int( ( data.size() + chunkSize - 1 ) / chunkSize ) );

	QList<HandInChunk> chunks;
	chunks.reserve( count );
	for( int i = 0; i < count; ++i )
	{
		chunks.append( { fileId, fileName, i, count, data.mid( qsizetype(i) * chunkSize, chunkSize ) } );
	}
	return chunks;
}



QString HandIn::safeFileName( const QString& fileName )
{
	static const QRegularExpression invalidCharacters( QStringLiteral("[\\x00-\\x1F<>:\"/\\\\|?*\\x7F]") );

	// only the last part of a path, with "/" or "\\" as separator
	auto name = fileName;
	name.replace( QLatin1Char('\\'), QLatin1Char('/') );
	name = name.section( QLatin1Char('/'), -1 );
	name.remove( invalidCharacters );
	name = name.trimmed();

	// no hidden files and no "." or ".."
	while( name.startsWith( QLatin1Char('.') ) )
	{
		name.remove( 0, 1 );
	}
	// Windows ignores trailing dots and spaces
	while( name.endsWith( QLatin1Char('.') ) || name.endsWith( QLatin1Char(' ') ) )
	{
		name.chop( 1 );
	}

	if( name.size() > 150 )
	{
		const auto suffix = QFileInfo( name ).suffix().left( 10 );
		name = name.left( 140 ) + ( suffix.isEmpty() ? QString{} : QLatin1Char('.') + suffix );
	}

	return name.isEmpty() ? QStringLiteral("file") : name;
}



QString HandIn::uniqueFilePath( const QString& directory, const QString& fileName )
{
	const QDir dir( directory );
	if( dir.exists( fileName ) == false )
	{
		return dir.filePath( fileName );
	}

	const QFileInfo info( fileName );
	const auto baseName = info.completeBaseName();
	const auto suffix = info.suffix().isEmpty() ? QString{} : QLatin1Char('.') + info.suffix();

	for( int i = 2; ; ++i )
	{
		const auto candidate = QStringLiteral("%1 (%2)%3").arg( baseName ).arg( i ).arg( suffix );
		if( dir.exists( candidate ) == false )
		{
			return dir.filePath( candidate );
		}
	}
}



std::optional<HandInAssembler::File> HandInAssembler::add( const QString& source, const HandInChunk& chunk )
{
	if( chunk.count <= 0 || chunk.index < 0 || chunk.index >= chunk.count ||
		chunk.count > HandIn::MaxFileSize / HandIn::ChunkSize + 1 )
	{
		return std::nullopt;
	}

	const QString key = source + QLatin1Char('/') + chunk.fileId.toString();
	auto& partial = m_partial[key];
	if( partial.count == 0 )
	{
		partial.count = chunk.count;
		partial.fileName = chunk.fileName;
	}
	else if( partial.count != chunk.count )
	{
		// inconsistent parts: give up this file
		m_partial.remove( key );
		return std::nullopt;
	}

	partial.chunks[chunk.index] = chunk.data;

	if( partial.chunks.size() < partial.count )
	{
		return std::nullopt;
	}

	File file{ partial.fileName, {} };
	for( const auto& data : std::as_const( partial.chunks ) )
	{
		file.data.append( data );
	}
	m_partial.remove( key );

	return file;
}



void HandInQueue::append( const HandInChunk& chunk, const QDateTime& time )
{
	m_entries.append( { ++m_lastSequence, time, chunk } );
	m_queuedBytes += chunk.data.size();
	purge( time );
}



QList<QPair<qint64, HandInChunk>> HandInQueue::chunksAfter( qint64 after, int maximum ) const
{
	QList<QPair<qint64, HandInChunk>> chunks;
	for( const auto& entry : m_entries )
	{
		if( entry.sequence > after )
		{
			chunks.append( { entry.sequence, entry.chunk } );
			if( chunks.size() >= maximum )
			{
				break;
			}
		}
	}
	return chunks;
}



void HandInQueue::purge( const QDateTime& now )
{
	while( m_entries.isEmpty() == false &&
		   ( m_queuedBytes > MaxQueuedBytes || m_entries.first().time.secsTo( now ) > KeepSeconds ) )
	{
		m_queuedBytes -= m_entries.first().chunk.data.size();
		m_entries.removeFirst();
	}
}
