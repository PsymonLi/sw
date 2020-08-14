// concatenated string versions
#define xstr(s) str(s)
#define str(s) #s

#define PENUTIL_VERSION_EXTENSION		xstr(PENUTIL_VERSIONINFO_EXTENSION)
#define PENUTIL_PRODUCT_NAME			"Penutil SPP library DLL " 
#define PENUTIL_NAME					"penutil_windll" // xstr(PENUTIL_NAME)
#define PENUTIL_VERSION_STRING			xstr(PENUTIL_MAJOR_VERSION) "." xstr(PENUTIL_MINOR_VERSION) "." xstr(PENUTIL_SP_VERSION) "." xstr(PENUTIL_BUILD_VERSION)
#define PENUTIL_PRODVERSION_STRING		xstr(PENUTIL_MAJOR_VERSION) "." xstr(PENUTIL_MINOR_VERSION) "." xstr(PENUTIL_SP_VERSION)

#define PENUTIL_VENDOR_VERSION          ((PENUTIL_MAJOR_VERSION << 16) | PENUTIL_MINOR_VERSION)

// Version Info structure for holding global versioning data
#define REVISION_MAX_STR_SIZE	64
// global VersionInfo structure
typedef struct _PENUTIL_VERSION_INFO {
	USHORT				VerMaj;   				// version: major
	USHORT				VerMin;   				// version: minor
	USHORT				VerSP;    				// version: SP
	ULONG				VerBuild; 				// version: build
	CHAR				VerString[REVISION_MAX_STR_SIZE];	// version string
} PENUTIL_VERSION_INFO, * PPENUTIL_VERSION_INFO;
