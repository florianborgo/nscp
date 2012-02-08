from NSCP import Settings, Registry, Core, log, status, log_error
from test_helper import BasicTest, TestResult, Callable, setup_singleton, install_testcases, init_testcases, shutdown_testcases

from sys import path
import os
path.append(os.getcwd() + '/scripts/python')

from test_nsca import NSCAServerTest
from test_nrpe import NRPEServerTest
#from test_pb import NSCAServerTest
from test_python import PythonTest

# 
all_tests = [NSCAServerTest, PythonTest, NRPEServerTest]
if os.name == 'nt':
	from test_eventlog import EventLogTest
	from test_w32_system import Win32SystemTest
	all_tests.extend([EventLogTest, Win32SystemTest])

def __main__():
	install_testcases(all_tests)
	
def init(plugin_id, plugin_alias, script_alias):
	init_testcases(plugin_id, plugin_alias, script_alias, all_tests)

def shutdown():
	shutdown_testcases()
