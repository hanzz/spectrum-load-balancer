#ifndef _HI_LOG_H
#define _HI_LOG_H

#include <time.h>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include "gloox/loghandler.h"
#include <glib.h>
#include <glib/gstdio.h>

using namespace gloox;


typedef enum {	LOG_AREA_XML = 2,
				LOG_AREA_PURPLE = 4
				} LogAreas;

class LogClass;

class LogMessage {
	public:
		LogMessage(std::ofstream &file, bool newline = true);
		~LogMessage();

		std::ostringstream& Get(const std::string &user);

	protected:
		std::ostringstream os;
		std::ofstream &m_file;
		bool m_newline;
};

class LogClass : public LogHandler {
	public:
		LogClass();
		~LogClass();

		void setLogFile(const std::string &file);
		std::ofstream &fileStream();
		void handleLog(LogLevel level, LogArea area, const std::string &message);

	private:
		std::ofstream m_file;
};
#ifdef TESTS
	#define Log(HEAD,STRING) 
#else
	#define Log(HEAD,STRING) LogMessage(Log_.fileStream()).Get(HEAD) << STRING;
#endif

#endif
