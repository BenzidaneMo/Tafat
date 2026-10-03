/*
 * PolicyStore.h - applies and removes browser policies on this computer
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

#include "WebPolicy.h"

// Writes the website policies to the locations the browsers read them from
// (registry on Windows, policy files on Linux) and removes them again.
// Policies an administrator configured are kept: only the entries this class
// added are removed. Requires administrator/root privileges.
class PolicyStore
{
public:
	static bool apply( const WebPolicy::ChromiumPolicies& chromium, const WebPolicy::FirefoxPolicies& firefox );
	static bool clear();

	// merges our entries into a list policy: entries added before are replaced,
	// entries of others are kept in front
	static QStringList mergedList( const QStringList& existing, const QStringList& previouslyAdded,
								   const QStringList& added );

};
