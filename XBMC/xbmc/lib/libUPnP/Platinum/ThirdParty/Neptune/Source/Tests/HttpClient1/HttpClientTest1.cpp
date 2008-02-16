/*****************************************************************
|
|      HTTP Client Test Program 1
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "NptDebug.h"

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifdef __GCCE__
#include <staticlibinit_gcce.h>  
#endif

#define TEST_URL
#if defined(TEST_URL)
/*----------------------------------------------------------------------
|       TestUrlParser
+---------------------------------------------------------------------*/
static void
TestUrlParser(const char* url)
{
    NPT_HttpUrl url_object(url);
    NPT_Debug("Parsing URL: '%s'\n", url ? url : "null");
    if (url_object.IsValid()) {
        NPT_Debug("  --> host=%s, port=%d, path=%s\n", 
            (const char*)url_object.GetHost(),
            url_object.GetPort(),
            (const char*)url_object.GetPath());
    } else {
        NPT_Debug("  --> Invalid URL\n");
    }
}
#endif

/*----------------------------------------------------------------------
|       ShowResponse
+---------------------------------------------------------------------*/
static void
ShowResponse(NPT_HttpResponse* response)
{
    // show response info
    NPT_Debug("RESPONSE: protocol=%s, code=%d, reason=%s\n",
              response->GetProtocol().GetChars(),
              response->GetStatusCode(),
              response->GetReasonPhrase().GetChars());

    // show headers
    NPT_HttpHeaders& headers = response->GetHeaders();
    NPT_List<NPT_HttpHeader*>::Iterator header = headers.GetHeaders().GetFirstItem();
    while (header) {
        NPT_Debug("%s: %s\n", 
          (const char*)(*header)->GetName(),
                  (const char*)(*header)->GetValue());
        ++header;
    }

    // show entity
    NPT_HttpEntity* entity = response->GetEntity();
    if (entity != NULL) {
        NPT_Debug("ENTITY: length=%d, type=%s, encoding=%s\n",
                  entity->GetContentLength(),
                  entity->GetContentType().GetChars(),
                  entity->GetContentEncoding().GetChars());

        NPT_DataBuffer body;
        NPT_Result result =entity->Load(body);
        if (NPT_FAILED(result)) {
            NPT_Debug("ERROR: failed to load entity (%d)\n", result);
        } else {
            NPT_Debug("BODY: loaded %d bytes\n", (int)body.GetDataSize());

            // dump the body
            NPT_OutputStreamReference output;
            NPT_File standard_out(NPT_FILE_STANDARD_OUTPUT);
            standard_out.Open(NPT_FILE_OPEN_MODE_WRITE);
            standard_out.GetOutputStream(output);
            output->Write(body.GetData(), body.GetDataSize());
        }
    }
}

#define TEST_GET
#if defined(TEST_GET)
/*----------------------------------------------------------------------
|       TestHttpGet
+---------------------------------------------------------------------*/
static void 
TestHttpGet(const char* arg)
{
    NPT_HttpUrl url(arg);
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    NPT_HttpClient client;
    NPT_HttpResponse* response;

    NPT_Result result = client.SendRequest(request, response);
    NPT_Debug("SendRequest returned %d\n", result);
    if (NPT_FAILED(result)) return;

    ShowResponse(response);

    delete response;
}
#endif

//#define TEST_PROXY
#if defined(TEST_PROXY)
/*----------------------------------------------------------------------
|       TestHttpGetWithProxy
+---------------------------------------------------------------------*/
static void 
TestHttpGetWithProxy(const char* arg)
{
    NPT_HttpUrl url(arg);
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    NPT_HttpClient client;
    NPT_HttpResponse* response;

    client.SetProxy("proxy", 8080);
    NPT_Result result = client.SendRequest(request, response);
    NPT_Debug("SendRequest returned %d\n", result);
    if (NPT_FAILED(result)) return;

    ShowResponse(response);

    delete response;
}
#endif

#if defined(TEST_POST)
/*----------------------------------------------------------------------
|       TestHttpPost
+---------------------------------------------------------------------*/
static void 
TestHttpPost(const char* arg)
{
    NPT_HttpUrl url(arg);
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_POST);
    NPT_HttpClient client;
    NPT_HttpResponse* response;

    NPT_HttpEntity* body_entity = new NPT_HttpEntity();
    NPT_InputStreamReference body_stream(new NPT_MemoryStream((void*)"hello blabla", 12));
    body_entity->SetInputStream(body_stream);

    request.SetEntity(body_entity);
    NPT_Result result = client.SendRequest(request, response);
    NPT_Debug("SendRequest returned %d\n", result);
    if (NPT_FAILED(result)) return;

    ShowResponse(response);

    delete response;
}
#endif

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    // setup debugging
#if defined(WIN32) && defined(_DEBUG)
    int flags = _crtDbgFlag       | 
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF;

    _CrtSetDbgFlag(flags);
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
#endif 

    // check args
#if 1
    if (argc != 2) {
        NPT_Debug("HttpClient: missing URL argument\n");
        return -1;
    }
#endif

#if defined(TEST_URL)
    // test URL parsing
    TestUrlParser(NULL);
    TestUrlParser("");
    TestUrlParser("http");
    TestUrlParser("http:/");
    TestUrlParser("http://");
    TestUrlParser("http://foo");
    TestUrlParser("http://foo.bar");
    TestUrlParser("http://foo.bar:");
    TestUrlParser("http://foo.bar:1");
    TestUrlParser("http://foo.bar:176");
    TestUrlParser("http://foo.bar:176a");
    TestUrlParser("http://foo.bar:176/");
    TestUrlParser("http://foo.bar:176/blabla");
    TestUrlParser("http://foo.bar:176/blabla/blibli");
    TestUrlParser("http://foo.bar:176/blabla/blibli/");
    TestUrlParser("http://foo.bar/");
    TestUrlParser("http://foo.bar/blabla");
#endif

#if defined(TEST_PROXY)
    TestHttpGetWithProxy(argv[1]);
#else
    TestHttpGet(argv[1]);
#endif

#if defined(TEST_POST)
    TestHttpPost(argv[1]);
#endif

#if defined(WIN32) && defined(_DEBUG)
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
