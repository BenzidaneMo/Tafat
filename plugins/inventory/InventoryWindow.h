/*
 * InventoryWindow.h - table with the inventory of the student computers
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

#include <QMap>
#include <QVariantMap>
#include <QWidget>

class QLabel;
class QTableWidget;

class InventoryWindow : public QWidget
{
	Q_OBJECT
public:
	explicit InventoryWindow( QWidget* parent = nullptr );

	void setComputer( const QString& key, const QString& computerName, const QString& userName );
	void setInventory( const QString& key, const QVariantMap& inventory );
	void clear();

	// header and rows as shown, for the CSV export
	QStringList headers() const;
	QList<QStringList> rows() const;

Q_SIGNALS:
	void refreshRequested();

private:
	enum Column
	{
		ComputerColumn,
		UserColumn,
		SystemColumn,
		ProcessorColumn,
		CoresColumn,
		MemoryColumn,
		DiskColumn,
		IpColumn,
		MacColumn,
		VersionColumn,
		ColumnCount
	};

	int rowOf( const QString& key ) const;
	void setCell( int row, int column, const QString& text, const QVariant& sortValue = {} );
	void updateSummary();
	void exportCsv();

	QTableWidget* m_table;
	QLabel* m_summary;
	QMap<QString, QVariantMap> m_inventories;

};
