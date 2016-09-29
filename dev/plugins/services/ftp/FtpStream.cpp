/*-------------------------------------------------------------------------
 File    : $Archive: $
 Author  : $Author: $
 Version : $Revision: $
 Orginal : 2006-07-26, 15:50
 Descr   : FTP Stream, encapsulates the Os X CoreNetwork FTP handling
  
 Modified: $Date: $ by $Author: $
 ---------------------------------------------------------------------------
 TODO: [ -:Not done, +:In progress, !:Completed]
 <pre>
 - Verify assert's in Connect functions, they cause native application to crash!
 ! Average throughput should report the average since last report
 ! Add "TotalThroughput" as the average throughput between start and lastByte
 - Proper stop/abort handling, sometime the transfer runloop hangs during stop (does not abort)
 </pre>
 
 
 \History
 - 08.04.09, FKling, Error reporting
 - 06.04.09, FKling, Rewrote throughput calculations
 - 02.04.09, FKling, Streamlined the FTP interfaces
 - 10.03.09, FKling, Upload implementation
 - 27.01.09, FKling, Implementation
 
 ---------------------------------------------------------------------------*/

#include <TargetConditionals.h>

#ifdef TARGET_OS_IPHONE
#include <CFNetwork/CFNetwork.h>
#else
#import <CoreServices/CoreServices.h>
#endif

#import <CoreFoundation/CoreFoundation.h>

#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h>         // getopt
#include <string.h>         // strmode
#include <stdlib.h>
#include <inttypes.h>
#include <string>


#include "nxcore.h"
#include "logger.h"
#include "Timer.h"
#include "FtpStream.h"
//#include "System.h"
//#include "Error.h"
#include "FtpErrorCodes.h"

using namespace DataServices;
using namespace Utils;
using namespace NXCore;
using namespace std;

#define AVG_COMPLETE_FILE 0		// Set to 1 if you want accumulated averaging over the complete file

#define WAIT_TIME_BEFORE_THROUGHPUT_SEC	2.0		// two seconds before throughtput value is calculated, in Complete mode - first throughput value, otherwise it is the seconds between blocks

//////////////////////////
//
// The FTPStream class is a helper class for the TaskFTPDownload/TaskFTPUpload
//

static const CFOptionFlags kNetworkEvents = 
kCFStreamEventOpenCompleted
| kCFStreamEventHasBytesAvailable
| kCFStreamEventEndEncountered
| kCFStreamEventCanAcceptBytes
| kCFStreamEventErrorOccurred;

#define FTP_TIMER_FLAG_START		1
#define FTP_TIMER_FLAG_FIRSTBYTE	2
#define FTP_TIMER_FLAG_LASTBYTE		4
#define FTP_TIMER_FLAGS_STOP		8

// CTOR, normal
FtpStream::FtpStream(IFTPStreamDelegates *pDelegates)
{
	this->pDelegates = pDelegates;
	buffer = (UInt8*)malloc(kFtpBufferSize);
	
    proxyDict = NULL;           // see discussion of <rdar://problem/3745574> below
    fileSize = 0;
    totalBytesWritten = 0;
    leftOverByteCount = 0;
	lastAvgBytesWritten = 0;
	pTimer = NULL; // new Timer();
	timerFlags = 0;
	bAbort = false;
	bShouldClose = false;
	bIsClosed = false;
	mode = kTMUnknown;
	ulFileSize = kFtpUploadDefaultSize;
	ulBytesLeft = ulFileSize;
	
	transState = kTSUnknown;
	eLastError = 0;			// should be: ERR_NO_ERROR
	
	// Stats, new stuff
	totalBytesTransferred = 0;
	StatsState = kSSNone;
	nBitsPerSecond = -1.0;
	nFinalBitsPerSecond = 1.0;
	
	pLogger = NULL; //Logger::GetLogger("FtpStream");
}

// DTOR, normal
FtpStream::~FtpStream()
{
	// Security...
	Close();
	free(buffer);
}
void FtpStream::Abort()
{
	bAbort = true;
	// TODO: this needs more, we must "stop" the runloop thing..

		// - 08.04.09, FKling, this does not work...
//	pLogger->Debug("Abort called, this is a very unreliable operation...");
//	try
//	{
//		bShouldClose = true;
//		Close();
//	} catch(...)
//	{
//		pLogger->Error("Exception during close...");
//	}
}

//
// Report an error to the global error handler..
//
void FtpStream::ReportError(CFIndex eCode)
{
	ErrorClass eclass = NXCore::ErrorClass::ecModuleError;
	int ecode;
	
	// Different codes depending on state and the error code reported by CoreFoundation
	if (((eCode ==  kCFHostErrorUnknown) || (eCode ==  kCFHostErrorHostNotFound)) && (transState == kTSConnecting)) eCode = FTP_ERR_FTPCONN;
	if ((eCode ==  kCFFTPErrorUnexpectedStatusCode) && (transState == kTSConnecting)) eCode = FTP_ERR_FTPAUTH;
	if ((eCode ==  kCFFTPErrorUnexpectedStatusCode) && (transState == kTSOpened)) eCode = FTP_ERR_FTPFILE;
	if (transState == kTSTransferring) eCode = FTP_ERR_FTPTRANS;
	
	eLastError = eCode;
	//System::ReportModuleError(SQ_ERR_MODULE_TEST,eSqCode);
	if (pDelegates != NULL)
	{
		try
		{
			pDelegates->OnError(this);
		} catch(...)
		{
			
		}
	}
}

//
// Set username and password
//
bool FtpStream::SetUsernamePassword(CFTypeRef stream, CFStringRef username, CFStringRef password)
{
    bool success;
    assert(stream != NULL);
    assert( (username != NULL) || (password == NULL) );
    
    if (username && CFStringGetLength(username) > 0) 
	{
		
        if (CFGetTypeID(stream) == CFReadStreamGetTypeID()) 
		{
            success = CFReadStreamSetProperty((CFReadStreamRef)stream, kCFStreamPropertyFTPUserName, username);
            assert(success);
            if (password) 
			{
                success = CFReadStreamSetProperty((CFReadStreamRef)stream, kCFStreamPropertyFTPPassword, password);
                assert(success);
            }
        } else 
			if (CFGetTypeID(stream) == CFWriteStreamGetTypeID()) 
			{
				success = CFWriteStreamSetProperty((CFWriteStreamRef)stream, kCFStreamPropertyFTPUserName, username);
				assert(success);
				if (password) {
					success = CFWriteStreamSetProperty((CFWriteStreamRef)stream, kCFStreamPropertyFTPPassword, password);
					assert(success);
				}
			} else 
			{
				assert(false);
			}
    }
	return success;
}

//
// Set passive mode
//
void FtpStream::SetPassiveMode(CFTypeRef stream, bool bPassive)
{
	CFBooleanRef isPassive;
	if (bPassive) isPassive = kCFBooleanTrue;
	else isPassive = kCFBooleanFalse;
	
	CFReadStreamSetProperty((CFReadStreamRef)stream, kCFStreamPropertyFTPUsePassiveMode, isPassive);
}

//
// Event handler, called by OS on stream events
//
void FtpStream::OnDownloadCallback(CFReadStreamRef readStream, CFStreamEventType type)
{
    CFIndex           bytesRead = 0, bytesWritten = 0;
    CFStreamError     error;
    CFNumberRef       cfSize;
    SInt64            size;
	double			  tOldLastByte;
    
    assert(readStream != NULL);
    assert(this->readStream == readStream);
	
	if (bAbort)
	{
		goto exit;
	}
	
	UpdateStatistics(type);
	
    switch (type) 
	{
			
        case kCFStreamEventOpenCompleted:
			transState = kTSOpened;
			
            cfSize = (CFNumberRef)CFReadStreamCopyProperty(this->readStream, kCFStreamPropertyFTPResourceSize);            
            if (cfSize) 
			{
                if (CFNumberGetValue(cfSize, kCFNumberSInt64Type, &size)) 
				{
					//printf("File size is %d\n",size);
                    fileSize = size;
                }
                CFRelease(cfSize);
            } else 
			{
                //pLogger->Warning("File size is unknown");
                assert(fileSize == 0);            // It was set up this way by MyStreamInfoCreate.
            }
			// Notify open/login complete
			pDelegates->OnOpenComplete(this);
            break;
        case kCFStreamEventHasBytesAvailable:
			transState = kTSTransferring;
			
			if (!(timerFlags & FTP_TIMER_FLAG_FIRSTBYTE))
			{
				timerFlags |= FTP_TIMER_FLAG_FIRSTBYTE;
				pDelegates->OnTransferBegin(this);
			} else
			{
				timerFlags |= FTP_TIMER_FLAG_LASTBYTE;
			}
            /* CFReadStreamRead will return the number of bytes read, or -1 if an error occurs
			 preventing any bytes from being read, or 0 if the stream's end was encountered. */
            bytesRead = CFReadStreamRead(this->readStream, buffer, kFtpBufferSize);
            if (bytesRead > 0) 
			{			
                bytesWritten = 0;				
				bytesWritten = bytesRead;
                totalBytesWritten += bytesWritten;
				
				totalBytesTransferred += bytesRead;
				// TODO: Fix this
				if (timerFlags & FTP_TIMER_FLAG_LASTBYTE)
				{
					pDelegates->OnProgress(this);
				}
				
            } else 
			{
                /* If bytesRead < 0, we've hit an error.  If bytesRead == 0, we've hit the end of the file.  
				 In either case, we do nothing, and rely on CF to call us with kCFStreamEventErrorOccurred 
				 or kCFStreamEventEndEncountered in order for us to do our clean up. */
            }
            break;
        case kCFStreamEventErrorOccurred:
            {
                CFErrorRef new_error = CFReadStreamCopyError(this->readStream);
                CFStringRef err_desc = CFErrorCopyDescription(new_error);
                char tmpbuff[256];
                CFStringGetCString(err_desc, tmpbuff, 256, kCFStringEncodingUTF8);
                //pLogger->Error("CFReadStreamCopyError: %s - transferState = %d",tmpbuff, (int)transState);	
				CFIndex eCode = CFErrorGetCode(new_error);
				ReportError(eCode);
            }
//			pDelegates->OnError(this);
            goto exit;
        case kCFStreamEventEndEncountered:
			//pLogger->Debug("OnDownloadCallback, Stream end event occurred");
			pDelegates->OnTransferComplete(this);
            goto exit;
        default:
            //pLogger->Error("Received unexpected CFStream event (%d)", type);
            break;
    }
    return;
    // Only to this point when stopping
exit:    
	tEnd = pTimer->Sample(NULL);	
    CFRunLoopStop(CFRunLoopGetCurrent());
    return;	
}

int FtpStream::FillUploadBuffer(int nBytes)
{
	int i;
	if (nBytes > kFtpBufferSize) nBytes = kFtpBufferSize;
	if (nBytes > ulBytesLeft) nBytes = ulBytesLeft;

	for (i=0;i<nBytes;i++)
	{
		buffer[i] = (UInt8) (i&255);
	}
	ulBytesLeft -= nBytes;
	return nBytes;
}


/* MyUploadCallBack is the stream callback for the CFFTPStream during an upload operation. 
 Its main purpose is to wait for space to become available in the FTP stream (the write stream), 
 and then read bytes from the file stream (the read stream) and write them to the FTP stream. */
void FtpStream::OnUploadCallback(CFWriteStreamRef writeStream, CFStreamEventType type)
{
    CFIndex          bytesRead;
    CFIndex          bytesAvailable;
    CFIndex          bytesWritten;
    CFStreamError    error;
	double			  tOldLastByte;
    
    assert(writeStream != NULL);
    assert(this->writeStream == writeStream);

	if (bAbort)
	{
		goto exit;
	}
	
	UpdateStatistics(type);
	
    switch (type) 
	{
	
        case kCFStreamEventOpenCompleted:
            //pLogger->Info("Open complete\n");
			// Notify open/login complete
			transState = kTSOpened;
			pDelegates->OnOpenComplete(this);
            break;
        case kCFStreamEventCanAcceptBytes:
			transState = kTSTransferring;
						
			if (!(timerFlags & FTP_TIMER_FLAG_FIRSTBYTE))
			{
				timerFlags |= FTP_TIMER_FLAG_FIRSTBYTE;
				pDelegates->OnTransferBegin(this);
			} else
			{
				timerFlags |= FTP_TIMER_FLAG_LASTBYTE;
			}
			
            /* The first thing we do is check to see if there's some leftover data that we read
			 in a previous callback, which we were unable to upload for whatever reason. */
            if (this->leftOverByteCount > 0) 
			{
                bytesRead = 0;
                bytesAvailable = this->leftOverByteCount;
            } else 
			{
                /* If not, we try to read some more data from the file.  CFReadStreamRead will 
				 return the number of bytes read, or -1 if an error occurs preventing 
				 any bytes from being read, or 0 if the stream's end was encountered. */
				
//                bytesRead = CFReadStreamRead(info->readStream, info->buffer, kMyBufferSize);
				bytesRead = FillUploadBuffer(kFtpBufferSize);
//				bytesRead = kFtpBufferSize;
                if (bytesRead < 0) 
				{
                    //pLogger->Error("CFReadStreamRead returned %ld", bytesRead);
                    goto exit;
                }
                bytesAvailable = bytesRead;
            }
            bytesWritten = 0;
            
            if (bytesAvailable == 0) 
			{
                /* We've hit the end of the file being uploaded.  Shut everything down. 
				 Previous versions of this sample would terminate the upload stream 
				 by writing zero bytes to the stream.  After discussions with CF engineering, 
				 we've decided that it's better to terminate the upload stream by just 
				 closing the stream. */
                //pLogger->Info("End uploaded file; forcing events and closing down");
				
				// The following event is not sent by CF in the case of upload
				UpdateStatistics(kCFStreamEventEndEncountered);
				pDelegates->OnTransferComplete(this);
                goto exit;
            } else 
			{
				
                /* CFWriteStreamWrite returns the number of bytes successfully written, -1 if an error has
				 occurred, or 0 if the stream has been filled to capacity (for fixed-length streams).
				 If the stream is not full, this call will block until at least one byte is written. 
				 However, as we're in the kCFStreamEventCanAcceptBytes callback, we know that at least 
				 one byte can be written, so we won't block. */
				
                bytesWritten = CFWriteStreamWrite(this->writeStream, this->buffer, bytesAvailable);
                if (bytesWritten > 0) 
				{
					
                    this->totalBytesWritten += bytesWritten;
					this->totalBytesTransferred += bytesWritten;
                    
                    /* If we couldn't upload all the data that we read, we temporarily store the data in our MyStreamInfo
					 context until our CFWriteStream callback is called again with a kCFStreamEventCanAcceptBytes event. 
					 Copying the data down inside the buffer is not the most efficient approach, but it makes the code 
					 significantly easier. */
                    if (bytesWritten < bytesAvailable) 
					{
                        this->leftOverByteCount = bytesAvailable - bytesWritten;
                    } else 
					{
                        this->leftOverByteCount = 0;
                    }
                } else if (bytesWritten < 0) 
				{
                    //pLogger->Error("CFWriteStreamWrite returned %ld", bytesWritten);
                }
            }
            
            /* Print a status update if we made any forward progress. */
            if ( (bytesRead > 0) || (bytesWritten > 0) ) 
			{
                //fprintf(stderr, "\rRead %7ld bytes; Wrote %8ld bytes\n", bytesRead, this->totalBytesWritten);
            }
			
            if (timerFlags & FTP_TIMER_FLAG_LASTBYTE)
			{
				pDelegates->OnProgress(this);
			}
			
            break;
        case kCFStreamEventErrorOccurred:
			{
				CFErrorRef new_error = CFWriteStreamCopyError(this->writeStream);
				CFStringRef err_desc = CFErrorCopyDescription(new_error);
				char tmpbuff[256];
				CFStringGetCString(err_desc, tmpbuff, 256, kCFStringEncodingUTF8);
				//pLogger->Error("CFWriteStreamCopyError: %s",tmpbuff);
				error = CFWriteStreamGetError(this->writeStream);
				//pLogger->Error("CFWriteStreamGetError returned (%d, %ld)", error.domain, error.error);
				
				CFIndex eCode = CFErrorGetCode(new_error);
				ReportError(eCode);
			}
//			pDelegates->OnError(this);
            goto exit;
        case kCFStreamEventEndEncountered:
            //printf("\nDownload complete\n");
			//pTaskHandler->EndDownload(tLastByte, totalBytesWritten);
			pDelegates->OnTransferComplete(this);
            goto exit;
        default:
            //pLogger->Error("Received unexpected CFStream event (%d)", type);
            break;
    }
    return;
    
exit:
//    MyStreamInfoDestroy(info);
	tEnd = pTimer->Sample(NULL);	
    CFRunLoopStop(CFRunLoopGetCurrent());
    return;
}

static void glbFTPDownloadCallback(CFReadStreamRef readStream, CFStreamEventType type, void * clientCallBackInfo)
{
	FtpStream *pStream = (FtpStream *)clientCallBackInfo;
	if (pStream != NULL)
	{
		pStream->OnDownloadCallback(readStream, type);
	}
}
static void glbFTPUploadCallback(CFWriteStreamRef writeStream, CFStreamEventType type, void * clientCallBackInfo)
{
	FtpStream *pStream = (FtpStream *)clientCallBackInfo;
	if (pStream != NULL)
	{
		pStream->OnUploadCallback(writeStream, type);
	}	
}

bool FtpStream::ConnectUpload(CFStringRef uploadDirectory, CFURLRef fileURL, CFStringRef username, CFStringRef password)
{
//    CFWriteStreamRef       writeStream;
//    CFReadStreamRef        readStream;
//    CFStreamClientContext  context = { 0, NULL, NULL, NULL, NULL };
    CFURLRef               uploadURL, destinationURL;
    CFStringRef            fileName;
    Boolean                success = true;
  //  MyStreamInfo           *streamInfo;

	mode = kTMUpload;
	
    assert(uploadDirectory != NULL);
    assert(fileURL != NULL);
    assert( (username != NULL) || (password == NULL) );
    
    /* Create a CFURL from the upload directory string */
    destinationURL = CFURLCreateWithString(kCFAllocatorDefault, uploadDirectory, NULL);
    assert(destinationURL != NULL);
	
    /* Copy the end of the file path and use it as the file name. */
    fileName = CFURLCopyLastPathComponent(fileURL);
    assert(fileName != NULL);
	
    /* Create the destination URL by taking the upload directory and appending the file name. */
    uploadURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, destinationURL, fileName, false);
    assert(uploadURL != NULL);
    CFRelease(destinationURL);
    CFRelease(fileName);
    
    /* Create a CFReadStream from the local file being uploaded. */
    //readStream = CFReadStreamCreateWithFile(kCFAllocatorDefault, fileURL);
    //assert(readStream != NULL);
    
    /* Create an FTP write stream for uploading operation to a FTP URL. If the URL specifies a
	 directory, the open will be followed by a close event/state and the directory will have been
	 created. Intermediary directory structure is not created. */
    writeStream = CFWriteStreamCreateWithFTPURL(kCFAllocatorDefault, uploadURL);
    assert(writeStream != NULL);
    CFRelease(uploadURL);
    
    /* Initialize our MyStreamInfo structure, which we use to store some information about the stream. */
//    MyStreamInfoCreate(&streamInfo, readStream, writeStream);
 //   context.info = (void *)streamInfo;
	
	memset((void *)&context,0,sizeof(context));
	//    MyStreamInfoCreate(pThis, &streamInfo, readStream, writeStream);
    context.info = (void *)this;
	// this->readStream = readStream;
	// this->writeStream = writeStream;
	
	
    /* CFReadStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
	 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
	 listen to the run loop source to find out when the open completes and whether it was successful. */
//    success = CFReadStreamOpen(readStream);
	success = true;
    if (success) 
	{
        
        /* CFWriteStreamSetClient registers a callback to hear about interesting events that occur on a stream. */
        success = CFWriteStreamSetClient(writeStream, kNetworkEvents, glbFTPUploadCallback, &context);
        if (success) 
		{
			pTimer->Reset();
			tStart = pTimer->Sample();
		
			
            /* Schedule a run loop on which the client can be notified about stream events.  The client
			 callback will be triggered via the run loop.  It's the caller's responsibility to ensure that
			 the run loop is running. */
            CFWriteStreamScheduleWithRunLoop(writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
            
            SetUsernamePassword(writeStream, username, password);
            //MyCFStreamSetFTPProxy(writeStream, &streamInfo->proxyDict);
            
            /* CFWriteStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
			 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
			 listen to the run loop source to find out when the open completes and whether it was successful. */
            success = CFWriteStreamOpen(writeStream);
            if (success == false) 
			{
                //pLogger->Error("CFWriteStreamOpen failed");
            }
        } else 
		{
            //pLogger->Error("CFWriteStreamSetClient failed");
        }
    }
	
	if (success)
	{
		//pLogger->Debug("Upload stream created ok");
		bShouldClose = true;
	}
	
    return success;
}

bool FtpStream::ConnectDownload(CFStringRef urlString, CFURLRef destinationFolder, CFStringRef username, CFStringRef password)
{
	//    CFStreamClientContext  context = { 0, NULL, NULL, NULL, NULL };
    CFURLRef               downloadPath, downloadURL;
    CFStringRef            fileName;
    bool                dirPath, success = true;

	mode = kTMDownload;
	
    assert(urlString != NULL);
    assert(destinationFolder != NULL);
    assert( (username != NULL) || (password == NULL) );
	
    /* Returns true if the CFURL path represents a directory. */
    dirPath = CFURLHasDirectoryPath(destinationFolder);
    if (!dirPath) 
	{
        //pLogger->Error("Download destination must be a directory.");
        return false;
    }
    
    /* Create a CFURL from the urlString. */
    downloadURL = CFURLCreateWithString(kCFAllocatorDefault, urlString, NULL);
    assert(downloadURL != NULL);
	
    /* Copy the end of the file path and use it as the file name. */
    fileName = CFURLCopyLastPathComponent(downloadURL);
    assert(fileName != NULL);
	
    /* Create the downloadPath by taking the destination folder and appending the file name. */
    downloadPath = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, destinationFolder, fileName, false);
    assert(downloadPath != NULL);
    CFRelease(fileName);
	
    /* Create a CFWriteStream for the file being downloaded. */
    //writeStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault, downloadPath);
    //assert(writeStream != NULL);
    CFRelease(downloadPath);
    
    /* CFReadStreamCreateWithFTPURL creates an FTP read stream for downloading from an FTP URL. */
    readStream = CFReadStreamCreateWithFTPURL(kCFAllocatorDefault, downloadURL);
    assert(readStream != NULL);
    CFRelease(downloadURL);
    
    /* Initialize our MyStreamInfo structure, which we use to store some information about the stream. */
	writeStream = NULL;
	
	// = { 0, NULL, NULL, NULL, NULL };
	memset((void *)&context,0,sizeof(context));
	//    MyStreamInfoCreate(pThis, &streamInfo, readStream, writeStream);
    context.info = (void *)this;
	// this->readStream = readStream;
	// this->writeStream = writeStream;
	
    /* CFWriteStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
	 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
	 listen to the run loop source to find out when the open completes and whether it was successful. */
	//  success = CFWriteStreamOpen(writeStream);
	success = true;
    if (success) 
	{		
        /* CFReadStreamSetClient registers a callback to hear about interesting events that occur on a stream. */
		//        success = CFReadStreamSetClient(readStream, kNetworkEvents, MyDownloadCallBack, &context);
		success = CFReadStreamSetClient(readStream, kNetworkEvents, glbFTPDownloadCallback, &context);
        if (success) 
		{
			pTimer->Reset();
			tStart = pTimer->Sample();
			
			/* Schedule a run loop on which the client can be notified about stream events.  The client
			 callback will be triggered via the run loop.  It's the caller's responsibility to ensure that
			 the run loop is running. */
            CFReadStreamScheduleWithRunLoop(readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
            
            SetUsernamePassword(readStream, username, password);
			//            MyCFStreamSetFTPProxy(readStream, &streamInfo->proxyDict);
            
            /* Setting the kCFStreamPropertyFTPFetchResourceInfo property will instruct the FTP stream
			 to fetch the file size before downloading the file.  Note that fetching the file size adds
			 some time to the length of the download.  Fetching the file size allows you to potentially
			 provide a progress dialog during the download operation. You will retrieve the actual file
			 size after your CFReadStream Callback gets called with a kCFStreamEventOpenCompleted event. */
            CFReadStreamSetProperty(readStream, kCFStreamPropertyFTPFetchResourceInfo, kCFBooleanTrue);
            
            /* CFReadStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
			 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
			 listen to the run loop source to find out when the open completes and whether it was successful. */
            success = CFReadStreamOpen(readStream);
            if (success == false) 
			{
                //pLogger->Error("CFReadStreamOpen failed");
            }
        } else 
		{
            //pLogger->Error("CFReadStreamSetClient failed");
        }
    }
	
	if (success)
	{
		//pLogger->Debug("Download stream created ok");
		bShouldClose = true;
	}
	
    return success;	
}

void FtpStream::Transfer()
{
	// Signal start...
	transState = kTSConnecting;
	UpdateStatistics(kCFStreamEventNone);
	//pLogger->Debug("Transfer, entering runloop");
	CFRunLoopRun();
	Close();
	// Notify that we have finished the stream
	pDelegates->OnStreamFinished(this, bAbort);	
	//pLogger->Debug("Transfer, runloop finished");
}

void FtpStream::Close()
{
	if ((bShouldClose) && (!bIsClosed))
	{
		if (mode == kTMDownload)
		{
			//pLogger->Debug("Close, Download mode");
			CFReadStreamUnscheduleFromRunLoop(readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
			(void) CFReadStreamSetClient(readStream, kCFStreamEventNone, NULL, NULL);
			
			/* CFReadStreamClose terminates the stream. */
			CFReadStreamClose(readStream);
			CFRelease(readStream);	
		} else 
		{
			//pLogger->Debug("Close, Upload mode");
			CFWriteStreamUnscheduleFromRunLoop(writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
			(void) CFWriteStreamSetClient(writeStream, kCFStreamEventNone, NULL, NULL);
			
			/* CFReadStreamClose terminates the stream. */
			CFWriteStreamClose(writeStream);
			CFRelease(writeStream);	
		}
		bIsClosed = true;
	} else
	{
		//pLogger->Debug("Close, did not close reason: bShouldClose=%s bIsClosed=%s",bShouldClose?"true":"false", bIsClosed?"true":"false");
	}
	//pLogger->Debug("Close, done");

}
void FtpStream::SetUploadFileSize(long nBytesToUpload)
{
	ulFileSize = nBytesToUpload;
	ulBytesLeft = ulFileSize;
}

void FtpStream::UpdateStatistics(CFStreamEventType event)
{
	switch (event) 
	{
		case kCFStreamEventNone :
			StatsState = kSSOpening;
			
			tStatStart = pTimer->Sample(NULL);
			tStatFirstByte = -1;
			break;
		case kCFStreamEventOpenCompleted :
			if (StatsState != kSSOpening)
			{
				//pLogger->Warning("StatsState != kSSOpening");
			}
			StatsState = kSSWaitingFirstByte;
			tStatOpenComplete = pTimer->Sample(NULL);

		// First time we have not yet read a single byte
		case kCFStreamEventCanAcceptBytes :				// Upload
		case kCFStreamEventHasBytesAvailable :			// Download
			tStatLastByte = pTimer->Sample(NULL);
			if (tStatFirstByte < 0)
			{
				//pLogger->Debug("Time for first byte");
				tStatFirstByte = tStatLastByte;
				tStatLastReport = tStatLastByte;
				nBytesLastReport = 0;
				StatsState = kSSTransferring;				
			} else
			{
				// Calculate current throughput
				if (AVG_COMPLETE_FILE)
				{
					// Accumulated FTP throughput mode
					if ((tStatLastByte - tStatFirstByte) > WAIT_TIME_BEFORE_THROUGHPUT_SEC)		// If fast, wait a little bit until it has settled...
					{
						nBitsPerSecond = ((double)totalBytesTransferred) / (tStatLastByte - tStatFirstByte);
					}
				} else
				{
					// Partial FTP throughput mode
					if ((tStatLastByte - tStatLastReport) > WAIT_TIME_BEFORE_THROUGHPUT_SEC)
					{
						nBitsPerSecond = ((double)(totalBytesTransferred - nBytesLastReport)) / (tStatLastByte - tStatLastReport);
						tStatLastReport = tStatLastByte;
						nBytesLastReport = totalBytesTransferred;
					}
				}					
			}
			break;
			
		case kCFStreamEventErrorOccurred :
			StatsState = kSSCompleted;
			// Signal error!
			break;
			
		case kCFStreamEventEndEncountered :
			//pLogger->Debug("UpdateStats: kCFStreamEventEndEncountered");
			StatsState = kSSCompleted;
			tStatEnd = pTimer->Sample(NULL);		
			nFinalBitsPerSecond	= ((double)totalBytesTransferred) / (tStatEnd - tStatFirstByte);
			break;
	}
}

long FtpStream::GetBytesWritten()
{
	return totalBytesTransferred;
}

long FtpStream::GetExpectedFileSize()
{
	long sz;
	if (mode == kTMDownload)
	{
		sz = fileSize;
	} else
	{
		sz = ulFileSize;
	}
	return sz;
}

double FtpStream::GetCompleteThroughPut()
{
	return nFinalBitsPerSecond;
}

double FtpStream::GetAverageThroughPut(bool bForce /* = false*/)
{
	return nBitsPerSecond;
}

double FtpStream::GetTimeOfFirstByte()
{
	return (double)tStatStart;
}

double FtpStream::GetTimeOfLastReadByte()
{
	return (double)tStatLastByte;
}

