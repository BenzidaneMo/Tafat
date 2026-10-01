/*
 * BrandTheme.h - product color theme
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

#pragma once

#include <QColor>
#include <QPalette>

#include "VeyonCore.h"

// Color tokens of the product theme. The names match the CSS custom
// properties of the established web theme (--cream, --paper, ...) so both
// stay in sync.
class VEYON_CORE_EXPORT BrandTheme
{
public:
	static constexpr const char* Cream = "#faf4ea";
	static constexpr const char* Cream2 = "#f4ebdd";
	static constexpr const char* Paper = "#fffdf9";
	static constexpr const char* Brown = "#3b271d";
	static constexpr const char* Brown2 = "#4b3427";
	static constexpr const char* Ink = "#2b1f18";
	static constexpr const char* Muted = "#7c6b5f";
	static constexpr const char* Line = "#eadfd0";
	static constexpr const char* Orange = "#f26b2d";
	static constexpr const char* OrangeSoft = "#fde6d7";
	static constexpr const char* Teal = "#1f9d86";
	static constexpr const char* TealDark = "#13705f";
	static constexpr const char* TealSoft = "#dff2ea";
	static constexpr const char* Yellow = "#f6bf3f";
	static constexpr const char* YellowSoft = "#fdf0cc";
	static constexpr const char* Hot = "#f5a524";
	static constexpr const char* Error = "#b42318";
	static constexpr const char* ErrorSoft = "#fde8e6";

	static QColor color( const char* token )
	{
		return QColor( QLatin1String( token ) );
	}

	static QPalette palette( bool dark );
	static QPalette toolTipPalette( bool dark );
	static QString styleSheet( bool dark );

	// registers bundled fonts; with preferTifinagh the UI font is switched
	// to the bundled Tifinagh font (for Tamazight in Tifinagh script)
	static void initFonts( bool preferTifinagh );

};
