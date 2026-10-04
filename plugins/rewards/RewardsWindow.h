/*
 * RewardsWindow.h - stars of the students on the teacher computer
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

#include <QWidget>

#include "RewardBook.h"

class QLabel;
class QTableWidget;

class RewardsWindow : public QWidget
{
	Q_OBJECT
public:
	explicit RewardsWindow( QWidget* parent = nullptr );

	void setBook( const RewardBook& book );

Q_SIGNALS:
	void removeStarRequested( const QString& student );
	void resetRequested();

private:
	void exportCsv();

	QTableWidget* m_table;
	QLabel* m_summary;
	RewardBook m_book;

};
