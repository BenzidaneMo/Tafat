/*
 * ChatLog.h - messages between the teacher and a student
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

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

struct ChatMessage
{
	qint64 id{0};
	bool fromTeacher{false};
	QString text;
	QDateTime time;

	QJsonObject toJson() const;
	static ChatMessage fromJson( const QJsonObject& object );

	bool operator==( const ChatMessage& other ) const
	{
		return id == other.id && fromTeacher == other.fromTeacher && text == other.text;
	}
};

using ChatMessageList = QList<ChatMessage>;


// Messages of one conversation with increasing IDs. Only the newest
// MaxMessages messages are kept.
class ChatLog
{
public:
	static constexpr int MaxMessages = 200;
	static constexpr int MaxTextLength = 2000;

	// trims and shortens a message text; returns an empty string for blank texts
	static QString cleanText( const QString& text );

	// appends a message and returns it; blank texts are not added (returned ID 0)
	ChatMessage append( bool fromTeacher, const QString& text,
						const QDateTime& time = QDateTime::currentDateTime() );

	// adds a message received from elsewhere, keeping its ID; returns false
	// for messages that are already known
	bool add( const ChatMessage& message );

	ChatMessageList messagesAfter( qint64 id ) const;

	const ChatMessageList& messages() const
	{
		return m_messages;
	}

	qint64 lastId() const
	{
		return m_lastId;
	}

	void clear()
	{
		m_messages.clear();
	}

	static QJsonArray toJson( const ChatMessageList& messages );
	static ChatMessageList fromJson( const QJsonArray& array );

private:
	void trim();

	ChatMessageList m_messages;
	qint64 m_lastId{0};

};
