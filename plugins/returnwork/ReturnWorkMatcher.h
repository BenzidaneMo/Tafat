/*
 * ReturnWorkMatcher.h - finds the folder of collected files of a student
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

#include <QStringList>

namespace ReturnWorkMatcher
{

// the file name characters the file collection removes from folder names
QString sanitized( const QString& name );

// Finds the folder with the collected files of a computer among the folders of
// a collection. The file collection names them "<student>_<computer>" by
// default, or after other attributes. Preference: student and computer,
// then the computer only (another student may use it now), then the student
// only. Comparisons ignore case. Returns an empty string if nothing matches.
QString matchFolder( const QStringList& folders, const QString& computerName, const QString& userName );

}
