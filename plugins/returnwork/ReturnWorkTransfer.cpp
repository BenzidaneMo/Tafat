/*
 * ReturnWorkTransfer.cpp - sends each student's files to the student's computer
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

#include <QFileInfo>

#include "ReturnWorkTransfer.h"

using Command = ReturnWorkProtocol::Command;
using Argument = ReturnWorkProtocol::Argument;


ReturnWorkTransfer::ReturnWorkTransfer( QObject* parent ) :
	QObject( parent )
{
	m_timer.setInterval( ProcessInterval );
	connect( &m_timer, &QTimer::timeout, this, &ReturnWorkTransfer::process );
}



ReturnWorkTransfer::~ReturnWorkTransfer()
{
	cancel();
}



void ReturnWorkTransfer::start( const QList<Job>& jobs, const QString& destinationDirectory, bool overwriteExistingFiles )
{
	cancel();

	m_jobs = jobs;
	m_destinationDirectory = destinationDirectory;
	m_overwriteExistingFiles = overwriteExistingFiles;
	m_jobIndex = 0;
	m_fileIndex = 0;
	m_filesDone = 0;
	m_filesTotal = 0;
	for( const auto& job : std::as_const( m_jobs ) )
	{
		m_filesTotal += int(job.files.size());
	}

	Q_EMIT progressChanged( 0, m_filesTotal );
	m_timer.start();
}



void ReturnWorkTransfer::cancel()
{
	if( m_file.isOpen() && m_jobIndex < m_jobs.size() )
	{
		send( FeatureMessage{ ReturnWorkProtocol::DistributeFilesFeatureUid, Command::CancelFileTransfer }
				  .addArgument( Argument::TransferId, m_transferId ) );
		m_file.close();
	}
	m_timer.stop();
	m_jobs.clear();
}



void ReturnWorkTransfer::process()
{
	if( m_jobIndex >= m_jobs.size() )
	{
		m_timer.stop();
		m_jobs.clear();
		Q_EMIT finished();
		return;
	}

	const auto& job = m_jobs[m_jobIndex];

	if( job.computer.isNull() || job.computer->state() != ComputerControlInterface::State::Connected )
	{
		Q_EMIT errorOccurred( tr( "%1 is not connected, its files were not returned." )
								  .arg( job.computer ? job.computer->computerName() : QString{} ) );
		m_filesDone += int(job.files.size()) - m_fileIndex;
		m_file.close();
		nextJob();
		return;
	}

	if( m_file.isOpen() == false )
	{
		if( m_fileIndex >= job.files.size() )
		{
			// all files of this computer sent: let the file transfer worker quit
			send( FeatureMessage{ ReturnWorkProtocol::DistributeFilesFeatureUid, Command::StopWorker } );
			nextJob();
			return;
		}

		m_file.setFileName( job.files[m_fileIndex] );
		if( m_file.open( QFile::ReadOnly ) == false )
		{
			Q_EMIT errorOccurred( tr( "Could not read %1." ).arg( m_file.fileName() ) );
			++m_fileIndex;
			++m_filesDone;
			Q_EMIT progressChanged( m_filesDone, m_filesTotal );
			return;
		}

		m_transferId = QUuid::createUuid();
		send( FeatureMessage{ ReturnWorkProtocol::DistributeFilesFeatureUid, Command::StartFileTransfer }
				  .addArgument( Argument::TransferId, m_transferId )
				  .addArgument( Argument::FileName, QFileInfo( m_file.fileName() ).fileName() )
				  .addArgument( Argument::OverwriteExistingFile, m_overwriteExistingFiles )
				  .addArgument( Argument::DestinationDirectory, m_destinationDirectory ) );
		return;
	}

	// wait until the computer received the previous chunk
	if( job.computer->isMessageQueueEmpty() == false )
	{
		return;
	}

	const auto data = m_file.read( ChunkSize );
	if( data.isEmpty() == false )
	{
		send( FeatureMessage{ ReturnWorkProtocol::DistributeFilesFeatureUid, Command::ContinueFileTransfer }
				  .addArgument( Argument::TransferId, m_transferId )
				  .addArgument( Argument::DataChunk, data ) );
	}

	if( m_file.atEnd() )
	{
		send( FeatureMessage{ ReturnWorkProtocol::DistributeFilesFeatureUid, Command::FinishFileTransfer }
				  .addArgument( Argument::TransferId, m_transferId )
				  .addArgument( Argument::FileName, QFileInfo( m_file.fileName() ).fileName() )
				  .addArgument( Argument::OpenFileInApplication, false ) );
		m_file.close();
		++m_fileIndex;
		++m_filesDone;
		Q_EMIT progressChanged( m_filesDone, m_filesTotal );
	}
}



void ReturnWorkTransfer::nextJob()
{
	++m_jobIndex;
	m_fileIndex = 0;
	Q_EMIT progressChanged( m_filesDone, m_filesTotal );
}



void ReturnWorkTransfer::send( const FeatureMessage& message )
{
	if( m_jobIndex < m_jobs.size() && m_jobs[m_jobIndex].computer )
	{
		m_jobs[m_jobIndex].computer->sendFeatureMessage( message );
	}
}
