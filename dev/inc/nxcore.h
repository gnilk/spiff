#pragma once

#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <functional>

#include <boost/any.hpp>
using boost::any_cast;


#include "logger.h"
#include "Timer.h"
#include "xmlparser.h"
#include "inifile.h"

using namespace Utils;

namespace Domain {
	class Message {

	};
};

namespace NXCore {


	typedef enum {
		ecSystemError,
		ecModuleError,
		ecOtherError,
	} ErrorClass;

	#ifdef WIN32
	#define CALLCONV __stdcall
	#undef GetObject
	#else
	#define CALLCONV
	#endif

	class ObjectContainer {
	public:
		ObjectContainer() {

		}
		virtual ~ObjectContainer() {

		}

		bool RegisterInstance(boost::any object, std::string identifier) {
			bool result = false;
			if (!HasInstance(identifier)) {
				objects.insert(std::make_pair(identifier, object));
				result = true;
			} else {
				Logger::GetLogger("ObjectContainer")->Error("Instance '%s' already existing", identifier.c_str());
			}
			return result;
		}
		bool HasInstance(std::string identifier)  {
			auto it = objects.find(identifier);
			if (it == objects.end()) {
				return false;
			}
			return true;
		}

		boost::any GetInstance(std::string identifier)  {
			auto it = objects.find(identifier);
			if (it == objects.end()) {
				return NULL;
			}
			return it->second;
		}

		// bool RegisterFactory(std::function<boost::any()> factory, std::string identifier);
		// boost::any CreateInstance(std::string identifier, bool autoregister);
		template <class T>
		bool RegisterFactory(std::function<T()> factory, std::string identifier)  {
			// check if factory exist
			factories.insert(std::make_pair(identifier, factory));
			return true;
		}


		template <class T>
		T CreateInstance(std::string identifier, bool autoregister)  {
			auto it = factories.find(identifier);
			if (it != factories.end()) {
			 	try
			    {	    	
			    	if (autoregister && HasInstance(identifier)) {
			    		Logger::GetLogger("conapp")->Debug("Instance already exist, returning existing");
			    		return boost::any_cast<T>(GetInstance(identifier));
			    	}
			    	std::function<T()> factory = boost::any_cast<std::function<T()> >(it->second);	
			    	T res = factory();
			    	RegisterInstance(boost::any(res), identifier);
			    	return res;
			    }
			    catch(const boost::bad_any_cast &)
			    {
					std::string name = it->second.type().name();
					std::string tname = typeid(T).name();
					Logger::GetLogger("conapp")->Debug("Bad type in factory type, name=%s != tname=%s",name.c_str(),tname.c_str());
			       // return false;
			    }	
			}
		}

	private:
		std::map<std::string, boost::any> objects;
		std::map<std::string, boost::any> factories;
	};

	class IConfig {
	public:
		virtual std::string GetValue(std::string section, std::string name, std::string defaultValue) = 0;
		virtual bool SetValue(std::string section, std::string name, std::string value) = 0;
		virtual bool HasValue(std::string section, std::string name) = 0;
		virtual void ReadIniFileStructure(std::string data) = 0;
		virtual bool DeleteValue(std::string section, std::string name) = 0;
		virtual bool DeleteSection(std::string section) = 0;

	};


	// some interfaces
	class ISystem {
	public:
		virtual Utils::ITimer *GetTimer() = 0;
		virtual ILogger *GetLogger(const char *name) = 0;
		virtual IConfig *GetConfig() = 0;
		virtual ObjectContainer *GetObjectContainer() = 0;
		virtual void RaiseError() = 0;
	};

	class IBaseManager {
	public:
		virtual ISystem *GetSystem() = 0;
	};

	// some classes


	// Should be implemented by every subsystem management class
	class BaseManager : public IBaseManager {
	public:
		virtual ISystem *GetSystem();
	};

	class Config : public IConfig {
	public:
		virtual std::string GetValue(std::string section, std::string name, std::string defaultValue);
		virtual bool SetValue(std::string section, std::string name, std::string value);
		virtual bool HasValue(std::string section, std::string name);
		virtual void ReadIniFileStructure(std::string data);
		virtual bool DeleteValue(std::string section, std::string name);
		virtual bool DeleteSection(std::string section);
	private:
		//gnilk::PropertyContainer container;
		gnilk::inifile::SectionContainer container;
	};

	// Note: System is a singleton for retrival of special class instance shared across plugins
	class System : public ISystem {
	public:
		virtual Utils::ITimer *GetTimer();
		virtual ILogger *GetLogger(const char *name);
		virtual void RaiseError();
		// - 
		static ISystem *GetInstance();
		virtual IConfig *GetConfig();
		virtual ObjectContainer *GetObjectContainer();
	private:
		Config configuration;
		ObjectContainer objectContainer;
	};


	// TODO: This should be a template
  	extern "C"
  	{
    	typedef int (CALLCONV *PFNINITIALIZEPLUGIN)(void *param);
	} 

	typedef std::function<void(std::string pathname, PFNINITIALIZEPLUGIN funcInitPlugin)> OnPluginLoadedDelegate;



	// TODO: Check if this can be done in order to load generic plugin initaliztion routines
/*
	template<class T> 
	class PluginScanner_T {
		typedef std::function<void(std::string pathname, T funcInitModule)> OnPluginLoadedDelegate;

	private:
		ILogger *pLogger;
		OnPluginLoadedDelegate handler;
		bool bRecurse;
		std::string initFuncName;
	protected:
		PluginScanner_T(bool recurse, std::string initFuncName, OnPluginLoadedDelegate handler);
		void DoScanDirectory(std::string path);
		void TryLoadLibrary(std::string &pathName);
		bool IsExtensionOk(std::string &extension);
		std::string GetExtension(std::string &pathName);
	public:
		static void ScanDirectory(bool recursive, std::string root, std::string initFuncName, OnPluginLoadedDelegate handler);

	};	
*/

	// This should take a template on the function from load
	class PluginScanner {
	private:
		ILogger *pLogger;
		OnPluginLoadedDelegate handler;
		bool bRecurse;
		std::string initFuncName;
	protected:
		PluginScanner(bool recurse, std::string initFuncName, OnPluginLoadedDelegate handler);
		void DoScanDirectory(std::string path);
		void TryLoadLibrary(std::string &pathName);
		bool IsExtensionOk(std::string &extension);
		std::string GetExtension(std::string &pathName);
	public:
		static void ScanDirectory(bool recursive, std::string root, std::string initFuncName, OnPluginLoadedDelegate handler);
	};

};

namespace Services {

	using namespace gnilk::xml;

	class IParameter {
	public:
		virtual std::string &GetName() = 0;
		virtual boost::any Get() = 0;
		virtual void Set(boost::any value) = 0;
	};


	class ParameterBinding;
	class Parameter : public IParameter {
		friend ParameterBinding;
	public:
		Parameter(std::string _name) {
			this->name = _name;
		}


		Parameter(std::string _name, boost::any _value) {
			this->name = _name;
			this->value = _value;
		}

		virtual std::string &GetName() { return name; }
		virtual boost::any Get() { return value; }
		virtual void Set(boost::any _value) { this->value = _value; }
	protected:
		std::string name;
		boost::any value;
	};

	class ParameterBinding : public IParameter {
	public:
		ParameterBinding(IParameter *_param, IParameter *_bindTo) {
			this->param = _param;
			this->bindTo = _bindTo;
		}
		static bool CanBind(IParameter *a, IParameter *b) {
			boost::any va = a->Get();
			boost::any vb = b->Get();
			return (va.type() == vb.type());
		}
		virtual std::string &GetName() { return param->GetName(); }
		virtual boost::any Get() { return bindTo->Get(); }
		virtual void Set(boost::any _value) { bindTo->Set(_value); }
	private:
		IParameter *param;
		IParameter *bindTo;
	};

	// TODO: refactor (separate implementation, follow style)
	class ParameterContainer {
	public:
		void Add(IParameter *value) {
			parameters.push_back(value);
		}

		IParameter *GetOrAdd(const std::string &name, boost::any value) {
			IParameter *param = Get(name);
			if (param != NULL) {
				if (param->Get().type() != value.type()) {
					printf("type mismatch for '%s'; %s != %s\n",
						name.c_str(),
						value.type().name(),
						param->Get().type().name());
					param = NULL;
				}
			} else {
				param = new Parameter(name, value);
				Add(param);
			}
			return param;
		}

		bool Has(const std::string &name) {
			if (Get(name) != NULL) {
				return true;
			}
			return false;
		}

		IParameter *Get(const std::string &name) {
			for(auto it = parameters.begin(); it!=parameters.end(); it++) {
				IParameter *param = *it;
				if (param->GetName() == name) return param;
			}		
			return NULL;
		}

		IParameter *Replace(const std::string &name, IParameter *newParam) {
			IParameter *oldParam;
			for(auto it = parameters.begin(); it!=parameters.end(); it++) {
				oldParam = *it;
				if (oldParam->GetName() == name) {
					parameters.erase(it);
					break;
				}
			}		
			Add(newParam);
			return oldParam;

		}

		void Dump() {
			for(auto it = parameters.begin(); it!=parameters.end(); it++) {
				IParameter *param = *it;
				printf("DUMP: %s\n",param->GetName().c_str());
			}
		}
	private:
		// todo: should be a map instead
		std::list<IParameter *> parameters;
	};

	// This is the service interface back to the system
	// TODO: this should have a 'RegisterForMessage' in order to intercept messages
	class IServiceInstanceManager {
	public:
		virtual IParameter *GetParameter(const std::string &name, boost::any defaultvalue) = 0;
		virtual IParameter *GetParameter(const std::string &name) = 0;
		virtual boost::any GetParameterValue(const std::string &name, boost::any defaultvalue) = 0;
		virtual void SetParameterValue(const std::string &name, boost::any value) = 0;
		virtual void BindParameter(const std::string &name, IParameter *bindTo) = 0;
	};


	// This is the external interface - to be implemented by any service
	//
	// TODO: this should have a 'OnMessage' function
	//
	class IServiceInstance {
	public:
		virtual void Initialize(IServiceInstanceManager *instanceManager = NULL) = 0;
		virtual void PreExecute() = 0;
		virtual void Execute() = 0;
		virtual void PostExecute() = 0;
		virtual void Dispose() = 0;
	};


	// This is the internal system/app interface for a service, basically execution control
	class IServiceInstanceControl {
	public:
		virtual void Initialize() = 0;
		virtual void PreExecute() = 0;
		virtual void Execute() = 0;
		virtual void PostExecute() = 0;
		virtual void Dispose() = 0;

		virtual IServiceInstanceManager *GetManagementInterface() = 0;
	};


	// The bridge between the service implementation and the system
	class ServiceInstanceBridge : public IServiceInstanceControl, public IServiceInstanceManager {
	public:
		ServiceInstanceBridge(IServiceInstance *instance);
		virtual ~ServiceInstanceBridge();

	public:	// IServiceInstanceControl interface
		virtual void Initialize();
		virtual void PreExecute();
		virtual void Execute();
		virtual void PostExecute();
		virtual void Dispose();	
		virtual IServiceInstanceManager *GetManagementInterface();	
	public:
		virtual IParameter *GetParameter(const std::string &name, boost::any defaultvalue);
		virtual IParameter *GetParameter(const std::string &name);
		virtual boost::any GetParameterValue(const std::string &name, boost::any defaultvalue);
		virtual void SetParameterValue(const std::string &name, boost::any value);
		virtual void BindParameter(const std::string &name, IParameter *bindTo);

	private:
		IServiceInstance *instance;
		ParameterContainer parameters;
	};


	class IServiceInstanceFactory;

	// TODO: could be made generic
	class IServiceManager : public NXCore::BaseManager {
	public:
		virtual void RegisterService(const char *service_id, IServiceInstanceFactory *factory) = 0;
		virtual IServiceInstanceControl *CreateInstance(const char *service_id) = 0;
	};

	// TODO: Template
	class IServiceInstanceFactory {
	public:
		virtual IServiceInstance *CreateInstance(const char *service_id, IServiceManager *serviceManager) = 0;
	};


  	extern "C"
  	{
    	typedef int (CALLCONV *PFNINITIALIZESERVICEMODULE)(IServiceManager *serviceManager);
	}

	class ParameterBase {
	public:
		ParameterBase(std::string _name);
		virtual ~ParameterBase();
		virtual std::string &GetName();
		virtual std::string &GetTypeName();
		virtual std::string &GetValueAsString();
	protected:
		std::string name;
		std::string typeName;
		std::string value;
	};

	class ParameterDefinition : public ParameterBase {
	public:
		ParameterDefinition(std::string _name);
		virtual ~ParameterDefinition();

		void SetTypeName(std::string _typeName);
		void SetDescription(std::string _description);
		void SetDisplayName(std::string _displayName);
		void SetDefaultValue(std::string _defaultValue);

		std::string ToXML();
	private:
		std::string description;
		std::string displayName;
		std::string defaultValue;
	};

	class IServiceDefinition {
	public:
		virtual std::string &GetName() = 0;
		virtual std::string &GetDescription() = 0;
		virtual std::list<ParameterDefinition *> &GetInputParameters() = 0;
		virtual std::list<ParameterDefinition *> &GetOutputParameters() = 0;
	};

	class ServiceDefinition : public IServiceDefinition {
	public:
		ServiceDefinition(std::string _name);
		virtual ~ServiceDefinition();
		virtual std::string &GetName();
		virtual std::string &GetDescription();
		virtual std::list<ParameterDefinition *> &GetInputParameters();
		virtual std::list<ParameterDefinition *> &GetOutputParameters();
		void SetDescription(std::string desc);
		std::string ToXML();
	private:
		std::string name;
		std::string description;
		std::list<ParameterDefinition *> inputParameters;
		std::list<ParameterDefinition *> outputParameters;
	};

	class ParameterDefinitionFromXML {
	public:
		static ParameterDefinition *CreateParameterDefinition(ITag *parameterTag);
	};

	class ServiceDefinitionFromXML {
	public:
		static ServiceDefinition *CreateServiceDefinition(ITag *serviceTag);
	private:
		static int ParseParameters(ITag *paramRootTag, std::list<ParameterDefinition *> &paramList);
	};



	// TODO: Move registration handling to base
	class ServiceManager : public IServiceManager {
	public:
		ServiceManager();
		virtual ~ServiceManager();
		void LoadServices();
		void LoadServiceDefinitions(std::string definitiondata);
		std::string SerializeDefinitionsToXML();
	public: // IServiceManager interfaces
		virtual void RegisterService(const char *service_id, IServiceInstanceFactory *factory);
		virtual IServiceInstanceControl *CreateInstance(const char *service_id);

	private:
		void OnPluginLoaded(std::string pathName, NXCore::PFNINITIALIZEPLUGIN funcInitPlugin);
		void OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes);
		void OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes);
	private:
		std::map<std::string, IServiceInstanceFactory *> serviceFactories;
		std::map<std::string, ServiceDefinition *> serviceDefinitions;
		//std::list<ServiceDefinition *>serviceDefinitions;
	};
};

namespace Script {
	using namespace gnilk::xml;
	using namespace Services;

	// TODO this is a pretty lousy name, but in lack for better I keep it for now (i.e. forever)
	class IScriptEngine {
	public:
		virtual void Load(std::string scriptdata) = 0;
		virtual void PreProcess(ServiceManager &serviceManager) = 0;
		virtual void Execute() = 0;

	};


	// TODO: want this to be modularized
	//	- hide behind factory (much like a services, plugin based)
	//  - need 'identify' before creation in order for factory to know which instance is applicable
	//  -> hence, each plugin to register factory and identity routine per script handler

	// Implementation specific
	class Node {
	public:
		virtual void Add(Node *node);
		virtual void PreProcess(ServiceManager &serviceManager);
		virtual void Execute();
	private:
		std::list<Node *> nodes;
	};

	class WhileControlNode : public Node {
	public:
		WhileControlNode();
		virtual void Execute();
		static WhileControlNode *CreateWhileFromXML(ITag *tag);
	private:
		int org_count;
		int count;
	};

	class ServiceControlParameter {
	public:
		ServiceControlParameter();
		static ServiceControlParameter *CreateParameterFromXML(ITag *tag);
	private:
		std::string name;
		std::string bind;
		std::string data;
	};

	class ServiceControlNode : public Node {
	public:
		ServiceControlNode(IServiceInstanceControl *_service);
		virtual void PreProcess(ServiceManager &serviceManager);
		virtual void Execute();

		static ServiceControlNode *CreateServiceFromXML(ITag *tag);
		static int CreateParametersFromXML(ITag *serviceTag, std::list<ServiceControlParameter *> &paramList);
	private:
		IServiceInstanceControl *service;
		std::list<ServiceControlParameter *> paramList;

		std::string name;
		std::string typeClass;
	};

	class EquipmentControlNode : public Node {
	public:
		EquipmentControlNode();
		static EquipmentControlNode *CreateEquipmentFromXML(ITag *tag);
	private:
		std::string name;
		std::string deviceId;
	};

	class Track : public Node {
	public:
		Track();
		static Track *CreateTrackFromXML(ITag *tag);
	};

	class EngineExecutionControl {
	public:
		EngineExecutionControl();
		void PushControlNode(Node *node);
		void PopControlNode();
		void AddNode(Node *node);
		void PreProcess(ServiceManager &serviceManager);
		void Execute();
	private:
		std::list<Node *> controlNodes;
		std::stack<Node *> currentParseStack;
		//std::stack<Node *>controlNodes;
		Node *current;
	};

	class ScriptEngine : public IScriptEngine {
	public:
		ScriptEngine();
		virtual void Load(std::string scriptdata);
		virtual void PreProcess(ServiceManager &serviceManager);
		virtual void Execute();
	private:
		void LoadXMLScript(std::string definitiondata);
		void OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes);
		void OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes);
	private:
		EngineExecutionControl *control;
	};

};

namespace MessageRouter {

	class Router;
	// this is an interface because of speed requirements, std::function is a 10x factor slower
	// messages are a core feature and need to be fast
	class IMessageSubscriber {
	public:
		virtual void OnMessage(int channelid, int sessionid, const Domain::Message &message) = 0;
	};

	class BaseMessageSubscriber : public IMessageSubscriber {
	public:
		virtual void OnMessage(int channelid, int sessionid, const Domain::Message &message);
	};

	class WriteChannel {
		friend Router;
	private:
		int id;
		Router &router;
	protected:
		WriteChannel(int _id, Router &_router);		
	public:
		void Write(int sessionid, const Domain::Message &message);

	};

	class FileWriter : public IMessageSubscriber {
	private:
		FILE *fp;
	public:
		bool Open(std::string &filename);
		bool Close();
		void Write(int nbytes, const void *buffer);
	public: // IMessageSubscriber interface
		virtual void OnMessage(int channelid, int sessionid, const Domain::Message &message);
	};

	#define MSG_CHANNEL_ANY 0x00

	class Router {

		friend WriteChannel;

	private:
		std::multimap<int, IMessageSubscriber *> channelSubscribers;
		std::map<int, WriteChannel *> channelWriters;
	public:
		Router();
		virtual ~Router();
		void InitalizeRouter();
		void SubscribeToChannel(int channelid, IMessageSubscriber *subscriber);
		void UnsubscribeFromChannel(int channelid, IMessageSubscriber *subscriber);
		WriteChannel *GetWriteChannel(int channelid);
	protected:
		void Write(int senderchannel, int sessionid, const Domain::Message &message);
		void WriteToAny(int senderchannel, int sessionid, const Domain::Message &message);
	};




};
