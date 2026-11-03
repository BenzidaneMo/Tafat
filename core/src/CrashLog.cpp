/*
 * CrashLog.cpp - crash report file for bug reports
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

#include <veyonconfig.h>

#include "CrashLog.h"

#ifdef Q_OS_WIN

#include <windows.h>

#include <cinttypes>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cwchar>

#ifndef UNW_FLAG_NHANDLER
#define UNW_FLAG_NHANDLER 0
#endif

namespace {

constexpr DWORD MaximumFileSize = 1024 * 1024;
constexpr int MaximumFrames = 40;
constexpr int MaximumModules = 300;

const char* volatile s_stage = "starting";
LPTOP_LEVEL_EXCEPTION_FILTER s_previousFilter = nullptr;
volatile LONG s_reportWritten = 0;
volatile LONG s_filterDepth = 0;
HANDLE s_file = INVALID_HANDLE_VALUE;

// static buffers, as the crash may be a stack overflow (only one report is written)
char s_line[6144];
char s_utf8[4096];
wchar_t s_wide[MAX_PATH * 2];
#if defined(__x86_64__) || defined(_M_X64)
CONTEXT s_context;
#endif


void write( const char* format, ... )
{
	if( s_file == INVALID_HANDLE_VALUE )
	{
		return;
	}

	va_list args;
	va_start( args, format );
	const auto length = vsnprintf( s_line, sizeof(s_line), format, args );
	va_end( args );

	if( length > 0 )
	{
		DWORD written = 0;
		WriteFile( s_file, s_line, DWORD( qMin<int>( length, int(sizeof(s_line)) - 1 ) ), &written, nullptr );
	}
}



const char* utf8( const wchar_t* text )
{
	if( WideCharToMultiByte( CP_UTF8, 0, text, -1, s_utf8, int(sizeof(s_utf8)), nullptr, nullptr ) == 0 )
	{
		s_utf8[0] = 0;
	}
	return s_utf8;
}



bool openFile()
{
	const auto length = GetTempPathW( MAX_PATH, s_wide );
	if( length == 0 || length >= MAX_PATH )
	{
		return false;
	}
	wcsncat( s_wide, L"" VEYON_PRODUCT_NAME L"-crash.log", MAX_PATH );

	s_file = CreateFileW( s_wide, GENERIC_WRITE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
						  nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr );
	if( s_file == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	// %TEMP% of the system account is writable by all users: don't write through links placed there
	BY_HANDLE_FILE_INFORMATION info{};
	if( GetFileInformationByHandle( s_file, &info ) == FALSE ||
		( info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ||
		info.nNumberOfLinks > 1 )
	{
		CloseHandle( s_file );
		s_file = INVALID_HANDLE_VALUE;
		return false;
	}

	if( info.nFileSizeHigh > 0 || info.nFileSizeLow > MaximumFileSize )
	{
		SetFilePointer( s_file, 0, nullptr, FILE_BEGIN );
		SetEndOfFile( s_file );
	}
	SetFilePointer( s_file, 0, nullptr, FILE_END );

	return true;
}



void closeFile()
{
	FlushFileBuffers( s_file );
	CloseHandle( s_file );
	s_file = INVALID_HANDLE_VALUE;
}



// "module.dll+0x1234", which can be resolved with addr2line/objdump on the same build
void writeAddress( const char* prefix, DWORD64 address )
{
	HMODULE module = nullptr;
	if( GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							reinterpret_cast<LPCWSTR>( static_cast<ULONG_PTR>( address ) ), &module ) &&
		module && GetModuleFileNameW( module, s_wide, MAX_PATH ) > 0 )
	{
		const auto name = wcsrchr( s_wide, L'\\' );
		write( "%s%s+0x%" PRIx64 "\r\n", prefix, utf8( name ? name + 1 : s_wide ),
			   uint64_t( address - reinterpret_cast<ULONG_PTR>( module ) ) );
	}
	else
	{
		write( "%s0x%" PRIx64 "\r\n", prefix, uint64_t( address ) );
	}
}



#if defined(__x86_64__) || defined(_M_X64)
void writeStack( const CONTEXT* initialContext )
{
	// 64-bit code has unwind tables, so the stack can be walked exactly
	write( "Stack:\r\n" );

	const auto tib = reinterpret_cast<const NT_TIB*>( NtCurrentTeb() );
	const auto stackLimit = reinterpret_cast<DWORD64>( tib->StackLimit );
	const auto stackBase = reinterpret_cast<DWORD64>( tib->StackBase );

	s_context = *initialContext;

	for( int i = 0; i < MaximumFrames && s_context.Rip != 0; ++i )
	{
		writeAddress( "  ", s_context.Rip );

		if( s_context.Rsp < stackLimit || s_context.Rsp + sizeof(DWORD64) > stackBase )
		{
			break;
		}

		DWORD64 imageBase = 0;
		const auto function = RtlLookupFunctionEntry( s_context.Rip, &imageBase, nullptr );
		if( function == nullptr )
		{
			// leaf function: the return address is on top of the stack
			s_context.Rip = *reinterpret_cast<const DWORD64*>( s_context.Rsp );
			s_context.Rsp += sizeof(DWORD64);
		}
		else
		{
			PVOID handlerData = nullptr;
			DWORD64 establisherFrame = 0;
			RtlVirtualUnwind( UNW_FLAG_NHANDLER, imageBase, s_context.Rip, function, &s_context,
							  &handlerData, &establisherFrame, nullptr );
		}
	}
}
#else
bool isCodeAddress( ULONG_PTR address )
{
	MEMORY_BASIC_INFORMATION info{};
	if( address < 0x10000 || VirtualQuery( reinterpret_cast<LPCVOID>( address ), &info, sizeof(info) ) == 0 )
	{
		return false;
	}

	return info.State == MEM_COMMIT && info.Type == MEM_IMAGE &&
			( info.Protect & ( PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY ) );
}



void writeStack( const CONTEXT* context )
{
	// 32-bit code usually has no frame pointers: list the code addresses found on the stack
	write( "Stack (code addresses found on the stack, may include stale ones):\r\n" );
	writeAddress( "  ", context->Eip );

	const auto tib = reinterpret_cast<const NT_TIB*>( NtCurrentTeb() );
	const auto end = static_cast<const ULONG_PTR*>( tib->StackBase );

	int frames = 1;
	for( auto entry = reinterpret_cast<const ULONG_PTR*>( context->Esp ); entry < end && frames < MaximumFrames; ++entry )
	{
		if( isCodeAddress( *entry ) )
		{
			writeAddress( "  ", *entry );
			++frames;
		}
	}
}
#endif



void writeModules()
{
	// walk the address space instead of using psapi/dbghelp, so nothing has to be loaded now
	write( "Modules:\r\n" );

	MEMORY_BASIC_INFORMATION info{};
	auto address = reinterpret_cast<const char*>( 0x10000 );
	int modules = 0;
	while( modules < MaximumModules && VirtualQuery( address, &info, sizeof(info) ) == sizeof(info) )
	{
		if( info.Type == MEM_IMAGE && info.AllocationBase == info.BaseAddress &&
			GetModuleFileNameW( static_cast<HMODULE>( info.AllocationBase ), s_wide, MAX_PATH ) > 0 )
		{
			write( "  0x%" PRIx64 " %s\r\n", uint64_t( reinterpret_cast<ULONG_PTR>( info.BaseAddress ) ), utf8( s_wide ) );
			++modules;
		}

		const auto next = static_cast<const char*>( info.BaseAddress ) + info.RegionSize;
		if( next <= address )
		{
			break;
		}
		address = next;
	}
}



void writeWindowsVersion()
{
	using RtlGetVersionFunction = LONG (WINAPI *)( PRTL_OSVERSIONINFOW );

	// GetVersionEx() reports Windows 8 on newer versions unless the manifest lists them
	OSVERSIONINFOEXW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunction>(
		reinterpret_cast<void *>( GetProcAddress( GetModuleHandleW( L"ntdll.dll" ), "RtlGetVersion" ) ) );

	BOOL wow64 = FALSE;
	IsWow64Process( GetCurrentProcess(), &wow64 );
	const auto bits = ( sizeof(void *) == 8 || wow64 ) ? "64-bit" : "32-bit";

	if( rtlGetVersion && rtlGetVersion( reinterpret_cast<PRTL_OSVERSIONINFOW>( &version ) ) == 0 )
	{
		write( "Windows: %lu.%lu build %lu, service pack %u, %s\r\n",
			   version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber,
			   unsigned( version.wServicePackMajor ), bits );
	}
	else
	{
		write( "Windows: unknown version, %s\r\n", bits );
	}
}



const char* exceptionName( DWORD code )
{
	switch( code )
	{
	case EXCEPTION_ACCESS_VIOLATION: return "access violation";
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction (CPU too old for this build?)";
	case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
	case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer division by zero";
	case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
	case EXCEPTION_IN_PAGE_ERROR: return "in-page error (file on a network or broken disk?)";
	case EXCEPTION_BREAKPOINT: return "breakpoint";
	case 0xC0000135: return "DLL not found";
	case 0xC0000139: return "entry point not found";
	case 0xC0000374: return "heap corruption";
	case 0xC0000409: return "stack buffer overrun / fast fail";
	case 0xE06D7363: return "MSVC C++ exception";
	case 0x20474343: return "GCC C++ exception";
	default: break;
	}
	return "unknown";
}



void writeReport( const char* reason, const EXCEPTION_RECORD* record, const CONTEXT* context )
{
	if( InterlockedExchange( &s_reportWritten, 1 ) != 0 || openFile() == false )
	{
		return;
	}

	SYSTEMTIME time{};
	GetLocalTime( &time );
	write( "==== %04u-%02u-%02u %02u:%02u:%02u " VEYON_PRODUCT_NAME " crash ====\r\n",
		   time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond );

	if( GetModuleFileNameW( nullptr, s_wide, MAX_PATH ) > 0 )
	{
		write( "Program: %s\r\n", utf8( s_wide ) );
	}
	write( "Command line: %s\r\n", utf8( GetCommandLineW() ) );
	write( "Version: " VEYON_VERSION " (Qt %s, %s build)\r\n", qVersion(), sizeof(void *) == 8 ? "64-bit" : "32-bit" );
	writeWindowsVersion();
	write( "Process %lu, thread %lu\r\n", GetCurrentProcessId(), GetCurrentThreadId() );
	write( "Stage: %s\r\n", s_stage );

	if( record )
	{
		write( "Exception: 0x%08lX (%s)\r\n", record->ExceptionCode, exceptionName( record->ExceptionCode ) );
		writeAddress( "Address: ", reinterpret_cast<ULONG_PTR>( record->ExceptionAddress ) );
		if( record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2 )
		{
			const auto operation = record->ExceptionInformation[0];
			write( "Access: %s 0x%" PRIx64 "\r\n",
				   operation == 0 ? "reading" : operation == 1 ? "writing" : "executing",
				   uint64_t( record->ExceptionInformation[1] ) );
		}
	}
	else
	{
		write( "Reason: %s\r\n", reason );
	}

	if( context )
	{
		writeStack( context );
	}
	writeModules();
	write( "\r\n" );

	closeFile();
}



LONG WINAPI handleException( EXCEPTION_POINTERS* pointers )
{
	// the C runtime's filter may call this one again as its previous filter
	if( InterlockedIncrement( &s_filterDepth ) > 1 )
	{
		InterlockedDecrement( &s_filterDepth );
		return EXCEPTION_CONTINUE_SEARCH;
	}

	writeReport( nullptr, pointers->ExceptionRecord, pointers->ContextRecord );

	// let Windows Error Reporting show its dialog and end the program as before
	const auto result = s_previousFilter ? s_previousFilter( pointers ) : EXCEPTION_CONTINUE_SEARCH;

	InterlockedDecrement( &s_filterDepth );

	return result;
}



void handleAbort( int )
{
	static CONTEXT context;
	RtlCaptureContext( &context );
	writeReport( "abort() called (fatal error or uncaught C++ exception)", nullptr, &context );
}



struct Installer
{
	Installer()
	{
		CrashLog::install();
	}
} s_installer;

}



void CrashLog::install()
{
	const auto previousFilter = SetUnhandledExceptionFilter( handleException );
	if( previousFilter != handleException )
	{
		s_previousFilter = previousFilter;
	}

	signal( SIGABRT, handleAbort );

	// room for writing the report after a stack overflow
	ULONG stackGuarantee = 32 * 1024;
	SetThreadStackGuarantee( &stackGuarantee );
}



void CrashLog::setStage( const char* stage )
{
	s_stage = stage;
}

#else

void CrashLog::install()
{
}



void CrashLog::setStage( const char* stage )
{
	Q_UNUSED(stage)
}

#endif
