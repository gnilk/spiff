#include "nxcore.h"
#include "xmlparser.h"
#include <functional>

using namespace Services;
using namespace NXCore;
using namespace gnilk::xml;


//
// Split this a bit
// 1) Loader
//     - Definitions
//     - Instance handler
// 2) Registration handler (Repository)
//

ServiceManager::ServiceManager() {

}

ServiceManager::~ServiceManager() {
	for(auto it=serviceDefinitions.begin(); it!=serviceDefinitions.end(); it++) {
		delete it->second;
	}
}

//
// TODO: Needs to be divided in to a 'ServiceLoader'
//
void ServiceManager::LoadServices() {
	PluginScanner::ScanDirectory(true, ".", "serviceModuleInitialize", std::bind(&ServiceManager::OnPluginLoaded, this, std::placeholders::_1, std::placeholders::_2));
}

void ServiceManager::LoadServiceDefinitions(std::string definitiondata) {
	Logger::GetLogger("ServiceManager")->Debug("Parsing XML");
	Document *doc = Parser::loadXML(definitiondata);
	Logger::GetLogger("ServiceManager")->Debug("Adding service definitions");
	doc->traverse(
		std::bind(&ServiceManager::OnDefinitionTagDataStart,this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&ServiceManager::OnDefinitionTagDataEnd,this, std::placeholders::_1, std::placeholders::_2));
}
// call back from XML Parser
void ServiceManager::OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes) {
	if (tag->getName() == "service") {
		Logger::GetLogger("ServiceManager")->Debug("Got service, creating definition from XML");	
		ServiceDefinition *srvDef = ServiceDefinitionFromXML::CreateServiceDefinition(tag);
		serviceDefinitions.insert(std::make_pair(srvDef->GetName(), srvDef));
		Logger::GetLogger("ServiceManager")->Debug("Definition ok, added service '%s' to system",srvDef->GetName().c_str());	
	}
}

void ServiceManager::OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes) {
	if (tag->getName() == "service") {
		Logger::GetLogger("ServiceManager")->Debug("Got service, creating definition from XML");	
		ServiceDefinition *srvDef = ServiceDefinitionFromXML::CreateServiceDefinition(tag);
		serviceDefinitions.insert(std::make_pair(srvDef->GetName(), srvDef));
		Logger::GetLogger("ServiceManager")->Debug("Definition ok, added service '%s' to system",srvDef->GetName().c_str());	
	}
}

// this is more or less just test code...  serialization should not be here..
std::string ServiceManager::SerializeDefinitionsToXML() {
	std::string xml("<xml>\n");
	for(auto it = serviceDefinitions.begin(); it!=serviceDefinitions.end(); it++) {
		ServiceDefinition *srvDef = it->second;
		xml += srvDef->ToXML();
		xml += std::string("\n");
	}
	xml += std::string("</xml>\n");
	return xml;
}

// call back from PluginScanner
void ServiceManager::OnPluginLoaded(std::string pathName, PFNINITIALIZEPLUGIN funcInitPlugin) {
	Logger::GetLogger("ServiceManager")->Debug("OnPluginLoaded, Initializing '%s'", pathName.c_str());
	PFNINITIALIZESERVICEMODULE funcInitModule = (PFNINITIALIZESERVICEMODULE)funcInitPlugin;
	funcInitModule(dynamic_cast<IServiceManager *>(this));
}

// TODO: This should be handled a bit differently
void ServiceManager::RegisterService(const char *service_id, IServiceInstanceFactory *factory) {

	// 1. get proxy instance

	auto it = serviceFactories.find(service_id);
	if (it == serviceFactories.end()) {
		Logger::GetLogger("ServiceManager")->Debug("Ok, RegisterService, '%s'", service_id);	
		serviceFactories.insert(std::make_pair(service_id, factory));
	} else {
		Logger::GetLogger("ServiceManager")->Error("Failed, service with id '%s' already registered", service_id);	
	}
}

IServiceInstanceControl *ServiceManager::CreateInstance(const char *service_id) {
	ServiceInstanceBridge *instance = NULL;

	auto it = serviceFactories.find(service_id);
	if (it != serviceFactories.end()) {
		IServiceInstance *extInstance = it->second->CreateInstance(service_id, dynamic_cast<IServiceManager *>(this));
		instance = new ServiceInstanceBridge(extInstance);
		Logger::GetLogger("ServiceManager")->Debug("Ok, Created instance of service '%s'", service_id);	

		// TODO: check if we have a definintion, if we should set parameter values from the definintion

	} else {
		Logger::GetLogger("ServiceManager")->Error("Failed to create service unknown id '%s' (not registered)", service_id);			
		return NULL;
	}
	return dynamic_cast<IServiceInstanceControl *>(instance);
}
