/*
 * ClassChatWindows.cpp - windows of the hand raise and chat feature
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

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBrowser>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include "BrandTheme.h"
#include "ClassChatWindows.h"


static QPoint globalPosition( const QMouseEvent* event )
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return event->globalPosition().toPoint();
#else
	return event->globalPos();
#endif
}



// isolates a name or number from the surrounding text direction
static QString isolated( const QString& text )
{
	return QStringLiteral("\u2068") + text + QStringLiteral("\u2069");
}



void ChatView::appendMessage( QTextBrowser* browser, const ChatMessage& message,
							  const QString& teacherName, const QString& studentName )
{
	const auto name = message.fromTeacher ? teacherName : studentName;
	const auto nameColor = QLatin1String( message.fromTeacher ? BrandTheme::TealDark : BrandTheme::Orange );
	const auto direction = message.text.isRightToLeft() ? QStringLiteral("rtl") : QStringLiteral("ltr");

	auto text = message.text.toHtmlEscaped();
	text.replace( QLatin1Char('\n'), QStringLiteral("<br/>") );

	browser->append( QStringLiteral("<p dir=\"%1\"><b style=\"color:%2\">%3</b> "
									"<span style=\"color:%4\">%5</span><br/>%6</p>")
						 .arg( direction, nameColor, isolated( name ).toHtmlEscaped(),
							   QLatin1String( BrandTheme::Muted ),
							   isolated( QLocale().toString( message.time.time(), QLocale::ShortFormat ) ),
							   text ) );
	browser->verticalScrollBar()->setValue( browser->verticalScrollBar()->maximum() );
}



StudentToolbar::StudentToolbar( QWidget* parent ) :
	QWidget( parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus ),
	m_handButton( new QToolButton( this ) ),
	m_chatButton( new QToolButton( this ) ),
	m_handInButton( new QToolButton( this ) )
{
	setAttribute( Qt::WA_TranslucentBackground );
	setAttribute( Qt::WA_ShowWithoutActivating );
	setWindowTitle( tr( "Hand raise and chat" ) );

	for( auto button : { m_handButton, m_chatButton, m_handInButton } )
	{
		button->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
		button->setIconSize( QSize( 24, 24 ) );
		button->setAutoRaise( true );
		button->setFocusPolicy( Qt::NoFocus );
	}

	m_handButton->setCheckable( true );
	m_handButton->setIcon( QIcon( QStringLiteral(":/classchat/hand-raised.png") ) );
	m_chatButton->setIcon( QIcon( QStringLiteral(":/classchat/classchat.png") ) );
	m_chatButton->setText( tr( "Chat with the teacher" ) );
	m_handInButton->setIcon( QIcon( QStringLiteral(":/classchat/hand-in.png") ) );
	m_handInButton->setText( tr( "Hand in work" ) );
	m_handInButton->setFocusPolicy( Qt::NoFocus );
	updateHandButton();

	connect( m_handButton, &QToolButton::toggled, this, [this]( bool raised ) {
		updateHandButton();
		Q_EMIT handRaisedChanged( raised );
	} );
	connect( m_chatButton, &QToolButton::clicked, this, &StudentToolbar::chatRequested );
	connect( m_handInButton, &QToolButton::clicked, this, &StudentToolbar::handInRequested );

	auto layout = new QHBoxLayout( this );
	layout->setContentsMargins( 10, 4, 10, 4 );
	layout->addWidget( m_handButton );
	layout->addWidget( m_chatButton );
	layout->addWidget( m_handInButton );

	adjustSize();

	// top center of the primary screen, below the title bars of maximized windows' edges
	if( const auto screen = QGuiApplication::primaryScreen() )
	{
		const auto area = screen->availableGeometry();
		move( area.center().x() - width() / 2, area.top() + 4 );
	}
}



void StudentToolbar::setHandRaised( bool raised )
{
	if( m_handButton->isChecked() != raised )
	{
		const QSignalBlocker blocker( m_handButton );
		m_handButton->setChecked( raised );
		updateHandButton();
	}
}



void StudentToolbar::mousePressEvent( QMouseEvent* event )
{
	if( event->button() == Qt::LeftButton )
	{
		m_dragOffset = globalPosition( event ) - frameGeometry().topLeft();
	}
	QWidget::mousePressEvent( event );
}



void StudentToolbar::mouseMoveEvent( QMouseEvent* event )
{
	if( event->buttons() & Qt::LeftButton )
	{
		move( globalPosition( event ) - m_dragOffset );
	}
	QWidget::mouseMoveEvent( event );
}



void StudentToolbar::paintEvent( QPaintEvent* event )
{
	Q_UNUSED(event)

	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.setPen( QPen( BrandTheme::color( BrandTheme::TealDark ), 2 ) );
	painter.setBrush( BrandTheme::color( BrandTheme::Paper ) );
	painter.drawRoundedRect( rect().adjusted( 1, 1, -1, -1 ), height() / 2.0 - 1, height() / 2.0 - 1 );
}



void StudentToolbar::updateHandButton()
{
	m_handButton->setText( m_handButton->isChecked() ? tr( "Lower hand" ) : tr( "Raise hand" ) );
	adjustSize();
}



StudentChatWindow::StudentChatWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window | Qt::WindowStaysOnTopHint ),
	m_view( new QTextBrowser( this ) ),
	m_input( new QLineEdit( this ) )
{
	setWindowTitle( tr( "Chat with the teacher" ) );
	setWindowIcon( QIcon( QStringLiteral(":/classchat/classchat.png") ) );
	resize( 420, 480 );

	m_input->setPlaceholderText( tr( "Write a message to the teacher" ) );
	m_input->setMaxLength( ChatLog::MaxTextLength );

	auto sendButton = new QPushButton( tr( "Send" ), this );
	connect( sendButton, &QPushButton::clicked, this, &StudentChatWindow::send );
	connect( m_input, &QLineEdit::returnPressed, this, &StudentChatWindow::send );

	auto inputLayout = new QHBoxLayout;
	inputLayout->addWidget( m_input, 1 );
	inputLayout->addWidget( sendButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( m_view, 1 );
	layout->addLayout( inputLayout );
}



void StudentChatWindow::addMessage( const ChatMessage& message )
{
	ChatView::appendMessage( m_view, message, tr( "Teacher" ), tr( "Me" ) );
}



void StudentChatWindow::send()
{
	const auto text = ChatLog::cleanText( m_input->text() );
	if( text.isEmpty() )
	{
		return;
	}

	m_input->clear();

	ChatMessage message;
	message.text = text;
	message.time = QDateTime::currentDateTime();
	addMessage( message );

	Q_EMIT messageSent( text );
}



TeacherChatWindow::TeacherChatWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_list( new QListWidget( this ) ),
	m_title( new QLabel( this ) ),
	m_view( new QTextBrowser( this ) ),
	m_input( new QLineEdit( this ) ),
	m_sendButton( new QPushButton( tr( "Send" ), this ) ),
	m_sendAllButton( new QPushButton( tr( "Send to all" ), this ) ),
	m_lowerHandButton( new QPushButton( QIcon( QStringLiteral(":/classchat/hand-raised.png") ), tr( "Lower hand" ), this ) ),
	m_handInFolderButton( new QPushButton( QIcon( QStringLiteral(":/classchat/hand-in.png") ), tr( "Open handed-in work" ), this ) )
{
	setWindowTitle( tr( "Raised hands and chat" ) );
	setWindowIcon( QIcon( QStringLiteral(":/classchat/classchat.png") ) );
	resize( 760, 520 );

	m_list->setIconSize( QSize( 20, 20 ) );
	m_list->setMinimumWidth( 220 );

	auto titleFont = m_title->font();
	titleFont.setBold( true );
	m_title->setFont( titleFont );

	m_input->setPlaceholderText( tr( "Write a message" ) );
	m_input->setMaxLength( ChatLog::MaxTextLength );
	m_sendAllButton->setToolTip( tr( "Send the message to all students in the list" ) );

	connect( m_list, &QListWidget::currentItemChanged, this, &TeacherChatWindow::showConversation );
	connect( m_sendButton, &QPushButton::clicked, this, [this]() { send( false ); } );
	connect( m_sendAllButton, &QPushButton::clicked, this, [this]() { send( true ); } );
	connect( m_input, &QLineEdit::returnPressed, this, [this]() { send( false ); } );
	connect( m_lowerHandButton, &QPushButton::clicked, this, [this]() {
		if( const auto key = currentKey(); key.isEmpty() == false )
		{
			Q_EMIT lowerHandRequested( key );
		}
	} );

	m_handInFolderButton->setVisible( false );
	connect( m_handInFolderButton, &QPushButton::clicked, this, [this]() {
		QDesktopServices::openUrl( QUrl::fromLocalFile( m_handInFolder ) );
	} );

	auto header = new QHBoxLayout;
	header->addWidget( m_title, 1 );
	header->addWidget( m_handInFolderButton );
	header->addWidget( m_lowerHandButton );

	auto inputLayout = new QHBoxLayout;
	inputLayout->addWidget( m_input, 1 );
	inputLayout->addWidget( m_sendButton );
	inputLayout->addWidget( m_sendAllButton );

	auto conversation = new QWidget( this );
	auto conversationLayout = new QVBoxLayout( conversation );
	conversationLayout->setContentsMargins( 0, 0, 0, 0 );
	conversationLayout->addLayout( header );
	conversationLayout->addWidget( m_view, 1 );
	conversationLayout->addLayout( inputLayout );

	auto splitter = new QSplitter( this );
	splitter->addWidget( m_list );
	splitter->addWidget( conversation );
	splitter->setStretchFactor( 1, 1 );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( splitter );

	updateButtons();
}



void TeacherChatWindow::addComputer( const QString& key, const QString& computerName, const QString& userName )
{
	auto& conversation = m_conversations[key];
	conversation.computerName = computerName;
	conversation.userName = userName;

	if( conversation.item == nullptr )
	{
		conversation.item = new QListWidgetItem( m_list );
		conversation.item->setData( Qt::UserRole, key );
	}

	updateItem( key );

	if( m_list->currentItem() == nullptr )
	{
		m_list->setCurrentItem( conversation.item );
	}
	else if( m_list->currentItem() == conversation.item )
	{
		m_title->setText( conversation.item->text() );
	}

	updateButtons();
}



void TeacherChatWindow::setHandRaised( const QString& key, bool raised )
{
	if( m_conversations.contains( key ) == false || m_conversations[key].handRaised == raised )
	{
		return;
	}

	m_conversations[key].handRaised = raised;
	updateItem( key );
	updateButtons();

	if( raised )
	{
		notify();
	}
}



void TeacherChatWindow::addMessages( const QString& key, const ChatMessageList& messages )
{
	if( m_conversations.contains( key ) == false || messages.isEmpty() )
	{
		return;
	}

	auto& conversation = m_conversations[key];
	auto newStudentMessage = false;
	for( const auto& message : messages )
	{
		conversation.messages.append( message );
		if( key == currentKey() )
		{
			ChatView::appendMessage( m_view, message, tr( "Me" ), conversation.userName );
		}
		if( message.fromTeacher == false )
		{
			newStudentMessage = true;
			if( key != currentKey() || isActiveWindow() == false )
			{
				conversation.unread++;
			}
		}
	}

	updateItem( key );

	if( newStudentMessage )
	{
		notify();
	}
}



void TeacherChatWindow::clearConversation( const QString& key )
{
	if( m_conversations.contains( key ) )
	{
		m_conversations[key].messages.clear();
		m_conversations[key].unread = 0;
		updateItem( key );
		if( key == currentKey() )
		{
			showConversation();
		}
	}
}



void TeacherChatWindow::setHandInFolder( const QString& folder )
{
	m_handInFolder = folder;
	m_handInFolderButton->setVisible( folder.isEmpty() == false );
	m_handInFolderButton->setToolTip( QDir::toNativeSeparators( folder ) );
}



void TeacherChatWindow::notify()
{
	if( isVisible() == false )
	{
		setAttribute( Qt::WA_ShowWithoutActivating );
		show();
		setAttribute( Qt::WA_ShowWithoutActivating, false );
	}
	QApplication::alert( this );
}



QString TeacherChatWindow::currentKey() const
{
	return m_list->currentItem() ? m_list->currentItem()->data( Qt::UserRole ).toString() : QString{};
}



void TeacherChatWindow::showConversation()
{
	m_view->clear();

	const auto key = currentKey();
	if( m_conversations.contains( key ) == false )
	{
		m_title->clear();
		updateButtons();
		return;
	}

	auto& conversation = m_conversations[key];
	conversation.unread = 0;
	updateItem( key );

	m_title->setText( conversation.item->text() );
	for( const auto& message : std::as_const( conversation.messages ) )
	{
		ChatView::appendMessage( m_view, message, tr( "Me" ), conversation.userName );
	}

	updateButtons();
	m_input->setFocus();
}



void TeacherChatWindow::updateItem( const QString& key )
{
	const auto& conversation = m_conversations[key];
	if( conversation.item == nullptr )
	{
		return;
	}

	auto text = conversation.userName.isEmpty() ? conversation.computerName
												: QStringLiteral("%1 – %2").arg( isolated( conversation.userName ),
																					  isolated( conversation.computerName ) );
	if( conversation.unread > 0 )
	{
		text += QStringLiteral(" (%1)").arg( isolated( QString::number( conversation.unread ) ) );
	}

	conversation.item->setText( text );
	conversation.item->setIcon( conversation.handRaised ? QIcon( QStringLiteral(":/classchat/hand-raised.png") ) : QIcon() );
	conversation.item->setToolTip( conversation.handRaised ? tr( "This student raised the hand." ) : QString{} );

	auto font = conversation.item->font();
	font.setBold( conversation.unread > 0 || conversation.handRaised );
	conversation.item->setFont( font );
}



void TeacherChatWindow::updateButtons()
{
	const auto key = currentKey();
	m_sendButton->setEnabled( key.isEmpty() == false );
	m_sendAllButton->setEnabled( m_conversations.isEmpty() == false );
	m_lowerHandButton->setVisible( m_conversations.contains( key ) && m_conversations[key].handRaised );
}



void TeacherChatWindow::send( bool toAll )
{
	const auto text = ChatLog::cleanText( m_input->text() );
	if( text.isEmpty() )
	{
		return;
	}

	const auto keys = toAll ? m_conversations.keys() : QStringList{ currentKey() };
	if( keys.isEmpty() || keys.first().isEmpty() )
	{
		return;
	}

	m_input->clear();
	Q_EMIT sendRequested( keys, text );
}
