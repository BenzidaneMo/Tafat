/*
 * InventoryWindow.cpp - table with the inventory of the student computers
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

#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include "InventoryWindow.h"
#include "SystemInventory.h"


static constexpr int KeyRole = Qt::UserRole;


// sorts numbers by value instead of text
class InventoryItem : public QTableWidgetItem
{
public:
	using QTableWidgetItem::QTableWidgetItem;

	bool operator<( const QTableWidgetItem& other ) const override
	{
		const auto a = data( Qt::UserRole + 1 );
		const auto b = other.data( Qt::UserRole + 1 );
		if( a.isValid() && b.isValid() )
		{
			return a.toLongLong() < b.toLongLong();
		}
		return QTableWidgetItem::operator<( other );
	}
};



InventoryWindow::InventoryWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_table( new QTableWidget( 0, ColumnCount, this ) ),
	m_summary( new QLabel( this ) )
{
	setWindowTitle( tr( "Computer inventory" ) );
	setWindowIcon( QIcon( QStringLiteral(":/inventory/inventory.png") ) );
	resize( 1000, 520 );

	m_table->setHorizontalHeaderLabels( headers() );
	m_table->setEditTriggers( QTableWidget::NoEditTriggers );
	m_table->setSelectionBehavior( QTableWidget::SelectRows );
	m_table->setAlternatingRowColors( true );
	m_table->verticalHeader()->hide();
	m_table->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
	m_table->horizontalHeader()->setStretchLastSection( true );
	m_table->setSortingEnabled( true );
	m_table->sortByColumn( ComputerColumn, Qt::AscendingOrder );

	auto refreshButton = new QPushButton( tr( "Refresh" ), this );
	auto exportButton = new QPushButton( tr( "Export CSV…" ), this );
	auto closeButton = new QPushButton( tr( "Close" ), this );

	connect( refreshButton, &QPushButton::clicked, this, &InventoryWindow::refreshRequested );
	connect( exportButton, &QPushButton::clicked, this, &InventoryWindow::exportCsv );
	connect( closeButton, &QPushButton::clicked, this, &QWidget::close );

	auto actions = new QHBoxLayout;
	actions->addWidget( m_summary, 1 );
	actions->addWidget( refreshButton );
	actions->addWidget( exportButton );
	actions->addWidget( closeButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( m_table, 1 );
	layout->addLayout( actions );
}



void InventoryWindow::setComputer( const QString& key, const QString& computerName, const QString& userName )
{
	auto row = rowOf( key );
	if( row < 0 )
	{
		m_table->setSortingEnabled( false );
		row = m_table->rowCount();
		m_table->insertRow( row );
		setCell( row, ComputerColumn, computerName );
		m_table->item( row, ComputerColumn )->setData( KeyRole, key );
		for( int column = UserColumn; column < ColumnCount; ++column )
		{
			setCell( row, column, {} );
		}
		m_table->setSortingEnabled( true );
		row = rowOf( key );
	}

	m_table->item( row, UserColumn )->setText( userName );
	updateSummary();
}



void InventoryWindow::setInventory( const QString& key, const QVariantMap& inventory )
{
	auto row = rowOf( key );
	if( row < 0 )
	{
		return;
	}

	m_inventories[key] = inventory;

	const auto value = [&inventory]( const char* field ) {
		return inventory.value( QLatin1String(field) );
	};

	m_table->setSortingEnabled( false );

	setCell( row, SystemColumn, SystemInventory::describeSystem( inventory ) );
	setCell( row, ProcessorColumn, value( SystemInventory::Processor ).toString() );
	setCell( row, CoresColumn, value( SystemInventory::Cores ).toString(), value( SystemInventory::Cores ).toLongLong() );
	setCell( row, MemoryColumn, SystemInventory::formatMegabytes( value( SystemInventory::MemoryMB ).toLongLong() ),
			 value( SystemInventory::MemoryMB ).toLongLong() );

	const auto diskFree = value( SystemInventory::DiskFreeMB ).toLongLong();
	const auto diskTotal = value( SystemInventory::DiskTotalMB ).toLongLong();
	setCell( row, DiskColumn,
			 diskTotal > 0 ? tr( "%1 free of %2" ).arg( SystemInventory::formatMegabytes( diskFree ),
														SystemInventory::formatMegabytes( diskTotal ) )
						   : QString{},
			 diskFree );

	setCell( row, IpColumn, value( SystemInventory::IpAddresses ).toString() );
	setCell( row, MacColumn, value( SystemInventory::MacAddresses ).toString() );

	auto version = value( SystemInventory::ProductVersion ).toString();
	if( value( SystemInventory::QtVersion ).toString().startsWith( QLatin1Char('5') ) )
	{
		version = tr( "%1 (legacy build)" ).arg( version );
	}
	setCell( row, VersionColumn, version );

	m_table->setSortingEnabled( true );

	updateSummary();
}



void InventoryWindow::clear()
{
	m_table->setRowCount( 0 );
	m_inventories.clear();
	updateSummary();
}



QStringList InventoryWindow::headers() const
{
	return { tr( "Computer" ), tr( "User" ), tr( "Operating system" ), tr( "Processor" ), tr( "Cores" ),
			 tr( "Memory" ), tr( "Disk" ), tr( "IP address" ), tr( "MAC address" ), tr( "Version" ) };
}



QList<QStringList> InventoryWindow::rows() const
{
	QList<QStringList> rows;
	for( int row = 0; row < m_table->rowCount(); ++row )
	{
		QStringList fields;
		for( int column = 0; column < ColumnCount; ++column )
		{
			fields.append( m_table->item( row, column ) ? m_table->item( row, column )->text() : QString{} );
		}
		rows.append( fields );
	}
	return rows;
}



int InventoryWindow::rowOf( const QString& key ) const
{
	for( int row = 0; row < m_table->rowCount(); ++row )
	{
		const auto item = m_table->item( row, ComputerColumn );
		if( item && item->data( KeyRole ).toString() == key )
		{
			return row;
		}
	}
	return -1;
}



void InventoryWindow::setCell( int row, int column, const QString& text, const QVariant& sortValue )
{
	auto item = m_table->item( row, column );
	if( item == nullptr )
	{
		item = new InventoryItem;
		m_table->setItem( row, column, item );
	}
	item->setText( text );
	item->setData( Qt::UserRole + 1, sortValue );
}



void InventoryWindow::updateSummary()
{
	m_summary->setText( tr( "%1 of %2 computers answered" ).arg( m_inventories.size() ).arg( m_table->rowCount() ) );
}



void InventoryWindow::exportCsv()
{
	const auto fileName = QFileDialog::getSaveFileName( this, tr( "Export inventory" ),
														tr( "inventory-%1.csv" ).arg( QDate::currentDate().toString( Qt::ISODate ) ),
														tr( "CSV files (*.csv)" ) );
	if( fileName.isEmpty() )
	{
		return;
	}

	QFile file( fileName );
	if( file.open( QFile::WriteOnly | QFile::Truncate | QFile::Text ) == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( fileName ) );
		return;
	}

	QTextStream stream( &file );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec( "UTF-8" );
#endif
	// UTF-8 byte order mark so that spreadsheet programs detect Arabic and Tifinagh text
	stream << QStringLiteral("\uFEFF");
	stream << SystemInventory::csvLine( headers() ) << QLatin1Char('\n');
	for( const auto& row : rows() )
	{
		stream << SystemInventory::csvLine( row ) << QLatin1Char('\n');
	}
}
