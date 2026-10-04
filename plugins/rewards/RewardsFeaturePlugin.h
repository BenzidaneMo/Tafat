/*
 * RewardsFeaturePlugin.h - give stars to the students
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

#include <QPointer>

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "RewardBook.h"

class RewardPopup;
class RewardsWindow;

class RewardsFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.Rewards")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	// sent on the wire: only append
	enum class Argument
	{
		Stars,
		Change
	};
	Q_ENUM(Argument)

	explicit RewardsFeaturePlugin( QObject* parent = nullptr );
	~RewardsFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("2e6c4b1a-8f3d-4a95-b7e2-5d9c1f6a3e87") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Rewards");
	}

	QString description() const override
	{
		return tr( "Give stars to the students" );
	}

	QString vendor() const override
	{
		return VeyonCore::productName();
	}

	QString copyright() const override
	{
		return tr( "%1 contributors" ).arg( VeyonCore::productName() );
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	// sent on the wire: only append
	enum class FeatureCommand
	{
		ShowReward
	};

	void changeStars( const ComputerControlInterfaceList& computers, int change );
	void loadBook();
	void saveBook();
	void updateWindow();

	const Feature m_rewardsFeature;
	const Feature m_giveStarFeature;
	const Feature m_removeStarFeature;
	const Feature m_showRewardsFeature;
	const FeatureList m_features;

	RewardBook m_book;
	bool m_bookLoaded{false};
	QPointer<RewardsWindow> m_window;
	QPointer<RewardPopup> m_popup;

};
