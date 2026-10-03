/*
 * ChatLog.cpp - messages between the teacher and a student
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

#include "ChatLog.h"


QJsonObject ChatMessage::toJson() const
{
	return {
		{ QStringLiteral("id"), QString::number( id ) },
		{ QStringLiteral("teacher"), fromTeacher },
		{ QStringLiteral("text"), text },
		{ QStringLiteral("time"), time.toString( Qt::ISODate ) }
	};
}



ChatMessage ChatMessage::fromJson( const QJsonObject& object )
{
	ChatMessage message;
	message.id = object.value( QStringLiteral("id") ).toString().toLongLong();
	message.fromTeacher = object.value( QStringLiteral("teacher") ).toBool();
	message.text = ChatLog::cleanText( object.value( QStringLiteral("text") ).toString() );
	message.time = QDateTime::fromString( object.value( QStringLiteral("time") ).toString(), Qt::ISODate );
	return message;
}



QString ChatLog::cleanText( const QString& text )
{
	auto cleaned = text.trimmed();
	if( cleaned.size() > MaxTextLength )
	{
		cleaned.truncate( MaxTextLength );
	}
	return cleaned;
}



ChatMessage ChatLog::append( bool fromTeacher, const QString& text, const QDateTime& time )
{
	ChatMessage message;
	message.text = cleanText( text );
	if( message.text.isEmpty() )
	{
		return message;
	}

	message.id = ++m_lastId;
	message.fromTeacher = fromTeacher;
	message.time = time;

	m_messages.append( message );
	trim();

	return message;
}



bool ChatLog::add( const ChatMessage& message )
{
	if( message.id <= m_lastId || message.text.isEmpty() )
	{
		return false;
	}

	m_lastId = message.id;
	m_messages.append( message );
	trim();

	return true;
}



ChatMessageList ChatLog::messagesAfter( qint64 id ) const
{
	ChatMessageList messages;
	for( const auto& message : m_messages )
	{
		if( message.id > id )
		{
			messages.append( message );
		}
	}
	return messages;
}



QJsonArray ChatLog::toJson( const ChatMessageList& messages )
{
	QJsonArray array;
	for( const auto& message : messages )
	{
		array.append( message.toJson() );
	}
	return array;
}



ChatMessageList ChatLog::fromJson( const QJsonArray& array )
{
	ChatMessageList messages;
	for( const auto& value : array )
	{
		const auto message = ChatMessage::fromJson( value.toObject() );
		if( message.id > 0 && message.text.isEmpty() == false )
		{
			messages.append( message );
		}
	}
	return messages;
}



void ChatLog::trim()
{
	while( m_messages.size() > MaxMessages )
	{
		m_messages.removeFirst();
	}
}
