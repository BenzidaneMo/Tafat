/*
 * RunningAppsWindow.cpp - shows the applications open on the student computers
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

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "RunningAppsWindow.h"


static constexpr int KeyRole = Qt::UserRole;
static constexpr int ApplicationRole = Qt::UserRole + 1;



static QString isolated( const QString& text )
{
	return QStringLiteral("\u2068") + text + QStringLiteral("\u2069");
}



RunningAppsWindow::RunningAppsWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_tree( new QTreeWidget( this ) ),
	m_summary( new QLabel( this ) ),
	m_closeButton( new QPushButton( tr( "Close on this computer" ), this ) ),
	m_closeEverywhereButton( new QPushButton( tr( "Close on all computers" ), this ) )
{
	setWindowTitle( tr( "Running applications" ) );
	setWindowIcon( QIcon( QStringLiteral(":/appcontrol/application-control.png") ) );
	resize( 520, 560 );

	m_tree->setHeaderHidden( true );
	m_tree->setRootIsDecorated( true );
	m_tree->setSortingEnabled( false );

	auto refreshButton = new QPushButton( tr( "Refresh" ), this );
	auto closeWindowButton = new QPushButton( tr( "Close" ), this );

	connect( refreshButton, &QPushButton::clicked, this, &RunningAppsWindow::refreshRequested );
	connect( closeWindowButton, &QPushButton::clicked, this, &QWidget::close );
	connect( m_tree, &QTreeWidget::currentItemChanged, this, &RunningAppsWindow::updateButtons );
	connect( m_closeButton, &QPushButton::clicked, this, [this]() {
		const auto key = selectedKey();
		const auto application = selectedApplication();
		if( key.isEmpty() == false && application.isEmpty() == false )
		{
			Q_EMIT closeRequested( key, application );
		}
	} );
	connect( m_closeEverywhereButton, &QPushButton::clicked, this, [this]() {
		const auto application = selectedApplication();
		if( application.isEmpty() == false &&
			QMessageBox::question( this, windowTitle(),
								   tr( "Close %1 on all computers in the list? Unsaved work in it is lost." )
									   .arg( isolated( application ) ) ) == QMessageBox::Yes )
		{
			Q_EMIT closeEverywhereRequested( application );
		}
	} );

	m_refreshTimer.setInterval( RefreshInterval );
	connect( &m_refreshTimer, &QTimer::timeout, this, &RunningAppsWindow::refreshRequested );

	auto actions = new QHBoxLayout;
	actions->addWidget( m_closeButton );
	actions->addWidget( m_closeEverywhereButton );
	actions->addStretch();
	actions->addWidget( refreshButton );
	actions->addWidget( closeWindowButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( m_tree, 1 );
	layout->addWidget( m_summary );
	layout->addLayout( actions );

	updateButtons();
}



void RunningAppsWindow::setComputer( const QString& key, const QString& title )
{
	for( int i = 0; i < m_tree->topLevelItemCount(); ++i )
	{
		if( m_tree->topLevelItem( i )->data( 0, KeyRole ).toString() == key )
		{
			m_tree->topLevelItem( i )->setText( 0, title );
			return;
		}
	}

	auto item = new QTreeWidgetItem( m_tree, { title } );
	item->setData( 0, KeyRole, key );
	auto font = item->font( 0 );
	font.setBold( true );
	item->setFont( 0, font );
	item->setExpanded( true );
}



void RunningAppsWindow::setApplications( const QString& key, const QStringList& applications )
{
	QTreeWidgetItem* computerItem = nullptr;
	for( int i = 0; i < m_tree->topLevelItemCount(); ++i )
	{
		if( m_tree->topLevelItem( i )->data( 0, KeyRole ).toString() == key )
		{
			computerItem = m_tree->topLevelItem( i );
		}
	}
	if( computerItem == nullptr || ( m_applications.value( key ) == applications && computerItem->childCount() > 0 ) )
	{
		return;
	}

	m_applications[key] = applications;

	const auto selected = selectedKey() == key ? selectedApplication() : QString{};

	qDeleteAll( computerItem->takeChildren() );
	for( const auto& application : applications )
	{
		auto item = new QTreeWidgetItem( computerItem, { isolated( application ) } );
		item->setData( 0, KeyRole, key );
		item->setData( 0, ApplicationRole, application );
		if( application == selected )
		{
			m_tree->setCurrentItem( item );
		}
	}
	if( applications.isEmpty() )
	{
		auto item = new QTreeWidgetItem( computerItem, { tr( "(no open applications)" ) } );
		item->setData( 0, KeyRole, key );
		item->setDisabled( true );
	}
	computerItem->setExpanded( true );

	int total = 0;
	for( const auto& list : std::as_const( m_applications ) )
	{
		total += int(list.size());
	}
	m_summary->setText( tr( "%1 applications open on %2 computers" ).arg( total ).arg( m_applications.size() ) );

	updateButtons();
}



void RunningAppsWindow::clear()
{
	m_tree->clear();
	m_applications.clear();
	m_summary->clear();
	updateButtons();
}



void RunningAppsWindow::showEvent( QShowEvent* event )
{
	m_refreshTimer.start();
	QWidget::showEvent( event );
}



void RunningAppsWindow::hideEvent( QHideEvent* event )
{
	m_refreshTimer.stop();
	QWidget::hideEvent( event );
}



void RunningAppsWindow::updateButtons()
{
	const auto hasApplication = selectedApplication().isEmpty() == false;
	m_closeButton->setEnabled( hasApplication );
	m_closeEverywhereButton->setEnabled( hasApplication );
}



QString RunningAppsWindow::selectedKey() const
{
	return m_tree->currentItem() ? m_tree->currentItem()->data( 0, KeyRole ).toString() : QString{};
}



QString RunningAppsWindow::selectedApplication() const
{
	return m_tree->currentItem() ? m_tree->currentItem()->data( 0, ApplicationRole ).toString() : QString{};
}
