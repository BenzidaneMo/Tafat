/*
 * AppControlFeaturePlugin.h - block or allow applications on student computers
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
#include <QTimer>

#include "AppControlDialog.h"
#include "AppHistory.h"
#include "ComputerControlInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "RunningAppsWindow.h"

class QMessageBox;

class AppControlFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.github.benzidanemo.Tafat.Plugins.AppControl")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument
	{
		Mode,
		Applications,
		BlockUsbStorage,
		BlockPrinting,
		History
	};
	Q_ENUM(Argument)

	explicit AppControlFeaturePlugin( QObject* parent = nullptr );
	~AppControlFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("4f029842-9dc4-459e-9604-ad44d3ca3de2") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("AppControl");
	}

	QString description() const override
	{
		return tr( "Block or allow applications on student computers" );
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

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	bool isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const override;

private:
	enum class FeatureCommand
	{
		Start,
		Stop,
		NotifyClosedApplications,
		QueryApplications,
		ApplicationList,
		CloseApplication
	};

	static constexpr int EnforcementInterval = 2000;
	static constexpr int HistoryInterval = 10000;

	void enforce( VeyonServerInterface& server );
	QStringList updateHistory();

	// master side
	void addRunningAppsComputer( const ComputerControlInterface::Pointer& computerControlInterface );
	ComputerControlInterfaceList runningAppsComputers() const;

	const Feature m_appControlFeature;
	const Feature m_runningAppsFeature;
	const FeatureList m_features;

	// master side
	bool m_masterModeActive{false};
	bool m_masterModeCancelled{false};
	QVariantMap m_masterStartArguments;
	QPointer<RunningAppsWindow> m_runningAppsWindow;
	QMap<QString, QPointer<ComputerControlInterface>> m_runningAppsComputers;

	// server side
	QTimer m_enforcementTimer;
	AppControlDialog::Mode m_mode{AppControlDialog::Mode::BlockListed};
	QStringList m_applications;
	QTimer m_historyTimer;
	AppHistory m_history;
	QString m_historyUser;

	// worker side
	QPointer<QMessageBox> m_notice;

};
