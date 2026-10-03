/*
 * BrandingTranslator.h - shows the product name in translated texts
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

#include <QTranslator>

#include "VeyonCore.h"

// Replaces the upstream name "Veyon" with the product name in all texts of
// the "veyon" translation catalog (and in the English source texts when no
// translation is loaded). This keeps the upstream source strings and all
// existing translations usable without modification.
class VEYON_CORE_EXPORT BrandingTranslator : public QTranslator
{
	Q_OBJECT
public:
	explicit BrandingTranslator( QTranslator* catalog, QObject* parent = nullptr );

	QString translate( const char* context, const char* sourceText,
					   const char* disambiguation = nullptr, int n = -1 ) const override;

	bool isEmpty() const override
	{
		return false;
	}

	static QString brand( QString text );

private:
	const QTranslator* m_catalog;

};
