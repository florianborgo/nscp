/**************************************************************************
*   Copyright (C) 2004-2007 by Michael Medin <michael@medin.name>         *
*                                                                         *
*   This code is part of NSClient++ - http://trac.nakednuns.org/nscp      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#pragma once

#include <string>
#include <map>

#include <boost/lexical_cast.hpp>

#include <strEx.h>
#include <error.hpp>
#include <WbemCli.h>

#include <atlbase.h>
//#include <atlcom.h>
//#include <atlstr.h>
#include <atlsafe.h>

#include <objidl.h>

namespace wmi_impl {
	class ComError {
	public:
		static std::wstring getWMIError(HRESULT hres) {
			switch (hres) {
			case WBEM_E_ACCESS_DENIED:
				return _T("The current user does not have permission to view the result set.");
			case WBEM_E_FAILED:
				return _T("This indicates other unspecified errors.");
			case WBEM_E_INVALID_PARAMETER:
				return _T("An invalid parameter was specified.");
			case WBEM_E_INVALID_QUERY:
				return _T("The query was not syntactically valid.");
			case WBEM_E_INVALID_QUERY_TYPE:
				return _T("The requested query language is not supported.");
			case WBEM_E_OUT_OF_MEMORY:
				return _T("There was not enough memory to complete the operation.");
			case WBEM_E_SHUTTING_DOWN:
				return _T("Windows Management service was stopped and restarted. A new call to ConnectServer is required.");
			case WBEM_E_TRANSPORT_FAILURE:
				return _T("This indicates the failure of the remote procedure call (RPC) link between the current process and Windows Management.");
			case WBEM_E_NOT_FOUND:
				return _T("The query specifies a class that does not exist.");
			default:
				return _T("");
			}
		}
		static std::string getComError(std::wstring inDesc = _T(""));
	};

	class wmi_exception : public std::exception {
		std::string message_;
	public:
		wmi_exception(std::string str, HRESULT code) {
			message_ = str + ":" + error::format::from_system(code);
		}
		wmi_exception(std::string str) {
			message_ = str;
		}
		~wmi_exception() throw() {}
		const char* what() const throw() {
			return reason().c_str();
		}

		std::string reason() const throw() {
			return message_;
		}
	};

	struct row {
		CComPtr<IWbemClassObject> row_obj;
		const std::list<std::string> &columns;
		row(const std::list<std::string> &columns) : columns(columns) {}

		std::string get_string(const std::string col);
		std::string to_string();
		long long get_int(const std::string col);
	};

	struct row_enumerator {
		row row_instance;
		CComPtr<IEnumWbemClassObject> enumerator_obj;
		row_enumerator(const std::list<std::string> &columns) : row_instance(columns) {}
		bool has_next();
		row& get_next();
	};
	struct header_enumerator {
		CComPtr<IEnumWbemClassObject> enumerator_obj;
		std::list<std::string> get();
	};
	struct wmi_service {
		CComPtr<IWbemServices> service;
		std::string ns;
		std::string username;
		std::string password;
		bool is_initialized;
		wmi_service(std::string ns, std::string username, std::string password) : ns(ns), username(username), password(password), is_initialized(false) {}
		CComPtr<IWbemServices>& get();
	};
	struct query {
		std::string wql_query;
		wmi_service instance;
		std::list<std::string> columns;
		query(std::string wql_query, std::string ns, std::string username, std::string password) : wql_query(wql_query), instance(ns, username, password) {}

		std::list<std::string> get_columns();
		row_enumerator execute();
	};

	struct instances {
		std::string super_class;
		wmi_service instance;
		std::list<std::string> columns;
		instances(std::string super_class, std::string ns, std::string username, std::string password) : super_class(super_class), instance(ns, username, password) {}
		row_enumerator get();
	};
	struct classes {
		std::string super_class;
		wmi_service instance;
		std::list<std::string> columns;
		classes(std::string super_class, std::string ns, std::string username, std::string password) : super_class(super_class), instance(ns, username, password) {}
		row_enumerator get();
	};
}