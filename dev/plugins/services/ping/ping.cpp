#include "nxcore.h"
#include "logger.h"
#include "process.h"

#ifdef WIN32
#include <malloc.h>
#endif

#include <boost/any.hpp>
using boost::any_cast;


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

/*
static const char * pingdef = \
	<service name=\"traceroute\">
		<description>records the route between the client and the destination</>
		<input>
			<property name=\"host\" display=\"Host\" type=\"string\" default=\"127.0.0.1\" description=\"name or ip for the destination host\" />
			<property name=\"packetsize\" display=\"Packet Size\" type=\"int\" default=\"128\" description=\"number of bytes per packet\" />			
			<property name=\"timeout\" display=\"Timeout per hop\" type=\"float\" default=\"5\" description=\"Maxmimum time (in seconds) to wait for a response per hop\" />
		</input>
		<output>
			<property name=\"hosts\" type=\"iplist\" />
		<output>
	</service>
";
*/

class PingService :
	public IServiceInstance
{
private:
	IServiceManager *serviceManager;
	IServiceInstanceManager *instanceManager;
	ILogger *pLogger;

	IParameter *host;
	IParameter *packetSize;
	IParameter *allowFragmentation;
	IParameter *packetsToSend;
	IParameter *timeoutSec;
	IParameter *ttl;

public:
	PingService(IServiceManager *serviceManager);
	virtual void Initialize(IServiceInstanceManager *instanceManager);
	virtual void PreExecute();
	virtual void Execute();
	virtual void PostExecute();
	virtual void Dispose();
};


class TracerouteService :
	public IServiceInstance
{
private:
	IServiceManager *serviceManager;
	IServiceInstanceManager *instanceManager;
	ILogger *pLogger;

	IParameter *host;
	IParameter *numeric;
	IParameter *timeoutSec;
	IParameter *ttl;

public:
	TracerouteService(IServiceManager *serviceManager);
	virtual void Initialize(IServiceInstanceManager *instanceManager);
	virtual void PreExecute();
	virtual void Execute();
	virtual void PostExecute();
	virtual void Dispose();
};

static ServiceFactory factory;

IServiceInstance *ServiceFactory::CreateInstance(const char *service_id, IServiceManager *serviceManager) {
	ILogger *pLogger = serviceManager->GetSystem()->GetLogger("PingService.Factory");
	IServiceInstance *service = NULL;
	if (!strcmp(service_id, "DataServices.Ping")) {
		service = dynamic_cast<IServiceInstance *> (new PingService(serviceManager));
	} else if (!strcmp(service_id, "DataServices.Traceroute")) {
		service = dynamic_cast<IServiceInstance *> (new TracerouteService(serviceManager));
	}

	// if (service != NULL)  {
	// 	pLogger->Debug("Ok, '%s' instance created", service_id);
	// } else {
	// 	pLogger->Debug("Failed to create service instance for '%s'", service_id);	
	// } 
	return service;
}

// ping service
PingService::PingService(IServiceManager *serviceManager) {
	this->serviceManager = serviceManager;
	pLogger = serviceManager->GetSystem()->GetLogger("DataServices.Ping");
}
void PingService::Initialize(IServiceInstanceManager *instanceManager) {
	pLogger->Debug("Initialize");
	this->instanceManager = instanceManager;

	host = instanceManager->GetParameter("host", std::string("127.0.0.1"));
	packetSize = instanceManager->GetParameter("packetsize", 256);
	allowFragmentation = instanceManager->GetParameter("frag", false);
	packetsToSend = instanceManager->GetParameter("packets", 3);
	timeoutSec = instanceManager->GetParameter("timeout",3.0f);
	ttl = instanceManager->GetParameter("ttl",128);
}
void PingService::PreExecute() {
	
}
void PingService::Execute() {
	pLogger->Debug("Execute");
	pLogger->Debug("Parameters:");
 	try
    {
		pLogger->Debug("  Host: %s",any_cast<std::string>(host->Get()).c_str());
		pLogger->Debug("  Packets: %d",any_cast<int>(packetsToSend->Get()));
		pLogger->Debug("  PacketSize: %d",any_cast<int>(packetSize->Get()));
		pLogger->Debug("  Timeout: %f", any_cast<float>(timeoutSec->Get()));
    }
    catch(const boost::bad_any_cast &)
    {
		pLogger->Debug("Bad type in Ping parameters");
       // return false;
    }	

    // -- this works --
	gnilk::Process proc("ping");
	proc.AddArgument("-c %d",any_cast<int>(packetsToSend->Get()));
	proc.AddArgument(any_cast<std::string>(host->Get()));
	proc.ExecuteAndWait();

}
void PingService::PostExecute() {
}
void PingService::Dispose() {
}


// ping service
TracerouteService::TracerouteService(IServiceManager *serviceManager) {
	this->serviceManager = serviceManager;
	pLogger = serviceManager->GetSystem()->GetLogger("DataServices.Traceroute");
}
void TracerouteService::Initialize(IServiceInstanceManager *instanceManager) {
	pLogger->Debug("Initialize");
	this->instanceManager = instanceManager;

	host = instanceManager->GetParameter("host", std::string("127.0.0.1"));
	numeric = instanceManager->GetParameter("numeric", false);
	timeoutSec = instanceManager->GetParameter("timeout",3);
	ttl = instanceManager->GetParameter("ttl",128);
}
void TracerouteService::PreExecute() {
	
}
void TracerouteService::Execute() {
	pLogger->Debug("Execute");
	pLogger->Debug("Parameters:");
 	try
    {
		pLogger->Debug("  Host: %s",any_cast<std::string>(host->Get()).c_str());
		//pLogger->Debug("  Numeric: %d",any_cast<int>(packetSize->Get()));
		pLogger->Debug("  Timeout: %d", any_cast<int>(timeoutSec->Get()));
    }
    catch(const boost::bad_any_cast &)
    {
		pLogger->Debug("Bad type in parameters");
       // return false;
    }	

    // -- this works --
	gnilk::Process proc("traceroute");
	bool use_numeric = any_cast<bool>(numeric->Get());
	if (use_numeric) {
		proc.AddArgument("-n");
	}
	proc.AddArgument("-w %d",any_cast<int>(timeoutSec->Get()));
	proc.AddArgument(any_cast<std::string>(host->Get()));
	proc.ExecuteAndWait();

}
void TracerouteService::PostExecute() {
}
void TracerouteService::Dispose() {
}

int CALLCONV serviceModuleInitialize(IServiceManager *serviceManager)
{
	serviceManager->RegisterService("DataServices.Ping", dynamic_cast<IServiceInstanceFactory *>(&factory));
	serviceManager->RegisterService("DataServices.Traceroute", dynamic_cast<IServiceInstanceFactory *>(&factory));
	return 0;
}
