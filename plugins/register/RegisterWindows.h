/*
 * RegisterWindows.h - attendance register windows for teacher and student
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

#include <QDateTime>
#include <QDialog>
#include <QMap>
#include <QWidget>

#include "ClassList.h"

class QLabel;
class QPushButton;
class QLineEdit;
class QTableWidget;

struct Registration
{
	QString computerName;
	QString studentName;
	QString group;
	QDateTime time;
};


// student: asks for the name (and class) for the attendance register
class RegistrationDialog : public QDialog
{
	Q_OBJECT
public:
	explicit RegistrationDialog( QWidget* parent = nullptr );

	QString studentName() const;
	QString group() const;

	void accept() override;

private:
	QLineEdit* m_nameEdit;
	QLineEdit* m_groupEdit;

};


// teacher: who registered on which computer
class RegisterWindow : public QWidget
{
	Q_OBJECT
public:
	explicit RegisterWindow( QWidget* parent = nullptr );

	void addComputer( const QString& key, const QString& computerName );
	void setRegistration( const QString& key, const Registration& registration );

Q_SIGNALS:
	void askAgainRequested();

private:
	struct Row
	{
		QString computerName;
		QString studentName;
		QString group;
		QString time;
		QString status;
	};

	QList<Row> rows() const;
	void refresh();
	void importClassList();
	void clearClassList();
	void exportCsv();

	QStringList m_order;
	QMap<QString, Registration> m_entries;
	ClassList m_classList;

	QLabel* m_summaryLabel;
	QTableWidget* m_table;
	QPushButton* m_clearListButton;

};
