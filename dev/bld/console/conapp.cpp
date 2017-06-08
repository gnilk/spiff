//
// TODO:
// - Need persistance layer for the inifiles (ability to save)
//   - Should have an option to automatically save on changes
// - Need observers
//
// - Create a proper application
//   - Web service interface
// 
// - Need result (KPI) handling
//   - Store to file (SQLite)
//   - Push to backend (JSON??)
// 
// - Fix services with 'proper' implementation (use 'Process' class)
// - use curl/libcurl for HTTP/FTP/IMAP/POP/SMTP tests
//
// 
//
//
#include <stdio.h>
#include <iostream>
#include <boost/any.hpp>
#include "nxcore.h"
#include "xmlparser.h"
#include "process.h"

#include <spawn.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <list>
#include <stack>

using namespace MessageRouter;
using namespace Domain;
using namespace NXCore;
using namespace Services;
using namespace Script;
using namespace gnilk::xml;

using boost::any_cast;

typedef std::function<void()> OnPluginLoadedDelegate;

long fsize(FILE *f) {
	unsigned int retval;
	long pos = ftell(f);
	fseek(f, 0, SEEK_END);
	retval = ftell(f);
	fseek(f, pos, SEEK_SET);
	return retval;
}

std::string fload(const char *filename) {
	FILE *f = fopen(filename,"rb");
	if (f == NULL)
	{
		return std::string("");		
	}
	long sz = fsize(f);
	char *buffer = (char *)malloc(sz+1);
	int bytes = fread(buffer, 1, sz, f);
	buffer[bytes]='\0';
	fclose(f);
	return std::string(buffer);		

}



ServiceManager *SM_Factory() {
	Logger::GetLogger("conapp")->Debug("SM_Factory invoked");
	return new ServiceManager();
}

void TestObjectContainer() {
	ISystem *sys = System::GetInstance();
	ObjectContainer *container = sys->GetObjectContainer();

	Logger::GetLogger("conapp")->Debug("Register factory)");
	container->RegisterFactory<ServiceManager *>(SM_Factory, "ServiceManager");
	// std::function<ServiceManager *()> func = SM_Factory;
	// ObjRegFactory<ServiceManager *>(func, "factory");
	Logger::GetLogger("conapp")->Debug("create Instance");
	ServiceManager *sm = container->CreateInstance<ServiceManager *>("ServiceManager", true);
	sm->LoadServices();
}



extern char **environ;

void TestConfig() {

	std::string buffer = fload("config.ini");

	IConfig *conf = System::GetInstance()->GetConfig();

	conf->ReadIniFileStructure(buffer);

	std::string name21 = conf->GetValue("section2","name21","bla");
	Logger::GetLogger("conapp")->Debug("TestConfig name21='%s' (should be 'value21')", name21.c_str());

	conf->DeleteValue("section2","name21");
	name21 = conf->GetValue("section2","name21","bla");
	Logger::GetLogger("conapp")->Debug("TestConfig name21='%s' (should be 'bla')", name21.c_str());

	conf->DeleteSection("section2");
	name21 = conf->GetValue("section2","name21","bla");
	Logger::GetLogger("conapp")->Debug("TestConfig name21='%s' (should be 'bla')", name21.c_str());

	

	conf->SetValue("section","property","value");
	if (conf->HasValue("section","property")) {
		std::string val = conf->GetValue("section","property","");
		Logger::GetLogger("conapp")->Debug("TestConfig x='%s' (should be 'value')", val.c_str());
		conf->SetValue("section","property","value2");
		val = conf->GetValue("section","property","");
		Logger::GetLogger("conapp")->Debug("TestConfig x='%s' (should be 'value2')", val.c_str());
	} else {
		Logger::GetLogger("conapp")->Error("value does not exists");
		exit(1);
	}
}

void TestProcess() {
	gnilk::Process proc("ping");
	proc.AddArgument("-c 3");
	proc.AddArgument("www.google.com");
	proc.ExecuteAndWait();
}

// This spawns a process and captures STDIO and STDERR  -> MOVED TO Process (process.cpp)
void TestSpawn() {
	int status;
	pid_t pid;
	char *argv[] = {"ping", "-c 3", "www.google.com", (char *)0};

	posix_spawn_file_actions_t child_fd_actions;
	status = posix_spawn_file_actions_init(&child_fd_actions);
	if (status != 0) {
		Logger::GetLogger("conapp")->Error("posix_spawn_file_actions_init %d, %s", status, strerror(status));
		exit(1);				
	}

	int filedes[2];
	status = pipe(filedes);
	if (status == -1) {
		Logger::GetLogger("conapp")->Error("pipe %d, %s", status, strerror(status));
		exit(1);				
	}

	// Need this to properly track process status
	status = fcntl(filedes[0], O_NONBLOCK);
	if (status == -1) {
		Logger::GetLogger("conapp")->Error("fcntl %d", errno);
		exit(1);				
	}
	status = fcntl(filedes[1], O_NONBLOCK);
	if (status == -1) {
		Logger::GetLogger("conapp")->Error("fcntl %d", errno);
		exit(1);				
	}

	// duplicate file descriptors

	status = posix_spawn_file_actions_adddup2(&child_fd_actions, filedes[1], 1);
	if (status) {
		Logger::GetLogger("conapp")->Error("posix_spawn_file_actions_adddup2 %d, %s", status, strerror(status));
		exit(1);						
	}
	status = posix_spawn_file_actions_adddup2(&child_fd_actions, filedes[1], 2);
	if (status) {
		Logger::GetLogger("conapp")->Error("posix_spawn_file_actions_adddup2 %d, %s", status, strerror(status));
		exit(1);						
	}

	// should probably close some here

	status = posix_spawnp(&pid, "/sbin/ping", &child_fd_actions, NULL, argv, environ);
	if (status == 0) {
		// call 'OnProcessStarted'
		Logger::GetLogger("conapp")->Debug("Spawn ok");
		char buffer[4096];
		while(1) {			
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == 0) {
			  // Child still alive
			} else if (result == -1) {
			  // Error 
				Logger::GetLogger("conapp")->Error("Process error");				
			} else {
			  // Child exited
				break;
			}			

			int count = read(filedes[0], buffer, sizeof(buffer));
			if (count == -1) {
				if (errno == EINTR) {
					Logger::GetLogger("conapp")->Debug("EINTR");
					continue;
				} else if (errno == EAGAIN) {
					continue;
				} else {
					Logger::GetLogger("conapp")->Error("read");
					exit(1);										
				}
			} else if (count == 0) {
				Logger::GetLogger("conapp")->Debug("count = 0, done!");
				break;
			} else {
				buffer[count]='\0';
				// call 'OnData(buffer)'
				printf("b:%s\n",buffer);
			}
		}

		// call 'OnProcessExit()'

		// if (waitpid(pid, &status, 0) != -1) {
		// 	Logger::GetLogger("conapp")->Debug("child exit with status %d", status);
		// } else {
		// 	Logger::GetLogger("conapp")->Error("waitpid: %d", status);
		// 	exit(1);
		// }
		close(filedes[0]);
		close(filedes[1]);
	} else {
		Logger::GetLogger("conapp")->Error("waitpid: %d, %s", status, strerror(status));
		exit(1);		
	}
}

void TestMessageRouting() {
 	Router router;
 	FileWriter fw;
 	BaseMessageSubscriber bms;
 	Message msg;

 	router.SubscribeToChannel(MSG_CHANNEL_ANY, dynamic_cast<IMessageSubscriber *>(&fw));
 	router.SubscribeToChannel(20, dynamic_cast<IMessageSubscriber *>(&bms));
 	WriteChannel *channel = router.GetWriteChannel(10);
 	channel->Write(10, msg);	
}


//
// showcase of using C++11 function delegates
//
class PluginScannerHandler {
private:
	void OnPluginLoaded(std::string pathName, PFNINITIALIZEPLUGIN funcInitPlugin);
public:
	void ScanThePlugs();
};

void PluginScannerHandler::OnPluginLoaded(std::string pathName, PFNINITIALIZEPLUGIN funcInitPlugin) {
	Logger::GetLogger("PluginScannerHandler")->Debug("OnPluginLoaded, Initializing '%s'\n", pathName.c_str());
}

void PluginScannerHandler::ScanThePlugs() {
	PluginScanner::ScanDirectory(true, ".", "yaptInitializePlugin", std::bind(&PluginScannerHandler::OnPluginLoaded, this, std::placeholders::_1, std::placeholders::_2));
}


void globalOnPluginLoaded(std::string pathName, PFNINITIALIZEPLUGIN funcInitPlugin) {
	Logger::GetLogger("globalOnPluginLoader")->Debug("Initializing '%s'\n", pathName.c_str());
}

void TestPluginScanner() {
	// also possible to just pass a function like thiss
//	PluginScanner::ScanDirectory(true, ".", "yaptInitializePlugin", globalOnPluginLoaded);
	PluginScannerHandler bla;
	bla.ScanThePlugs();
	
}

void TestParameterBinding() {
	ServiceManager serviceManager;
	serviceManager.LoadServices();
	IServiceInstanceControl *ping = serviceManager.CreateInstance("ExampleService");
	IServiceInstanceControl *ping2 = serviceManager.CreateInstance("ExampleService");
	IServiceInstanceManager *pingManager = ping->GetManagementInterface();
	pingManager->SetParameterValue("host", std::string("www.google.com"));

	// not needed, just verification that the parameter was created
	IParameter *pingHost = pingManager->GetParameter("host");
	if (pingHost == NULL) {
		Logger::GetLogger("conapp")->Error("Host Parameter is NULL");
		exit(1);
	}

	pingManager = ping2->GetManagementInterface();
	// bind ping2.host to ping.host
	pingManager->BindParameter("host", pingHost);

	ping->Initialize();
	ping2->Initialize();

	// both ping and ping2 should be 'www.google.com'
	ping->Execute();
	ping2->Execute();	
}

void TestServiceManager() {
	ServiceManager serviceManager;
	serviceManager.LoadServices();
	IServiceInstanceControl *ping = serviceManager.CreateInstance("DataServices.Ping");
	IServiceInstanceManager *pingManager = ping->GetManagementInterface();
	pingManager->SetParameterValue("host", std::string("www.google.com"));
	pingManager->SetParameterValue("packets", 1);
	ping->Initialize();
	ping->Execute();
}


void TestServiceManagerDefinintions() {
	FILE *f = fopen("services.xml","rb");
	if (f != NULL) {
		long sz = fsize(f);
		char *buffer = (char *)malloc(sz+1);
		int bytes = fread(buffer, 1, sz, f);
		buffer[bytes]='\0';
		fclose(f);
		ServiceManager serviceManager;
		serviceManager.LoadServiceDefinitions(std::string(buffer));
		// Logger::GetLogger("conapp")->Debug("Definitions loaded, dumping XML");
		// std::string xml = serviceManager.SerializeDefinitionsToXML();
		// cout << xml;
		//Logger::GetLogger("conapp")->Debug("\n%s",xml.c_str());	
	}

}

class TestMe {
public:
	TestMe(std::string _name) {
		name = _name;
		printf("TestMe::CTOR()\n");
	}
	virtual ~TestMe() {
		printf("TestMe::DTOR()\n");
	}
	virtual std::string &GetName() {
		return name;
	}
private:
	std::string name;
};
class Manager {
public:
	Manager() {
		printf("Manager::CTOR()\n");
	}
	virtual ~Manager() {
		for(auto it=mapme.begin();it!=mapme.end();it++) {
			// TestMe *tme = it->second;
			// delete tme;
			delete it->second;
		}
		mapme.clear();
		printf("Manager::DTOR()\n");
	}
	void DoStuff();
private:
	std::map<std::string, TestMe *> mapme;
};
void Manager::DoStuff() {

		printf("Inserting stuff\n");
		TestMe *tmp = new TestMe("test1");
		mapme.insert(std::make_pair(tmp->GetName(),tmp));
		TestMe *tmp2 = new TestMe("test2");
		mapme.insert(std::make_pair(tmp2->GetName(), tmp2));
		printf("done\n");	
}
void TestStdMapLocality() {
	Manager manager;
	printf("Testing std::map locality\n");
	manager.DoStuff();
	printf("Test concluded\n");
}


void TestScriptEngine() {

	std::string serviceDef = fload("services.xml");
	std::string script = fload("script.xml");
	if (!serviceDef.empty() && !script.empty()) {
		ServiceManager serviceManager;
		ScriptEngine engine;

		serviceManager.LoadServices();
		serviceManager.LoadServiceDefinitions(std::string(serviceDef));	
		engine.Load(script);
		engine.PreProcess(serviceManager);
		engine.Execute();
	}
}

// to create a data connection on an interface
/*
class IDataConnection {
public:
	virtual bool Open() = 0;
	virtual bool Close() = 0;
private:
};

class IVoiceCall {
public:
	virtual void Dial() = 0;
	virtual void Hangup() = 0;
private:
	// state handling!!!
};


class IEquipment {
public:
	virtual void Initialize() = 0;
	virtual void GetCapabilities() = 0;
private:
	IVoiceCall *voice;
	IDataConnection *data;
};

class Ethernet : 
	public IEquipment, IDataConnection {
public:
	virtual void Initialize();
	virtual void GetCapabilities();
public:	// IDataConnection
	virtual bool Open();
	virtual bool Close();
private:
};

void Ethernet::Initialize() {
	voice = NULL;	// not supported
	data = this;
}

void Ethernet::GetCapabilities() {

}

//
bool Ethernet::Open() {
	return true;
}

bool Ethernet::Close() {
	return true;
}
*/



void init() {
	int logLevel = Logger::kMCDebug;
	Logger::Initialize();
	if (logLevel != Logger::kMCNone) {
		Logger::AddSink(Logger::CreateSink("LogConsoleSink"), "console", 0, NULL);
	}
	Logger::SetAllSinkDebugLevel(logLevel);

	ILogger *pLogger = Logger::GetLogger("main");
	pLogger->Debug("NXCore - running tests");

}

int main(int argc, char **argv) {
	init();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Plugin Scanner");
	TestPluginScanner();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Message Routing");
 	TestMessageRouting();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Service Manager");
 	TestServiceManager();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Parameter Bindings");
 	TestParameterBinding();
 	// Logger::GetLogger("conapp")->Debug("Test std::map");
 	// TestStdMapLocality();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Service Manager Definitions");
 	TestServiceManagerDefinintions();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Script Engine");
 	TestScriptEngine();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Object container");
 	TestObjectContainer();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Config");
 	TestConfig();
	Logger::GetLogger("conapp")->Debug("--------------------------------------");
	Logger::GetLogger("conapp")->Debug("Test Process");
 	TestProcess();
	// Logger::GetLogger("conapp")->Debug("Testing Parameter Handling");
	// TestParameterHandling();
	printf("Hello world!\n");
}
