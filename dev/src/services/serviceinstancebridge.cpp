#include "nxcore.h"
#include "xmlparser.h"
#include <functional>

using namespace Services;
using namespace NXCore;
using namespace gnilk::xml;

// -----------------
//
// Service Instance Brigde
//

// IServiceInstanceControl implementation
ServiceInstanceBridge::ServiceInstanceBridge(IServiceInstance *instance) {
	this->instance = instance;
}

ServiceInstanceBridge::~ServiceInstanceBridge() {
	Dispose();
}

void ServiceInstanceBridge::Initialize() {
	this->instance->Initialize(dynamic_cast<IServiceInstanceManager *>(this));
}

void ServiceInstanceBridge::PreExecute() {
	this->instance->PreExecute();
}

void ServiceInstanceBridge::Execute() {
	this->instance->Execute();
}

void ServiceInstanceBridge::PostExecute() {
	this->instance->PostExecute();
}

void ServiceInstanceBridge::Dispose() {
	this->instance->Dispose();
}

IServiceInstanceManager *ServiceInstanceBridge::GetManagementInterface() {
	return dynamic_cast<IServiceInstanceManager *>(this);
}

// IServiceInstangeManager implementation
boost::any ServiceInstanceBridge::GetParameterValue(const std::string &name, boost::any defaultvalue) {
	IParameter *parameter = parameters.GetOrAdd(name,defaultvalue);
	return parameter->Get();
}

IParameter *ServiceInstanceBridge::GetParameter(const std::string &name, boost::any defaultvalue) {
	return parameters.GetOrAdd(name, defaultvalue);
}

IParameter *ServiceInstanceBridge::GetParameter(const std::string &name) {
	return parameters.Get(name);
}

void ServiceInstanceBridge::SetParameterValue(const std::string &name, boost::any value) {
	IParameter *parameter = parameters.GetOrAdd(name, value);
	parameter->Set(value);	// assure this is set, if the parameter exists the value is not added to the param.
}

void ServiceInstanceBridge::BindParameter(const std::string &name, IParameter *bindTo) {
	IParameter *currentParam = parameters.Get(name);
	if (currentParam == NULL) {
		// there was no parameter created, so let's create one..
		// this behavior ease up using services programatically
		currentParam = GetParameter(name, bindTo->Get());
	}
	if (!ParameterBinding::CanBind(currentParam, bindTo)) {
		return;
	}

	ParameterBinding *boundParameter = new ParameterBinding(currentParam, bindTo);
	parameters.Replace(name, boundParameter);
}


