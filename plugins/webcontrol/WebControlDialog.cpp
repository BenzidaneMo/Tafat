/*
 * WebControlDialog.cpp - dialog for choosing blocked or allowed websites
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

#include "VeyonCore.h"
#include "WebControlDialog.h"


static QSettings settings()
{
	return QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("WebControl") );
}



WebControlDialog::WebControlDialog( QWidget* parent ) :
	QDialog( parent ),
	m_blockButton( new QRadioButton( tr( "Block the websites in the list" ), this ) ),
	m_allowButton( new QRadioButton( tr( "Allow only the websites in the list (an empty list blocks all websites)" ), this ) ),
	m_sitesEdit( new QPlainTextEdit( this ) ),
	m_blockInternetBox( new QCheckBox( tr( "Also block the internet for all other programs (Windows only)" ), this ) )
{
	setWindowTitle( tr( "Control websites" ) );
	setMinimumWidth( 480 );

	auto layout = new QVBoxLayout( this );

	layout->addWidget( m_blockButton );
	layout->addWidget( m_allowButton );
	layout->addWidget( new QLabel( tr( "Websites (one per line, e.g. youtube.com). Subdomains are included." ), this ) );
	layout->addWidget( m_sitesEdit );

	auto presetsLayout = new QHBoxLayout;
	auto socialButton = new QPushButton( tr( "Add social networks" ), this );
	auto videoButton = new QPushButton( tr( "Add video sites" ), this );
	auto gamesButton = new QPushButton( tr( "Add game sites" ), this );
	presetsLayout->addWidget( socialButton );
	presetsLayout->addWidget( videoButton );
	presetsLayout->addWidget( gamesButton );
	presetsLayout->addStretch();
	layout->addLayout( presetsLayout );

	connect( socialButton, &QPushButton::clicked, this, [this]() {
		addSites( { QStringLiteral("facebook.com"), QStringLiteral("instagram.com"), QStringLiteral("tiktok.com"),
					QStringLiteral("x.com"), QStringLiteral("twitter.com"), QStringLiteral("snapchat.com"),
					QStringLiteral("messenger.com"), QStringLiteral("whatsapp.com") } );
	} );
	connect( videoButton, &QPushButton::clicked, this, [this]() {
		addSites( { QStringLiteral("youtube.com"), QStringLiteral("youtu.be"), QStringLiteral("netflix.com"),
					QStringLiteral("twitch.tv"), QStringLiteral("dailymotion.com") } );
	} );
	connect( gamesButton, &QPushButton::clicked, this, [this]() {
		addSites( { QStringLiteral("poki.com"), QStringLiteral("crazygames.com"), QStringLiteral("miniclip.com"),
					QStringLiteral("roblox.com"), QStringLiteral("friv.com") } );
	} );

	m_blockInternetBox->setToolTip( tr( "Blocks web access to the internet for all programs with a firewall rule. "
										"The school network and the connection to this computer keep working." ) );
	layout->addWidget( m_blockInternetBox );

	auto hint = new QLabel( tr( "Applies to Google Chrome, Microsoft Edge, Brave, Chromium and Mozilla Firefox. "
								"Firefox applies the change after it was restarted. Use \"Block apps\" "
								"for other browsers." ), this );
	hint->setWordWrap( true );
	hint->setForegroundRole( QPalette::PlaceholderText );
	layout->addWidget( hint );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
	layout->addWidget( buttonBox );

	auto store = settings();
	const auto allow = store.value( QStringLiteral("Mode") ).toInt() == int(WebPolicy::Mode::AllowListedOnly);
	( allow ? m_allowButton : m_blockButton )->setChecked( true );
	m_sitesEdit->setPlainText( store.value( QStringLiteral("Sites") ).toStringList().join( QLatin1Char('\n') ) );
	m_blockInternetBox->setChecked( store.value( QStringLiteral("BlockInternet") ).toBool() );
}



WebPolicy::Mode WebControlDialog::mode() const
{
	return m_allowButton->isChecked() ? WebPolicy::Mode::AllowListedOnly : WebPolicy::Mode::BlockListed;
}



bool WebControlDialog::blockInternet() const
{
	return m_blockInternetBox->isChecked();
}



QStringList WebControlDialog::sites() const
{
	QStringList sites;
	const auto lines = m_sitesEdit->toPlainText().split( QLatin1Char('\n') );
	for( const auto& line : lines )
	{
		const auto site = WebPolicy::normalizedSite( line );
		if( site.isEmpty() == false && sites.contains( site ) == false )
		{
			sites.append( site );
		}
	}
	return sites;
}



void WebControlDialog::accept()
{
	auto store = settings();
	store.setValue( QStringLiteral("Mode"), int(mode()) );
	store.setValue( QStringLiteral("Sites"), sites() );
	store.setValue( QStringLiteral("BlockInternet"), blockInternet() );

	QDialog::accept();
}



void WebControlDialog::addSites( const QStringList& sites )
{
	auto current = this->sites();
	for( const auto& site : sites )
	{
		if( current.contains( site ) == false )
		{
			current.append( site );
		}
	}
	m_sitesEdit->setPlainText( current.join( QLatin1Char('\n') ) );
}
