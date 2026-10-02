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

#include "BrandTheme.h"
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



static QSettings teacherSettings()
{
	return QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("Register") );
}



RegisterWindow::RegisterWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_classList( ClassListUtils::fromStringList( teacherSettings().value( QStringLiteral("ClassList") ).toStringList() ) ),
	m_summaryLabel( new QLabel( this ) ),
	m_table( new QTableWidget( this ) ),
	m_clearListButton( new QPushButton( tr( "Remove class list" ), this ) )
{
	setWindowTitle( tr( "Attendance register: %1" ).arg( QLocale().toString( QDate::currentDate(), QLocale::LongFormat ) ) );
	resize( 880, 520 );
	setAttribute( Qt::WA_DeleteOnClose );

	m_table->setColumnCount( 5 );
	m_table->setHorizontalHeaderLabels( { tr( "Computer" ), tr( "Student" ), tr( "Class" ), tr( "Registered at" ), tr( "Status" ) } );
	m_table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
	m_table->verticalHeader()->hide();
	m_table->setEditTriggers( QAbstractItemView::NoEditTriggers );
	m_table->setSortingEnabled( true );

	auto askAgainButton = new QPushButton( tr( "Ask again" ), this );
	auto importButton = new QPushButton( tr( "Import class list" ), this );
	importButton->setToolTip( tr( "Load the students of the class from a CSV file (name in the first column, "
								  "class in the second column) to see who is absent." ) );
	auto exportButton = new QPushButton( tr( "Export (CSV)" ), this );
	connect( importButton, &QPushButton::clicked, this, &RegisterWindow::importClassList );
	connect( m_clearListButton, &QPushButton::clicked, this, &RegisterWindow::clearClassList );
	auto closeButton = new QPushButton( tr( "Close" ), this );
	connect( askAgainButton, &QPushButton::clicked, this, &RegisterWindow::askAgainRequested );
	connect( exportButton, &QPushButton::clicked, this, &RegisterWindow::exportCsv );
	connect( closeButton, &QPushButton::clicked, this, &QWidget::close );

	auto buttons = new QHBoxLayout;
	buttons->addWidget( m_summaryLabel, 1 );
	buttons->addWidget( askAgainButton );
	buttons->addWidget( importButton );
	buttons->addWidget( m_clearListButton );
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



QList<RegisterWindow::Row> RegisterWindow::rows() const
{
	const auto hasClassList = m_classList.isEmpty() == false;

	QList<Row> rows;
	QStringList registeredNames;

	for( const auto& key : m_order )
	{
		const auto& entry = m_entries[key];

		Row row{ entry.computerName, entry.studentName, entry.group,
				 entry.time.isValid() ? QLocale().toString( entry.time.time(), QLocale::ShortFormat ) : QString(),
				 {} };

		if( entry.studentName.isEmpty() )
		{
			row.status = tr( "Waiting" );
		}
		else
		{
			registeredNames.append( entry.studentName );
			row.status = hasClassList && ClassListUtils::contains( m_classList, entry.studentName ) == false
							 ? tr( "Not on the class list" ) : tr( "Present" );
		}

		rows.append( row );
	}

	for( const auto& student : ClassListUtils::absentStudents( m_classList, registeredNames ) )
	{
		rows.append( { {}, student.name, student.group, {}, tr( "Absent" ) } );
	}

	return rows;
}



void RegisterWindow::refresh()
{
	const auto allRows = rows();

	m_table->setSortingEnabled( false );
	m_table->setRowCount( int(allRows.size()) );

	int registered = 0;
	int absent = 0;
	for( int row = 0; row < allRows.size(); ++row )
	{
		const auto& entry = allRows[row];
		registered += entry.computerName.isEmpty() == false && entry.studentName.isEmpty() == false ? 1 : 0;
		absent += entry.computerName.isEmpty() ? 1 : 0;

		const QStringList cells{
			isolated( entry.computerName ),
			entry.studentName.isEmpty() ? tr( "(waiting)" ) : isolated( entry.studentName ),
			isolated( entry.group ),
			entry.time,
			entry.status
		};
		for( int column = 0; column < cells.size(); ++column )
		{
			auto item = new QTableWidgetItem( cells[column] );
			if( entry.computerName.isEmpty() )
			{
				item->setForeground( BrandTheme::color( BrandTheme::Error ) );
			}
			m_table->setItem( row, column, item );
		}
	}
	m_table->setSortingEnabled( true );

	m_clearListButton->setVisible( m_classList.isEmpty() == false );

	if( m_classList.isEmpty() )
	{
		m_summaryLabel->setText( tr( "%1 of %2 students registered" ).arg( registered ).arg( m_order.size() ) );
	}
	else
	{
		m_summaryLabel->setText( tr( "%1 registered, %2 of %3 students of the class list absent" )
									 .arg( registered ).arg( absent ).arg( m_classList.size() ) );
	}
}



void RegisterWindow::importClassList()
{
	const auto fileName = QFileDialog::getOpenFileName( this, tr( "Import class list" ), {},
														tr( "CSV files (*.csv *.txt)" ) );
	if( fileName.isEmpty() )
	{
		return;
	}

	QFile file( fileName );
	if( file.open( QFile::ReadOnly ) == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not read %1." ).arg( fileName ) );
		return;
	}

	const auto classList = ClassListUtils::parseCsv( file.readAll() );
	if( classList.isEmpty() )
	{
		QMessageBox::warning( this, windowTitle(),
							  tr( "No students found in %1. Save the class list as CSV with the names in the "
								  "first column." ).arg( fileName ) );
		return;
	}

	m_classList = classList;
	teacherSettings().setValue( QStringLiteral("ClassList"), ClassListUtils::toStringList( m_classList ) );
	refresh();
}



void RegisterWindow::clearClassList()
{
	m_classList.clear();
	teacherSettings().remove( QStringLiteral("ClassList") );
	refresh();
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
						   quote( tr( "Class" ) ), quote( tr( "Registered at" ) ), quote( tr( "Status" ) ) }
				  .join( QLatin1Char(',') ) << QLatin1Char('\n');

	const auto date = QDate::currentDate().toString( Qt::ISODate );
	for( const auto& row : rows() )
	{
		stream << QStringList{ date, quote( row.computerName ), quote( row.studentName ), quote( row.group ),
							   quote( row.time ), quote( row.status ) }
					  .join( QLatin1Char(',') ) << QLatin1Char('\n');
	}
}
