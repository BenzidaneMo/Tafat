/*
 * ReturnWorkDialog.cpp - returns corrected work to the students
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

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

#include "Filesystem.h"
#include "ReturnWorkDialog.h"
#include "ReturnWorkMatcher.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


static QSettings settings()
{
	return QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("ReturnWork") );
}



static QString isolated( const QString& text )
{
	return QStringLiteral("\u2068") + text + QStringLiteral("\u2069");
}



enum Column { ComputerColumn, StudentColumn, FolderColumn, FilesColumn, ColumnCount };



ReturnWorkDialog::ReturnWorkDialog( QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_folderEdit( new QLineEdit( this ) ),
	m_table( new QTableWidget( this ) ),
	m_destinationEdit( new QLineEdit( this ) ),
	m_overwriteBox( new QCheckBox( tr( "Replace files with the same name" ), this ) ),
	m_progressBar( new QProgressBar( this ) ),
	m_statusLabel( new QLabel( this ) ),
	m_returnButton( new QPushButton( tr( "Return work" ), this ) )
{
	setWindowTitle( tr( "Return work to the students" ) );
	setWindowIcon( QIcon( QStringLiteral(":/returnwork/return-work.png") ) );
	resize( 760, 520 );

	auto intro = new QLabel( tr( "Choose the folder of collected files. Each student gets back the files of "
								 "their own folder, for example after you corrected them." ), this );
	intro->setWordWrap( true );

	m_folderEdit->setReadOnly( true );
	auto browseButton = new QPushButton( tr( "Choose folder…" ), this );
	connect( browseButton, &QPushButton::clicked, this, &ReturnWorkDialog::chooseFolder );

	auto folderLayout = new QHBoxLayout;
	folderLayout->addWidget( m_folderEdit, 1 );
	folderLayout->addWidget( browseButton );

	m_table->setColumnCount( ColumnCount );
	m_table->setHorizontalHeaderLabels( { tr( "Computer" ), tr( "Student" ), tr( "Folder" ), tr( "Files" ) } );
	m_table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
	m_table->horizontalHeader()->setSectionResizeMode( FilesColumn, QHeaderView::ResizeToContents );
	m_table->verticalHeader()->hide();
	m_table->setEditTriggers( QAbstractItemView::NoEditTriggers );
	m_table->setSelectionMode( QAbstractItemView::NoSelection );

	m_destinationEdit->setText( settings().value( QStringLiteral("DestinationFolder"), tr( "Returned work" ) ).toString() );
	m_overwriteBox->setChecked( settings().value( QStringLiteral("Overwrite"), false ).toBool() );

	auto options = new QFormLayout;
	options->addRow( tr( "Folder on the student computers (in their home folder):" ), m_destinationEdit );
	options->addRow( QString{}, m_overwriteBox );

	m_progressBar->setVisible( false );
	m_statusLabel->setWordWrap( true );

	auto closeButton = new QPushButton( tr( "Close" ), this );
	connect( closeButton, &QPushButton::clicked, this, &QWidget::close );
	connect( m_returnButton, &QPushButton::clicked, this, &ReturnWorkDialog::startReturn );

	auto buttons = new QHBoxLayout;
	buttons->addWidget( m_progressBar, 1 );
	buttons->addStretch();
	buttons->addWidget( m_returnButton );
	buttons->addWidget( closeButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( intro );
	layout->addLayout( folderLayout );
	layout->addWidget( m_table, 1 );
	layout->addLayout( options );
	layout->addWidget( m_statusLabel );
	layout->addLayout( buttons );

	connect( &m_transfer, &ReturnWorkTransfer::progressChanged, this, [this]( int done, int total ) {
		m_progressBar->setMaximum( qMax( 1, total ) );
		m_progressBar->setValue( done );
	} );
	connect( &m_transfer, &ReturnWorkTransfer::errorOccurred, this, [this]( const QString& message ) {
		m_errors.append( message );
	} );
	connect( &m_transfer, &ReturnWorkTransfer::finished, this, [this]() {
		setRunning( false );
		m_statusLabel->setText( m_errors.isEmpty() ? tr( "All files were returned." )
												   : m_errors.join( QLatin1Char('\n') ) );
	} );

	auto folder = settings().value( QStringLiteral("CollectionFolder") ).toString();
	if( folder.isEmpty() )
	{
		folder = VeyonCore::filesystem().expandPath(
			VeyonCore::config().value( QStringLiteral("CollectedFilesDestinationDirectory"), QStringLiteral("FileTransfer"),
									   QStringLiteral("%HOME%") ).toString() );
	}
	setFolder( folder );
}



void ReturnWorkDialog::setComputers( const ComputerControlInterfaceList& computers )
{
	m_computers = computers;
	m_computers.removeLocalHostInterfaces();
	updateRows();
}



void ReturnWorkDialog::closeEvent( QCloseEvent* event )
{
	if( m_transfer.isRunning() &&
		QMessageBox::question( this, windowTitle(), tr( "The files are still being returned. Stop now?" ) ) != QMessageBox::Yes )
	{
		event->ignore();
		return;
	}

	m_transfer.cancel();
	setRunning( false );
	QWidget::closeEvent( event );
}



void ReturnWorkDialog::chooseFolder()
{
	const auto folder = QFileDialog::getExistingDirectory( this, tr( "Folder of collected files" ), m_folder );
	if( folder.isEmpty() == false )
	{
		settings().setValue( QStringLiteral("CollectionFolder"), folder );
		setFolder( folder );
	}
}



void ReturnWorkDialog::setFolder( const QString& folder )
{
	m_folder = folder;
	m_folderEdit->setText( QDir::toNativeSeparators( folder ) );
	m_subfolders = QDir( folder ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase );
	updateRows();
}



void ReturnWorkDialog::updateRows()
{
	m_table->setRowCount( int(m_computers.size()) );

	for( int row = 0; row < m_computers.size(); ++row )
	{
		const auto& computer = m_computers[row];
		const auto userName = computer->userFullName().isEmpty() ? computer->userLoginName() : computer->userFullName();

		m_table->setItem( row, ComputerColumn, new QTableWidgetItem( isolated( computer->computerName() ) ) );
		m_table->setItem( row, StudentColumn, new QTableWidgetItem( isolated( VeyonCore::stripDomain( userName ) ) ) );

		auto folderBox = new QComboBox( m_table );
		folderBox->addItem( tr( "(nothing)" ), QString{} );
		for( const auto& subfolder : std::as_const( m_subfolders ) )
		{
			folderBox->addItem( isolated( subfolder ), subfolder );
		}
		const auto match = ReturnWorkMatcher::matchFolder( m_subfolders, computer->computerName(),
														   VeyonCore::stripDomain( userName ) );
		folderBox->setCurrentIndex( qMax( 0, folderBox->findData( match ) ) );
		connect( folderBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, [this, row]() {
			updateFileCount( row );
		} );
		m_table->setCellWidget( row, FolderColumn, folderBox );

		updateFileCount( row );
	}
}



void ReturnWorkDialog::updateFileCount( int row )
{
	auto folderBox = qobject_cast<QComboBox *>( m_table->cellWidget( row, FolderColumn ) );
	const auto subfolder = folderBox ? folderBox->currentData().toString() : QString{};
	const auto count = subfolder.isEmpty() ? 0 : filesOf( subfolder ).size();
	m_table->setItem( row, FilesColumn, new QTableWidgetItem( QString::number( count ) ) );
}



QStringList ReturnWorkDialog::filesOf( const QString& subfolder ) const
{
	QStringList files;
	const auto entries = QDir( m_folder + QLatin1Char('/') + subfolder ).entryInfoList( QDir::Files, QDir::Name );
	for( const auto& entry : entries )
	{
		files.append( entry.absoluteFilePath() );
	}
	return files;
}



void ReturnWorkDialog::startReturn()
{
	QList<ReturnWorkTransfer::Job> jobs;
	for( int row = 0; row < m_computers.size(); ++row )
	{
		auto folderBox = qobject_cast<QComboBox *>( m_table->cellWidget( row, FolderColumn ) );
		const auto subfolder = folderBox ? folderBox->currentData().toString() : QString{};
		if( subfolder.isEmpty() == false )
		{
			const auto files = filesOf( subfolder );
			if( files.isEmpty() == false )
			{
				jobs.append( { m_computers[row], files } );
			}
		}
	}

	if( jobs.isEmpty() )
	{
		QMessageBox::information( this, windowTitle(), tr( "There are no files to return. Choose the folder of "
														   "collected files and the folder of each student." ) );
		return;
	}

	// a plain folder name below the student's home folder
	auto destination = ReturnWorkMatcher::sanitized( m_destinationEdit->text() ).remove( QStringLiteral("..") );
	if( destination.isEmpty() )
	{
		destination = tr( "Returned work" );
	}

	settings().setValue( QStringLiteral("DestinationFolder"), destination );
	settings().setValue( QStringLiteral("Overwrite"), m_overwriteBox->isChecked() );

	m_errors.clear();
	m_statusLabel->setText( tr( "Returning the files…" ) );
	setRunning( true );
	m_transfer.start( jobs, destination, m_overwriteBox->isChecked() );
}



void ReturnWorkDialog::setRunning( bool running )
{
	m_returnButton->setEnabled( running == false );
	m_progressBar->setVisible( running || m_progressBar->value() > 0 );
	m_table->setEnabled( running == false );
}
