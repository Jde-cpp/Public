﻿#pragma once
#ifndef EXTERNAL_LOGGER_H
#define EXTERNAL_LOGGER_H
#include "Message.h"

#define Φ Γ auto

namespace Jde::Logging{
	struct MessageBase;

	struct Γ ExternalMessage final : Message{
		ExternalMessage( const MessageBase& base )ι:ExternalMessage{ base, {} }{}
		ExternalMessage( const MessageBase& base, vector<string> args, TimePoint tp=Clock::now() )ι:Message{base}, TimePoint{tp}, Args{move(args)}{}
		ExternalMessage( const Message& b, vector<string> args )ι: Message{ b }, TimePoint{Clock::now()}, Args{ move(args) }{}
		ExternalMessage( const ExternalMessage& rhs ):Message{ rhs }, TimePoint{rhs.TimePoint}, Args{rhs.Args}{}
		const Jde::TimePoint TimePoint;
		vector<string> Args;
	};

	struct Γ IExternalLogger{
		virtual ~IExternalLogger(){}
		β Destroy(SRCE)ι->void=0;
		β DefaultLevel()ι->ELogLevel{ return _defaultLevel; }
		β Name()ι->string=0;
		β Log( ExternalMessage&& m, SRCE )ι->void=0;
		β Log( const ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void=0;
		β MinLevel()ι->ELogLevel{ return _minLevel; }
		β SetMinLevel( ELogLevel level )ι->void=0;
		concurrent_flat_map<ELogTags,ELogLevel> Tags;
	protected:
		ELogLevel	_minLevel{ ELogLevel::NoLog };
		ELogLevel _defaultLevel{ ELogLevel::NoLog };
	};
namespace External{
	Φ Add( up<IExternalLogger>&& l )ι->void;
	Φ Size()ι->uint;
	Φ Loggers()ι->const vector<up<Logging::IExternalLogger>>&;
	Φ Log( const Logging::MessageBase& messageBase )ι->void;
	Φ Log( const Logging::ExternalMessage& message )ι->void;
	Φ MinLevel()ι->ELogLevel;
	Φ MinLevel( sv externalName )ι->ELogLevel;
	Φ MinLevel( ELogTags tags )ι->ELogLevel;
	Φ DestroyLoggers()ι->void;
	Φ SetMinLevel( ELogLevel level )ι->void;
}}
#undef Φ
#endif