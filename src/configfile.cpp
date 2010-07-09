/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "configfile.h"
#include "string.h"
#include <stdio.h>
#include  <stdlib.h>

#define LOAD_REQUIRED_STRING(VARIABLE, SECTION, KEY) {if (!loadString((VARIABLE), (SECTION), (KEY))) return DummyConfiguration;}
#define LOAD_REQUIRED_STRING_DEFAULT(VARIABLE, SECTION, KEY, DEFAULT) {if (!loadString((VARIABLE), (SECTION), (KEY), (DEFAULT))) return DummyConfiguration;}

Configuration DummyConfiguration;

ConfigFile::ConfigFile(const std::string &config) {
	m_loaded = true;
	m_jid = "";
	m_protocol = "";
	m_filename = "";

	keyfile = g_key_file_new ();
	if (!config.empty())
		loadFromFile(config);
}

void ConfigFile::loadFromFile(const std::string &config) {
	int flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

	if (!g_key_file_load_from_file (keyfile, config.c_str(), (GKeyFileFlags)flags, NULL)) {
		if (!g_key_file_load_from_file (keyfile, std::string( "/etc/spectrum/" + config + ".cfg").c_str(), (GKeyFileFlags)flags, NULL))
		{
			if (!g_key_file_load_from_file (keyfile, std::string( m_appdata + "/spectrum/" + config).c_str(), (GKeyFileFlags)flags, NULL))
			{
				Log("loadConfigFile", "Can't load config file! Tried these paths:");
				Log("loadConfigFile", std::string(config));
				Log("loadConfigFile", std::string("/etc/spectrum/" + config + ".cfg"));
				Log("loadConfigFile", std::string("./" + config));
				m_loaded = false;
			}
		}
	}
}

void ConfigFile::loadFromData(const std::string &data) {
	int flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

	if (!g_key_file_load_from_data (keyfile, data.c_str(), (int) data.size(), (GKeyFileFlags) flags, NULL)) {
		Log("loadConfigFile", "Bad data");
		m_loaded = false;
	}
}

bool ConfigFile::loadString(std::string &variable, const std::string &section, const std::string &key, const std::string &def) {
	char *value;
	if ((value = g_key_file_get_string(keyfile, section.c_str(), key.c_str(), NULL)) != NULL) {
		variable.assign(value);
		g_free(value);
	}
	else {
		if (def == "required") {
			Log("loadConfigFile", "You have to specify `" << key << "` in [" << section << "] section of config file.");
			return false;
		}
		else
			variable = def;
	}
	return true;
}

bool ConfigFile::loadInteger(int &variable, const std::string &section, const std::string &key, int def) {
	if (g_key_file_has_key(keyfile, section.c_str(), key.c_str(), NULL)) {
		GError *error = NULL;
		variable = (int) g_key_file_get_integer(keyfile, section.c_str(), key.c_str(), &error);
		if (error) {
			if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE) {
				char *value;
				if ((value = g_key_file_get_string(keyfile, section.c_str(), key.c_str(), NULL)) != NULL) {
					if (strcmp( value,"$filename:port") == 0) {
						variable = m_port;
						g_error_free(error);
						return true;
					}
				}
				Log("loadConfigFile", "Value of key `" << key << "` in [" << section << "] section is not integer.");
			}
			g_error_free(error);
			return false;
		}
	}
	else {
		if (def == INT_MAX) {
			Log("loadConfigFile", "You have to specify `" << key << "` in [" << section << "] section of config file.");
			return false;
		}
		else
			variable = def;
	}
	return true;
}

bool ConfigFile::loadBoolean(bool &variable, const std::string &section, const std::string &key, bool def, bool required) {
	if (g_key_file_has_key(keyfile, section.c_str(), key.c_str(), NULL))
		variable = g_key_file_get_boolean(keyfile, section.c_str(), key.c_str(), NULL);
	else {
		if (required) {
			Log("loadConfigFile", "You have to specify `" << key << "` in [" << section << "] section of config file.");
			return false;
		}
		else
			variable = def;
	}
	return true;
}

bool ConfigFile::loadStringList(std::list <std::string> &variable, const std::string &section, const std::string &key) {
	char **bind;
	variable.clear();
	if (g_key_file_has_key(keyfile, section.c_str(), key.c_str(), NULL)) {
		bind = g_key_file_get_string_list(keyfile, section.c_str(), key.c_str(), NULL, NULL);
		for (int i = 0; bind[i]; i++) {
			variable.push_back(bind[i]);
		}
		g_strfreev (bind);
		return true;
	}
	return false;
}

bool ConfigFile::loadHostPort(std::string &host, int &port, const std::string &section, const std::string &key, const std::string &def_host, const int &def_port) {
	std::string str;
	if (!g_key_file_has_key(keyfile, section.c_str(), key.c_str(), NULL)) {
		if (def_host == "required") {
			Log("loadConfigFile", "You have to specify `" << key << "` in [" << section << "] section of config file.");
			return false;
		}
		host = def_host;
		port = def_port;
		return true;
	}
	loadString(str, section, key);
	
	if (str.find_last_of(':') == std::string::npos)
		port = 0;
	else
		port = atoi(str.substr(str.find_last_of(':') + 1, str.size()).c_str());
	host = str.substr(0, str.find_last_of(':'));
	return true;
}


Configuration ConfigFile::getConfiguration() {
	Configuration configuration;
	char **bind;
	int i;
	
	if (!m_loaded)
		return DummyConfiguration;

	// Service section
	LOAD_REQUIRED_STRING(configuration.jid, "service", "jid");
	LOAD_REQUIRED_STRING(configuration.server, "service", "server");
	LOAD_REQUIRED_STRING(configuration.password, "service", "password");
	if (!loadInteger(configuration.port, "service", "port")) return DummyConfiguration;
	loadString(configuration.pid_f, "service", "pid_file", "/var/run/spectrum-load-balancer/" + configuration.jid);
	
	// Logging section
	loadString(configuration.logfile, "logging", "log_file", "");

	return configuration;
}

ConfigFile::~ConfigFile() {
	g_key_file_free(keyfile);
}

