/*
 * StudentInstallerDialog.cpp - creates the student installers for a USB stick
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
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include <veyonconfig.h>

#include "Filesystem.h"
#include "StudentInstallerDialog.h"
#include "StudentPackage.h"
#include "StudentSetup.h"
#include "VeyonConfiguration.h"


StudentInstallerDialog::StudentInstallerDialog( QWidget* parent ) :
	QDialog( parent ),
	m_installers( new QListWidget( this ) ),
	m_keyLabel( new QLabel( this ) ),
	m_resultLabel( new QLabel( this ) ),
	m_createButton( new QPushButton( tr( "Create on USB stick or folder…" ), this ) ),
	m_keyName( teacherKeyName() )
{
	setWindowTitle( tr( "Create student installer" ) );
	setWindowIcon( QIcon( QStringLiteral(":/labsetup/lab-setup.png") ) );
	resize( 620, 460 );

	auto intro = new QLabel( tr( "Creates the installer for the student computers. Put it on a USB stick, then on each "
								 "student computer double-click it and answer \"Yes\". It installs %1 for students with "
								 "the key and the settings of this computer, so the student computer can be controlled "
								 "from here right away." ).arg( VeyonCore::productName() ), this );
	intro->setWordWrap( true );

	auto listLabel = new QLabel( tr( "Installers to use (one for each kind of Windows in the lab):" ), this );

	auto addButton = new QPushButton( tr( "Add installer…" ), this );
	auto releasesLabel = new QLabel( tr( "Installers for other Windows versions (for example Windows 7) are on the "
										 "<a href=\"%1\">download page</a>." )
										 .arg( QStringLiteral(VEYON_WEBSITE "/releases/latest") ), this );
	releasesLabel->setOpenExternalLinks( true );
	releasesLabel->setWordWrap( true );

	auto addLayout = new QHBoxLayout;
	addLayout->addWidget( addButton );
	addLayout->addWidget( releasesLabel, 1 );

	m_keyLabel->setWordWrap( true );
	m_resultLabel->setWordWrap( true );
	m_resultLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );

	auto buttons = new QDialogButtonBox( QDialogButtonBox::Close, this );
	buttons->addButton( m_createButton, QDialogButtonBox::ActionRole );
	m_createButton->setDefault( true );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( intro );
	layout->addWidget( listLabel );
	layout->addWidget( m_installers, 1 );
	layout->addLayout( addLayout );
	layout->addWidget( m_keyLabel );
	layout->addWidget( m_resultLabel );
	layout->addWidget( buttons );

	connect( addButton, &QPushButton::clicked, this, &StudentInstallerDialog::chooseInstaller );
	connect( m_createButton, &QPushButton::clicked, this, &StudentInstallerDialog::create );
	connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::close );
	connect( m_installers, &QListWidget::itemChanged, this, &StudentInstallerDialog::updateState );

	const auto installers = QDir( installerFolder() ).entryInfoList( { VeyonCore::productSlug() + QStringLiteral("-*-setup.exe") },
																   QDir::Files, QDir::Name );
	for( const auto& installer : installers )
	{
		addInstaller( installer.filePath(), true );
	}

	if( m_keyName.isEmpty() )
	{
		m_keyLabel->setText( tr( "This computer has no teacher key yet. Install %1 again on this computer, or create a "
								 "key in Settings → Authentication keys." ).arg( VeyonCore::productName() ) );
	}
	else
	{
		m_keyLabel->setText( tr( "Key of this teacher computer: %1" ).arg( m_keyName ) );
	}

	updateState();
}



QString StudentInstallerDialog::installerFolder()
{
	return QDir( QCoreApplication::applicationDirPath() ).filePath( QStringLiteral("setup") );
}



QString StudentInstallerDialog::teacherKeyName()
{
	return StudentSetup::teacherKeyName( VeyonCore::filesystem().expandPath( VeyonCore::config().publicKeyBaseDir() ) );
}



void StudentInstallerDialog::addInstaller( const QString& file, bool checked )
{
	for( int i = 0; i < m_installers->count(); ++i )
	{
		if( QFileInfo( m_installers->item( i )->data( Qt::UserRole ).toString() ) == QFileInfo( file ) )
		{
			return;
		}
	}

	const auto versions = StudentPackage::windowsVersions( file );
	auto item = new QListWidgetItem( versions.isEmpty() ? QFileInfo( file ).fileName()
														: QStringLiteral("%1 – %2").arg( versions, QFileInfo( file ).fileName() ),
									 m_installers );
	item->setData( Qt::UserRole, file );
	item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
	item->setCheckState( checked ? Qt::Checked : Qt::Unchecked );
}



void StudentInstallerDialog::chooseInstaller()
{
	const auto files = QFileDialog::getOpenFileNames( this, tr( "Add installer" ), QDir::homePath(),
													  tr( "Installers (*.exe)" ) );
	for( const auto& file : files )
	{
		addInstaller( file, true );
	}
	updateState();
}



void StudentInstallerDialog::updateState()
{
	bool anyChecked = false;
	for( int i = 0; i < m_installers->count(); ++i )
	{
		anyChecked |= m_installers->item( i )->checkState() == Qt::Checked;
	}
	m_createButton->setEnabled( anyChecked && m_keyName.isEmpty() == false );
}



void StudentInstallerDialog::create()
{
	const auto folder = QFileDialog::getExistingDirectory( this, tr( "USB stick or folder for the student installer" ),
														   QDir::homePath() );
	if( folder.isEmpty() )
	{
		return;
	}

	QString error;
	const auto content = StudentPackage::fromCurrentConfiguration( m_keyName, VeyonCore::filesystem().publicKeyPath( m_keyName ),
																   &error );
	if( content.isValid() == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not read %1." ).arg( error ) );
		return;
	}

	QStringList created;
	for( int i = 0; i < m_installers->count(); ++i )
	{
		const auto item = m_installers->item( i );
		if( item->checkState() != Qt::Checked )
		{
			continue;
		}

		const auto installer = item->data( Qt::UserRole ).toString();
		const auto output = QDir( folder ).filePath( StudentPackage::packageFileName( installer ) );
		QApplication::setOverrideCursor( Qt::WaitCursor );
		const auto success = StudentPackage::create( installer, output, content, &error );
		QApplication::restoreOverrideCursor();
		if( success == false )
		{
			QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( error ) );
			return;
		}
		created.append( QFileInfo( output ).fileName() );
	}

	m_resultLabel->setText( tr( "Done. On each student computer, double-click:\n%1" ).arg( created.join( QLatin1Char('\n') ) ) );
	QDesktopServices::openUrl( QUrl::fromLocalFile( folder ) );
}
