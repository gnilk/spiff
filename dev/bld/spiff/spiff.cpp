#include <stdio.h>
#include <stdlib.h>
#include "nxcore.h"
#include "xmlparser.h"
#include "process.h"

using namespace MessageRouter;
using namespace Domain;
using namespace NXCore;
using namespace Services;
using namespace Script;
using namespace gnilk::xml;

using boost::any_cast;

// Global variables
static std::string glb_configFileName = "config.ini";
static std::string glb_serviceDefFileName = "services.xml";
static std::string glb_scriptFileName = "script.xml";
static ILogger *pLogger = NULL;
static IConfig *configuration = NULL;

// Forward function declarations (helpers)
static std::string fload(const char *filename);
static long fsize(FILE *f);
static void InitializeServiceManager(ObjectContainer *container);
static int LogLevelFromString(std::string debugLevel);

static ScriptEngine *ScriptEngineFactory() {
	return new ScriptEngine();
}

static ServiceManager *SM_Factory() {
	return new ServiceManager();
}

static void GlobalInit() {

	// TODO, read from config
	int logLevel = Logger::kMCDebug;
	Logger::Initialize();
	if (logLevel != Logger::kMCNone) {
		Logger::AddSink(Logger::CreateSink("LogConsoleSink"), "console", 0, NULL);
	}
	Logger::SetAllSinkDebugLevel(logLevel);

	pLogger = Logger::GetLogger("spiff");
	pLogger->Info("spiff running");


	// 1. Drag in configuration

	configuration = System::GetInstance()->GetConfig();
	std::string buffer = fload(glb_configFileName.c_str());
	if (!buffer.empty()) {
		pLogger->Info("Loading configuration from '%s'", glb_configFileName.c_str());
		configuration->ReadIniFileStructure(buffer);
		pLogger->Info("Configuration loaded");		
	} else {
		pLogger->Warning("Configuration file '%s' empty or does not exists", glb_configFileName.c_str());
	}


	// reconfigure log levels from config
	std::string debugLevel = configuration->GetValue("spiff","debug","debug");
	logLevel = LogLevelFromString(debugLevel);
	Logger::SetAllSinkDebugLevel(logLevel);

	// 2. Setup objects and register
	ObjectContainer *container = System::GetInstance()->GetObjectContainer();
	container->RegisterFactory<ScriptEngine *>(ScriptEngineFactory, "ScriptEngine");
	container->RegisterFactory<ServiceManager *>(SM_Factory, "ServiceManager");
	InitializeServiceManager(container);

}

static void LoadAndRunScript() {

	ObjectContainer *container = System::GetInstance()->GetObjectContainer();

	// don't register this instance
	ScriptEngine *engine = container->CreateInstance<ScriptEngine *>("ScriptEngine",false);

	if (engine != NULL) {
		ServiceManager *sm = boost::any_cast<ServiceManager *>(container->GetInstance("ServiceManager"));
		if (sm != NULL) {
			pLogger->Info("Loading script: '%s'", glb_scriptFileName.c_str());
			std::string script = fload(glb_scriptFileName.c_str());
			engine->Load(script);		pLogger->Info("Preprocessing script");
			engine->PreProcess(*sm);	pLogger->Info("Execting script");
			engine->Execute();			pLogger->Info("Done!");
		} else {
			pLogger->Error("Unable to locate ServiceManager");
		}
		delete engine;
	} else {
		pLogger->Error("Unable to create script engine!");
	}
}


static bool ParseOpts(char *opts) {
	char *op = opts;
	while(*op != '\0') {
		// switch(*op) {
		// 	case 'c' : 
		// 		break;
		// }
		op++;
	}
	return true;
}
static void ParseArguments(int argc, char **argv) {
	for(int i=0; i<argc; i++) {	
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
				case 'c' :
					glb_configFileName = std::string(argv[++i]);
					break;
				case 'd' :
					glb_serviceDefFileName = std::string(argv[++i]);
					break;
				case 's' :
					glb_scriptFileName = std::string(argv[++i]);
					break;
				default:
					ParseOpts(&argv[i][1]);
			}
		} else {

		}
	}
}

int main(int argc, char **argv) {
	ParseArguments(argc, argv);
	GlobalInit();
	LoadAndRunScript();
}

//////// HELPERS
static void InitializeServiceManager(ObjectContainer *container) {
	ServiceManager *sm = container->CreateInstance<ServiceManager *>("ServiceManager", true);

	pLogger->Info("Loading service definitions from '%s'", glb_serviceDefFileName.c_str());
	std::string serviceDef = fload(glb_serviceDefFileName.c_str());	
	sm->LoadServiceDefinitions(serviceDef);

	pLogger->Info("Scanning plugins and registering services");
	sm->LoadServices();
}

static int LogLevelFromString(std::string debugLevel) {

	if (debugLevel == "none") return Logger::kMCNone;
	if (debugLevel == "debug") return Logger::kMCDebug;
	if (debugLevel == "info") return Logger::kMCInfo;
	if (debugLevel == "warning") return Logger::kMCWarning;
	if (debugLevel == "error") return Logger::kMCError;
	if (debugLevel == "critical") return Logger::kMCCritical;
	pLogger->Error("Illegal debug level '%s', reverting to debug", debugLevel.c_str());

	return Logger::kMCDebug;
}


static long fsize(FILE *f) {
	unsigned int retval;
	long pos = ftell(f);
	fseek(f, 0, SEEK_END);
	retval = ftell(f);
	fseek(f, pos, SEEK_SET);
	return retval;
}

static std::string fload(const char *filename) {
	FILE *f = fopen(filename,"rb");
	if (f == NULL)
	{
		pLogger->Error("can't find file: %s\n", filename);
		exit(1);
		return std::string("");		
	}
	long sz = fsize(f);
	pLogger->Debug("Loading %s of size %d\n", filename, sz);
	char *buffer = (char *)malloc(sz+1);
	int bytes = fread(buffer, 1, sz, f);
	buffer[bytes]='\0';
	fclose(f);
	return std::string(buffer);		

}
