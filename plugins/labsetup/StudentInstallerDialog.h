/*
 * StudentInstallerDialog.h - creates the student installers for a USB stick
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

class QLabel;
class QListWidget;
class QPushButton;

class StudentInstallerDialog : public QDialog
{
	Q_OBJECT
public:
	explicit StudentInstallerDialog( QWidget* parent = nullptr );

	// folder with the installer that installed this computer
	static QString installerFolder();
	// the teacher key: "teacher" if it exists, else the first valid one
	static QString teacherKeyName();

private:
	void addInstaller( const QString& file, bool checked );
	void chooseInstaller();
	void create();
	void updateState();

	QListWidget* m_installers;
	QLabel* m_keyLabel;
	QLabel* m_resultLabel;
	QPushButton* m_createButton;
	QString m_keyName;

};
