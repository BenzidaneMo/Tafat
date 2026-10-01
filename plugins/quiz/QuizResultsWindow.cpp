/*
 * QuizResultsWindow.cpp - teacher window with the live results of a quiz
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

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "QuizResultsWindow.h"


// numbers like "3 / 4" and names in another script must not be reordered in
// right-to-left layouts (e.g. "3 / 4" displayed as "4 / 3")
static QString leftToRight( const QString& text )
{
	return QStringLiteral("\u2066") + text + QStringLiteral("\u2069");
}

static QString isolated( const QString& text )
{
	return QStringLiteral("\u2068") + text + QStringLiteral("\u2069");
}


QuizResultsWindow::QuizResultsWindow( const Quiz& quiz, QWidget* parent ) :
	QWidget( parent, Qt::Window ),
	m_quiz( quiz ),
	m_summaryLabel( new QLabel( this ) ),
	m_table( new QTableWidget( this ) ),
	m_questionTree( new QTreeWidget( this ) )
{
	setWindowTitle( tr( "Results: %1" ).arg( quiz.title ) );
	resize( 900, 600 );
	setAttribute( Qt::WA_DeleteOnClose );

	m_table->setColumnCount( 5 );
	m_table->setHorizontalHeaderLabels( { tr( "Computer" ), tr( "Student" ), tr( "Answered" ), tr( "Score" ), tr( "Status" ) } );
	m_table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
	m_table->verticalHeader()->hide();
	m_table->setEditTriggers( QAbstractItemView::NoEditTriggers );
	m_table->setSortingEnabled( true );

	m_questionTree->setHeaderLabels( { tr( "Question / answer" ), tr( "Students" ) } );
	m_questionTree->header()->setSectionResizeMode( 0, QHeaderView::Stretch );

	auto splitter = new QSplitter( Qt::Vertical, this );
	splitter->addWidget( m_table );
	splitter->addWidget( m_questionTree );

	auto exportButton = new QPushButton( tr( "Export (CSV)" ), this );
	auto closeButton = new QPushButton( tr( "Close" ), this );
	connect( exportButton, &QPushButton::clicked, this, &QuizResultsWindow::exportCsv );
	connect( closeButton, &QPushButton::clicked, this, &QWidget::close );

	auto buttons = new QHBoxLayout;
	buttons->addWidget( m_summaryLabel, 1 );
	buttons->addWidget( exportButton );
	buttons->addWidget( closeButton );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( splitter );
	layout->addLayout( buttons );

	refresh();
}



void QuizResultsWindow::addParticipant( const QString& key, const QString& computerName, const QString& studentName )
{
	if( m_participants.contains( key ) == false )
	{
		m_order.append( key );
	}
	auto& participant = m_participants[key];
	participant.computerName = computerName;
	participant.studentName = studentName;
	refresh();
}



void QuizResultsWindow::updateAnswers( const QString& key, const QuizAnswers& answers, bool finished )
{
	if( m_participants.contains( key ) == false )
	{
		m_order.append( key );
		m_participants[key].computerName = key;
	}
	auto& participant = m_participants[key];
	participant.answers = answers;
	participant.finished = finished;
	refresh();
}



void QuizResultsWindow::setEnded()
{
	m_ended = true;
	refresh();
}



void QuizResultsWindow::refresh()
{
	const auto graded = m_quiz.isGraded();
	const auto questionCount = int(m_quiz.questions.size());

	m_table->setSortingEnabled( false );
	m_table->setRowCount( int(m_order.size()) );

	int finishedCount = 0;
	for( int row = 0; row < m_order.size(); ++row )
	{
		const auto& participant = m_participants[m_order[row]];
		finishedCount += participant.finished ? 1 : 0;

		const auto status = participant.finished ? tr( "Submitted" )
												 : ( m_ended ? tr( "Not submitted" ) : tr( "In progress" ) );
		const QStringList cells{
			isolated( participant.computerName ),
			isolated( participant.studentName ),
			leftToRight( QStringLiteral("%1 / %2").arg( m_quiz.answeredCount( participant.answers ) ).arg( questionCount ) ),
			graded ? leftToRight( QStringLiteral("%1 / %2").arg( m_quiz.score( participant.answers ) ).arg( m_quiz.maximumScore() ) )
				   : QStringLiteral("-"),
			status
		};
		for( int column = 0; column < cells.size(); ++column )
		{
			m_table->setItem( row, column, new QTableWidgetItem( cells[column] ) );
		}
	}
	m_table->setSortingEnabled( true );

	m_summaryLabel->setText( tr( "%1 of %2 students submitted" ).arg( finishedCount ).arg( m_order.size() ) );

	// per-question overview
	QList<QuizAnswers> allAnswers;
	for( const auto& participant : std::as_const( m_participants ) )
	{
		allAnswers.append( participant.answers );
	}

	m_questionTree->clear();
	for( int i = 0; i < m_quiz.questions.size(); ++i )
	{
		const auto& question = m_quiz.questions[i];
		auto item = new QTreeWidgetItem( m_questionTree, { QStringLiteral("%1. %2").arg( i + 1 ).arg( question.text.simplified() ) } );

		if( question.type == QuizQuestion::Type::Text )
		{
			int correct = 0;
			int answered = 0;
			for( const auto& answers : std::as_const( allAnswers ) )
			{
				const auto answer = answers.value( question.id );
				answered += answer.isEmpty() ? 0 : 1;
				correct += m_quiz.isCorrect( question, answer ) ? 1 : 0;
			}
			item->setText( 1, question.isGraded() ? tr( "%1 answered, %2 correct" ).arg( answered ).arg( correct )
												  : tr( "%1 answered" ).arg( answered ) );
		}
		else
		{
			const auto tally = m_quiz.optionTally( question, allAnswers );
			for( int option = 0; option < question.options.size(); ++option )
			{
				const auto marker = question.correctOptions.contains( option ) ? QStringLiteral(" ✓") : QString();
				const auto percent = allAnswers.isEmpty() ? 0 : tally[option] * 100 / int(allAnswers.size());
				new QTreeWidgetItem( item, { isolated( question.options[option] ) + marker,
											 leftToRight( QStringLiteral("%1 (%2%)").arg( tally[option] ).arg( percent ) ) } );
			}
		}
		item->setExpanded( true );
	}
}



QString QuizResultsWindow::answerText( const QuizQuestion& question, const QuizAnswer& answer ) const
{
	if( question.type == QuizQuestion::Type::Text )
	{
		return answer.text.simplified();
	}

	QStringList selected;
	for( const auto option : answer.selectedOptions )
	{
		if( option >= 0 && option < question.options.size() )
		{
			selected.append( question.options[option] );
		}
	}
	return selected.join( QStringLiteral(" | ") );
}



void QuizResultsWindow::exportCsv()
{
	const auto fileName = QFileDialog::getSaveFileName( this, tr( "Export results" ),
														m_quiz.title + QStringLiteral(".csv"),
														tr( "CSV files (*.csv)" ) );
	if( fileName.isEmpty() )
	{
		return;
	}

	QFile file( fileName );
	if( file.open( QFile::WriteOnly | QFile::Truncate | QFile::Text ) == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( fileName ) );
		return;
	}

	const auto quote = []( QString value ) {
		return QLatin1Char('"') + value.replace( QLatin1Char('"'), QStringLiteral("\"\"") ) + QLatin1Char('"');
	};

	QStringList header{ tr( "Computer" ), tr( "Student" ), tr( "Answered" ), tr( "Score" ), tr( "Maximum score" ), tr( "Submitted" ) };
	for( const auto& question : std::as_const( m_quiz.questions ) )
	{
		header.append( question.text.simplified() );
	}

	QTextStream stream( &file );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec( "UTF-8" );
#endif
	// UTF-8 byte order mark so that spreadsheet programs detect Arabic and Tifinagh text
	stream << QStringLiteral("﻿");

	QStringList quotedHeader;
	for( const auto& value : std::as_const( header ) )
	{
		quotedHeader.append( quote( value ) );
	}
	stream << quotedHeader.join( QLatin1Char(',') ) << QLatin1Char('\n');

	for( const auto& key : std::as_const( m_order ) )
	{
		const auto& participant = m_participants[key];
		QStringList row{
			quote( participant.computerName ),
			quote( participant.studentName ),
			QString::number( m_quiz.answeredCount( participant.answers ) ),
			QString::number( m_quiz.score( participant.answers ) ),
			QString::number( m_quiz.maximumScore() ),
			participant.finished ? tr( "yes" ) : tr( "no" )
		};
		for( const auto& question : std::as_const( m_quiz.questions ) )
		{
			row.append( quote( answerText( question, participant.answers.value( question.id ) ) ) );
		}
		stream << row.join( QLatin1Char(',') ) << QLatin1Char('\n');
	}
}
