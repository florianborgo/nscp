///////////////////////////////////////////////////////////////////////////
// NSClient++ Base Service
//
// Copyright (c) 2004 MySolutions NORDIC (http://www.medin.name)
//
// Date: 2004-03-13
// Author: Michael Medin (michael@medin.name)
//
// Part of this file is based on work by Bruno Vais (bvais@usa.net)
//
// This software is provided "AS IS", without a warranty of any kind.
// You are free to use/modify this code but leave this header intact.
//
//////////////////////////////////////////////////////////////////////////
#include "NSClient++.h"

#include <config.h>
#include "core_api.h"
#include <charEx.h>
#include <string.h>
#include <settings/settings_core.hpp>
#include <nscapi/nscapi_helper.hpp>
#ifdef _WIN32
#include <ServiceCmd.h>
#endif
#include <nsclient/logger.hpp>

#ifdef HAVE_JSON_SPIRIT
//#define JSON_SPIRIT_VALUE_ENABLED
#include <json_spirit.h>
#include <nscapi/nscapi_protobuf_functions.hpp>
#include <plugin.pb-json.h>
#endif

#define LOG_ERROR_STD(msg) LOG_ERROR(((std::string)msg).c_str())
#define LOG_CRITICAL_STD(msg) LOG_CRITICAL(((std::string)msg).c_str())
#define LOG_MESSAGE_STD(msg) LOG_MESSAGE(((std::string)msg).c_str())
#define LOG_DEBUG_STD(msg) LOG_DEBUG(((std::string)msg).c_str())

#define LOG_ERROR(msg) { nsclient::logging::logger::get_logger()->error("core", __FILE__, __LINE__, msg); }
#define LOG_CRITICAL(msg) { nsclient::logging::logger::get_logger()->error("core", __FILE__, __LINE__, msg); }
#define LOG_MESSAGE(msg) { nsclient::logging::logger::get_logger()->info("core", __FILE__, __LINE__, msg); }
#define LOG_DEBUG(msg) { nsclient::logging::logger::get_logger()->debug("core", __FILE__, __LINE__, msg); }

NSCAPI::errorReturn NSAPIExpandPath(const char* key, char* buffer, unsigned int bufLen) {
	try {
		return nscapi::plugin_helper::wrapReturnString(buffer, bufLen, mainClient->expand_path(key), NSCAPI::api_return_codes::isSuccess);
	} catch (...) {
		LOG_ERROR_STD("Failed to getString: " + utf8::cvt<std::string>(key));
		return NSCAPI::api_return_codes::hasFailed;
	}
}

NSCAPI::errorReturn NSAPIGetApplicationName(char *buffer, unsigned int bufLen) {
	return nscapi::plugin_helper::wrapReturnString(buffer, bufLen, utf8::cvt<std::string>(APPLICATION_NAME), NSCAPI::api_return_codes::isSuccess);
}
NSCAPI::errorReturn NSAPIGetApplicationVersionStr(char *buffer, unsigned int bufLen) {
	return nscapi::plugin_helper::wrapReturnString(buffer, bufLen, utf8::cvt<std::string>(CURRENT_SERVICE_VERSION), NSCAPI::api_return_codes::isSuccess);
}
void NSAPISimpleMessage(const char* module, int loglevel, const char* file, int line, const char* message) {
	nsclient::logging::logger::get_logger()->log(module, loglevel, file, line, message);
}
void NSAPIMessage(const char* data, unsigned int count) {
	std::string message(data, count);
	nsclient::logging::logger::get_logger()->raw(message);
}
void NSAPIStopServer(void) {
	mainClient->get_service_control().stop();
}
NSCAPI::nagiosReturn NSAPIInject(const char *request_buffer, const unsigned int request_buffer_len, char **response_buffer, unsigned int *response_buffer_len) {
	std::string request(request_buffer, request_buffer_len), response;
	NSCAPI::nagiosReturn ret = mainClient->injectRAW(request, response);
	*response_buffer_len = static_cast<unsigned int>(response.size());
	if (response.empty())
		*response_buffer = NULL;
	else {
		*response_buffer = new char[*response_buffer_len + 10];
		memcpy(*response_buffer, response.c_str(), *response_buffer_len);
	}
	return ret;
}

NSCAPI::nagiosReturn NSAPIExecCommand(const char* target, const char *request_buffer, const unsigned int request_buffer_len, char **response_buffer, unsigned int *response_buffer_len) {
	std::string request(request_buffer, request_buffer_len), response;
	NSCAPI::nagiosReturn ret = mainClient->exec_command(target, request, response);
	*response_buffer_len = static_cast<unsigned int>(response.size());
	if (response.empty())
		*response_buffer = NULL;
	else {
		*response_buffer = new char[*response_buffer_len + 10];
		memcpy(*response_buffer, response.c_str(), *response_buffer_len);
	}
	return ret;
}

NSCAPI::boolReturn NSAPICheckLogMessages(int messageType) {
	return nsclient::logging::logger::get_logger()->should_log(messageType);
}

NSCAPI::errorReturn NSAPISettingsQuery(const char *request_buffer, const unsigned int request_buffer_len, char **response_buffer, unsigned int *response_buffer_len) {
	return mainClient->settings_query(request_buffer, request_buffer_len, response_buffer, response_buffer_len);
}
NSCAPI::errorReturn NSAPIRegistryQuery(const char *request_buffer, const unsigned int request_buffer_len, char **response_buffer, unsigned int *response_buffer_len) {
	return mainClient->registry_query(request_buffer, request_buffer_len, response_buffer, response_buffer_len);
}

wchar_t* copyString(const std::wstring &str) {
	std::size_t sz = str.size();
	wchar_t *tc = new wchar_t[sz + 2];
	wcsncpy(tc, str.c_str(), sz);
	return tc;
}

NSCAPI::errorReturn NSAPIReload(const char *module) {
	try {
		return mainClient->reload(module);
	} catch (const std::exception &e) {
		LOG_ERROR_STD("Reload failed: " + utf8::utf8_from_native(e.what()));
		return NSCAPI::api_return_codes::hasFailed;
	} catch (...) {
		LOG_ERROR_STD("Reload failed");
		return NSCAPI::api_return_codes::hasFailed;
	}
}

nscapi::core_api::FUNPTR NSAPILoader(const char* buffer) {
	if (strcmp(buffer, "NSAPIGetApplicationName") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIGetApplicationName);
	if (strcmp(buffer, "NSAPIGetApplicationVersionStr") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIGetApplicationVersionStr);
	if (strcmp(buffer, "NSAPIMessage") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIMessage);
	if (strcmp(buffer, "NSAPISimpleMessage") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPISimpleMessage);
	if (strcmp(buffer, "NSAPIInject") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIInject);
	if (strcmp(buffer, "NSAPIExecCommand") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIExecCommand);
	if (strcmp(buffer, "NSAPICheckLogMessages") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPICheckLogMessages);
	if (strcmp(buffer, "NSAPINotify") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPINotify);
	if (strcmp(buffer, "NSAPIDestroyBuffer") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIDestroyBuffer);
	if (strcmp(buffer, "NSAPIExpandPath") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIExpandPath);
	if (strcmp(buffer, "NSAPIReload") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIReload);
	if (strcmp(buffer, "NSAPIGetLoglevel") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIGetLoglevel);
	if (strcmp(buffer, "NSAPISettingsQuery") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPISettingsQuery);
	if (strcmp(buffer, "NSAPIRegistryQuery") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSAPIRegistryQuery);
	if (strcmp(buffer, "NSCAPIJson2Protobuf") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSCAPIJson2Protobuf);
	if (strcmp(buffer, "NSCAPIProtobuf2Json") == 0)
		return reinterpret_cast<nscapi::core_api::FUNPTR>(&NSCAPIProtobuf2Json);
	LOG_ERROR_STD("Function not found: " + buffer);
	return NULL;
}

NSCAPI::errorReturn NSAPINotify(const char* channel, const char* request_buffer, unsigned int request_buffer_len, char ** response_buffer, unsigned int *response_buffer_len) {
	std::string request(request_buffer, request_buffer_len), response;
	NSCAPI::nagiosReturn ret = mainClient->send_notification(channel, request, response);
	*response_buffer_len = static_cast<unsigned int>(response.size());
	if (response.empty())
		*response_buffer = NULL;
	else {
		*response_buffer = new char[*response_buffer_len + 10];
		memcpy(*response_buffer, response.c_str(), *response_buffer_len);
	}
	return ret;
}

void NSAPIDestroyBuffer(char**buffer) {
	delete[] * buffer;
}

NSCAPI::log_level::level NSAPIGetLoglevel() {
	return nsclient::logging::logger::get_logger()->get_log_level();
}

#ifdef HAVE_JSON_SPIRIT
#include <nscapi/nscapi_protobuf.hpp>

NSCAPI::errorReturn NSCAPIJson2Protobuf(const char* request_buffer, unsigned int request_buffer_len, char ** response_buffer, unsigned int *response_buffer_len) {
	std::string request(request_buffer, request_buffer_len), response;
	try {
		json_spirit::Value root;
		json_spirit::read_or_throw(request, root);
		std::string object_type;
		json_spirit::Object o = root.getObject();
		BOOST_FOREACH(const json_spirit::Object::value_type &p, o) {
			if (p.first == "type" && p.second.type() == json_spirit::Value::STRING_TYPE)
				object_type = p.second.getString();
		}
		std::string response;
		if (object_type.empty()) {
			LOG_ERROR_STD("Missing type or payload.");
			return NSCAPI::api_return_codes::hasFailed;
		} else if (object_type == "SettingsRequestMessage") {
			Plugin::SettingsRequestMessage request_message;
			json_pb::Plugin::SettingsRequestMessage::to_pb(&request_message, o);
			response = request_message.SerializeAsString();
		} else if (object_type == "RegistryRequestMessage") {
			Plugin::RegistryRequestMessage request_message;
			json_pb::Plugin::RegistryRequestMessage::to_pb(&request_message, o);
			response = request_message.SerializeAsString();
		} else {
			LOG_ERROR_STD("Missing type or payload.");
			return NSCAPI::api_return_codes::hasFailed;
		}
		*response_buffer_len = static_cast<unsigned int>(response.size());
		if (response.empty())
			*response_buffer = NULL;
		else {
			*response_buffer = new char[*response_buffer_len + 10];
			memcpy(*response_buffer, response.c_str(), *response_buffer_len);
		}
	} catch (const json_spirit::ParseError &e) {
		LOG_ERROR_STD("Failed to parse JSON: " + e.reason_);
		return NSCAPI::api_return_codes::hasFailed;
	}
	return NSCAPI::api_return_codes::isSuccess;
}

NSCAPI::errorReturn NSCAPIProtobuf2Json(const char* object, const char* request_buffer, unsigned int request_buffer_len, char ** response_buffer, unsigned int *response_buffer_len) {
	std::string request(request_buffer, request_buffer_len), response, obj(object);
	try {
		json_spirit::Object root;
		if (obj == "SettingsResponseMessage") {
			Plugin::SettingsResponseMessage message;
			message.ParseFromString(request);
			root = json_pb::Plugin::SettingsResponseMessage::to_json(message);
		} else if (obj == "RegistryResponseMessage") {
			Plugin::RegistryResponseMessage message;
			message.ParseFromString(request);
			root = json_pb::Plugin::RegistryResponseMessage::to_json(message);
		} else if (obj == "QueryResponseMessage") {
			Plugin::QueryResponseMessage message;
			message.ParseFromString(request);
			root = json_pb::Plugin::QueryResponseMessage::to_json(message);
		} else if (obj == "ExecuteResponseMessage") {
			Plugin::ExecuteResponseMessage message;
			message.ParseFromString(request);
			root = json_pb::Plugin::ExecuteResponseMessage::to_json(message);
		} else {
			LOG_ERROR_STD("Invalid type: " + obj);
			return NSCAPI::api_return_codes::hasFailed;
		}
		std::string response = json_spirit::write(root);
		*response_buffer_len = static_cast<unsigned int>(response.size());
		if (response.empty())
			*response_buffer = NULL;
		else {
			*response_buffer = new char[*response_buffer_len + 10];
			memcpy(*response_buffer, response.c_str(), *response_buffer_len);
		}
	} catch (const json_spirit::ParseError &e) {
		LOG_ERROR_STD("Failed to parse JSON: " + e.reason_);
		return NSCAPI::api_return_codes::hasFailed;
	}
	return NSCAPI::api_return_codes::isSuccess;
}
#else
NSCAPI::errorReturn NSCAPIJson2protobuf(const char* request_buffer, unsigned int request_buffer_len, char ** response_buffer, unsigned int *response_buffer_len) {
	LOG_ERROR_STD("Not compiled with jason spirit so json not supported");
	return NSCAPI::hasFailed;
}
NSCAPI::errorReturn NSCAPIProtobuf2Json(const char* object, const char* request_buffer, unsigned int request_buffer_len, char ** response_buffer, unsigned int *response_buffer_len) {
	LOG_ERROR_STD("Not compiled with jason spirit so json not supported");
	return NSCAPI::hasFailed;
}
#endif