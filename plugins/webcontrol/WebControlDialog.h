/*
 * WebControlDialog.h - dialog for choosing blocked or allowed websites
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

#include "WebPolicy.h"

class QPlainTextEdit;
class QRadioButton;

class WebControlDialog : public QDialog
{
	Q_OBJECT
public:
	explicit WebControlDialog( QWidget* parent = nullptr );

	WebPolicy::Mode mode() const;
	QStringList sites() const;

	void accept() override;

private:
	void addSites( const QStringList& sites );

	QRadioButton* m_blockButton;
	QRadioButton* m_allowButton;
	QPlainTextEdit* m_sitesEdit;

};
