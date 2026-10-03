/*
 * RunningAppsWindow.h - shows the applications open on the student computers
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
#include <QTimer>
#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;

class RunningAppsWindow : public QWidget
{
	Q_OBJECT
public:
	static constexpr int RefreshInterval = 5000;

	explicit RunningAppsWindow( QWidget* parent = nullptr );

	void setComputer( const QString& key, const QString& title );
	void setApplications( const QString& key, const QStringList& applications );
	void clear();

Q_SIGNALS:
	void refreshRequested();
	void closeRequested( const QString& key, const QString& application );
	void closeEverywhereRequested( const QString& application );

protected:
	void showEvent( QShowEvent* event ) override;
	void hideEvent( QHideEvent* event ) override;

private:
	void updateButtons();
	QString selectedKey() const;
	QString selectedApplication() const;

	QTreeWidget* m_tree;
	QLabel* m_summary;
	QPushButton* m_closeButton;
	QPushButton* m_closeEverywhereButton;
	QTimer m_refreshTimer;

	QMap<QString, QStringList> m_applications;

};
