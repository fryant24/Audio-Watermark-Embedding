//
// MATLAB Compiler: 6.1 (R2015b)
// Date: Tue Oct 16 17:08:49 2018
// Arguments: "-B" "macro_default" "-W" "cpplib:embed" "-T" "link:lib"
// "embed.m" 
//

#include <stdio.h>
#define EXPORTING_embed 1
#include "embed.h"

static HMCRINSTANCE _mcr_inst = NULL;


#if defined( _MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__LCC__)
#ifdef __LCC__
#undef EXTERN_C
#endif
#include <windows.h>

static char path_to_dll[_MAX_PATH];

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, void *pv)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (GetModuleFileName(hInstance, path_to_dll, _MAX_PATH) == 0)
            return FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultPrintHandler(const char *s)
{
  return mclWrite(1 /* stdout */, s, sizeof(char)*strlen(s));
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultErrorHandler(const char *s)
{
  int written = 0;
  size_t len = 0;
  len = strlen(s);
  written = mclWrite(2 /* stderr */, s, sizeof(char)*len);
  if (len > 0 && s[ len-1 ] != '\n')
    written += mclWrite(2 /* stderr */, "\n", sizeof(char));
  return written;
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_embed_C_API
#define LIB_embed_C_API /* No special import/export declaration */
#endif

LIB_embed_C_API 
bool MW_CALL_CONV embedInitializeWithHandlers(
    mclOutputHandlerFcn error_handler,
    mclOutputHandlerFcn print_handler)
{
    int bResult = 0;
  if (_mcr_inst != NULL)
    return true;
  if (!mclmcrInitialize())
    return false;
  if (!GetModuleFileName(GetModuleHandle("embed"), path_to_dll, _MAX_PATH))
    return false;
    {
        mclCtfStream ctfStream = 
            mclGetEmbeddedCtfStream(path_to_dll);
        if (ctfStream) {
            bResult = mclInitializeComponentInstanceEmbedded(   &_mcr_inst,
                                                                error_handler, 
                                                                print_handler,
                                                                ctfStream);
            mclDestroyStream(ctfStream);
        } else {
            bResult = 0;
        }
    }  
    if (!bResult)
    return false;
  return true;
}

LIB_embed_C_API 
bool MW_CALL_CONV embedInitialize(void)
{
  return embedInitializeWithHandlers(mclDefaultErrorHandler, mclDefaultPrintHandler);
}

LIB_embed_C_API 
void MW_CALL_CONV embedTerminate(void)
{
  if (_mcr_inst != NULL)
    mclTerminateInstance(&_mcr_inst);
}

LIB_embed_C_API 
void MW_CALL_CONV embedPrintStackTrace(void) 
{
  char** stackTrace;
  int stackDepth = mclGetStackTrace(&stackTrace);
  int i;
  for(i=0; i<stackDepth; i++)
  {
    mclWrite(2 /* stderr */, stackTrace[i], sizeof(char)*strlen(stackTrace[i]));
    mclWrite(2 /* stderr */, "\n", sizeof(char)*strlen("\n"));
  }
  mclFreeStackTrace(&stackTrace, stackDepth);
}


LIB_embed_C_API 
bool MW_CALL_CONV mlxEmbed(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "embed", nlhs, plhs, nrhs, prhs);
}

LIB_embed_CPP_API 
void MW_CALL_CONV embed(const mwArray& audio_filename, const mwArray& watermark_filename)
{
  mclcppMlfFeval(_mcr_inst, "embed", 0, 0, 2, &audio_filename, &watermark_filename);
}

