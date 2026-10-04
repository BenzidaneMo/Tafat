/*
 * RewardPopup.cpp - star shown to a student
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

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QTimer>

#include "BrandTheme.h"
#include "RewardPopup.h"


RewardPopup::RewardPopup( QWidget* parent ) :
	QWidget( parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint ),
	m_icon( new QLabel( this ) ),
	m_text( new QLabel( this ) ),
	m_hideTimer( new QTimer( this ) )
{
	setAttribute( Qt::WA_ShowWithoutActivating );
	setObjectName( QStringLiteral("RewardPopup") );
	setStyleSheet( QStringLiteral("#RewardPopup { background: %1; border: 2px solid %2; border-radius: 12px; }"
								  "QLabel { color: %3; }")
					   .arg( BrandTheme::color( BrandTheme::Paper ).name(), BrandTheme::color( BrandTheme::Yellow ).name(),
							 BrandTheme::color( BrandTheme::Ink ).name() ) );
	setAttribute( Qt::WA_StyledBackground );

	m_icon->setPixmap( QIcon( QStringLiteral(":/rewards/rewards.png") ).pixmap( 56, 56 ) );
	auto font = m_text->font();
	font.setPointSizeF( font.pointSizeF() * 1.25 );
	font.setBold( true );
	m_text->setFont( font );

	auto layout = new QHBoxLayout( this );
	layout->setContentsMargins( 16, 12, 20, 12 );
	layout->setSpacing( 14 );
	layout->addWidget( m_icon );
	layout->addWidget( m_text, 1 );

	m_hideTimer->setSingleShot( true );
	m_hideTimer->setInterval( DisplayTimeMs );
	connect( m_hideTimer, &QTimer::timeout, this, &QWidget::hide );
}



void RewardPopup::showReward( int stars, int change )
{
	m_text->setText( ( change >= 0 ? tr( "Well done! Your teacher gave you a star." )
								   : tr( "Your teacher took a star back." ) ) +
					 QLatin1Char('\n') + tr( "Your stars: %1" ).arg( stars ) );
	adjustSize();

	// bottom corner on the trailing side (like system notifications), above the task bar
	if( const auto screen = QGuiApplication::primaryScreen() )
	{
		const auto area = screen->availableGeometry();
		const auto x = QGuiApplication::isRightToLeft() ? area.left() + 24 : area.right() - width() - 24;
		move( x, area.bottom() - height() - 24 );
	}

	show();
	raise();
	m_hideTimer->start();
}
