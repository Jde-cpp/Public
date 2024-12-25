#pragma once
//#include <jde/framework/io/json.h>
#include <jde/framework/settings.h>

namespace Jde::Access{
	struct AccessSettings{
		AccessSettings()ε;
		const string ConnectionString;
		const string Driver;
		const fs::path Meta;
		const string Target;
	};

	AccessSettings::AccessSettings()ε:
		//ConnectionString{ Settings::At("um/connectionString") },
		//Driver{ Settings::Get("um/driver").value_or(Settings::Get("db/connectionString").value_or("Jde.DB.Odbc.dll")) },
		//Meta{ Settings::Get<fs::path>("um/metaPath").value_or(Settings::Get<fs::path>("db/meta").value_or("um.json")) },
		//Target{ Settings::Get("um/target").value_or("/um/") }//TODO! see if can change to um
	{}
}