#pragma once
#include <jde/io/Json.h>
#include "../../../Framework/source/Settings.h"

namespace Jde::UM{
	struct UMSettings{
		UMSettings()ε;
		const string ConnectionString;
		const string Driver;
		const fs::path Meta;
		const string Target;
	};

	UMSettings::UMSettings()ε:
		ConnectionString{ Settings::Envɛ("um/connectionString") },
		Driver{ Settings::Get("um/driver").value_or(Settings::Get("db/connectionString").value_or("Jde.DB.Odbc.dll")) },
		Meta{ Settings::Get<fs::path>("um/metaPath").value_or(Settings::Get<fs::path>("db/meta").value_or("um.json")) },
		Target{ Settings::Get("un/target").value_or("/um/") }//TODO! see if can change to um
	{}
}