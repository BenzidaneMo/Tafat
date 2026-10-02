/*
 * ClassChatWindows.h - windows of the hand raise and chat feature
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

#include <QMap>
#include <QPoint>
#include <QWidget>

#include "ChatLog.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTextBrowser;
class QToolButton;


// appends messages to a text browser, one paragraph per message
class ChatView
{
public:
	static void appendMessage( QTextBrowser* browser, const ChatMessage& message,
							   const QString& teacherName, const QString& studentName );
};


// student side: small bar that always stays on top of the screen
class StudentToolbar : public QWidget
{
	Q_OBJECT
public:
	explicit StudentToolbar( QWidget* parent = nullptr );

	void setHandRaised( bool raised );

Q_SIGNALS:
	void handRaisedChanged( bool raised );
	void chatRequested();

protected:
	void mousePressEvent( QMouseEvent* event ) override;
	void mouseMoveEvent( QMouseEvent* event ) override;
	void paintEvent( QPaintEvent* event ) override;

private:
	void updateHandButton();

	QToolButton* m_handButton;
	QToolButton* m_chatButton;
	QPoint m_dragOffset;

};


// student side: conversation with the teacher
class StudentChatWindow : public QWidget
{
	Q_OBJECT
public:
	explicit StudentChatWindow( QWidget* parent = nullptr );

	void addMessage( const ChatMessage& message );

Q_SIGNALS:
	void messageSent( const QString& text );

private:
	void send();

	QTextBrowser* m_view;
	QLineEdit* m_input;

};


// teacher side: raised hands and conversations with all students
class TeacherChatWindow : public QWidget
{
	Q_OBJECT
public:
	explicit TeacherChatWindow( QWidget* parent = nullptr );

	void addComputer( const QString& key, const QString& computerName, const QString& userName );
	void setHandRaised( const QString& key, bool raised );
	void addMessages( const QString& key, const ChatMessageList& messages );
	void clearConversation( const QString& key );

	// shows the window without taking the focus from the teacher's work
	void notify();

Q_SIGNALS:
	void sendRequested( const QStringList& keys, const QString& text );
	void lowerHandRequested( const QString& key );

private:
	struct Conversation
	{
		QString computerName;
		QString userName;
		bool handRaised{false};
		int unread{0};
		ChatMessageList messages;
		QListWidgetItem* item{nullptr};
	};

	QString currentKey() const;
	void showConversation();
	void updateItem( const QString& key );
	void updateButtons();
	void send( bool toAll );

	QListWidget* m_list;
	QLabel* m_title;
	QTextBrowser* m_view;
	QLineEdit* m_input;
	QPushButton* m_sendButton;
	QPushButton* m_sendAllButton;
	QPushButton* m_lowerHandButton;

	QMap<QString, Conversation> m_conversations;

};
