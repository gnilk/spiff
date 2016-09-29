#include <stdio.h>
#include <iostream>
#include <boost/any.hpp>
#include "nxcore.h"
#include "xmlparser.h"

#include <list>
#include <stack>

using namespace MessageRouter;
using namespace Domain;
using namespace NXCore;
using namespace Services;
using namespace Script;
using namespace gnilk::xml;

using boost::any_cast;
////////////////////////////

//
// PROTOTYPING SCRIPT ENGINE
//


// Node class
void Node::Add(Node *node) {
	this->nodes.push_back(node);
}	

void Node::PreProcess(ServiceManager &serviceManager) {
	for(auto it=nodes.begin(); it != nodes.end(); it++) {
		Node *node = *it;
		node->PreProcess(serviceManager);
	}			
}

void Node::Execute() {
	for(auto it=nodes.begin(); it != nodes.end(); it++) {
		Node *node = *it;
		node->Execute();
	}
}




//
// class while control node
//

WhileControlNode::WhileControlNode() {
	count = 0;
}

void WhileControlNode::Execute() {
	count = org_count;
	Logger::GetLogger("ScriptEngine")->Debug("Executing loop: %d times", count);
	while(count > 0) {
		// execute sub nodes
		Logger::GetLogger("ScriptEngine")->Debug("Iteration: %d\n",count);

		Node::Execute();
		count--;
	}
}

// static 
WhileControlNode *WhileControlNode::CreateWhileFromXML(ITag *tag) {
	std::string scount = tag->getAttributeValue("count","");
	if (scount.empty()) {
		Logger::GetLogger("ScriptEngine")->Debug("Parse error for loop, count missing");
		return NULL;			
	}
	WhileControlNode *node = new WhileControlNode();
	node->org_count = atoi(scount.c_str());
	Logger::GetLogger("ScriptEngine")->Debug("Parsed loop for execution %d (%s) times", node->org_count, scount.c_str());		
	return node;
}


//
// class service control parameter
//
ServiceControlParameter::ServiceControlParameter() {

}

//static 
ServiceControlParameter *ServiceControlParameter::CreateParameterFromXML(ITag *tag) {
	ServiceControlParameter *param = new ServiceControlParameter();

	std::string name = tag->getAttributeValue("name","");
	std::string bind = tag->getAttributeValue("bind","");
	std::string data = tag->getContent();

	Logger::GetLogger("ScriptEngine")->Debug("Got parameter '%s'",name.c_str());


	param->name = name;
	param->bind = bind;
	param->data = data;
	return param;
}


//
// class ServiceControlNode
//
ServiceControlNode::ServiceControlNode(IServiceInstanceControl *_service) {
	this->service = _service;
}

// static
ServiceControlNode *ServiceControlNode::CreateServiceFromXML(ITag *tag) {
	ServiceControlNode *node = NULL;
	// parameter instance parsing
	std::string name = tag->getAttributeValue("name","");
	std::string typeClass = tag->getAttributeValue("class","");
	if (typeClass.empty()) {
		Logger::GetLogger("ScriptEngine")->Error("Empty class for service instance named '%s'",name.c_str());
		exit(1); // TODO: Raise error properly
	}

	Logger::GetLogger("ScriptEngine")->Debug("Got service '%s'", name.c_str());
	node = new ServiceControlNode(NULL);
	node->name = name;
	node->typeClass = typeClass;
	CreateParametersFromXML(tag, node->paramList);
	return node;
}

// static
int ServiceControlNode::CreateParametersFromXML(ITag *serviceTag, std::list<ServiceControlParameter *> &paramList) {
	int count = 0;
	std::list<ITag *> &paramTags = serviceTag->getChildren();
	for(auto it = paramTags.begin(); it != paramTags.end(); it++) {
		ServiceControlParameter *param = ServiceControlParameter::CreateParameterFromXML(*it);
		if (param != NULL) {
			paramList.push_back(param);			
		} else {
			return -1;
		}
		count++;
	}
	return count;
}

void ServiceControlNode::PreProcess(ServiceManager &serviceManager) {
	service = serviceManager.CreateInstance(typeClass.c_str());
	if (service == NULL) {
		Logger::GetLogger("ScriptEngine")->Debug("Failed to create instance for service, '%s' of class '%s'", 
			name.c_str(),
			typeClass.c_str());	

		return;
	}

	service->Initialize();
	// TODO: Go through parameter list and set values from the parameters
	// -> NOTE: Need to figure out how to handle type conversion

	// boost::any TypeConverter::Convert(std::string input, std::string toType)

}


void ServiceControlNode::Execute() {
	if (this->service != NULL) {
		this->service->PreExecute();
		this->service->Execute();
		//Node::Execute();
		this->service->PostExecute();			
	} else {
		Logger::GetLogger("ScriptEngine")->Debug("Execute service, '%s'", name.c_str());			
	}
}


//
// class track
//
Track::Track() {

}
// static
Track *Track::CreateTrackFromXML(ITag *tag) {
	Track *track = new Track();
	// As of now there is nothing to it...
	return track;
}

//
// class EngineExecutionControl
//
EngineExecutionControl::EngineExecutionControl() {
	current = NULL;
	currentParseStack.push(NULL);
}

void EngineExecutionControl::PushControlNode(Node *node) {
	controlNodes.push_back(node);
	currentParseStack.push(node);
	current = currentParseStack.top();	
}

void EngineExecutionControl::PopControlNode() {
	currentParseStack.pop();
	current = currentParseStack.top();
}

void EngineExecutionControl::AddNode(Node *node) {
	if (current != NULL) {
		current->Add(node);
	}
}
	
void EngineExecutionControl::PreProcess(ServiceManager &serviceManager) {
	Logger::GetLogger("ScriptEngine")->Debug("Preprocess control nodes");					
	for (auto it=controlNodes.begin(); it != controlNodes.end(); it++) {
		Node *n = *it;
		n->PreProcess(serviceManager);
	}
}
	
void EngineExecutionControl::Execute() {
	Logger::GetLogger("ScriptEngine")->Debug("Executing control nodes");			

	for (auto it=controlNodes.begin(); it != controlNodes.end(); it++) {
		Node *n = *it;
		Logger::GetLogger("ScriptEngine")->Debug("Control node");// %s",n->type().name().c_str());			
		n->Execute();
	}
}



// Class script engine
// - public
ScriptEngine::ScriptEngine() {
	control = new EngineExecutionControl();
}

void ScriptEngine::Load(std::string scriptdata) {
	// TODO: Deserialise the format
	LoadXMLScript(scriptdata);
}

void ScriptEngine::PreProcess(ServiceManager &serviceManager) {
	Logger::GetLogger("ScriptEngine")->Debug("Preprocess");							
	control->PreProcess(serviceManager);
}

void ScriptEngine::Execute() {
	// preprocess script here
	Logger::GetLogger("ScriptEngine")->Debug("Execute");		
	control->Execute();
}

// private

void ScriptEngine::LoadXMLScript(std::string definitiondata) {
	Logger::GetLogger("ScriptEngine")->Debug("Parsing XML");
	Document *doc = Parser::loadXML(definitiondata);
	Logger::GetLogger("ScriptEngine")->Debug("Adding traversing XML-DOM data");
	doc->traverse(
		std::bind(&ScriptEngine::OnDefinitionTagDataStart, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&ScriptEngine::OnDefinitionTagDataEnd, this, std::placeholders::_1, std::placeholders::_2)
		);
	Logger::GetLogger("ScriptEngine")->Debug("Done XML-DOM Data");
}

void ScriptEngine::OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes) {
	if (tag->getName() == "track") {
		Logger::GetLogger("ScriptEngine")->Debug("Got track, parsing from XML");	
		Track *track = Track::CreateTrackFromXML(tag);
		control->PushControlNode(track);
	} else if (tag->getName() == "service") {
		//Logger::GetLogger("ScriptEngine")->Debug("Got service, parsing from XML");	
		ServiceControlNode *node = ServiceControlNode::CreateServiceFromXML(tag);
		control->AddNode(node);
	} else if (tag->getName() == "while") {
		WhileControlNode *node = WhileControlNode::CreateWhileFromXML(tag);
		control->PushControlNode(node);
	}
}

void ScriptEngine::OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes) {
	if (tag->getName() == "track") {
		Logger::GetLogger("ScriptEngine")->Debug("pop track");	
		control->PopControlNode();
		Logger::GetLogger("ScriptEngine")->Debug("pop track");	
	} else if (tag->getName() == "service") {
	} else if (tag->getName() == "while") {
		Logger::GetLogger("ScriptEngine")->Debug("pop while");	
		control->PopControlNode();
		Logger::GetLogger("ScriptEngine")->Debug("pop while");	
	}
}

