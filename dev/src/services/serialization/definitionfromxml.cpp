#include "nxcore.h"
#include "xmlparser.h"
#include <functional>

using namespace Services;
using namespace NXCore;
using namespace gnilk::xml;

// -----------------
//
// Service Definition From XML
//
ServiceDefinition *ServiceDefinitionFromXML::CreateServiceDefinition(ITag *serviceTag) {
	ServiceDefinition *srvDef = NULL;

	std::string name = serviceTag->getAttributeValue("name","");
	if (!name.empty()) {
		Logger::GetLogger("ServiceManager")->Debug("name = %s",name.c_str());	

	 	srvDef = new ServiceDefinition(name);
	 	ITag *descTag = serviceTag->getFirstChild("description");
	 	if (descTag != NULL) {
	 		std::string description = descTag->getContent();
	 		srvDef->SetDescription(description);
	 		//Logger::GetLogger("ServiceManager")->Debug("desc = %s",description.c_str());	
	 	}
	 	int inputCount = ParseParameters(serviceTag->getFirstChild("input"), srvDef->GetInputParameters());
	 	int outputCount = ParseParameters(serviceTag->getFirstChild("output"), srvDef->GetOutputParameters());
	 	Logger::GetLogger("ServiceManager")->Debug("Parameters Input: %d, Output: %d",inputCount, outputCount);
	} else {
		Logger::GetLogger("ServiceManager")->Error("XML Violation - Service name is empty, not allowed");
		// TODO: create a system function 'raise error'
	}
	return srvDef;
}

int ServiceDefinitionFromXML::ParseParameters(ITag *paramRootTag, std::list<ParameterDefinition *> &paramList) {
	int count = 0;
	std::list<ITag *> &paramTags = paramRootTag->getChildren();
	for(auto it = paramTags.begin(); it != paramTags.end(); it++) {
		ParameterDefinition *paramDef = ParameterDefinitionFromXML::CreateParameterDefinition(*it);
		if (paramDef != NULL) {
			paramList.push_back(paramDef);			
		} else {
			return -1;
		}
		count++;
	}
	return count;
}


// -----------------
//
// Parameter Definition From XML
//
ParameterDefinition *ParameterDefinitionFromXML::CreateParameterDefinition(ITag *paramTag) {
	ParameterDefinition *paramDef = NULL;
	std::string name = paramTag->getAttributeValue("name","");
	if (!name.empty()) {
		paramDef = new ParameterDefinition(name);
//<parameter name="host" display="Host" type="string" default="127.0.0.1" description="name or ip for host to ping" />		
		std::string display = paramTag->getAttributeValue("display","");
		std::string typeName = paramTag->getAttributeValue("type","");
		std::string defaultValue = paramTag->getAttributeValue("default","");
		std::string description = paramTag->getAttributeValue("description","");

		paramDef->SetDisplayName(display);
		paramDef->SetTypeName(typeName);
		paramDef->SetDefaultValue(defaultValue);
		paramDef->SetDescription(description);


	} else {
		Logger::GetLogger("ServiceManager")->Error("XML Violation - Parameter name is empty, not allowed");
		// TODO: create a system function 'raise error'
	}
	return paramDef;
}
