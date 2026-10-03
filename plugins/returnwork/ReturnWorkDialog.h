/*
 * ReturnWorkDialog.h - returns corrected work to the students
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

#include "ComputerControlInterface.h"
#include "ReturnWorkTransfer.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;

class ReturnWorkDialog : public QWidget
{
	Q_OBJECT
public:
	explicit ReturnWorkDialog( QWidget* parent = nullptr );

	void setComputers( const ComputerControlInterfaceList& computers );

protected:
	void closeEvent( QCloseEvent* event ) override;

private:
	void chooseFolder();
	void setFolder( const QString& folder );
	void updateRows();
	void updateFileCount( int row );
	void startReturn();
	void setRunning( bool running );

	QStringList filesOf( const QString& subfolder ) const;

	ComputerControlInterfaceList m_computers;
	QString m_folder;
	QStringList m_subfolders;

	QLineEdit* m_folderEdit;
	QTableWidget* m_table;
	QLineEdit* m_destinationEdit;
	QCheckBox* m_overwriteBox;
	QProgressBar* m_progressBar;
	QLabel* m_statusLabel;
	QPushButton* m_returnButton;

	ReturnWorkTransfer m_transfer;
	QStringList m_errors;

};
