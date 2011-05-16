
#if _WINDOWS && defined Hugin_shared 

#if defined huginbase_EXPORTS
#define IMPEX __declspec(dllexport)
#else
#define IMPEX __declspec(dllimport)
#endif

#if defined celeste_EXPORTS
#define CELESTEIMPEX __declspec(dllexport)
#else
#define CELESTEIMPEX __declspec(dllimport)
#endif

#if defined makefilelib_EXPORTS
#define MAKEIMPEX __declspec(dllexport)
#else
#define MAKEIMPEX __declspec(dllimport)
#endif

#if defined icpfindlib_EXPORTS
#define ICPIMPEX __declspec(dllexport)
#else
#define ICPIMPEX __declspec(dllimport)
#endif

#if defined huginbasewx_EXPORTS
#define WXIMPEX __declspec(dllexport)
#else
#define WXIMPEX __declspec(dllimport)
#endif

#if defined localfeatures_EXPORTS
#define LFIMPEX __declspec(dllexport)
#else
#define LFIMPEX __declspec(dllimport)
#endif

#if defined huginlines_EXPORTS
#define LINESIMPEX __declspec(dllexport)
#else
#define LINESIMPEX __declspec(dllimport)
#endif

#pragma warning( disable: 4251 )

#else
#define IMPEX
#define WXIMPEX
#define MAKEIMPEX
#define LFIMPEX
#define ICPIMPEX
#define CELESTEIMPEX
#define LINESIMPEX
#endif
