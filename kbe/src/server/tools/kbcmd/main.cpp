/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2018 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/kbemain.h"
#include "kbcmd.h"
#include "client_sdk.h"
#include "server_assets.h"
#include "entitydef/entitydef.h"
#include "pyscript/py_zipfile.h"
#include <experimental/filesystem>

#undef DEFINE_IN_INTERFACE
#include "machine/machine_interface.h"
#define DEFINE_IN_INTERFACE
#include "machine/machine_interface.h"

#undef DEFINE_IN_INTERFACE
#include "client_lib/client_interface.h"
#define DEFINE_IN_INTERFACE
#include "client_lib/client_interface.h"

#undef DEFINE_IN_INTERFACE
#include "baseappmgr/baseappmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "baseappmgr/baseappmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "cellappmgr/cellappmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellappmgr/cellappmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "cellapp/cellapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellapp/cellapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "baseapp/baseapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "baseapp/baseapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "loginapp/loginapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "loginapp/loginapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "dbmgr/dbmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "dbmgr/dbmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/logger/logger_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/logger/logger_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/bots/bots_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/bots/bots_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/interfaces/interfaces_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/interfaces/interfaces_interface.h"

using namespace KBEngine;

#define PARSE_COMMAND_ARG_BEGIN()	\
	for (int argIdx = 1; argIdx < argc; ++argIdx)	\
	{	\
		std::string cmd = argv[argIdx];	\
		std::string findcmd;	\
		std::string::size_type fi1;	\

#define PARSE_COMMAND_ARG_DO_FUNC(NAME, EXEC)	\
	cmd = argv[argIdx];	\
	findcmd = NAME;	\
	fi1 = cmd.find(findcmd); \
	if (fi1 != std::string::npos)	\
	{	\
		cmd.erase(fi1, findcmd.size());	\
		try \
		{ \
			if (EXEC == -1) \
				return -1; \
		} \
		catch (...) \
		{ \
			ERROR_MSG("parseCommandArgs: "#NAME"? invalid, no set! type is uint64\n"); \
		} \
		\
		continue; \
	} \

#define PARSE_COMMAND_ARG_DO_FUNC_RETURN(NAME, EXEC)	\
	cmd = argv[argIdx];	\
	findcmd = NAME;	\
	fi1 = cmd.find(findcmd); \
	if (fi1 != std::string::npos)	\
	{	\
		cmd.erase(fi1, findcmd.size());	\
		try \
		{ \
			return EXEC; \
		} \
		catch (...) \
		{ \
			ERROR_MSG("parseCommandArgs: "#NAME"? invalid, no set! type is uint64\n"); \
		} \
		\
		continue; \
	} \

#define PARSE_COMMAND_ARG_GET_VALUE(NAME, VAL)	\
	cmd = argv[argIdx];	\
	findcmd = NAME;	\
	fi1 = cmd.find(findcmd);	\
	if (fi1 != std::string::npos)	\
	{	\
		cmd.erase(fi1, findcmd.size());	\
		if (cmd.size() > 0)	\
		{	\
			try \
			{ \
				VAL = cmd;	\
			} \
			catch (...) \
			{ \
				ERROR_MSG("parseCommandArgs: "#NAME"? invalid, no set! type is uint64\n"); \
			} \
		} \
		 \
		continue; \
	} \

#define PARSE_COMMAND_ARG_END()	}

int process_make_client_sdk(int argc, char* argv[], const std::string clientType)
{
	Resmgr::getSingleton().initialize();
	setEvns();
	loadConfig();

	DebugHelper::initialize(g_componentType);

	INFO_MSG("-----------------------------------------------------------------------------------------\n\n\n");

	Resmgr::getSingleton().print();

	Network::EventDispatcher dispatcher;
	DebugHelper::getSingleton().pDispatcher(&dispatcher);

	Network::g_SOMAXCONN = g_kbeSrvConfig.tcp_SOMAXCONN(g_componentType);

	Network::NetworkInterface networkInterface(&dispatcher);

	DebugHelper::getSingleton().pNetworkInterface(&networkInterface);

	KBCMD app(dispatcher, networkInterface, g_componentType, g_componentID);

	START_MSG(COMPONENT_NAME_EX(g_componentType), g_componentID);

	if (!app.initialize())
	{
		ERROR_MSG("app::initialize(): initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::vector<PyTypeObject*> scriptBaseTypes;
	if (!EntityDef::initialize(scriptBaseTypes, g_componentType)) 
	{
		ERROR_MSG("app::initialize(): EntityDef initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::string path = "";
	std::string zipfile = "";

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_GET_VALUE("--outpath=", path);
	PARSE_COMMAND_ARG_END();

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_GET_VALUE("--zip=", zipfile);
	PARSE_COMMAND_ARG_END();

	// 如果检测到设置了zip文件，那么从zip文件得到path
	if (zipfile.size() > 0)
	{
		std::vector<std::string> tmpvec;
		KBEngine::strutil::kbe_splits(zipfile, ".", tmpvec);
		path = tmpvec[0];
		zipfile = tmpvec[1];
	}

	ClientSDK* pClientSDK = ClientSDK::createClientSDK(clientType);
	
	int ret = 0;

	if (pClientSDK)
	{
		try
		{
			if (!pClientSDK->create(path))
			{
				ret = -1;
			}
		}
		catch (std::exception &err)
		{
			ERROR_MSG(fmt::format("app::initialize(): create clientsdk error({})!\n", err.what()));
		}
	}
	else
	{
		ERROR_MSG(fmt::format("app::initialize(): create clientsdk error! nonsupport type={}\n", clientType));
		ret = -1;
	}

	// 开始打包
	if (zipfile.size() > 0)
	{
		if (!script::PyZipFile::compressDirectory(path, (path + "." + zipfile)))
		{
			ERROR_MSG("app::initialize(): compress error!\n");
		}

		std::error_code errorCode;
		if (!std::experimental::filesystem::remove_all(path.c_str(), errorCode)) {
			ERROR_MSG(fmt::format("app::initialize(): delete Directorys error! message={}\n", errorCode.message()));
		}
	}

	app.finalise();
	INFO_MSG(fmt::format("{}({}) has shut down. ClientSDK={}\n", COMPONENT_NAME_EX(g_componentType), g_componentID, pClientSDK->good()));

	// 如果还有日志未同步完成， 这里会继续同步完成才结束
	DebugHelper::getSingleton().finalise();

	if(pClientSDK)
		delete pClientSDK;

	return ret;
}

int process_newassets(int argc, char* argv[], const std::string assetsType)
{
	// 根据assetsType设置KBE_RES_PATH的项目环境变量
	std::string res_path = getenv("KBE_RES_PATH") == NULL ? "" : getenv("KBE_RES_PATH");
	std::string root_path = getenv("KBE_ROOT") == NULL ? "" : getenv("KBE_ROOT");

	if (root_path[root_path.size() - 1] != '\\' && root_path[root_path.size() - 1] != '/')
		root_path += "/";

	std::string assets_sys_path_root = root_path + "kbe/res/sdk_templates/server/";

#if KBE_PLATFORM != PLATFORM_WIN32
	char splitflag = ':';
#else
	char splitflag = ';';
#endif

	if (assetsType == "python")
		assets_sys_path_root += "python_assets/";
	else
		assets_sys_path_root += "python_assets/";

	res_path += fmt::format("{}{}{}{}res/{}{}scripts/", splitflag, assets_sys_path_root, splitflag, assets_sys_path_root, splitflag, assets_sys_path_root);
	setenv("KBE_RES_PATH", res_path.c_str(), 1);

	Resmgr::getSingleton().initialize();
	setEvns();
	loadConfig();

	DebugHelper::initialize(g_componentType);

	INFO_MSG("-----------------------------------------------------------------------------------------\n\n\n");

	Resmgr::getSingleton().print();

	Network::EventDispatcher dispatcher;
	DebugHelper::getSingleton().pDispatcher(&dispatcher);

	Network::g_SOMAXCONN = g_kbeSrvConfig.tcp_SOMAXCONN(g_componentType);

	Network::NetworkInterface networkInterface(&dispatcher);

	DebugHelper::getSingleton().pNetworkInterface(&networkInterface);

	KBCMD app(dispatcher, networkInterface, g_componentType, g_componentID);

	START_MSG(COMPONENT_NAME_EX(g_componentType), g_componentID);

	if (!app.initialize())
	{
		ERROR_MSG("app::initialize(): initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::vector<PyTypeObject*> scriptBaseTypes;
	if (!EntityDef::initialize(scriptBaseTypes, g_componentType))
	{
		ERROR_MSG("app::initialize(): EntityDef initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::string path = "";

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_GET_VALUE("--outpath=", path);
	PARSE_COMMAND_ARG_END();

	ServerAssets* pServerAssets = ServerAssets::createServerAssets(assetsType);

	int ret = 0;

	if (pServerAssets)
	{
		try
		{
			if (!pServerAssets->create(path))
			{
				ret = -1;
			}
		}
		catch (std::exception &err)
		{
			ERROR_MSG(fmt::format("app::initialize(): create serverassets error({})!\n", err.what()));
		}
	}
	else
	{
		ERROR_MSG(fmt::format("app::initialize(): create serverassets error! nonsupport type={}\n", assetsType));
		ret = -1;
	}

	app.finalise();
	INFO_MSG(fmt::format("{}({}) has shut down. ServerAssets={}\n", COMPONENT_NAME_EX(g_componentType), g_componentID, pServerAssets->good()));

	// 如果还有日志未同步完成， 这里会继续同步完成才结束
	DebugHelper::getSingleton().finalise();

	if (pServerAssets)
		delete pServerAssets;

	return ret;
}

int process_getuid(int argc, char* argv[])
{
	if (getUserUID() == 0)
	{
		autoFixUserDigestUID();
	}

	setenv("UID", fmt::format("{}", getUserUID()).c_str(), 1);
	printf("%s", fmt::format("{}", getUserUID()).c_str());
	return getUserUID();
}

int process_help(int argc, char* argv[])
{
	printf("Usage:\n");
	printf("--clientsdk:\n");
	printf("\tAutomatically generate client code based on entity_defs file. Environment variables based on KBE.\n");
	printf("\tkbcmd.exe --clientsdk=unity --outpath=c:/unity_kbesdk\n");
	printf("\tkbcmd.exe --clientsdk=ue4 --zip=c:/unity_kbesdk.zip\n");
	printf("\tkbcmd.exe --clientsdk=ue4 --outpath=c:/unity_kbesdk --KBE_ROOT=\"*\"  --KBE_RES_PATH=\"*\"  --KBE_BIN_PATH=\"*\"\n");

	printf("\n--getuid\n");
	printf("\tReturns the ID of the server group.\n");

	printf("\n--newassets\n");
	printf("\tCreate a new server game asset library, contains the necessary files.\n");
	printf("\tkbcmd.exe --newassets=python --outpath=c:/xserver_assets\n");

	printf("\n--help:\n");
	printf("\tDisplay help information.\n");
	return 0;
}

int main(int argc, char* argv[])
{
	g_componentType = TOOL_TYPE;
	g_componentID = 0;

	if (argc == 1)
	{
		return process_help(argc, argv);
	}

	parseMainCommandArgs(argc, argv);

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_DO_FUNC("--clientsdk=", process_make_client_sdk(argc, argv, cmd));
	PARSE_COMMAND_ARG_DO_FUNC_RETURN("--getuid", process_getuid(argc, argv));
	PARSE_COMMAND_ARG_DO_FUNC("--newassets=", process_newassets(argc, argv, cmd));
	PARSE_COMMAND_ARG_DO_FUNC("--help", process_help(argc, argv));
	PARSE_COMMAND_ARG_END();

	return 0;
}

