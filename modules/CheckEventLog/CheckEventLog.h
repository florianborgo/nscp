NSC_WRAPPERS_MAIN();

#include <strEx.h>

class CheckEventLog {
private:

public:
	CheckEventLog();
	virtual ~CheckEventLog();
	// Module calls
	bool loadModule();
	bool unloadModule();
	std::string getModuleName();
	NSCModuleWrapper::module_version getModuleVersion();
	bool hasCommandHandler();
	bool hasMessageHandler();
	NSCAPI::nagiosReturn handleCommand(const strEx::blindstr command, const unsigned int argLen, char **char_args, std::string &message, std::string &perf);
};