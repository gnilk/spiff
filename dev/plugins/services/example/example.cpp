#include "nxcore.h"
#include "logger.h"

#ifdef WIN32
#include <malloc.h>
#endif

using namespace NXCore;
using namespace Services;

extern "C"
{
	int CALLCONV serviceModuleInitialize(IServiceManager *serviceManager);
}

class ServiceFactory :
	public IServiceInstanceFactory
{
public:	
	virtual IServiceInstance *CreateInstance(const char *service_id, IServiceManager *serviceManager);
};


class ExampleService :
	public IServiceInstance
{
public:
	ExampleService(IServiceManager *serviceManager);
	virtual void Initialize(IServiceInstanceManager *instanceManager);
	virtual void PreExecute();
	virtual void Execute();
	virtual void PostExecute();
	virtual void Dispose();
private:
	IServiceManager *serviceManager;
	IServiceInstanceManager *instanceManager;
	ILogger *pLogger;
};

static ServiceFactory factory;

IServiceInstance *ServiceFactory::CreateInstance(const char *service_id, IServiceManager *serviceManager)
{
	IServiceInstance *service = NULL;
	if (!strcmp(service_id, "ExampleService"))
	{
		service = dynamic_cast<IServiceInstance *> (new ExampleService(serviceManager));
	}
	return service;
}

// -- Base effect - simple pass through
ExampleService::ExampleService(IServiceManager *serviceManager) {
	this->serviceManager = serviceManager;
	pLogger = serviceManager->GetSystem()->GetLogger("ExampleService");
}
void ExampleService::Initialize(IServiceInstanceManager *instanceManager) {
	this->instanceManager = instanceManager;
	pLogger->Debug("Initialize");
}
void ExampleService::PreExecute() {
	pLogger->Debug("PreExecute");
}
void ExampleService::Execute() {
	pLogger->Debug("Execute");
}
void ExampleService::PostExecute() {
	pLogger->Debug("PostExecute");
}
void ExampleService::Dispose() {
	pLogger->Debug("Dispose");
}


int CALLCONV serviceModuleInitialize(IServiceManager *serviceManager)
{
	serviceManager->RegisterService("ExampleService", dynamic_cast<IServiceInstanceFactory *>(&factory));
	return 0;
}
