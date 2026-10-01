/*
 * RegisterWindows.cpp - attendance register windows for teacher and student
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

#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include "RegisterWindows.h"
#include "VeyonCore.h"


static QString isolated( const QString& text )
{
	// keep names in another script readable in right-to-left layouts
	return QStringLiteral("\u2068") + text + QStringLiteral("\u2069");
}



RegistrationDialog::RegistrationDialog( QWidget* parent ) :
	QDialog( parent, Qt::WindowStaysOnTopHint ),
	m_nameEdit( new QLineEdit( this ) ),
	m_groupEdit( new QLineEdit( this ) )
{
	setWindowTitle( tr( "Attendance" ) );
	setMinimumWidth( 420 );

	auto intro = new QLabel( tr( "Your teacher takes the attendance. Please enter your first and last name." ), this );
	intro->setWordWrap( true );

	m_nameEdit->setPlaceholderText( tr( "First and last name" ) );
	m_groupEdit->setPlaceholderText( tr( "e.g. 2AS2" ) );

	// the class usually stays the same on a computer, the name changes with every student
	m_groupEdit->setText( QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("Register") )
							  .value( QStringLiteral("Group") ).toString() );

	auto form = new QFormLayout;
	form->addRow( tr( "Name:" ), m_nameEdit );
	form->addRow( tr( "Class:" ), m_groupEdit );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( intro );
	layout->addLayout( form );
	layout->addWidget( buttonBox );
}



QString RegistrationDialog::studentName() const
{
	return m_nameEdit->text().simplified();
}



QString RegistrationDialog::group() const
{
	return m_groupEdit->text().simplified();
}



void RegistrationDialog::accept()
{
	if( studentName().size() < 2 )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please enter your name." ) );
		return;
	}

	QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("Register") )
		.setValue( QStringLiteral("Group"), group() );

	QDialog::accept();
}



RegisterWindow::RegisterWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_summaryLabel( new QLabel( this ) ),
	m_table( new QTableWidget( this ) )
{
	setWindowTitle( tr( "Attendance register: %1" ).arg( QLocale().toString( QDate::currentDate(), QLocale::LongFormat ) ) );
	resize( 720, 480 );
	setAttribute( Qt::WA_DeleteOnClose );

	m_table->setColumnCount( 4 );
	m_table->setHorizontalHeaderLabels( { tr( "Computer" ), tr( "Student" ), tr( "Class" ), tr( "Registered at" ) } );
	m_table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
	m_table->verticalHeader()->hide();
	m_table->setEditTriggers( QAbstractItemView::NoEditTriggers );
	m_table->setSortingEnabled( true );

	auto askAgainButton = new QPushButton( tr( "Ask again" ), this );
	auto exportButton = new QPushButton( tr( "Export (CSV)" ), this );
	auto closeButton = new QPushButton( tr( "Close" ), this );
	connect( askAgainButton, &QPushButton::clicked, this, &RegisterWindow::askAgainRequested );
	connect( exportButton, &QPushButton::clicked, this, &RegisterWindow::exportCsv );
	connect( closeButton, &QPushButton::clicked, this, &QWidget::close );

	auto buttons = new QHBoxLayout;
	buttons->addWidget( m_summaryLabel, 1 );
	buttons->addWidget( askAgainButton );
	buttons->addWidget( exportButton );
	buttons->addWidget( closeButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( m_table );
	layout->addLayout( buttons );

	refresh();
}



void RegisterWindow::addComputer( const QString& key, const QString& computerName )
{
	if( m_entries.contains( key ) == false )
	{
		m_order.append( key );
		m_entries[key].computerName = computerName;
		refresh();
	}
}



void RegisterWindow::setRegistration( const QString& key, const Registration& registration )
{
	if( m_entries.contains( key ) == false )
	{
		m_order.append( key );
	}
	m_entries[key] = registration;
	refresh();
}



void RegisterWindow::refresh()
{
	m_table->setSortingEnabled( false );
	m_table->setRowCount( int(m_order.size()) );

	int registered = 0;
	for( int row = 0; row < m_order.size(); ++row )
	{
		const auto& entry = m_entries[m_order[row]];
		registered += entry.studentName.isEmpty() ? 0 : 1;

		const QStringList cells{
			isolated( entry.computerName ),
			entry.studentName.isEmpty() ? tr( "(waiting)" ) : isolated( entry.studentName ),
			isolated( entry.group ),
			entry.time.isValid() ? QLocale().toString( entry.time.time(), QLocale::ShortFormat ) : QString()
		};
		for( int column = 0; column < cells.size(); ++column )
		{
			m_table->setItem( row, column, new QTableWidgetItem( cells[column] ) );
		}
	}
	m_table->setSortingEnabled( true );

	m_summaryLabel->setText( tr( "%1 of %2 students registered" ).arg( registered ).arg( m_order.size() ) );
}



void RegisterWindow::exportCsv()
{
	const auto fileName = QFileDialog::getSaveFileName( this, tr( "Export attendance" ),
														tr( "attendance-%1.csv" ).arg( QDate::currentDate().toString( Qt::ISODate ) ),
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

	const auto quote = []( QString value ) {
		return QLatin1Char('"') + value.replace( QLatin1Char('"'), QStringLiteral("\"\"") ) + QLatin1Char('"');
	};

	QTextStream stream( &file );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec( "UTF-8" );
#endif
	// UTF-8 byte order mark so that spreadsheet programs detect Arabic and Tifinagh text
	stream << QStringLiteral("\uFEFF");
	stream << QStringList{ quote( tr( "Date" ) ), quote( tr( "Computer" ) ), quote( tr( "Student" ) ),
						   quote( tr( "Class" ) ), quote( tr( "Registered at" ) ) }.join( QLatin1Char(',') ) << QLatin1Char('\n');

	for( const auto& key : std::as_const( m_order ) )
	{
		const auto& entry = m_entries[key];
		stream << QStringList{ QDate::currentDate().toString( Qt::ISODate ), quote( entry.computerName ),
							   quote( entry.studentName ), quote( entry.group ),
							   entry.time.isValid() ? entry.time.time().toString( QStringLiteral("HH:mm") ) : QString() }
					  .join( QLatin1Char(',') ) << QLatin1Char('\n');
	}
}
