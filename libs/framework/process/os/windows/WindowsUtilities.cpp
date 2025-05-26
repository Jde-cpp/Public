using namespace std::chrono;
#include <codecvt>
#include "WindowsUtilities.h"
#include "../../Framework/source/DateTime.h"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#pragma warning( disable : 4996 )
namespace Jde{
	α Windows::ToWString( const string& value)ι->std::wstring{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes( value );
	}

	α Windows::ToString( const std::wstring& value)ι->string{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.to_bytes( value );
	}

	α Windows::ToSystemTime( TimePoint time )ι->SYSTEMTIME{
		DateTime date{time};
		SYSTEMTIME systemTime;
		systemTime.wYear = static_cast<WORD>( date.Year() );
		systemTime.wMonth = date.Month(); 
		systemTime.wDay = date.Day(); 
		systemTime.wHour = date.Hour();
		systemTime.wMinute = date.Minute();
		systemTime.wSecond = date.Second();
		systemTime.wMilliseconds = (WORD)( duration_cast<milliseconds>(time.time_since_epoch()).count()-duration_cast<seconds>(time.time_since_epoch()).count()*1000 );
		return systemTime;
	}

	α Windows::ToTimePoint( SYSTEMTIME systemTime )ι->TimePoint
	{
		return DateTime{ systemTime.wYear, static_cast<uint8>(systemTime.wMonth), static_cast<uint8>(systemTime.wDay), static_cast<uint8>(systemTime.wHour), static_cast<uint8>(systemTime.wMinute), static_cast<uint8>(systemTime.wSecond), milliseconds(systemTime.wMilliseconds) }.GetTimePoint();
	}
}