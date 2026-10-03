/*
 * LabSetupPlugin.cpp - lab setup page, teacher buttons and command line module
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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#ifdef Q_OS_WIN
#include <array>

#include <windows.h>
#include <sddl.h>
#endif

#include "AddComputersDialog.h"
#include "ConfigurationManager.h"
#include "Filesystem.h"
#include "LabSetupPage.h"
#include "LabSetupPlugin.h"
#include "StudentInstallerDialog.h"
#include "StudentPackage.h"
#include "StudentSetup.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"


LabSetupPlugin::LabSetupPlugin( QObject* parent ) :
	QObject( parent ),
	m_studentInstallerFeature( QStringLiteral("StudentInstaller"),
							   Feature::Flag::Action | Feature::Flag::Master,
							   Feature::Uid( "7b3f1d92-5c4e-4a8b-9e61-2d7c8f0a3b15" ),
							   Feature::Uid(),
							   tr( "Student installer" ), {},
							   tr( "Create the installer for the student computers, for example on a USB stick. It "
								   "contains the key and the settings of this computer." ),
							   QStringLiteral(":/labsetup/lab-setup.png") ),
	m_addComputersFeature( QStringLiteral("AddComputers"),
						   Feature::Flag::Action | Feature::Flag::Master,
						   Feature::Uid( "c41e8a27-9d3b-4f60-8b2a-5e7f1c9d4a36" ),
						   Feature::Uid(),
						   tr( "Add computers" ), {},
						   tr( "Search the network for the student computers and add them to a room." ),
						   QStringLiteral(":/labsetup/add-computers.png") ),
	m_settingsFeature( QStringLiteral("Settings"),
					   Feature::Flag::Action | Feature::Flag::Master,
					   Feature::Uid( "e9a05c3b-1f7d-4e28-a6b4-8c2d9f5e0a71" ),
					   Feature::Uid(),
					   tr( "Settings" ), {},
					   tr( "Open the settings of %1." ).arg( VeyonCore::productName() ),
					   QStringLiteral(":/labsetup/settings.png") ),
	m_features( { m_addComputersFeature, m_studentInstallerFeature, m_settingsFeature } ),
	m_commands( {
		{ QStringLiteral("setupteacher"), tr( "Set up this computer as the teacher computer" ) },
		{ QStringLiteral("createpackage"), tr( "Create a student installer from an installer" ) },
		{ QStringLiteral("extractpackage"), tr( "Extract the key and the settings of a student installer" ) },
	} )
{
}



LabSetupPlugin::~LabSetupPlugin()
{
	delete m_studentInstallerDialog;
	delete m_addComputersDialog;
}



ConfigurationPage* LabSetupPlugin::createConfigurationPage()
{
	return new LabSetupPage;
}



bool LabSetupPlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
									 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(featureUid)
	Q_UNUSED(operation)
	Q_UNUSED(arguments)
	Q_UNUSED(computerControlInterfaces)
	// interactive only
	return false;
}



bool LabSetupPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
								   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(computerControlInterfaces)

	if( feature.uid() == m_studentInstallerFeature.uid() )
	{
		if( m_studentInstallerDialog == nullptr )
		{
			m_studentInstallerDialog = new StudentInstallerDialog( master.mainWindow() );
		}
		m_studentInstallerDialog->show();
		m_studentInstallerDialog->raise();
		m_studentInstallerDialog->activateWindow();
		return true;
	}

	if( feature.uid() == m_addComputersFeature.uid() )
	{
		// a new dialog each time, so it knows the computers added meanwhile
		delete m_addComputersDialog;
		m_addComputersDialog = new AddComputersDialog( master.allComputerControlInterfaces(), master.mainWindow() );
		m_addComputersDialog->show();
		m_addComputersDialog->raise();
		m_addComputersDialog->activateWindow();
		return true;
	}

	if( feature.uid() == m_settingsFeature.uid() )
	{
		// the configurator asks for administrator rights itself
		return QProcess::startDetached( QDir( QCoreApplication::applicationDirPath() )
											.filePath( VeyonCore::executableName( QStringLiteral("configurator") ) +
													   VeyonCore::executableSuffix() ), {} );
	}

	return false;
}



QStringList LabSetupPlugin::commands() const
{
	return m_commands.keys();
}



QString LabSetupPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult LabSetupPlugin::handle_setupteacher( const QStringList& arguments )
{
	const auto keyName = arguments.value( 0, QStringLiteral("teacher") );
	if( StudentSetup::isValidKeyName( keyName ) == false )
	{
		error( tr( "Invalid key name %1." ).arg( keyName ) );
		return InvalidArguments;
	}

	if( QFileInfo::exists( VeyonCore::filesystem().privateKeyPath( keyName ) ) == false &&
		runCommandLine( { QStringLiteral("authkeys"), QStringLiteral("create"), keyName } ) != 0 )
	{
		error( tr( "Could not create the key %1." ).arg( keyName ) );
		return Failed;
	}

	// the teacher works with a normal account, which must be able to read the private key
	const auto usersGroup = localUsersGroupName();
	if( usersGroup.isEmpty() == false &&
		runCommandLine( { QStringLiteral("authkeys"), QStringLiteral("setaccessgroup"),
						  keyName + QStringLiteral("/private"), usersGroup } ) != 0 )
	{
		error( tr( "Could not allow the group %1 to use the key %2." ).arg( usersGroup, keyName ) );
	}

	VeyonCore::config().setAuthenticationMethod( VeyonCore::AuthenticationMethod::KeyFileAuthentication );
	ConfigurationManager configurationManager;
	if( configurationManager.saveConfiguration() == false )
	{
		error( configurationManager.errorString() );
		return Failed;
	}

	info( tr( "This computer is set up as the teacher computer with the key %1." ).arg( keyName ) );
	return Successful;
}



CommandLinePluginInterface::RunResult LabSetupPlugin::handle_createpackage( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto keyName = arguments.value( 2, StudentSetup::teacherKeyName(
											   VeyonCore::filesystem().expandPath( VeyonCore::config().publicKeyBaseDir() ) ) );
	QString errorMessage;
	const auto content = StudentPackage::fromCurrentConfiguration( keyName, VeyonCore::filesystem().publicKeyPath( keyName ),
																   &errorMessage );
	if( content.isValid() == false )
	{
		error( tr( "Could not read %1." ).arg( errorMessage.isEmpty() ? keyName : errorMessage ) );
		return Failed;
	}

	if( StudentPackage::create( arguments[0], arguments[1], content, &errorMessage ) == false )
	{
		error( tr( "Could not write %1." ).arg( errorMessage ) );
		return Failed;
	}

	info( tr( "Student installer written to %1." ).arg( arguments[1] ) );
	return Successful;
}



CommandLinePluginInterface::RunResult LabSetupPlugin::handle_extractpackage( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	QString errorMessage;
	const auto files = StudentPackage::extract( arguments[0], arguments[1], &errorMessage );
	if( files.isEmpty() )
	{
		error( errorMessage );
		return Failed;
	}

	for( const auto& file : files )
	{
		info( file );
	}
	return Successful;
}



QString LabSetupPlugin::localUsersGroupName()
{
#ifdef Q_OS_WIN
	// the name of the built-in group "Users" depends on the language of Windows
	PSID sid = nullptr;
	if( ConvertStringSidToSidW( L"S-1-5-32-545", &sid ) == false )
	{
		return {};
	}

	std::array<wchar_t, 256> name{};
	std::array<wchar_t, 256> domain{};
	DWORD nameLength = DWORD( name.size() );
	DWORD domainLength = DWORD( domain.size() );
	SID_NAME_USE use;
	const auto found = LookupAccountSidW( nullptr, sid, name.data(), &nameLength, domain.data(), &domainLength, &use );
	LocalFree( sid );

	return found ? QString::fromWCharArray( name.data() ) : QString();
#else
	return {};
#endif
}



int LabSetupPlugin::runCommandLine( const QStringList& arguments )
{
	return QProcess::execute( QCoreApplication::applicationFilePath(), arguments );
}
