/*
 * AddComputersDialog.cpp - finds the student computers and adds them to a room
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

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTimer>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "AddComputersDialog.h"
#include "ComputerScanner.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformPluginInterface.h"
#include "VeyonConfiguration.h"


AddComputersDialog::AddComputersDialog( const ComputerControlInterfaceList& knownComputers, QWidget* parent ) :
	QDialog( parent ),
	m_roomComboBox( new QComboBox( this ) ),
	m_searchButton( new QPushButton( tr( "Search" ), this ) ),
	m_progressBar( new QProgressBar( this ) ),
	m_list( new QTreeWidget( this ) ),
	m_manualEdit( new QLineEdit( this ) ),
	m_rangeEdit( new QLineEdit( this ) ),
	m_statusLabel( new QLabel( this ) ),
	m_addButton( new QPushButton( tr( "Add to the room" ), this ) ),
	m_scanner( new ComputerScanner( VeyonCore::config().veyonServerPort(), this ) ),
	m_refreshTimer( new QTimer( this ) )
{
	setWindowTitle( tr( "Add computers" ) );
	setWindowIcon( QIcon( QStringLiteral(":/labsetup/add-computers.png") ) );
	resize( 640, 520 );

	QStringList rooms;
	for( const auto& computer : knownComputers )
	{
		const auto& info = computer->computer();
		m_knownHosts.append( info.hostName().toLower() );
		if( info.hostAddress().isNull() == false )
		{
			m_knownHosts.append( info.hostAddress().toString() );
		}
		if( info.location().isEmpty() == false && rooms.contains( info.location() ) == false )
		{
			rooms.append( info.location() );
		}
	}
	if( rooms.isEmpty() )
	{
		rooms.append( tr( "Computer lab" ) );
	}
	m_roomComboBox->setEditable( true );
	m_roomComboBox->addItems( rooms );

	auto intro = new QLabel( tr( "Click \"Search\" to find the student computers in the network. Tick the ones of this "
								 "room and click \"Add to the room\". Student computers are found when %1 is installed "
								 "on them and they are switched on." ).arg( VeyonCore::productName() ), this );
	intro->setWordWrap( true );

	m_progressBar->setTextVisible( false );
	m_progressBar->hide();

	m_list->setColumnCount( 3 );
	m_list->setHeaderLabels( { tr( "Computer" ), tr( "IP address" ), tr( "Status" ) } );
	m_list->setRootIsDecorated( false );
	m_list->header()->setSectionResizeMode( NameColumn, QHeaderView::Stretch );

	m_manualEdit->setPlaceholderText( tr( "Computer name or IP address" ) );
	auto manualButton = new QPushButton( tr( "Add to list" ), this );

	auto searchLayout = new QHBoxLayout;
	searchLayout->addWidget( m_searchButton );
	searchLayout->addWidget( m_progressBar, 1 );

	auto manualLayout = new QHBoxLayout;
	manualLayout->addWidget( m_manualEdit, 1 );
	manualLayout->addWidget( manualButton );

	auto form = new QFormLayout;
	form->addRow( tr( "Room:" ), m_roomComboBox );
	m_rangeEdit->setPlaceholderText( tr( "Optional, for another network: 192.168.2.0/24 or 10.0.5.10-80" ) );
	m_rangeEdit->setClearButtonEnabled( true );
	form->addRow( tr( "Network to search:" ), m_rangeEdit );

	m_statusLabel->setWordWrap( true );

	auto buttons = new QDialogButtonBox( QDialogButtonBox::Close, this );
	buttons->addButton( m_addButton, QDialogButtonBox::ActionRole );

	auto layout = new QVBoxLayout( this );
	layout->addWidget( intro );
	layout->addLayout( form );
	layout->addLayout( searchLayout );
	layout->addWidget( m_list, 1 );
	layout->addLayout( manualLayout );
	layout->addWidget( m_statusLabel );
	layout->addWidget( buttons );

	connect( m_searchButton, &QPushButton::clicked, this, &AddComputersDialog::search );
	connect( manualButton, &QPushButton::clicked, this, &AddComputersDialog::addManual );
	connect( m_manualEdit, &QLineEdit::returnPressed, this, &AddComputersDialog::addManual );
	connect( m_addButton, &QPushButton::clicked, this, &AddComputersDialog::addToRoom );
	connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::close );
	connect( m_list, &QTreeWidget::itemChanged, this, &AddComputersDialog::updateState );

	m_refreshTimer->setInterval( RefreshIntervalMs );
	connect( m_refreshTimer, &QTimer::timeout, this, &AddComputersDialog::refreshDirectory );

	connect( m_scanner, &ComputerScanner::found, this, [this]( const QHostAddress& address ) {
		addFound( address.toString() );
	} );
	connect( m_scanner, &ComputerScanner::progress, this, [this]( int done, int total ) {
		m_progressBar->setMaximum( total );
		m_progressBar->setValue( done );
	} );
	connect( m_scanner, &ComputerScanner::finished, this, [this]() {
		m_progressBar->hide();
		m_searchButton->setEnabled( true );
		if( m_foundCount == 0 )
		{
			m_statusLabel->setText( tr( "No student computers found. Check that %1 is installed on them, that they are "
										"switched on and in the same network, and that no firewall blocks port %2. "
										"You can also type a computer name or IP address below." )
										.arg( VeyonCore::productName() ).arg( VeyonCore::config().veyonServerPort() ) );
		}
		else
		{
			m_statusLabel->setText( tr( "Computers found: %1" ).arg( m_foundCount ) );
		}
	} );

	// Enter in the name field adds to the list, it must not press "Add to the room"
	for( auto button : findChildren<QPushButton*>() )
	{
		button->setAutoDefault( false );
		button->setDefault( false );
	}

	updateState();
}



void AddComputersDialog::search()
{
	QList<QHostAddress> hosts;
	const auto range = m_rangeEdit->text().trimmed();
	if( range.isEmpty() )
	{
		hosts = ComputerScanner::localCandidateHosts();
		if( hosts.isEmpty() )
		{
			m_statusLabel->setText( tr( "This computer is not connected to a local network." ) );
			return;
		}
	}
	else
	{
		hosts = ComputerScanner::rangeHosts( range );
		if( hosts.isEmpty() )
		{
			m_statusLabel->setText( tr( "\"%1\" is not a local network range. Examples: 192.168.2.0/24, "
										"10.0.5.10-80 (at most %2 addresses)." ).arg( range ).arg( ComputerScanner::MaximumHosts ) );
			return;
		}
		// the teacher computer is not one of the student computers
		for( const auto& address : QNetworkInterface::allAddresses() )
		{
			hosts.removeAll( address );
		}
	}

	m_foundCount = 0;
	m_searchButton->setEnabled( false );
	m_progressBar->setValue( 0 );
	m_progressBar->show();
	m_statusLabel->setText( tr( "Searching…" ) );
	m_scanner->start( hosts );
}



void AddComputersDialog::addFound( const QString& host )
{
	++m_foundCount;
	auto item = addItem( host, host );
	if( item == nullptr )
	{
		return;
	}

	// show the computer name once it is known
	QHostInfo::lookupHost( host, this, [this, host]( const QHostInfo& info ) {
		if( info.error() != QHostInfo::NoError || info.hostName().isEmpty() || info.hostName() == host )
		{
			return;
		}
		const auto name = info.hostName().section( QLatin1Char('.'), 0, 0 );
		for( int i = 0; i < m_list->topLevelItemCount(); ++i )
		{
			auto item = m_list->topLevelItem( i );
			if( item->text( AddressColumn ) == host )
			{
				item->setText( NameColumn, name );
				if( isKnown( name ) )
				{
					item->setCheckState( NameColumn, Qt::Unchecked );
					item->setText( StatusColumn, tr( "already added" ) );
				}
			}
		}
	} );
}



void AddComputersDialog::addManual()
{
	const auto host = m_manualEdit->text().trimmed();
	if( host.isEmpty() || host.contains( QLatin1Char(';') ) || host.contains( QLatin1Char(' ') ) )
	{
		return;
	}
	addItem( host, host );
	m_manualEdit->clear();
}



QTreeWidgetItem* AddComputersDialog::addItem( const QString& name, const QString& host )
{
	for( int i = 0; i < m_list->topLevelItemCount(); ++i )
	{
		if( m_list->topLevelItem( i )->text( AddressColumn ).compare( host, Qt::CaseInsensitive ) == 0 )
		{
			return nullptr;
		}
	}

	auto item = new QTreeWidgetItem( m_list, { name, host, {} } );
	const auto known = isKnown( host ) || isKnown( name );
	item->setFlags( item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable );
	item->setCheckState( NameColumn, known ? Qt::Unchecked : Qt::Checked );
	item->setText( StatusColumn, known ? tr( "already added" ) : tr( "new" ) );
	updateState();
	return item;
}



bool AddComputersDialog::isKnown( const QString& host ) const
{
	return m_knownHosts.contains( host.toLower() ) || m_knownHosts.contains( host );
}



void AddComputersDialog::updateState()
{
	int checked = 0;
	for( int i = 0; i < m_list->topLevelItemCount(); ++i )
	{
		checked += m_list->topLevelItem( i )->checkState( NameColumn ) == Qt::Checked ? 1 : 0;
	}
	m_addButton->setEnabled( checked > 0 );
	m_addButton->setText( checked > 0 ? tr( "Add to the room (%1)" ).arg( checked ) : tr( "Add to the room" ) );
}



void AddComputersDialog::addToRoom()
{
	const auto room = m_roomComboBox->currentText().trimmed();
	if( room.isEmpty() )
	{
		QMessageBox::warning( this, windowTitle(), tr( "Please enter the name of the room." ) );
		return;
	}

	QList<QPair<QString, QString>> computers;
	QStringList names;
	for( int i = 0; i < m_list->topLevelItemCount(); ++i )
	{
		const auto item = m_list->topLevelItem( i );
		if( item->checkState( NameColumn ) == Qt::Checked )
		{
			computers.append( { item->text( NameColumn ), item->text( AddressColumn ) } );
			names.append( item->text( NameColumn ).trimmed() );
		}
	}
	if( computers.isEmpty() )
	{
		return;
	}

	// kept until the elevated import has read it; the temporary folder is cleaned by the system
	QTemporaryFile csv( QDir::temp().filePath( VeyonCore::productSlug() + QStringLiteral("-computers-XXXXXX.csv") ) );
	csv.setAutoRemove( false );
	if( csv.open() == false || csv.write( ComputerScanner::importData( computers ) ) < 0 )
	{
		QMessageBox::critical( this, windowTitle(), tr( "Could not write %1." ).arg( csv.fileName() ) );
		return;
	}
	csv.close();

	if( runImport( csv.fileName(), room ) == false )
	{
		QMessageBox::critical( this, windowTitle(), tr( "The computers could not be added. Please try again and "
														"allow the change when you are asked for administrator rights." ) );
		return;
	}

	for( int i = 0; i < m_list->topLevelItemCount(); ++i )
	{
		auto item = m_list->topLevelItem( i );
		if( item->checkState( NameColumn ) == Qt::Checked )
		{
			m_knownHosts.append( item->text( AddressColumn ) );
			item->setCheckState( NameColumn, Qt::Unchecked );
			item->setText( StatusColumn, tr( "added" ) );
		}
	}

	m_statusLabel->setText( tr( "The computers were added to the room \"%1\". They appear in %2 within a minute." )
								.arg( room, VeyonCore::productName() ) );

	m_addedRoom = room;
	m_addedNames = names;
	m_refreshAttempts = 0;
	m_refreshTimer->start();
	refreshDirectory();
}



void AddComputersDialog::refreshDirectory()
{
	// the import may still wait for the permission prompt, so try again for a while
	VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();

	++m_refreshAttempts;
	if( showRoom() || m_refreshAttempts >= RefreshAttempts )
	{
		m_refreshTimer->stop();
	}
}



bool AddComputersDialog::showRoom()
{
	// the tree of the master panel "Locations & computers"; ticking the room shows its computers
	const auto panel = parentWidget() ? parentWidget()->findChild<QWidget*>( QStringLiteral("ComputerSelectPanel") ) : nullptr;
	const auto treeView = panel ? panel->findChild<QTreeView*>( QStringLiteral("treeView") ) : nullptr;
	const auto model = treeView ? treeView->model() : nullptr;
	if( model == nullptr )
	{
		return false;
	}

	for( int row = 0; row < model->rowCount(); ++row )
	{
		const auto roomIndex = model->index( row, 0 );
		if( roomIndex.data().toString() != m_addedRoom )
		{
			continue;
		}

		QStringList names;
		for( int child = 0; child < model->rowCount( roomIndex ); ++child )
		{
			names.append( model->index( child, 0, roomIndex ).data().toString() );
		}
		for( const auto& name : std::as_const( m_addedNames ) )
		{
			if( names.contains( name ) == false )
			{
				return false;
			}
		}

		model->setData( roomIndex, Qt::Checked, Qt::CheckStateRole );
		treeView->expand( roomIndex );
		return true;
	}

	return false;
}



bool AddComputersDialog::runImport( const QString& csvFile, const QString& room )
{
	const QStringList arguments{
		QStringLiteral("networkobjects"), QStringLiteral("import"), QDir::toNativeSeparators( csvFile ),
		QStringLiteral("location"), room,
		QStringLiteral("format"), QStringLiteral("%name%;%host%")
	};

#ifdef Q_OS_WIN
	const auto cli = QDir( QCoreApplication::applicationDirPath() ).filePath( VeyonCore::executableName( QStringLiteral("wcli") ) +
																				VeyonCore::executableSuffix() );
#else
	const auto cli = QDir( QCoreApplication::applicationDirPath() ).filePath( VeyonCore::executableName( QStringLiteral("cli") ) +
																				VeyonCore::executableSuffix() );
#endif

	auto& coreFunctions = VeyonCore::platform().coreFunctions();
	if( coreFunctions.isRunningAsAdmin() )
	{
		return QProcess::execute( cli, arguments ) == 0;
	}

#ifdef Q_OS_WIN
	// the arguments are joined with spaces for ShellExecuteEx, so quote them
	QStringList quoted;
	for( const auto& argument : arguments )
	{
		quoted.append( QStringLiteral("\"%1\"").arg( argument ) );
	}
	return coreFunctions.runProgramAsAdmin( cli, quoted );
#else
	return coreFunctions.runProgramAsAdmin( cli, arguments );
#endif
}
