/*
 * HandIn.h - files students hand in to the teacher
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

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>
#include <QUuid>

#include <optional>

// one part of a handed-in file; files are sent in parts so that the
// connections stay responsive
struct HandInChunk
{
	QUuid fileId;
	QString fileName;
	int index{0};
	int count{0};
	QByteArray data;
};


namespace HandIn
{
	static constexpr qint64 MaxFileSize = 20 * 1024 * 1024;
	static constexpr int MaxFiles = 10;
	// below HandInQueue::MaxQueuedBytes so that nothing is dropped before it was sent
	static constexpr qint64 MaxTotalSize = 50 * 1024 * 1024;
	static constexpr int ChunkSize = 256 * 1024;

	QList<HandInChunk> split( const QString& fileName, const QByteArray& data, int chunkSize = ChunkSize );

	// the file name without folders and characters Windows does not allow
	QString safeFileName( const QString& fileName );

	// a path in directory that does not exist yet: "name.ext", "name (2).ext", ...
	QString uniqueFilePath( const QString& directory, const QString& fileName );
}


// teacher side: puts the parts of the files of all students together again
class HandInAssembler
{
public:
	struct File
	{
		QString fileName;
		QByteArray data;
	};

	// returns the file when its last part arrived; source identifies the computer
	std::optional<File> add( const QString& source, const HandInChunk& chunk );

	int pendingFiles() const
	{
		return int(m_partial.size());
	}

private:
	struct Partial
	{
		QString fileName;
		int count{0};
		QMap<int, QByteArray> chunks;
	};

	QMap<QString, Partial> m_partial;

};


// student's computer: keeps the parts until every teacher computer got them
class HandInQueue
{
public:
	static constexpr qint64 MaxQueuedBytes = 64 * 1024 * 1024;
	static constexpr int KeepSeconds = 10 * 60;

	void append( const HandInChunk& chunk, const QDateTime& time = QDateTime::currentDateTimeUtc() );

	// parts with a sequence number above after, oldest first
	QList<QPair<qint64, HandInChunk>> chunksAfter( qint64 after, int maximum ) const;

	qint64 lastSequence() const
	{
		return m_lastSequence;
	}

	qint64 queuedBytes() const
	{
		return m_queuedBytes;
	}

	void purge( const QDateTime& now = QDateTime::currentDateTimeUtc() );

private:
	struct Entry
	{
		qint64 sequence;
		QDateTime time;
		HandInChunk chunk;
	};

	QList<Entry> m_entries;
	qint64 m_lastSequence{0};
	qint64 m_queuedBytes{0};

};
