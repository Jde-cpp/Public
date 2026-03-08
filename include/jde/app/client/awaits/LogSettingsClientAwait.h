#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/app/log/ArchiveFile.h>
#include <jde/app/log/LogSettingsAwait.h>

namespace Jde::App::Client{
	struct LogSettingsClientAwait final : LogSettingsAwait{
		using base = LogSettingsAwait;
		LogSettingsClientAwait( QL::TableQL&& ql, SRCE )ι:base{move(ql), sl}{}
		α await_ready()ι->bool;
	};

	struct LogSettingsClientMAwait final : LogSettingsMAwait{
		using base = LogSettingsMAwait;
		LogSettingsClientMAwait( QL::MutationQL&& m, sp<App::IApp> appClient, UserPK executer, SRCE )ι:base{move(m), move(appClient), executer, sl}{}
		α Suspend()ι->void override;
		α UpdateApp( QL::MutationQL&& m )ι->TAwait<jvalue>::Task;
		Ω IsApplicable( const QL::MutationQL& m )ι->bool{ return m.CommandName=="updateLogSettings"; }
	};
}