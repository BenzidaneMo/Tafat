/*
 * QuizWindow.cpp - student window for answering a quiz
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

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "QuizWindow.h"


QuizWindow::QuizWindow( const Quiz& quiz, QWidget* parent ) :
	QWidget( parent, Qt::Window | Qt::WindowStaysOnTopHint ),
	m_quiz( quiz ),
	m_timeLabel( new QLabel( this ) ),
	m_submitButton( new QPushButton( tr( "Submit answers" ), this ) ),
	m_remainingSeconds( quiz.timeLimitMinutes * 60 )
{
	setWindowTitle( quiz.title );
	resize( 820, 640 );

	auto titleLabel = new QLabel( quiz.title, this );
	auto titleFont = titleLabel->font();
	titleFont.setPointSizeF( titleFont.pointSizeF() * 1.6 );
	titleFont.setBold( true );
	titleLabel->setFont( titleFont );
	titleLabel->setWordWrap( true );

	auto header = new QHBoxLayout;
	header->addWidget( titleLabel, 1 );
	header->addWidget( m_timeLabel );

	auto questionsWidget = new QWidget;
	auto questionsLayout = new QVBoxLayout( questionsWidget );

	for( int i = 0; i < quiz.questions.size(); ++i )
	{
		const auto& question = quiz.questions[i];
		auto box = new QGroupBox( tr( "Question %1 of %2" ).arg( i + 1 ).arg( quiz.questions.size() ), questionsWidget );
		auto boxLayout = new QVBoxLayout( box );

		auto text = new QLabel( question.text, box );
		text->setWordWrap( true );
		auto textFont = text->font();
		textFont.setPointSizeF( textFont.pointSizeF() * 1.2 );
		text->setFont( textFont );
		boxLayout->addWidget( text );

		QuestionWidgets widgets;
		if( question.type == QuizQuestion::Type::Text )
		{
			widgets.textEdit = new QLineEdit( box );
			widgets.textEdit->setPlaceholderText( tr( "Your answer" ) );
			boxLayout->addWidget( widgets.textEdit );
			connect( widgets.textEdit, &QLineEdit::textChanged, this, [this]() { Q_EMIT answersChanged( answers(), false ); } );
		}
		else
		{
			auto group = new QButtonGroup( box );
			group->setExclusive( question.type == QuizQuestion::Type::SingleChoice );
			for( const auto& option : question.options )
			{
				QAbstractButton* button = nullptr;
				if( question.type == QuizQuestion::Type::SingleChoice )
				{
					button = new QRadioButton( option, box );
				}
				else
				{
					button = new QCheckBox( option, box );
				}
				group->addButton( button );
				boxLayout->addWidget( button );
				widgets.optionButtons.append( button );
				connect( button, &QAbstractButton::toggled, this, [this]() { Q_EMIT answersChanged( answers(), false ); } );
			}
		}

		m_widgets[question.id] = widgets;
		questionsLayout->addWidget( box );
	}
	questionsLayout->addStretch();

	auto scrollArea = new QScrollArea( this );
	scrollArea->setWidget( questionsWidget );
	scrollArea->setWidgetResizable( true );

	auto footer = new QHBoxLayout;
	footer->addStretch();
	footer->addWidget( m_submitButton );

	auto layout = new QVBoxLayout( this );
	layout->addLayout( header );
	layout->addWidget( scrollArea );
	layout->addLayout( footer );

	connect( m_submitButton, &QPushButton::clicked, this, [this]() { submit( true ); } );

	if( m_remainingSeconds > 0 )
	{
		connect( &m_timer, &QTimer::timeout, this, &QuizWindow::updateRemainingTime );
		m_timer.start( 1000 );
	}
	updateRemainingTime();
}



QuizAnswers QuizWindow::answers() const
{
	QuizAnswers answers;
	for( const auto& question : m_quiz.questions )
	{
		const auto& widgets = m_widgets[question.id];
		QuizAnswer answer;
		if( widgets.textEdit )
		{
			answer.text = widgets.textEdit->text();
		}
		for( int i = 0; i < widgets.optionButtons.size(); ++i )
		{
			if( widgets.optionButtons[i]->isChecked() )
			{
				answer.selectedOptions.append( i );
			}
		}
		if( answer.isEmpty() == false )
		{
			answers[question.id] = answer;
		}
	}
	return answers;
}



void QuizWindow::closeEvent( QCloseEvent* event )
{
	// students cannot close the quiz before submitting; the teacher ends it
	if( m_submitted == false )
	{
		event->ignore();
		submit( true );
		return;
	}

	QWidget::closeEvent( event );
}



void QuizWindow::submit( bool confirm )
{
	if( m_submitted )
	{
		return;
	}

	if( confirm )
	{
		const auto unanswered = int(m_quiz.questions.size()) - m_quiz.answeredCount( answers() );
		const auto question = unanswered > 0
								  ? tr( "%n question(s) not answered yet. Submit your answers anyway?", nullptr, unanswered )
								  : tr( "Submit your answers? You cannot change them afterwards." );
		if( QMessageBox::question( this, windowTitle(), question ) != QMessageBox::Yes )
		{
			return;
		}
	}

	m_submitted = true;
	m_timer.stop();

	for( const auto& widgets : std::as_const( m_widgets ) )
	{
		for( auto button : widgets.optionButtons )
		{
			button->setEnabled( false );
		}
		if( widgets.textEdit )
		{
			widgets.textEdit->setReadOnly( true );
		}
	}

	m_submitButton->setEnabled( false );
	m_submitButton->setText( tr( "Answers submitted" ) );
	m_timeLabel->setText( tr( "Thank you!" ) );

	Q_EMIT answersChanged( answers(), true );
}



void QuizWindow::updateRemainingTime()
{
	if( m_quiz.timeLimitMinutes <= 0 )
	{
		m_timeLabel->clear();
		return;
	}

	if( m_remainingSeconds <= 0 )
	{
		submit( false );
		return;
	}

	// keep "mm:ss" in left-to-right order also in right-to-left layouts
	const auto time = QStringLiteral("\u2066%1:%2\u2069").arg( m_remainingSeconds / 60 )
						  .arg( m_remainingSeconds % 60, 2, 10, QLatin1Char('0') );
	m_timeLabel->setText( tr( "Time left: %1" ).arg( time ) );
	--m_remainingSeconds;
}
