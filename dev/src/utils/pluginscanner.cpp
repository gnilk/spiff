// ----------------------
//
// Plugin scanner
//

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "nxcore.h"

using namespace NXCore;


// add this to the list of extensions we support
static const char *lExtensions[]=
{
".dll",
".dylib",
NULL
};

// implements the plugin scanner for windows
PluginScanner::PluginScanner(bool recurse, std::string initFuncName, OnPluginLoadedDelegate handler)
{
	pLogger = Logger::GetLogger("PluginScanner_Nix");
	bRecurse = recurse;
	this->handler = handler;
	this->initFuncName = initFuncName;
}

// returns the extension of a file name
std::string PluginScanner::GetExtension(std::string &pathName)
{
	std::string ext;
	int l = pathName.length()-1;
	while ((l>0) && (pathName[l]!='.'))
	{
		ext.insert(ext.begin(),pathName[l]);
		l--;
	}
	if ((l > 0) && (pathName[l]=='.'))
	{
		ext.insert(0,".");
	}
	return ext;
}

// Check extension from the list of supported extensions
bool PluginScanner::IsExtensionOk(std::string &extension)
{
	int i;
	for (i=0;lExtensions[i]!=NULL;i++)
	{
		if (!strcmp(extension.c_str(),lExtensions[i]))
		{
			return true;
		}
	}
	return false;
}


//
void PluginScanner::TryLoadLibrary(std::string &pathName)
{
	std::string ext = GetExtension(pathName);
	if (IsExtensionOk(ext))
	{
		//pLogger->Debug("Trying '%s'",pathName.c_str());
		
		void *pLib;
		pLib = dlopen(pathName.c_str(), RTLD_LAZY);
		
		if (pLib != NULL)
		{
			PFNINITIALIZEPLUGIN pFunc;
			pFunc = (PFNINITIALIZEPLUGIN)dlsym(pLib,initFuncName.c_str());
			if (pFunc != NULL)
			{
				pLogger->Info("Library '%s' ok, found: '%s'", pathName.c_str(), initFuncName.c_str());
				//IBaseInstance *pPlugin;
				//pPlugin = ySys->RegisterAndInitializePlugin(pFunc, pathName.c_str());
				handler(pathName, pFunc);
				// if (handler != NULL) {
				// 	handler->OnPluginLoaded(pathName, pFunc);
				// }
				
				
			} else
			{
				pLogger->Warning("Library '%s' failed - can't find init function, skipping...", pathName.c_str());
			}
		} else
		{
			pLogger->Warning("LoadLibrary failed, skipping..");
		}
	}
}

// assumes that path ends with '\'
void PluginScanner::DoScanDirectory(std::string path)
{
	std::string searchPath = path;
	DIR *pDir = NULL;
	pDir = opendir(path.c_str());
	
	if (pDir == NULL)
	{
		pLogger->Error("Unable to open directory: %s",searchPath.c_str());
		return;
	}
	//	searchPath.append("*.*");
	pLogger->Debug("Scanning: %s",searchPath.c_str());
	
	
	struct dirent *dp;
	while((dp = readdir(pDir)) != NULL)
	{
		struct stat _stat;
		
		std::string entryName(path+"/"+dp->d_name);
		lstat(entryName.c_str(), &_stat);
		
		if (bRecurse && S_ISDIR(_stat.st_mode))
		{
			if((strcmp(dp->d_name,".")!=0) && (strcmp(dp->d_name, "..")!=0))
			{
				DoScanDirectory(entryName);
			}
		} else
		{
			TryLoadLibrary(entryName);
		}
	}
	closedir(pDir);
}


// static
void PluginScanner::ScanDirectory(bool recurse, std::string root, std::string initFuncName, OnPluginLoadedDelegate handler)
{
	PluginScanner scanner(recurse, initFuncName, handler);
	scanner.DoScanDirectory(root);
}

