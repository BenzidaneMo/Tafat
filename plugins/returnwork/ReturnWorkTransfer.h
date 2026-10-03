/*
 * ReturnWorkTransfer.h - sends each student's files to the student's computer
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

#include <QFile>
#include <QTimer>
#include <QUuid>

#include "ComputerControlInterface.h"
#include "Feature.h"
#include "FeatureMessage.h"

// The messages of the "Distribute" feature of the file transfer plugin
// (plugins/filetransfer/FileTransferPlugin.h), which receives the files on
// the student computers. The values must match the upstream enums;
// ReturnWorkTest checks this.
namespace ReturnWorkProtocol
{
	static const Feature::Uid DistributeFilesFeatureUid{ QStringLiteral("4a70bd5a-fab2-4a4b-a92a-a1e81d2b75ed") };

	enum class Command
	{
		StartFileTransfer = 0,
		ContinueFileTransfer = 1,
		CancelFileTransfer = 2,
		FinishFileTransfer = 3,
		StopWorker = 5
	};

	enum class Argument
	{
		TransferId = 0,
		FileName = 1,
		DataChunk = 2,
		OpenFileInApplication = 3,
		OverwriteExistingFile = 4,
		DestinationDirectory = 11
	};
}


// Unlike the file distribution, which sends the same files to all computers,
// this sends different files to each computer, one computer after another.
class ReturnWorkTransfer : public QObject
{
	Q_OBJECT
public:
	struct Job
	{
		ComputerControlInterface::Pointer computer;
		QStringList files;
	};

	explicit ReturnWorkTransfer( QObject* parent = nullptr );
	~ReturnWorkTransfer() override;

	// destinationDirectory: relative to the student's home folder
	void start( const QList<Job>& jobs, const QString& destinationDirectory, bool overwriteExistingFiles );
	void cancel();

	bool isRunning() const
	{
		return m_timer.isActive();
	}

Q_SIGNALS:
	void progressChanged( int filesDone, int filesTotal );
	void errorOccurred( const QString& message );
	void finished();

private:
	static constexpr int ProcessInterval = 25;
	static constexpr int ChunkSize = 256 * 1024;

	void process();
	void nextJob();
	void send( const FeatureMessage& message );

	QList<Job> m_jobs;
	QString m_destinationDirectory;
	bool m_overwriteExistingFiles{false};

	int m_jobIndex{0};
	int m_fileIndex{0};
	int m_filesDone{0};
	int m_filesTotal{0};
	QFile m_file;
	QUuid m_transferId;

	QTimer m_timer;

};
