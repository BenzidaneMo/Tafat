/*
 * AppControlDialog.cpp - dialog for choosing blocked or allowed applications
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
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>

#include "AppControlDialog.h"
#include "ProcessControl.h"
#include "VeyonCore.h"


static QSettings settings()
{
	return QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("AppControl") );
}



AppControlDialog::AppControlDialog( QWidget* parent ) :
	QDialog( parent ),
	m_blockButton( new QRadioButton( tr( "Block the applications in the list" ), this ) ),
	m_allowButton( new QRadioButton( tr( "Allow only the applications in the list" ), this ) ),
	m_applicationsEdit( new QPlainTextEdit( this ) ),
	m_blockUsbStorageBox( new QCheckBox( tr( "Also block USB sticks and other removable storage (Windows only)" ), this ) ),
	m_blockPrintingBox( new QCheckBox( tr( "Also block printing (Windows only)" ), this ) )
{
	setWindowTitle( tr( "Control applications" ) );
	setMinimumWidth( 460 );

	auto layout = new QVBoxLayout( this );

	layout->addWidget( m_blockButton );
	layout->addWidget( m_allowButton );

	if( ProcessControl::supportsWindowDetection() == false )
	{
		auto hint = new QLabel( tr( "Allowing only listed applications is supported on Windows computers. "
									"Linux computers only block the listed applications." ), this );
		hint->setWordWrap( true );
		hint->setForegroundRole( QPalette::PlaceholderText );
		layout->addWidget( hint );
	}

	layout->addWidget( new QLabel( tr( "Applications (program names, one per line, e.g. chrome or game.exe):" ), this ) );
	layout->addWidget( m_applicationsEdit );

	auto presetsLayout = new QHBoxLayout;
	auto browsersButton = new QPushButton( tr( "Add web browsers" ), this );
	auto officeButton = new QPushButton( tr( "Add office programs" ), this );
	presetsLayout->addWidget( browsersButton );
	presetsLayout->addWidget( officeButton );
	presetsLayout->addStretch();
	layout->addLayout( presetsLayout );

	connect( browsersButton, &QPushButton::clicked, this, [this]() {
		addApplications( { QStringLiteral("chrome"), QStringLiteral("msedge"), QStringLiteral("firefox"),
						   QStringLiteral("opera"), QStringLiteral("brave"), QStringLiteral("iexplore"),
						   QStringLiteral("chromium") } );
	} );
	connect( officeButton, &QPushButton::clicked, this, [this]() {
		addApplications( { QStringLiteral("winword"), QStringLiteral("excel"), QStringLiteral("powerpnt"),
						   QStringLiteral("soffice"), QStringLiteral("notepad") } );
	} );

	m_blockUsbStorageBox->setToolTip( tr( "Students cannot open USB sticks, memory cards and external disks while "
										   "this mode is active. Applies to devices that are connected afterwards." ) );
	layout->addWidget( m_blockUsbStorageBox );

	m_blockPrintingBox->setToolTip( tr( "Stops the print service on the student computers while this mode is "
										 "active, so nothing can be printed (also not to PDF)." ) );
	layout->addWidget( m_blockPrintingBox );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
	layout->addWidget( buttonBox );

	// restore the previous choice
	auto store = settings();
	const auto allow = store.value( QStringLiteral("Mode") ).toInt() == int(Mode::AllowListedOnly);
	( allow ? m_allowButton : m_blockButton )->setChecked( true );
	m_applicationsEdit->setPlainText( store.value( QStringLiteral("Applications") ).toStringList().join( QLatin1Char('\n') ) );
	m_blockUsbStorageBox->setChecked( store.value( QStringLiteral("BlockUsbStorage") ).toBool() );
	m_blockPrintingBox->setChecked( store.value( QStringLiteral("BlockPrinting") ).toBool() );
}



AppControlDialog::Mode AppControlDialog::mode() const
{
	return m_allowButton->isChecked() ? Mode::AllowListedOnly : Mode::BlockListed;
}



QStringList AppControlDialog::applications() const
{
	QStringList applications;
	const auto lines = m_applicationsEdit->toPlainText().split( QLatin1Char('\n') );
	for( const auto& line : lines )
	{
		const auto name = ProcessControl::normalizedName( line );
		if( name.isEmpty() == false && applications.contains( name ) == false )
		{
			applications.append( name );
		}
	}
	return applications;
}



bool AppControlDialog::blockUsbStorage() const
{
	return m_blockUsbStorageBox->isChecked();
}



bool AppControlDialog::blockPrinting() const
{
	return m_blockPrintingBox->isChecked();
}



void AppControlDialog::accept()
{
	auto store = settings();
	store.setValue( QStringLiteral("Mode"), int(mode()) );
	store.setValue( QStringLiteral("Applications"), applications() );
	store.setValue( QStringLiteral("BlockUsbStorage"), blockUsbStorage() );
	store.setValue( QStringLiteral("BlockPrinting"), blockPrinting() );

	QDialog::accept();
}



void AppControlDialog::addApplications( const QStringList& applications )
{
	auto current = this->applications();
	for( const auto& application : applications )
	{
		if( current.contains( application ) == false )
		{
			current.append( application );
		}
	}
	m_applicationsEdit->setPlainText( current.join( QLatin1Char('\n') ) );
}
