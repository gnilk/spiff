/*------------------------------------------------------------------------- 
 NOTE: Precision is important, can't use "float" when computing timing values...
 Must use double, this can lead to really strange errors!!!!
 
 ---------------------------------------------------------------------------*/
#pragma once

#include <TargetConditionals.h>

#ifdef TARGET_OS_IPHONE
#include <CFNetwork/CFNetwork.h>
#else
#import <CoreServices/CoreServices.h>
#endif

#import <CoreFoundation/CoreFoundation.h>

#include "NXCore.h"
#include "Timer.h"
#include "logger.h"

using namespace NXCore;
using namespace Utils;

namespace DataServices
{
	/* When using file streams, the 32KB buffer is probably not enough.
	 A good way to establish a buffer size is to increase it over time.
	 If every read consumes the entire buffer, start increasing the buffer
	 size, and at some point you would then cap it. 32KB is fine for network
	 sockets, although using the technique described above is still a good idea.
	 This sample avoids the technique because of the added complexity it
	 would introduce. */
	#define kFtpBufferSize			32768
	#define kFtpUploadDefaultSize	(1024*1024*10)
	
	class FtpStream;
	class IFTPStreamDelegates
		{
		public:
			virtual void OnError(FtpStream *pStream) = 0;
			virtual void OnOpenComplete(FtpStream *pStream) = 0;
			virtual void OnTransferBegin(FtpStream *pStream) = 0;
			virtual void OnProgress(FtpStream *pStream) = 0;
			virtual void OnTransferComplete(FtpStream *pStream) = 0;
			virtual void OnStreamFinished(FtpStream *pStream, bool bWasAborted) = 0;
		};
	
	class FtpStream
	{
	public:
		typedef enum
		{
			kTMUnknown = 0,
			kTMDownload = 1,
			kTMUpload = 2,
		} TransferMode;
		typedef enum
		{
			kTSUnknown,
			kTSConnecting,
			kTSOpened,
			kTSTransferring,
		} TransferState;
			
	private:
		ILogger *pLogger;
		
		CFStreamClientContext  context;
		
		CFWriteStreamRef  writeStream;              // download (destination file stream) and upload (FTP stream) only
		CFReadStreamRef   readStream;               // download (FTP stream), upload (source file stream), directory list (FTP stream)
		CFDictionaryRef   proxyDict;                // necessary to workaround <rdar://problem/3745574>, per discussion below
		SInt64            fileSize;                 // download only, 0 indicates unknown
		UInt32			  ulFileSize;				// Upload only, how many bytes to upload
		UInt32			  ulBytesLeft;				// Upload only, indicates how many bytes left to upload
		UInt32            totalBytesWritten;        // download and upload only
		UInt32			  lastAvgBytesWritten;		// Total bytes written when last average throughput was calculated
		UInt32            leftOverByteCount;        // upload and directory list only, number of valid bytes at start of buffer
		UInt8			 *buffer;
		
		// runtime states
		TransferState transState;
		unsigned int eLastError;
		
		// Timing info
		ITimer	*pTimer;
		int		timerFlags;
		double	tStart;
		double	tFirstByte;
		double  tLastAvgThroughPutByte;
		double  tLastByte;
		double  tEnd;
		IFTPStreamDelegates *pDelegates;
		
		bool	bAbort;
		bool	bShouldClose;
		bool	bIsClosed;
		
		TransferMode mode;
		// 060409, New statistics 
		typedef enum
		{
			kSSNone = 0,
			kSSOpening,
			kSSWaitingFirstByte,
			kSSTransferring,
			kSSClosing,
			kSSCompleted,
		} StreamState;
		bool bStatFirst;
		double tStatStart;
		double tStatEnd;
		double tStatOpenComplete;
		double tStatFirstByte;
		double tStatLastByte;
		double tStatLastReport;
		UInt32 nBytesLastReport;
		UInt32 totalBytesTransferred;
		double nBitsPerSecond;
		double nFinalBitsPerSecond;
		StreamState StatsState;
		
		void UpdateStatistics(CFStreamEventType event);
		// - End of new stats
		
		int FillUploadBuffer(int nBytes);
		bool SetUsernamePassword(CFTypeRef stream, CFStringRef username, CFStringRef password);
		void SetPassiveMode(CFTypeRef stream, bool bPassive);		
		void ReportError(CFIndex eCode);
	public:
		FtpStream(IFTPStreamDelegates *pDelegates);
		virtual ~FtpStream();
		
		bool ConnectDownload(CFStringRef urlString, CFURLRef destinationFolder, CFStringRef username, CFStringRef password);
		bool ConnectUpload(CFStringRef uploadDirectory, CFURLRef fileURL, CFStringRef username, CFStringRef password);
		void Transfer();
		void Abort();
		void Close();
		
		unsigned int GetLastError() { return eLastError; };
		
		void OnUploadCallback(CFWriteStreamRef writeStream, CFStreamEventType type);
		void OnDownloadCallback(CFReadStreamRef readStream, CFStreamEventType type);

		void SetUploadFileSize(long nBytesToUpload);
		long GetBytesWritten();
		long GetExpectedFileSize();
		double GetCompleteThroughPut();
		double GetAverageThroughPut(bool bForce=false);
		double GetTimeOfFirstByte();
		double GetTimeOfLastReadByte();
		
	};

}
