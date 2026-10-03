/*
 * AboutDialog.cpp - implementation of AboutDialog
 *
 * Copyright (c) 2011-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QDesktopServices>
#include <QFile>

#include <veyonconfig.h>

#include "AboutDialog.h"
#include "VeyonCore.h"

#include "ui_AboutDialog.h"


AboutDialog::AboutDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::AboutDialog )
{
	ui->setupUi( this );

	setWindowTitle(tr("About Veyon %1").arg(VeyonCore::versionString()));

	ui->versionLabel->setText( VeyonCore::versionString() );

	// product identity, keeping the credit to the upstream Veyon project
	const auto upstreamName = QStringLiteral("Veyon");
	const auto upstreamWebsite = QStringLiteral("https://veyon.io");
	const auto website = QStringLiteral(VEYON_WEBSITE);
	const auto developer = QStringLiteral("<a href=\"%1\">%2</a>").arg( QStringLiteral(VEYON_DEVELOPER_URL),
																		 QStringLiteral(VEYON_DEVELOPER) );
	ui->label_3->setText( tr( "%1 - based on %2" ).arg( VeyonCore::productName(),
		QStringLiteral("<a href=\"%1\">%2</a>").arg( upstreamWebsite, upstreamName ) ) +
		QStringLiteral("<br/>") + tr( "Developed by %1" ).arg( developer ) );
	ui->label_3->setOpenExternalLinks( true );
	ui->label_4->setText( QStringLiteral("<a href=\"%1\">%1</a>").arg( website ) );
	ui->label_8->setText( QStringLiteral( "Copyright © 2026 %1 and %2 contributors<br/>"
										  "Copyright © 2004-2026 Tobias Junghans / Veyon Solutions" )
							  .arg( QStringLiteral(VEYON_DEVELOPER), VeyonCore::productName() ) );
	ui->label_8->setTextFormat( Qt::RichText );

	// the donation link of the upstream project does not apply to this product
	ui->donateButton->hide();

	QFile authors( QStringLiteral( ":/CONTRIBUTORS" ) );
	if (authors.open(QFile::ReadOnly))
	{
		ui->authors->setPlainText(QString::fromUtf8(authors.readAll()));
	}

	QFile license( QStringLiteral( ":/core/COPYING" ) );
	if (license.open(QFile::ReadOnly))
	{
		ui->license->setPlainText(QString::fromUtf8(license.readAll()));
	}
}



AboutDialog::~AboutDialog()
{
	delete ui;
}



void AboutDialog::openDonationWebsite()
{
	  QDesktopServices::openUrl( QUrl( QStringLiteral( "https://www.paypal.com/cgi-bin/webscr?item_name=Donation+to+Veyon+-+OpenSource+classroom+management&cmd=_donations&business=donate%40veyon.io" ) ) );
}
