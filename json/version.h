#define VERSIONMAJOR 1
#define VERSIONMINOR 00

#define STRINGIZE_(_s) #_s
#define WSTRINGIZE_(_s) L#_s
#define STRINGIZE(_s) STRINGIZE_(_s)
#define WSTRINGIZE(_s) WSTRINGIZE_(_s)
#define VERSION STRINGIZE(VERSIONMAJOR) "." STRINGIZE(VERSIONMINOR)
#define WVERSION WSTRINGIZE(VERSIONMAJOR) L"." WSTRINGIZE(VERSIONMINOR)

#ifdef UNICODE
#define TVERSION WVERSION
#else
#define TVERSION VERSION
#endif
