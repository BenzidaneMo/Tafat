/*
 * LabSetupPage.cpp - configurator page for setting up the lab
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
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Filesystem.h"
#include "LabSetupPage.h"
#include "StudentSetup.h"
#include "VeyonConfiguration.h"


LabSetupPage::LabSetupPage( QWidget* parent ) :
	ConfigurationPage( parent ),
	m_keyComboBox( new QComboBox( this ) ),
	m_noKeyLabel( new QLabel( this ) ),
	m_exportButton( new QPushButton( tr( "Export student setup…" ), this ) ),
	m_resultLabel( new QLabel( this ) )
{
	setWindowTitle( tr( "Lab setup" ) );
	setWindowIcon( QIcon( QStringLiteral(":/labsetup/lab-setup.png") ) );

	auto title = new QLabel( tr( "Prepare the student computers" ), this );
	auto font = title->font();
	font.setBold( true );
	font.setPointSizeF( font.pointSizeF() * 1.2 );
	title->setFont( font );

	auto intro = new QLabel( tr( "Exports everything the student computers need into a folder, for example on a "
								 "USB stick: the teacher's public key, these settings and an installation script.\n\n"
								 "1. Choose the key pair of the teacher computer (key file authentication).\n"
								 "2. Export the student setup into an empty folder.\n"
								 "3. Copy the %1 installers (%2-…-setup.exe) into the same folder.\n"
								 "4. On each student computer, run %3 as administrator. It picks the right "
								 "installer for the Windows version and installs %1 silently without the "
								 "teacher program." )
							 .arg( VeyonCore::productName(), VeyonCore::productSlug(), StudentSetup::scriptFileName() ),
							 this );
	intro->setWordWrap( true );

	m_noKeyLabel->setText( tr( "There is no key pair yet. Create one on the page \"Authentication keys\" first "
							   "and choose key file authentication on the page \"General\"." ) );
	m_noKeyLabel->setWordWrap( true );
	m_noKeyLabel->setForegroundRole( QPalette::PlaceholderText );

	m_resultLabel->setWordWrap( true );
	m_resultLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );

	auto form = new QFormLayout;
	form->addRow( tr( "Key pair:" ), m_keyComboBox );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( title );
	layout->addWidget( intro );
	layout->addLayout( form );
	layout->addWidget( m_noKeyLabel );
	layout->addWidget( m_exportButton, 0, Qt::AlignLeft );
	layout->addWidget( m_resultLabel );
	layout->addStretch( 1 );

	connect( m_exportButton, &QPushButton::clicked, this, &LabSetupPage::exportStudentSetup );

	updateKeys();
}



void LabSetupPage::resetWidgets()
{
	updateKeys();
}



void LabSetupPage::connectWidgetsToProperties()
{
}



void LabSetupPage::applyConfiguration()
{
}



void LabSetupPage::showEvent( QShowEvent* event )
{
	// keys may have been created on another page meanwhile
	updateKeys();
	ConfigurationPage::showEvent( event );
}



void LabSetupPage::updateKeys()
{
	const auto current = m_keyComboBox->currentText();
	const auto baseDir = VeyonCore::filesystem().expandPath( VeyonCore::config().publicKeyBaseDir() );

	QStringList keys;
	for( const auto& name : StudentSetup::publicKeyNames( baseDir ) )
	{
		if( StudentSetup::isValidKeyName( name ) )
		{
			keys.append( name );
		}
	}

	m_keyComboBox->clear();
	m_keyComboBox->addItems( keys );
	if( keys.contains( current ) )
	{
		m_keyComboBox->setCurrentText( current );
	}

	m_keyComboBox->setEnabled( keys.isEmpty() == false );
	m_exportButton->setEnabled( keys.isEmpty() == false );
	m_noKeyLabel->setVisible( keys.isEmpty() );
}



void LabSetupPage::exportStudentSetup()
{
	const auto keyName = m_keyComboBox->currentText();
	if( keyName.isEmpty() )
	{
		return;
	}

	const auto folder = QFileDialog::getExistingDirectory( this, tr( "Folder for the student setup" ), QDir::homePath() );
	if( folder.isEmpty() )
	{
		return;
	}

	const auto result = StudentSetup::write( folder, keyName, VeyonCore::filesystem().publicKeyPath( keyName ) );
	if( result.success == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( result.error ) );
		return;
	}

	m_resultLabel->setText( tr( "The student setup was written to %1:\n%2\n\n"
								"Now copy the %3 installers into this folder." )
								.arg( QDir::toNativeSeparators( folder ), result.files.join( QStringLiteral(", ") ),
									  VeyonCore::productName() ) );

	QDesktopServices::openUrl( QUrl::fromLocalFile( folder ) );
}
