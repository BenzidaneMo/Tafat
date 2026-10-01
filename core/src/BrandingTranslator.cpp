/*
 * BrandingTranslator.cpp - shows the product name in translated texts
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

#include "BrandingTranslator.h"


static const auto UpstreamName = QLatin1String( "Veyon" );


BrandingTranslator::BrandingTranslator( QTranslator* catalog, QObject* parent ) :
	QTranslator( parent ),
	m_catalog( catalog )
{
	setObjectName( QStringLiteral("branding") );
}



QString BrandingTranslator::translate( const char* context, const char* sourceText,
									   const char* disambiguation, int n ) const
{
	auto text = m_catalog ? m_catalog->translate( context, sourceText, disambiguation, n ) : QString();

	if( text.isEmpty() )
	{
		const auto source = QString::fromUtf8( sourceText );
		if( source.contains( UpstreamName ) == false )
		{
			// let other translators or the untranslated text handle it
			return {};
		}
		text = source;
	}

	return brand( text );
}



QString BrandingTranslator::brand( QString text )
{
	return text.replace( UpstreamName, VeyonCore::productName() );
}
