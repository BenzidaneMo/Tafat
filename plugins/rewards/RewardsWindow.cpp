/*
 * RewardsWindow.cpp - stars of the students on the teacher computer
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

#include <QComboBox>
#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include "BrandTheme.h"
#include "RewardsWindow.h"
#include "VeyonCore.h"


RewardsWindow::RewardsWindow( QWidget* parent ) :
	QWidget( parent ),
	m_classComboBox( new QComboBox( this ) ),
	m_removeClassButton( new QPushButton( tr( "Delete class" ), this ) ),
	m_table( new QTableWidget( 0, 2, this ) ),
	m_summary( new QLabel( this ) )
{
	setWindowTitle( tr( "Stars" ) );
	setWindowIcon( QIcon( QStringLiteral(":/rewards/rewards.png") ) );
	resize( 460, 520 );

	m_table->setHorizontalHeaderLabels( { tr( "Student" ), tr( "Stars" ) } );
	m_table->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
	m_table->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
	m_table->verticalHeader()->hide();
	m_table->setEditTriggers( QAbstractItemView::NoEditTriggers );
	m_table->setSelectionBehavior( QAbstractItemView::SelectRows );

	auto removeButton = new QPushButton( tr( "Remove a star" ), this );
	auto resetButton = new QPushButton( tr( "Start again" ), this );
	auto exportButton = new QPushButton( tr( "Export (CSV)" ), this );

	auto buttons = new QHBoxLayout;
	buttons->addWidget( removeButton );
	buttons->addWidget( resetButton );
	buttons->addStretch( 1 );
	buttons->addWidget( exportButton );

	auto addClassButton = new QPushButton( tr( "New class" ), this );
	m_classComboBox->setSizeAdjustPolicy( QComboBox::AdjustToContents );
	m_classComboBox->setToolTip( tr( "Each class has its own stars. Choose the class you are teaching now." ) );

	auto classLayout = new QHBoxLayout;
	classLayout->addWidget( new QLabel( tr( "Class:" ), this ) );
	classLayout->addWidget( m_classComboBox, 1 );
	classLayout->addWidget( addClassButton );
	classLayout->addWidget( m_removeClassButton );

	auto layout = new QVBoxLayout( this );
	layout->addLayout( classLayout );
	layout->addWidget( m_summary );
	layout->addWidget( m_table, 1 );
	layout->addLayout( buttons );

	connect( removeButton, &QPushButton::clicked, this, [this]() {
		const auto item = m_table->item( m_table->currentRow(), 0 );
		if( item )
		{
			Q_EMIT removeStarRequested( item->text() );
		}
	} );
	connect( resetButton, &QPushButton::clicked, this, [this]() {
		if( QMessageBox::question( this, windowTitle(), tr( "Remove the stars of all students of this class?" ) ) == QMessageBox::Yes )
		{
			Q_EMIT resetRequested();
		}
	} );
	connect( exportButton, &QPushButton::clicked, this, &RewardsWindow::exportCsv );
	connect( addClassButton, &QPushButton::clicked, this, &RewardsWindow::addClass );
	connect( m_classComboBox, QOverload<int>::of( &QComboBox::activated ), this, [this]( int index ) {
		Q_EMIT classSelected( m_classComboBox->itemText( index ) );
	} );
	connect( m_removeClassButton, &QPushButton::clicked, this, [this]() {
		if( QMessageBox::question( this, windowTitle(),
								   tr( "Delete the class %1 and its stars?" ).arg( QStringLiteral("\u2068%1\u2069").arg( m_className ) ) )
			== QMessageBox::Yes )
		{
			Q_EMIT removeClassRequested( m_className );
		}
	} );
}



void RewardsWindow::setClasses( const RewardClasses& classes )
{
	const QSignalBlocker blocker( m_classComboBox );
	m_classComboBox->clear();
	m_classComboBox->addItems( classes.classes() );
	m_classComboBox->setCurrentText( classes.currentClass() );
	m_removeClassButton->setEnabled( m_classComboBox->count() > 1 );

	if( m_className != classes.currentClass() )
	{
		m_table->setCurrentCell( -1, -1 );
	}
	m_className = classes.currentClass();
	setBook( classes.book() );
}



void RewardsWindow::addClass()
{
	const auto name = QInputDialog::getText( this, tr( "New class" ), tr( "Name of the class, e.g. 2AS1:" ) ).simplified();
	if( name.isEmpty() == false )
	{
		Q_EMIT classSelected( name );
	}
}



void RewardsWindow::setBook( const RewardBook& book )
{
	const auto current = m_table->currentRow();
	m_book = book;

	const auto students = book.students();
	m_table->setRowCount( int( students.size() ) );
	int total = 0;
	auto starFont = m_table->font();
	starFont.setPointSizeF( starFont.pointSizeF() * 1.4 );
	for( int row = 0; row < students.size(); ++row )
	{
		const auto stars = book.stars( students[row] );
		total += stars;
		m_table->setItem( row, 0, new QTableWidgetItem( students[row] ) );
		const QChar star( 0x2605 );
		auto starsItem = new QTableWidgetItem( stars <= 10 ? QString( stars, star )
														   : QStringLiteral("%1 %2").arg( stars ).arg( star ) );
		starsItem->setData( Qt::ToolTipRole, stars );
		starsItem->setForeground( BrandTheme::color( BrandTheme::Hot ) );
		starsItem->setFont( starFont );
		m_table->setItem( row, 1, starsItem );
	}
	m_table->resizeRowsToContents();
	m_table->setCurrentCell( qMin( current, m_table->rowCount() - 1 ), 0 );

	m_summary->setText( book.isEmpty() ? tr( "No stars yet. Select computers and click \"Give a star\"." )
									   : tr( "Stars given: %1" ).arg( total ) );
}



void RewardsWindow::exportCsv()
{
	auto safeClassName = m_className;
	safeClassName.replace( QRegularExpression( QStringLiteral("[\\\\/:*?\"<>|]") ), QStringLiteral("-") );

	const auto fileName = QFileDialog::getSaveFileName( this, tr( "Export stars" ),
														tr( "stars-%1-%2.csv" ).arg( safeClassName, QDate::currentDate().toString( Qt::ISODate ) ),
														tr( "CSV files (*.csv)" ) );
	if( fileName.isEmpty() )
	{
		return;
	}

	QFile file( fileName );
	if( file.open( QFile::WriteOnly | QFile::Truncate ) == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( fileName ) );
		return;
	}

	// UTF-8 byte order mark so that spreadsheet programs detect Arabic and Tifinagh text
	file.write( "\xEF\xBB\xBF" );
	file.write( m_book.toCsv( tr( "Student" ), tr( "Stars" ) ).toUtf8() );
}
