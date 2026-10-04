/*
 * AddComputersDialog.h - finds the student computers and adds them to a room
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

#include <QDialog>

#include "ComputerControlInterface.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTreeWidget;
class QTimer;
class QTreeWidgetItem;
class ComputerScanner;

class AddComputersDialog : public QDialog
{
	Q_OBJECT
public:
	AddComputersDialog( const ComputerControlInterfaceList& knownComputers, QWidget* parent = nullptr );

private:
	enum Column {
		NameColumn,
		AddressColumn,
		StatusColumn
	};

	void search();
	void addFound( const QString& host );
	void addManual();
	QTreeWidgetItem* addItem( const QString& name, const QString& host );
	void addToRoom();
	bool runImport( const QString& csvFile, const QString& room );
	void updateState();
	bool isKnown( const QString& host ) const;
	void refreshDirectory();
	bool showRoom();

	static constexpr int RefreshIntervalMs = 2000;
	static constexpr int RefreshAttempts = 30;

	QStringList m_knownHosts;
	QComboBox* m_roomComboBox;
	QPushButton* m_searchButton;
	QProgressBar* m_progressBar;
	QTreeWidget* m_list;
	QLineEdit* m_manualEdit;
	QLineEdit* m_rangeEdit;
	QLabel* m_statusLabel;
	QPushButton* m_addButton;
	ComputerScanner* m_scanner;
	int m_foundCount{0};

	// after adding: reload the computers until the room appears and tick it in "Locations & computers"
	QTimer* m_refreshTimer;
	int m_refreshAttempts{0};
	QString m_addedRoom;
	QStringList m_addedNames;

};
