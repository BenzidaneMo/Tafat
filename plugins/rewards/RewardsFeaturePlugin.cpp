/*
 * RewardsFeaturePlugin.cpp - give stars to the students
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

#include <QSettings>
#include <QTimer>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "RewardPopup.h"
#include "RewardsFeaturePlugin.h"
#include "RewardsWindow.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


// time after the popup disappeared until the worker is stopped
static constexpr int WorkerStopMargin = 2000;



RewardsFeaturePlugin::RewardsFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_rewardsFeature( QStringLiteral("Rewards"),
					  Feature::Flag::Action | Feature::Flag::AllComponents,
					  Feature::Uid( "9a4f2c7e-1b6d-4e83-a5c0-7f3e8d2b6a19" ),
					  Feature::Uid(),
					  tr( "Rewards" ), {},
					  tr( "Give stars to the selected students, for example for good work. Each student "
						  "sees the star and the number of stars." ),
					  QStringLiteral(":/rewards/rewards.png") ),
	m_giveStarFeature( QStringLiteral("GiveStar"),
					   Feature::Flag::Action | Feature::Flag::Master,
					   Feature::Uid( "c3e8a1f5-6d2b-4c97-8e4a-1b5f9d3c7a20" ),
					   m_rewardsFeature.uid(),
					   tr( "Give a star" ), {},
					   tr( "Give one star to each selected student." ) ),
	m_removeStarFeature( QStringLiteral("RemoveStar"),
						 Feature::Flag::Action | Feature::Flag::Master,
						 Feature::Uid( "5b7d3e9a-2f4c-4a18-b6e1-8c0a4f2d9e31" ),
						 m_rewardsFeature.uid(),
						 tr( "Remove a star" ), {},
						 tr( "Take one star back from each selected student." ) ),
	m_showRewardsFeature( QStringLiteral("ShowRewards"),
						  Feature::Flag::Action | Feature::Flag::Master,
						  Feature::Uid( "e1a6c4b8-9d3f-4e52-a7b0-3f8e2c6d1a42" ),
						  m_rewardsFeature.uid(),
						  tr( "Show stars" ), {},
						  tr( "Show the stars of all students and export them." ) ),
	m_features( { m_rewardsFeature, m_giveStarFeature, m_removeStarFeature, m_showRewardsFeature } )
{
}



RewardsFeaturePlugin::~RewardsFeaturePlugin()
{
	delete m_window;
	delete m_popup;
}



bool RewardsFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid != m_rewardsFeature.uid() || operation != Operation::Start )
	{
		return false;
	}

	sendFeatureMessage( FeatureMessage{ m_rewardsFeature.uid(), FeatureCommand::ShowReward }
							.addArgument( Argument::Stars, arguments.value( argToString( Argument::Stars ) ).toInt() )
							.addArgument( Argument::Change, arguments.value( argToString( Argument::Change ) ).toInt() ),
						computerControlInterfaces );
	return true;
}



bool RewardsFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces )
{
	loadBook();

	if( feature.uid() == m_giveStarFeature.uid() || feature.uid() == m_rewardsFeature.uid() )
	{
		changeStars( computerControlInterfaces, 1 );
		return true;
	}

	if( feature.uid() == m_removeStarFeature.uid() )
	{
		changeStars( computerControlInterfaces, -1 );
		return true;
	}

	if( feature.uid() == m_showRewardsFeature.uid() )
	{
		if( m_window == nullptr )
		{
			m_window = new RewardsWindow( master.mainWindow() );
			m_window->setWindowFlags( Qt::Window );
			connect( m_window, &RewardsWindow::removeStarRequested, this, [this]( const QString& student ) {
				m_book.remove( student );
				saveBook();
				updateWindow();
			} );
			connect( m_window, &RewardsWindow::resetRequested, this, [this]() {
				m_book.reset();
				saveBook();
				updateWindow();
			} );
		}
		updateWindow();
		m_window->show();
		m_window->raise();
		m_window->activateWindow();
		return true;
	}

	return false;
}



bool RewardsFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server, const MessageContext& messageContext,
												 const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_rewardsFeature.uid() )
	{
		auto& workerManager = server.featureWorkerManager();
		workerManager.sendMessageToUnmanagedSessionWorker( message );

		// stop the worker once the last popup has disappeared, so that the computer
		// does not keep reporting the feature as active
		const auto shown = ++m_rewardsShown;
		QTimer::singleShot( RewardPopup::DisplayTimeMs + WorkerStopMargin, this, [this, &workerManager, shown]() {
			if( shown == m_rewardsShown )
			{
				workerManager.stopWorker( m_rewardsFeature.uid() );
			}
		} );
		return true;
	}

	return false;
}



bool RewardsFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() != m_rewardsFeature.uid() || message.command<FeatureCommand>() != FeatureCommand::ShowReward )
	{
		return false;
	}

	if( m_popup == nullptr )
	{
		m_popup = new RewardPopup;
	}
	m_popup->showReward( message.argument( Argument::Stars ).toInt(), message.argument( Argument::Change ).toInt() );
	VeyonCore::platform().coreFunctions().raiseWindow( m_popup, true );

	return true;
}



void RewardsFeaturePlugin::changeStars( const ComputerControlInterfaceList& computers, int change )
{
	for( const auto& computer : computers )
	{
		const auto student = RewardBook::key( computer->userFullName(), computer->computerName() );
		if( student.isEmpty() || ( change < 0 && m_book.stars( student ) == 0 ) )
		{
			continue;
		}

		const auto stars = change > 0 ? m_book.add( student, change ) : m_book.remove( student, -change );
		controlFeature( m_rewardsFeature.uid(), Operation::Start,
						{ { argToString( Argument::Stars ), stars }, { argToString( Argument::Change ), change } },
						{ computer } );
	}

	saveBook();
	updateWindow();
}



void RewardsFeaturePlugin::loadBook()
{
	if( m_bookLoaded == false )
	{
		m_book = RewardBook::fromVariantMap(
			QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("Rewards") )
				.value( QStringLiteral("Stars") ).toMap() );
		m_bookLoaded = true;
	}
}



void RewardsFeaturePlugin::saveBook()
{
	QSettings( QSettings::UserScope, VeyonCore::productName(), QStringLiteral("Rewards") )
		.setValue( QStringLiteral("Stars"), m_book.toVariantMap() );
}



void RewardsFeaturePlugin::updateWindow()
{
	if( m_window )
	{
		m_window->setBook( m_book );
	}
}
