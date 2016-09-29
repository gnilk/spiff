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


class FtpDownloadService :
	public IServiceInstance
{
public:
	FtpDownloadService(IServiceManager *serviceManager);
	virtual void Initialize(IServiceInstanceManager *instanceManager);
	virtual void PreExecute();
	virtual void Execute();
	virtual void PostExecute();
	virtual void Dispose();
private:
	IServiceManager *serviceManager;
	IServiceInstanceManager *instanceManager;
};

static ServiceFactory factory;

IServiceInstance *ServiceFactory::CreateInstance(const char *service_id, IServiceManager *serviceManager) {
	ILogger *pLogger = serviceManager->GetSystem()->GetLogger("DataServices.FtpFactory");
	IServiceInstance *service = NULL;
	if (!strcmp(service_id, "DataServices.FtpDownload")) {
		service = dynamic_cast<IServiceInstance *> (new FtpDownloadService(serviceManager));
	}

	if (service != NULL)  {
		pLogger->Debug("Ok, created service with id: '%s'", service_id);
	} else {
		pLogger->Error("Failed to create service with id: '%s'", service_id);
	}

	return service;
}

// -- Base effect - simple pass through
FtpDownloadService::FtpDownloadService(IServiceManager *serviceManager) {
	this->serviceManager = serviceManager;
}
void FtpDownloadService::Initialize(IServiceInstanceManager *instanceManager) {
	this->instanceManager = instanceManager;
}
void FtpDownloadService::PreExecute() {
}
void FtpDownloadService::Execute() {
}
void FtpDownloadService::PostExecute() {
}
void FtpDownloadService::Dispose() {
}


int CALLCONV serviceModuleInitialize(IServiceManager *serviceManager)
{
	serviceManager->RegisterService("DataServices.FtpDownload", dynamic_cast<IServiceInstanceFactory *>(&factory));
	return 0;
}
