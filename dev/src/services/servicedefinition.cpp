#include "nxcore.h"
#include "xmlparser.h"
#include <functional>

using namespace Services;
using namespace NXCore;
using namespace gnilk::xml;

// -----------------
//
// Service Definition
//
ServiceDefinition::ServiceDefinition(std::string _name) {
	name = _name;
}
ServiceDefinition::~ServiceDefinition() {

}
std::string &ServiceDefinition::GetName() {
	return name;
}
std::string &ServiceDefinition::GetDescription() {
	return description;
}
std::list<ParameterDefinition *> &ServiceDefinition::GetInputParameters() {
	return inputParameters;
}
std::list<ParameterDefinition *> &ServiceDefinition::GetOutputParameters() {
	return outputParameters;
}
void ServiceDefinition::SetDescription(std::string desc) {
	description = desc;
}
std::string ServiceDefinition::ToXML() {
	std::string xml("<service ");
	xml += std::string("name=\"")+name+std::string(">\n");
	xml += std::string("  <description>")+description+std::string("</description>\n");
	xml += std::string("  <input>\n");
	for(auto it=inputParameters.begin();it!=inputParameters.end();it++) {
		ParameterDefinition *param = *it;
		xml += param->ToXML();
		xml += std::string("\n");
	}
	xml += std::string("  </input>\n");
	xml += std::string("  <output>\n");
	xml += std::string("  </output>\n");
	xml += std::string("</service>");
	return xml;
}
