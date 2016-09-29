#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <functional>

#include "nxcore.h"

//#include <SafeQueue.h>


using namespace MessageRouter;
using namespace Domain;

// -------------------
//
// File writer
//
bool FileWriter::Open(std::string &filename) {
	if (fp != NULL) {
		// 
		return true;
	}

	fp = fopen(filename.c_str(), "wb");
	if (fp == NULL) return false;
	return true;
}

bool FileWriter::Close() {
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	return true;
}

void FileWriter::Write(int nbytes, const void *buffer) {
	if (fp == NULL) return;
	fwrite(buffer, nbytes, 1, fp);
}


void FileWriter::OnMessage(int channelid, int sessionid, const Domain::Message &message) {
	//
	// 1. binary serialize the message
	// 2. compose the buffer
	// 3. enqueue for writing
	//
	printf("FileWriter::OnMessage(%d,%d)\n",channelid,sessionid);
}
