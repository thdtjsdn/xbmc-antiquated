//  CCdInfo		 -  Information about media type of an inserted cd
//  CCdIoSupport -  Wrapper class for libcdio with the interface of CIoSupport
//					and detecting the filesystem on the Disc.
//
//	by Bobbin007 in 2003
//	
//	
//

#pragma once

#include <xtl.h>
#include "../xbox/iosupport.h"
#include "stdstring.h"
#include "../lib/libcdio/cdio.h"

namespace MEDIA_DETECT 
{

#define STRONG "__________________________________\n"
#define NORMAL ""

#define FS_NO_DATA              0   /* audio only */
#define FS_HIGH_SIERRA			1
#define FS_ISO_9660				2
#define FS_INTERACTIVE			3
#define FS_HFS					4
#define FS_UFS					5
#define FS_EXT2					6
#define FS_ISO_HFS              7  /* both hfs & isofs filesystem */
#define FS_ISO_9660_INTERACTIVE 8  /* both CD-RTOS and isofs filesystem */
#define FS_3DO					9
#define FS_UDFX					10
#define FS_UDF					11
#define FS_UNKNOWN				15
#define FS_MASK					15

#define XA						16
#define MULTISESSION			32
#define PHOTO_CD				64
#define HIDDEN_TRACK			128
#define CDTV					256
#define BOOTABLE				512
#define VIDEOCDI				1024
#define ROCKRIDGE				2048
#define JOLIET					4096
#define CVD						8192   /* Choiji Video CD */

#define IS_ISOFS				0
#define IS_CD_I					1
#define IS_CDTV					2
#define IS_CD_RTOS				3
#define IS_HS					4
#define IS_BRIDGE				5
#define IS_XA					6
#define IS_PHOTO_CD				7
#define IS_EXT2					8
#define IS_UFS					9
#define IS_BOOTABLE				10
#define IS_VIDEO_CD				11 /* Video CD */
#define IS_CVD					12 /* Chinese Video CD - slightly incompatible with SVCD */
#define IS_UDFX					13
#define IS_UDF					14

	typedef struct signature
	{
		unsigned int buf_num;
		unsigned int offset;
		char *sig_str;
		char *description;
	} signature_t;

	typedef struct TRACKINFO
	{
		int nfsInfo;		//	Information of the Tracks Filesystem
		int nJolietLevel;	//	Jouliet Level
		int ms_offset;		//	Multisession Offset
		int isofs_size;		//	Size of the ISO9660 Filesystem
		int nFrames;		//	Can be used for cddb query 
		int nMins;			//	minutes playtime part of Track
		int nSecs;			//	seconds playtime part of Track
	} trackinfo;


	class CCdInfo
	{
	public:
		CCdInfo() { m_bHasCDDBInfo=true; ZeroMemory( (void*)&m_ti, sizeof( m_ti ) ); m_nLenght = m_nFirstTrack = m_nNumTrack = m_nNumAudio = m_nFirstAudio = m_nNumData = m_nFirstData = 0; }
		virtual ~CCdInfo() {}
		trackinfo GetTrackInformation( int nTrack ) { return m_ti[nTrack-1]; }
		bool HasDataTracks() { return (m_nNumData > 0); }
		bool HasAudioTracks() { return (m_nNumAudio > 0); }
		int GetFirstTrack() { return m_nFirstTrack; }
		int GetTrackCount() { return m_nNumTrack; }
		int GetFirstAudioTrack() { return m_nFirstAudio; }
		int GetFirstDataTrack() { return m_nFirstData; }
		int GetDataTrackCount() { return m_nNumData; }
		int GetAudioTrackCount() { return m_nNumAudio; }
		ULONG GetCddbDiscId() { return m_ulCddbDiscId; }
		int GetDiscLength() { return m_nLenght; }

		//	CD-ROM with ISO 9660 filesystem
		bool IsIso9660( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_9660); }
		//	CD-ROM with joliet extension
		bool IsJoliet( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & JOLIET) ? false : true; }
		//	Joliet extension level
		int GetJolietLevel( int nTrack ) { return m_ti[nTrack - 1].nJolietLevel; }
		//	ISO filesystem size
		int GetIsoSize( int nTrack ) { return m_ti[nTrack - 1].isofs_size; }
		//	CD-ROM with rockridge extensions
		bool IsRockridge( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & ROCKRIDGE) ? false : true; }

		//	CD-ROM with CD-RTOS and ISO 9660 filesystem
		bool IsIso9660Interactive( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_9660_INTERACTIVE); }

		//	CD-ROM with High Sierra filesystem
		bool IsHighSierra( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_HIGH_SIERRA); }

		//	CD-Interactive, with audiotracks > 0 CD-Interactive/Ready
		bool IsCDInteractive( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_INTERACTIVE); }

		//	CD-ROM with Macintosh HFS
		bool IsHFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_HFS); }

		//	CD-ROM with both Macintosh HFS and ISO 9660 filesystem
		bool IsISOHFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_HFS); }

		//	CD-ROM with Unix UFS
		bool IsUFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UFS); }

		//	CD-ROM with Linux second extended filesystem
		bool IsEXT2( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_EXT2); }

		//	CD-ROM with Panasonic 3DO filesystem
		bool Is3DO( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_3DO); }

		//	CD-ROM with XBOX UDFX filesystem
		bool IsUDFX( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UDFX); }

		//	Mixed Mode CD-ROM
		bool IsMixedMode( int nTrack ) { return (m_nFirstData == 1 && m_nNumAudio > 0); }

		//	CD-ROM with XA sectors
		bool IsXA( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & XA) ? false : true; }

		//	Multisession CD-ROM
		bool IsMultiSession( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & MULTISESSION) ? false : true; }
		//	Gets multisession offset
		int GetMultisessionOffset( int nTrack ) { return m_ti[nTrack - 1].ms_offset; }

		//	Hidden Track on Audio CD
		bool IsHiddenTrack( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & HIDDEN_TRACK) ? false : true; }

		//	Photo CD, with audiotracks > 0 Portfolio Photo CD
		bool IsPhotoCd( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & PHOTO_CD) ? false : true; }

		//	CD-ROM with Commodore CDTV
		bool IsCdTv( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & CDTV) ? false : true; }

		//	CD-Plus/Extra
		bool IsCDExtra( int nTrack ) { return (m_nFirstData > 1); }

		//	Bootable CD
		bool IsBootable( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & BOOTABLE) ? false : true; }

		//	Video CD
		bool IsVideoCd( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & VIDEOCDI && m_nNumAudio == 0); }

		//	Chaoji Video CD
		bool IsChaojiVideoCD( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & CVD) ? false : true; }

		//	Audio Track
		bool IsAudio( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_NO_DATA); }

		//	UDF filesystem
		bool IsUDF( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UDF); }

		void SetFirstTrack( int nTrack ) { m_nFirstTrack = nTrack; }
		void SetTrackCount( int nCount ) { m_nNumTrack = nCount; }
		void SetFirstAudioTrack( int nTrack ) { m_nFirstAudio = nTrack; }
		void SetFirstDataTrack( int nTrack ) { m_nFirstData = nTrack; }
		void SetDataTrackCount( int nCount ) { m_nNumData = nCount; }
		void SetAudioTrackCount( int nCount ) { m_nNumAudio = nCount; }
		void SetTrackInformation( int nTrack, trackinfo nInfo ) { if ( nTrack > 0 && nTrack <= 99  ) m_ti[nTrack-1] = nInfo; }

		void SetCddbDiscId( ULONG ulCddbDiscId ) { m_ulCddbDiscId = ulCddbDiscId; }
		void SetDiscLength( int nLenght ) { m_nLenght = nLenght; }
		bool HasCDDBInfo() { return m_bHasCDDBInfo; }
		void SetNoCDDBInfo() { m_bHasCDDBInfo=false; }

	private:
		int m_nFirstData;        /* # of first data track */
		int m_nNumData;               /* # of data tracks */
		int m_nFirstAudio;      /* # of first audio track */
		int m_nNumAudio;             /* # of audio tracks */
		int m_nNumTrack;
		int m_nFirstTrack;
		trackinfo m_ti[100];
		ULONG m_ulCddbDiscId;
		int m_nLenght;			//	Disclenght can be used for cddb query, also see trackinfo.nFrames
		bool m_bHasCDDBInfo;
	};

	class CCdIoSupport
	{
	public:
		CCdIoSupport();
		virtual ~CCdIoSupport();

		HRESULT EjectTray();
		HRESULT CloseTray();

		HANDLE	OpenCDROM();
		HANDLE  OpenIMAGE( CStdString& strFilename );
		INT		ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
		INT 	ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
		INT 	ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
		VOID	CloseCDROM(HANDLE hDevice);

#ifdef _DEBUG
		void	PrintAnalysis(int fs, int num_audio);
#endif
		CCdInfo*		GetCdInfo();
	protected:
		int		ReadBlock(int superblock, uint32_t offset, uint8_t bufnum, track_t track_num);
		bool	IsIt(int num);
		int		IsHFS(void);
		int		Is3DO(void);
		int		IsJoliet(void);
		int		IsUDF(void);
		int		GetSize(void);
		int		GetJolietLevel( void );
		int		GuessFilesystem(int start_session, track_t track_num);

		ULONG	CddbDiscId();
		int		CddbDecDigitSum(int n);
		UINT	MsfSeconds(msf_t *msf);

	private:

		char buffer[7][CDIO_CD_FRAMESIZE_RAW];  /* for CD-Data */
		static signature_t sigs[17];
		int i,j;                                                           /* index */
		int m_nStartTrack;                                   /* first sector of track */
		int m_nIsofsSize;                                      /* size of session */
		int m_nJolietLevel;
		int m_nMsOffset;                /* multisession offset found by track-walking */
		int m_nDataStart;                                       /* start of data area */
		int m_nFs;

		CdIo* cdio;
		track_t m_nNumTracks;
		track_t m_nFirstTrackNum;

		int                        m_nFirstData;        /* # of first data track */
		int                        m_nNumData;                /* # of data tracks */
		int                        m_nFirstAudio;      /* # of first audio track */
		int                        m_nNumAudio;              /* # of audio tracks */
	};

}