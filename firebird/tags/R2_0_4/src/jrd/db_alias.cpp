/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include <algorithm>
#include "../common/config/config.h"
#include "../common/config/config_file.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/gds_proto.h"

typedef Firebird::PathName string;

const char* ALIAS_FILE = "aliases.conf";

bool ResolveDatabaseAlias(const string& alias, string& database)
{
	string alias_filename;
	Firebird::Prefix(alias_filename, ALIAS_FILE);
	ConfigFile aliasConfig(false);
	aliasConfig.setConfigFilePath(alias_filename);

	const char correct_dir_sep = PathUtils::dir_sep;
	const char incorrect_dir_sep = (correct_dir_sep == '/') ? '\\' : '/';
	string corrected_alias = alias;
	std::replace(corrected_alias.begin(), corrected_alias.end(), incorrect_dir_sep, correct_dir_sep);
	
	database = aliasConfig.getString(corrected_alias);

	if (!database.empty())
	{
		std::replace(database.begin(), database.end(), incorrect_dir_sep, correct_dir_sep);
		if (PathUtils::isRelative(database)) {
			gds__log("Value %s configured for alias %s "
				"is not a fully qualified path name, ignored", 
						database.c_str(), alias.c_str());
			return false;
		}
		return true;
	}

	return false;
}
