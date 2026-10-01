/*
 * BrandTheme.cpp - product color theme
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

#include "BrandTheme.h"


QPalette BrandTheme::palette( bool dark )
{
	QPalette pal;

	const auto set = [&pal]( QPalette::ColorRole role, const char* token ) {
		pal.setColor( QPalette::All, role, color( token ) );
	};

	if( dark )
	{
		set( QPalette::Window, Brown );
		set( QPalette::WindowText, Cream );
		set( QPalette::Base, Ink );
		set( QPalette::AlternateBase, Brown2 );
		set( QPalette::Text, Cream );
		set( QPalette::Button, Brown2 );
		set( QPalette::ButtonText, Cream );
		set( QPalette::BrightText, Paper );
		set( QPalette::Highlight, Teal );
		set( QPalette::HighlightedText, Paper );
		set( QPalette::Link, TealSoft );
		set( QPalette::LinkVisited, Line );
		set( QPalette::PlaceholderText, Muted );
		set( QPalette::ToolTipBase, Cream );
		set( QPalette::ToolTipText, Ink );
		set( QPalette::Light, Muted );
		set( QPalette::Midlight, Brown2 );
		set( QPalette::Mid, Brown2 );
		set( QPalette::Dark, Ink );
		set( QPalette::Shadow, Ink );
	}
	else
	{
		set( QPalette::Window, Cream );
		set( QPalette::WindowText, Ink );
		set( QPalette::Base, Paper );
		set( QPalette::AlternateBase, Cream2 );
		set( QPalette::Text, Ink );
		set( QPalette::Button, Cream2 );
		set( QPalette::ButtonText, Ink );
		set( QPalette::BrightText, Paper );
		set( QPalette::Highlight, TealDark );
		set( QPalette::HighlightedText, Paper );
		set( QPalette::Link, TealDark );
		set( QPalette::LinkVisited, Brown2 );
		set( QPalette::PlaceholderText, Muted );
		set( QPalette::ToolTipBase, Brown );
		set( QPalette::ToolTipText, Cream );
		set( QPalette::Light, Paper );
		set( QPalette::Midlight, Line );
		set( QPalette::Mid, Line );
		set( QPalette::Dark, Muted );
		set( QPalette::Shadow, Brown );
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	set( QPalette::Accent, Orange );
#endif

	for( auto role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText } )
	{
		pal.setColor( QPalette::Disabled, role, color( Muted ) );
	}

	return pal;
}



QPalette BrandTheme::toolTipPalette( bool dark )
{
	auto pal = palette( dark );
	pal.setColor( QPalette::Window, pal.color( QPalette::ToolTipBase ) );
	pal.setColor( QPalette::WindowText, pal.color( QPalette::ToolTipText ) );
	return pal;
}



QString BrandTheme::styleSheet( bool dark )
{
	return QStringLiteral( "QToolButton:checked {background-color:%1; border:1px solid %2; border-radius:4px;}"
						   "QToolTip {padding:5px; border:0px;}"
						   "QSplitter::handle:hover {background-color:%3;}" )
		.arg( QLatin1String( dark ? TealDark : OrangeSoft ),
			  QLatin1String( dark ? Teal : Orange ),
			  QLatin1String( Orange ) );
}
