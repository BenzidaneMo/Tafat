/*
 * AppControlDialog.h - dialog for choosing blocked or allowed applications
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

#include "ProcessControl.h"

class QPlainTextEdit;
class QRadioButton;

class AppControlDialog : public QDialog
{
	Q_OBJECT
public:
	using Mode = ProcessControl::Policy;

	explicit AppControlDialog( QWidget* parent = nullptr );

	Mode mode() const;
	QStringList applications() const;

	void accept() override;

private:
	void addApplications( const QStringList& applications );

	QRadioButton* m_blockButton;
	QRadioButton* m_allowButton;
	QPlainTextEdit* m_applicationsEdit;

};
