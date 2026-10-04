/*
 * QuizEditorDialogs.cpp - dialogs for managing and editing quizzes
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

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUuid>
#include <QVBoxLayout>

#include "QuizEditorDialogs.h"


static const auto CorrectMarker = QLatin1Char('*');



QString QuizLibrary::directory()
{
	return QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) + QStringLiteral("/quizzes");
}



QList<Quiz> QuizLibrary::quizzes()
{
	QList<Quiz> result;
	QDir dir( directory() );
	const auto files = dir.entryList( { QStringLiteral("*.json") }, QDir::Files, QDir::Name );
	for( const auto& fileName : files )
	{
		QFile file( dir.filePath( fileName ) );
		if( file.open( QFile::ReadOnly ) )
		{
			result.append( Quiz::fromJson( QJsonDocument::fromJson( file.readAll() ).object() ) );
		}
	}

	std::sort( result.begin(), result.end(), []( const Quiz& a, const Quiz& b ) {
		return a.title.localeAwareCompare( b.title ) < 0;
	} );

	return result;
}



bool QuizLibrary::save( const Quiz& quiz )
{
	QDir().mkpath( directory() );
	QFile file( directory() + QStringLiteral("/%1.json").arg( quiz.id ) );
	return file.open( QFile::WriteOnly | QFile::Truncate ) &&
		   file.write( QJsonDocument( quiz.toJson() ).toJson() ) > 0;
}



bool QuizLibrary::remove( const Quiz& quiz )
{
	return QFile::remove( directory() + QStringLiteral("/%1.json").arg( quiz.id ) );
}



QuestionDialog::QuestionDialog( const QuizQuestion& question, QWidget* parent ) :
	QDialog( parent ),
	m_question( question ),
	m_textEdit( new QPlainTextEdit( question.text, this ) ),
	m_typeComboBox( new QComboBox( this ) ),
	m_answersEdit( new QPlainTextEdit( this ) ),
	m_pointsSpinBox( new QSpinBox( this ) ),
	m_answersHint( new QLabel( this ) )
{
	setWindowTitle( tr( "Question" ) );
	setMinimumWidth( 480 );

	m_typeComboBox->addItem( tr( "Single choice" ), int(QuizQuestion::Type::SingleChoice) );
	m_typeComboBox->addItem( tr( "Multiple choice" ), int(QuizQuestion::Type::MultipleChoice) );
	m_typeComboBox->addItem( tr( "Text answer" ), int(QuizQuestion::Type::Text) );
	m_typeComboBox->addItem( tr( "True or false" ), TrueFalseType );
	const auto isTrueFalse = question.type == QuizQuestion::Type::SingleChoice &&
							 question.options == trueFalseOptions();
	m_typeComboBox->setCurrentIndex( m_typeComboBox->findData( isTrueFalse ? TrueFalseType : int(question.type) ) );

	m_pointsSpinBox->setRange( 1, 100 );
	m_pointsSpinBox->setValue( question.points );

	m_textEdit->setMaximumHeight( 90 );
	m_answersHint->setWordWrap( true );
	m_answersHint->setForegroundRole( QPalette::PlaceholderText );

	QStringList lines;
	if( question.type == QuizQuestion::Type::Text )
	{
		lines = question.acceptedAnswers;
	}
	else
	{
		for( int i = 0; i < question.options.size(); ++i )
		{
			lines.append( ( question.correctOptions.contains( i ) ? QString( CorrectMarker ) : QString() ) + question.options[i] );
		}
	}
	m_answersEdit->setPlainText( lines.join( QLatin1Char('\n') ) );

	auto layout = new QFormLayout( this );
	layout->addRow( tr( "Question:" ), m_textEdit );
	layout->addRow( tr( "Type:" ), m_typeComboBox );
	layout->addRow( tr( "Answers:" ), m_answersEdit );
	layout->addRow( QString(), m_answersHint );
	layout->addRow( tr( "Points:" ), m_pointsSpinBox );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
	layout->addRow( buttonBox );

	connect( m_typeComboBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &QuestionDialog::updateType );
	updateHint();
}



QuizQuestion QuestionDialog::question() const
{
	auto question = m_question;
	if( question.id.isEmpty() )
	{
		question.id = QUuid::createUuid().toString( QUuid::WithoutBraces );
	}
	question.text = m_textEdit->toPlainText().trimmed();
	question.type = selectedType();
	question.points = m_pointsSpinBox->value();
	question.options.clear();
	question.correctOptions.clear();
	question.acceptedAnswers.clear();

	const auto lines = m_answersEdit->toPlainText().split( QLatin1Char('\n') );
	for( auto line : lines )
	{
		line = line.trimmed();
		if( line.isEmpty() )
		{
			continue;
		}

		if( question.type == QuizQuestion::Type::Text )
		{
			question.acceptedAnswers.append( line );
		}
		else
		{
			if( line.startsWith( CorrectMarker ) )
			{
				line = line.mid( 1 ).trimmed();
				question.correctOptions.append( question.options.size() );
			}
			question.options.append( line );
		}
	}

	return question;
}



void QuestionDialog::accept()
{
	const auto q = question();
	if( q.text.isEmpty() )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please enter the question." ) );
		return;
	}
	if( q.type != QuizQuestion::Type::Text && q.options.size() < 2 )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please enter at least two answer options, one per line." ) );
		return;
	}
	if( q.type == QuizQuestion::Type::SingleChoice && q.correctOptions.size() > 1 )
	{
		QMessageBox::warning( this, windowTitle(), tr( "A single choice question can have only one correct option." ) );
		return;
	}

	QDialog::accept();
}



QuizQuestion::Type QuestionDialog::selectedType() const
{
	const auto type = m_typeComboBox->currentData().toInt();
	return type == TrueFalseType ? QuizQuestion::Type::SingleChoice : QuizQuestion::Type( type );
}



QStringList QuestionDialog::trueFalseOptions() const
{
	return { tr( "True" ), tr( "False" ) };
}



void QuestionDialog::updateType()
{
	if( m_typeComboBox->currentData().toInt() == TrueFalseType )
	{
		// keep the * mark if the options are already "True" and "False"
		auto options = m_answersEdit->toPlainText().split( QLatin1Char('\n') );
		for( auto& option : options )
		{
			option = option.trimmed();
			if( option.startsWith( CorrectMarker ) )
			{
				option = option.mid( 1 ).trimmed();
			}
		}
		options.removeAll( QString() );
		if( options != trueFalseOptions() )
		{
			m_answersEdit->setPlainText( trueFalseOptions().join( QLatin1Char('\n') ) );
		}
	}

	updateHint();
}



void QuestionDialog::updateHint()
{
	if( m_typeComboBox->currentData().toInt() == TrueFalseType )
	{
		m_answersHint->setText( tr( "Mark the correct answer with * at the beginning, e.g. *True. "
									"Without a mark the question is not graded." ) );
	}
	else if( selectedType() == QuizQuestion::Type::Text )
	{
		m_answersHint->setText( tr( "Accepted answers, one per line (not case sensitive). "
									"Leave empty for questions without a correct answer." ) );
	}
	else
	{
		m_answersHint->setText( tr( "Answer options, one per line. Mark correct options with * at the "
									"beginning, e.g. *Algiers. Without a marked option the question is not graded." ) );
	}
}



QuizEditorDialog::QuizEditorDialog( const Quiz& quiz, QWidget* parent ) :
	QDialog( parent ),
	m_quiz( quiz ),
	m_titleEdit( new QLineEdit( quiz.title, this ) ),
	m_timeLimitSpinBox( new QSpinBox( this ) ),
	m_questionList( new QListWidget( this ) )
{
	setWindowTitle( tr( "Edit quiz" ) );
	setMinimumSize( 560, 460 );

	m_timeLimitSpinBox->setRange( 0, 240 );
	m_timeLimitSpinBox->setSuffix( tr( " minutes" ) );
	m_timeLimitSpinBox->setSpecialValueText( tr( "No time limit" ) );
	m_timeLimitSpinBox->setValue( quiz.timeLimitMinutes );

	auto form = new QFormLayout;
	form->addRow( tr( "Title:" ), m_titleEdit );
	form->addRow( tr( "Time limit:" ), m_timeLimitSpinBox );

	auto addButton = new QPushButton( tr( "Add question" ), this );
	auto editButton = new QPushButton( tr( "Edit" ), this );
	auto removeButton = new QPushButton( tr( "Remove" ), this );
	auto upButton = new QPushButton( tr( "Move up" ), this );
	auto downButton = new QPushButton( tr( "Move down" ), this );

	auto questionButtons = new QVBoxLayout;
	for( auto button : { addButton, editButton, removeButton, upButton, downButton } )
	{
		questionButtons->addWidget( button );
	}
	questionButtons->addStretch();

	auto questionsLayout = new QHBoxLayout;
	questionsLayout->addWidget( m_questionList );
	questionsLayout->addLayout( questionButtons );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );

	auto layout = new QVBoxLayout( this );
	layout->addLayout( form );
	layout->addWidget( new QLabel( tr( "Questions:" ), this ) );
	layout->addLayout( questionsLayout );
	layout->addWidget( buttonBox );

	connect( addButton, &QPushButton::clicked, this, [this]() { editQuestion( -1 ); } );
	connect( editButton, &QPushButton::clicked, this, [this]() { editQuestion( m_questionList->currentRow() ); } );
	connect( m_questionList, &QListWidget::itemDoubleClicked, this, [this]() { editQuestion( m_questionList->currentRow() ); } );
	connect( removeButton, &QPushButton::clicked, this, [this]() {
		const auto row = m_questionList->currentRow();
		if( row >= 0 )
		{
			m_quiz.questions.removeAt( row );
			updateQuestionList();
		}
	} );
	connect( upButton, &QPushButton::clicked, this, [this]() {
		const auto row = m_questionList->currentRow();
		if( row > 0 )
		{
			m_quiz.questions.swapItemsAt( row, row - 1 );
			updateQuestionList();
			m_questionList->setCurrentRow( row - 1 );
		}
	} );
	connect( downButton, &QPushButton::clicked, this, [this]() {
		const auto row = m_questionList->currentRow();
		if( row >= 0 && row < m_quiz.questions.size() - 1 )
		{
			m_quiz.questions.swapItemsAt( row, row + 1 );
			updateQuestionList();
			m_questionList->setCurrentRow( row + 1 );
		}
	} );

	updateQuestionList();
}



Quiz QuizEditorDialog::quiz() const
{
	auto quiz = m_quiz;
	quiz.title = m_titleEdit->text().trimmed();
	quiz.timeLimitMinutes = m_timeLimitSpinBox->value();
	return quiz;
}



void QuizEditorDialog::accept()
{
	if( m_titleEdit->text().trimmed().isEmpty() )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please enter a title." ) );
		return;
	}
	if( m_quiz.questions.isEmpty() )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please add at least one question." ) );
		return;
	}

	QDialog::accept();
}



void QuizEditorDialog::updateQuestionList()
{
	m_questionList->clear();
	for( int i = 0; i < m_quiz.questions.size(); ++i )
	{
		m_questionList->addItem( QStringLiteral("%1. %2").arg( i + 1 ).arg( m_quiz.questions[i].text.simplified() ) );
	}
}



void QuizEditorDialog::editQuestion( int index )
{
	const auto isNew = index < 0 || index >= m_quiz.questions.size();
	QuestionDialog dialog( isNew ? QuizQuestion{} : m_quiz.questions[index], this );
	if( dialog.exec() == QDialog::Accepted )
	{
		if( isNew )
		{
			m_quiz.questions.append( dialog.question() );
		}
		else
		{
			m_quiz.questions[index] = dialog.question();
		}
		updateQuestionList();
	}
}



QuizLauncherDialog::QuizLauncherDialog( QWidget* parent ) :
	QDialog( parent ),
	m_quizList( new QListWidget( this ) )
{
	setWindowTitle( tr( "Quizzes and polls" ) );
	setMinimumSize( 520, 400 );

	auto newQuizButton = new QPushButton( tr( "New quiz" ), this );
	auto newPollButton = new QPushButton( tr( "New poll" ), this );
	auto editButton = new QPushButton( tr( "Edit" ), this );
	auto removeButton = new QPushButton( tr( "Delete" ), this );

	auto buttons = new QVBoxLayout;
	for( auto button : { newQuizButton, newPollButton, editButton, removeButton } )
	{
		buttons->addWidget( button );
	}
	buttons->addStretch();

	auto listLayout = new QHBoxLayout;
	listLayout->addWidget( m_quizList );
	listLayout->addLayout( buttons );

	auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Cancel, this );
	auto startButton = buttonBox->addButton( tr( "Start" ), QDialogButtonBox::AcceptRole );
	startButton->setDefault( true );
	connect( buttonBox, &QDialogButtonBox::accepted, this, [this]() {
		if( m_quizList->currentRow() >= 0 )
		{
			accept();
		}
	} );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( new QLabel( tr( "Choose the quiz or poll to start on the selected computers:" ), this ) );
	layout->addLayout( listLayout );
	layout->addWidget( buttonBox );

	connect( newQuizButton, &QPushButton::clicked, this, [this]() {
		Quiz quiz;
		quiz.id = QUuid::createUuid().toString( QUuid::WithoutBraces );
		edit( quiz );
	} );
	connect( newPollButton, &QPushButton::clicked, this, [this]() {
		edit( Quiz::pollTemplate( tr( "Did you understand the lesson?" ),
								  { tr( "Yes" ), tr( "Partly" ), tr( "No" ) } ) );
	} );
	connect( editButton, &QPushButton::clicked, this, [this]() {
		const auto row = m_quizList->currentRow();
		if( row >= 0 )
		{
			edit( m_quizzes[row] );
		}
	} );
	connect( removeButton, &QPushButton::clicked, this, [this]() {
		const auto row = m_quizList->currentRow();
		if( row >= 0 &&
			QMessageBox::question( this, windowTitle(), tr( "Delete \"%1\"?" ).arg( m_quizzes[row].title ) ) == QMessageBox::Yes )
		{
			QuizLibrary::remove( m_quizzes[row] );
			reload();
		}
	} );
	connect( m_quizList, &QListWidget::itemDoubleClicked, this, &QDialog::accept );

	reload();
}



Quiz QuizLauncherDialog::selectedQuiz() const
{
	const auto row = m_quizList->currentRow();
	return row >= 0 && row < m_quizzes.size() ? m_quizzes[row] : Quiz{};
}



void QuizLauncherDialog::reload( const QString& selectId )
{
	m_quizzes = QuizLibrary::quizzes();
	m_quizList->clear();
	for( int i = 0; i < m_quizzes.size(); ++i )
	{
		const auto& quiz = m_quizzes[i];
		const auto details = quiz.isGraded() ? tr( "%n question(s)", nullptr, int(quiz.questions.size()) ) : tr( "poll" );
		m_quizList->addItem( QStringLiteral("%1 (%2)").arg( quiz.title, details ) );
		if( quiz.id == selectId )
		{
			m_quizList->setCurrentRow( i );
		}
	}
	if( m_quizList->currentRow() < 0 && m_quizList->count() > 0 )
	{
		m_quizList->setCurrentRow( 0 );
	}
}



void QuizLauncherDialog::edit( const Quiz& quiz )
{
	QuizEditorDialog editor( quiz, this );
	if( editor.exec() == QDialog::Accepted )
	{
		const auto edited = editor.quiz();
		if( QuizLibrary::save( edited ) == false )
		{
			QMessageBox::critical( this, windowTitle(), tr( "Could not save the quiz in %1." ).arg( QuizLibrary::directory() ) );
		}
		reload( edited.id );
	}
}
