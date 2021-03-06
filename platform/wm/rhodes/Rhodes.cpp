/*------------------------------------------------------------------------
* (The MIT License)
* 
* Copyright (c) 2008-2011 Rhomobile, Inc.
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
* http://rhomobile.com
*------------------------------------------------------------------------*/

#include "stdafx.h"
#include "MainWindow.h"
#include "IEBrowserEngine.h"
#include "CEBrowserEngine.h"
#include "LogMemory.h"

#include "common/RhodesApp.h"
#include "common/StringConverter.h"
#include "common/rhoparams.h"
#include "rho/rubyext/GeoLocationImpl.h"
#include "ruby/ext/rho/rhoruby.h"
#include "net/NetRequestImpl.h"
#include "sync/RhoconnectClientManager.h"
#include "common/RhoFilePath.h"
#include "common/app_build_capabilities.h"
#include "common/app_build_configs.h"

using namespace rho;
using namespace rho::common;
using namespace std;
using namespace stdext;

#ifndef RUBY_RUBY_H
typedef unsigned long VALUE;
#endif //!RUBY_RUBY_H

LPTSTR parseToken(LPCTSTR start, LPCTSTR* next_token);
extern "C" void rho_ringtone_manager_stop();
extern "C" void rho_sysimpl_sethas_network(int nValue);
extern "C" void rho_sysimpl_sethas_cellnetwork(int nValue);
extern "C" HINSTANCE rho_wmimpl_get_appinstance();
extern "C" int rho_sys_check_rollback_bundle(const char* szRhoPath);
extern "C" void registerRhoExtension();
extern "C" void rho_webview_navigate(const char* url, int index);
extern "C" void createPowerManagementThread();
static void rho_platform_check_restart_application();

#ifdef APP_BUILD_CAPABILITY_WEBKIT_BROWSER
class CEng;
extern rho::IBrowserEngine* rho_wmimpl_get_webkitBrowserEngine(HWND hwndParent, HINSTANCE rhoAppInstance);
extern "C" CEng* rho_wmimpl_get_webkitbrowser(HWND hParentWnd, HINSTANCE hInstance);

#if !defined(APP_BUILD_CAPABILITY_MOTOROLA)
extern "C" LRESULT	rho_wm_appmanager_ProcessOnTopMostWnd(WPARAM wParam, LPARAM lParam){ return 0;}
#endif

#else
/*extern "C" void rho_wm_impl_SetApplicationLicenseObj(void* pAppLicenseObj)
{
    if (pAppLicenseObj)
        delete pAppLicenseObj;
}*/
#endif // APP_BUILD_CAPABILITY_WEBKIT_BROWSER
#ifdef APP_BUILD_CAPABILITY_SHARED_RUNTIME
extern "C" {
	void rho_wmimpl_set_configfilepath(const char* path);
	TCHAR* rho_wmimpl_get_startpage();
	void rho_wmimpl_set_startpage(const char* path);
	const char* rho_wmimpl_get_logpath();
	const char* rho_wmimpl_get_logurl();
	bool rho_wmimpl_get_fullscreen();
	void rho_wmimpl_set_is_version2(const char* path);
	bool rho_wmimpl_get_is_version2();
    const wchar_t* rho_wmimpl_sharedconfig_getvalue(const wchar_t* szName);
#if !defined( APP_BUILD_CAPABILITY_WEBKIT_BROWSER ) && !defined(APP_BUILD_CAPABILITY_MOTOROLA)
    bool rho_wmimpl_get_is_version2(){ return 1;}
    void rho_wmimpl_set_is_version2(const char* path){}
    void rho_wmimpl_set_configfilepath(const char* path){}
    void rho_wmimpl_set_configfilepath_wchar(const WCHAR* path){}
    void rho_wmimpl_set_startpage(const char* path){}
    TCHAR* rho_wmimpl_get_startpage(){ return L""; }
    const unsigned int* rho_wmimpl_get_logmemperiod(){ return 0; }
    const unsigned int* rho_wmimpl_get_logmaxsize(){ return 0; }
    const char* rho_wmimpl_get_logurl(){ return ""; }
    bool rho_wmimpl_get_fullscreen(){ return 0; }
    const char* rho_wmimpl_get_logpath(){ return ""; }
    int rho_wmimpl_is_loglevel_enabled(int nLogLevel){ return true; }
	const int* rho_wmimpl_get_loglevel(){ return NULL; }
    const wchar_t* rho_wmimpl_sharedconfig_getvalue(const wchar_t* szName){return 0;}
#endif

	const unsigned int* rho_wmimpl_get_logmaxsize();
	const int* rho_wmimpl_get_loglevel();
	const unsigned int* rho_wmimpl_get_logmemperiod();
};
#endif // APP_BUILD_CAPABILITY_SHARED_RUNTIME

#if !defined( APP_BUILD_CAPABILITY_MOTOROLA ) && !defined( APP_BUILD_CAPABILITY_WEBKIT_BROWSER)
extern "C" bool rho_wmimpl_get_resize_on_sip()
{
    return true;
}
#endif

#if defined(_WIN32_WCE) && !defined(OS_PLATFORM_MOTCE)
#include <regext.h>

// Global Notification Handle
HREGNOTIFY g_hNotify = NULL, g_hNotifyCell = NULL;

// ConnectionsNetworkCount
// Gets a value indicating the number of network connections that are currently connected.
#define SN_CONNECTIONSNETWORKCOUNT_ROOT HKEY_LOCAL_MACHINE
#define SN_CONNECTIONSNETWORKCOUNT_PATH TEXT("System\\State\\Hardware")
#define SN_CONNECTIONSNETWORKCOUNT_VALUE TEXT("WiFi")

#define SN_CELLSYSTEMCONNECTED_PATH TEXT("System\\State\\Phone")
#define SN_CELLSYSTEMCONNECTED_VALUE TEXT("Cellular System Connected")

#endif

//This is hack. MC4900 device failed to enable barcode after webkit initialization. So we enable it before.
#if defined(APP_BUILD_CAPABILITY_BARCODE) && defined(APP_BUILD_CAPABILITY_MOTOROLA) && defined (OS_PLATFORM_MOTCE)
extern "C" void rho_scanner_before_webkit();
extern "C" void rho_scanner_after_webkit();
struct CBarcodeInit
{
    bool m_bMC4900;
    CBarcodeInit()
    {
        m_bMC4900 = false;
/*
        OSVERSIONINFO osv = {0};
		osv.dwOSVersionInfoSize = sizeof(osv);
		if (GetVersionEx(&osv))
			m_bMC4900 = osv.dwMajorVersion == 5;

        RAWLOG_INFO1("CBarcodeInit : OS version :  %d", osv.dwMajorVersion);

        if ( m_bMC4900 )
            rho_scanner_before_webkit();*/
    }

    static DWORD afterWebkit(LPVOID ){ rho_scanner_after_webkit(); return 0; }
    ~CBarcodeInit()
    {
        if ( m_bMC4900 )
            CloseHandle(CreateThread(NULL, 0, &afterWebkit, 0, 0, NULL));
    }
};
#else
struct CBarcodeInit{};
#endif

class CRhodesModule : public CAtlExeModuleT< CRhodesModule >
{
    static HINSTANCE m_hInstance;
    CMainWindow m_appWindow;
    rho::String m_strRootPath, m_strRhodesPath, m_logPort, m_strRuntimePath, m_strAppName;
	bool m_bRestarting;
    bool m_bMinimized;
	bool m_isRhoConnectPush;
    bool m_startAtBoot, m_bJSApplication;
    String m_strTabName;

	HANDLE m_hMutex;

public :
    static HINSTANCE GetModuleInstance(){return m_hInstance;}
    static void SetModuleInstance(HINSTANCE hInstance){m_hInstance = hInstance;}
    HWND GetMainWindow() { return m_appWindow.m_hWnd;}
	CMainWindow* GetMainWindowObject() { return &m_appWindow;}
	CMainWindow& GetAppWindow() { return m_appWindow; }
	HWND GetWebViewWindow(int index) { return m_appWindow.getWebViewHWND(); }

    bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode ) throw( );
    HRESULT PreMessageLoop(int nShowCmd) throw();
    void RunMessageLoop( ) throw( );
    const rho::String& getRhoRootPath();
    const rho::String& getRhoRuntimePath();
    const rho::String& getAppName();
    void createAutoStartShortcut();

    bool isJSApplication()const{ return m_bJSApplication; }
};

void parseHttpProxyURI(const rho::String &http_proxy);

static String g_strCmdLine;
HINSTANCE CRhodesModule::m_hInstance;
CRhodesModule _AtlModule;
bool g_restartOnExit = false;

rho::IBrowserEngine* rho_wmimpl_createBrowserEngine(HWND hwndParent)
{
#if defined(APP_BUILD_CAPABILITY_WEBKIT_BROWSER)
    return rho_wmimpl_get_webkitBrowserEngine(hwndParent, rho_wmimpl_get_appinstance());
#elif defined(OS_PLATFORM_MOTCE)
    return new CEBrowserEngine(hwndParent, rho_wmimpl_get_appinstance());
#else
    return new CIEBrowserEngine(hwndParent, rho_wmimpl_get_appinstance());
#endif //APP_BUILD_CAPABILITY_WEBKIT_BROWSER
}

bool CRhodesModule::ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode ) throw()
{
	m_bRestarting      = false;
    m_bMinimized       = false;
    m_startAtBoot      = false;
#ifdef RHO_NO_RUBY
    m_bJSApplication   = true;        
#else
    m_bJSApplication   = false;
#endif

    m_logPort          = "";
    m_isRhoConnectPush = false;
    LPCTSTR lpszToken  = lpCmdLine;
	LPCTSTR nextToken;

    getRhoRootPath();

	while (lpszToken != NULL)
	{
		// skip leading spaces and double-quote (if present)
	    bool doubleQuote = false;
		while ((*lpszToken != 0) && ((*lpszToken==' ') || ((!doubleQuote) && (*lpszToken=='"')))) {
			if (*lpszToken=='"')
				doubleQuote = true;
			lpszToken++;
		}
		// skip leading spaces and check for leading '/' or '-' of command line option
		bool isCmdLineOpt = false;
		while ((*lpszToken != 0) && ((*lpszToken==' ') || ((!isCmdLineOpt) && ((*lpszToken=='/') || (*lpszToken=='-'))))) {
			if ((*lpszToken=='/') || (*lpszToken=='-'))
				isCmdLineOpt = true;
			lpszToken++;
		}
		// finish command line processing on EOL
		if (*lpszToken == 0) break;

		// if option starts with double-quote, find its end by next double-quote;
		// otherwise the end will be found automatically
		nextToken = doubleQuote ? FindOneOf(lpszToken, _T("\"")) : NULL;

		//parseToken will allocate extra byte at the end of the returned token value
		LPTSTR value = parseToken( lpszToken, &nextToken );

		if (isCmdLineOpt) {
			if (WordCmpI(lpszToken, _T("Restarting"))==0) {
				m_bRestarting = true;
			}

            if (wcsncmp(lpszToken, _T("minimized"), 9)==0) {
				m_bMinimized = true;
			}

            if (wcsncmp(lpszToken, _T("tabname"), 7)==0) {
				m_strTabName = convertToStringA(value);
			}

			if (WordCmpI(lpszToken, _T("rhoconnectpush"))==0) {
				m_isRhoConnectPush = true;
			}

			else if (wcsncmp(lpszToken, _T("log"), 3)==0) 
			{
				if (value) {
					m_logPort = convertToStringA(value);
				}
				else {
					m_logPort = rho::String("11000");
				}
			}
            else if (wcsnicmp(lpszToken, _T("approot"),7)==0 || wcsnicmp(lpszToken, _T("jsapproot"),9)==0)
			{
				if (value) 
                {
					m_strRootPath = convertToStringA(value);
					if (m_strRootPath.substr(0,7).compare("file://")==0)
						m_strRootPath.erase(0,7);
					String_replace(m_strRootPath, '\\', '/');
					if (m_strRootPath.at(m_strRootPath.length()-1)!='/')
						m_strRootPath.append("/");

#if defined(OS_WINCE)
					m_strRootPath.append("rho/");
#ifdef APP_BUILD_CAPABILITY_SHARED_RUNTIME
					rho_wmimpl_set_is_version2(m_strRootPath.c_str());
#endif
#endif
        		}

                if ( wcsnicmp(lpszToken, _T("jsapproot"),9)==0 )
                    m_bJSApplication = true;
            }
#if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
			else if (wcsnicmp(lpszToken, _T("s"),1)==0)
			{
				if (value) {
					// RhoElements v1.0 compatibility mode
					String strPath = convertToStringA(value);
					rho_wmimpl_set_startpage(strPath.c_str());
				}
			}
			else if (wcsnicmp(lpszToken, _T("c"),1)==0)
			{
				if (value) {
					String strPath = convertToStringA(value);
					if (strPath.substr(0,7).compare("file://")==0)
						strPath.erase(0,7);
					rho_wmimpl_set_configfilepath(strPath.c_str());
				}
			}
#endif // APP_BUILD_CAPABILITY_SHARED_RUNTIME

		}
		if (value) free(value);
		lpszToken = nextToken;
	}

	return __super::ParseCommandLine(lpCmdLine, pnRetCode);
}

extern "C" void rho_sys_impl_exit_with_errormessage(const char* szTitle, const char* szMsg)
{
    //alert_show_status( "Bundle update.", ("Error happen when replace bundle: " + strError).c_str(), 0 );
    StringW strMsgW, strTitleW;
    convertToStringW(szMsg, strMsgW);
    convertToStringW(szTitle, strTitleW);
    ::MessageBoxW(0, strMsgW.c_str(), strTitleW.c_str(), MB_ICONERROR | MB_OK);
}

extern "C" void rho_scanner_TEST(HWND hwnd);
extern "C" void rho_scanner_TEST2();
extern "C" void rho_wm_impl_CheckLicense();

// This method is called immediately before entering the message loop.
// It contains initialization code for the application.
// Returns:
// S_OK => Success. Continue with RunMessageLoop() and PostMessageLoop().
// S_FALSE => Skip RunMessageLoop(), call PostMessageLoop().
// error code => Failure. Skip both RunMessageLoop() and PostMessageLoop().
HRESULT CRhodesModule::PreMessageLoop(int nShowCmd) throw()
{
    HRESULT hr = __super::PreMessageLoop(nShowCmd);
    if (FAILED(hr))
    {
        return hr;
    }
    // Note: In this sample, we don't respond differently to different hr success codes.

    SetLastError(0);
    HANDLE hEvent = CreateEvent( NULL, false, false, CMainWindow::GetWndClassInfo().m_wc.lpszClassName );

    if ( !m_bRestarting && hEvent != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Rho Running so could bring to foreground
        HWND hWnd = FindWindow(CMainWindow::GetWndClassInfo().m_wc.lpszClassName, NULL);

        if (hWnd)
        {
            ShowWindow(hWnd, SW_SHOW);
            SendMessage( hWnd, PB_WINDOW_RESTORE, NULL, TRUE);
            SetForegroundWindow( hWnd );

            COPYDATASTRUCT cds = {0};
            cds.cbData = m_strTabName.length()+1;
            cds.lpData = (char*)m_strTabName.c_str();
            SendMessage( hWnd, WM_COPYDATA, (WPARAM)WM_WINDOW_SWITCHTAB, (LPARAM)(LPVOID)&cds);
        }

        return S_FALSE;
    }

    if ( !rho_sys_check_rollback_bundle(rho_native_rhopath()) )
    {
        rho_sys_impl_exit_with_errormessage( "Bundle update", "Application is corrupted. Reinstall it, please.");
        return S_FALSE;
    }

#if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
    rho_logconf_Init((rho_wmimpl_get_logpath()[0]==0 ? m_strRootPath.c_str() : rho_wmimpl_get_logpath()), m_strRootPath.c_str(), m_logPort.c_str());
    if (rho_wmimpl_get_logurl()[0]!=0)
        LOGCONF().setLogURL(rho_wmimpl_get_logurl());
    if (rho_wmimpl_get_logmaxsize())
        LOGCONF().setMaxLogFileSize(*rho_wmimpl_get_logmaxsize());
    if (rho_wmimpl_get_loglevel())
        LOGCONF().setMinSeverity(*rho_wmimpl_get_loglevel());
    if (rho_wmimpl_get_fullscreen())
        RHOCONF().setBool("full_screen", true, false);
    if (rho_wmimpl_get_logmemperiod())
        LOGCONF().setCollectMemoryInfoInterval(*rho_wmimpl_get_logmemperiod());
#else
    rho_logconf_Init(m_strRootPath.c_str(), m_strRootPath.c_str(), m_logPort.c_str());
#endif // APP_BUILD_CAPABILITY_SHARED_RUNTIME

    LOGCONF().setMemoryInfoCollector(CLogMemory::getInstance());

    if ( !rho_rhodesapp_canstartapp(g_strCmdLine.c_str(), " /-,") )
    {
		LOG(INFO) + "This is hidden app and can be started only with security key.";
		if (RHOCONF().getString("invalid_security_token_start_path").length() <= 0)
        {
			return S_FALSE;
        }
    }

	LOG(INFO) + "Rhodes started";
	if (RHOCONF().isExist("http_proxy_url")) {
		parseHttpProxyURI(RHOCONF().getString("http_proxy_url"));
	}


	//Check for bundle directory is exists.
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	
	// rootpath + "rho/"
	if (m_strRootPath.at(m_strRootPath.length()-1) == '/') 
    {
		hFind = FindFirstFile(convertToStringW(m_strRootPath.substr(0, m_strRootPath.find_last_of('/'))).c_str(), &wfd);
	} 
    else if (m_strRootPath.at(m_strRootPath.length()-1) == '\\') 
    {
		//delete all '\' from the end of the pathname
		int i = m_strRootPath.length();
		for ( ; i != 1; i--) {
			if (m_strRootPath.at(i-1) != '\\')
				break;
		}

		hFind = FindFirstFile(convertToStringW(m_strRootPath.substr(0, i)).c_str(), &wfd);
	}

	if (INVALID_HANDLE_VALUE == hFind || !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
    {
		int last = 0, pre_last = 0;
		last = getRhoRootPath().find_last_of('\\');
		pre_last = getRhoRootPath().substr(0, last).find_last_of('\\');
		String appName = getRhoRootPath().substr(pre_last + 1, last - pre_last - 1);

		String messageText = "Bundle directory \"" + 
								m_strRootPath.substr(0, m_strRootPath.find_last_of('/')) + 
								"\" is  missing\n";

		LOG(INFO) + messageText;
		int msgboxID = MessageBox(NULL,
									convertToStringW(messageText).c_str(),
									convertToStringW(appName).c_str(),
									MB_ICONERROR | MB_OK);


		return S_FALSE;
    }

    createPowerManagementThread();

    if (RHOCONF().getBool("Application.autoStart"))
        createAutoStartShortcut();

    rho::common::CRhodesApp::Create(m_strRootPath, m_strRootPath, m_strRuntimePath);

    bool bRE1App = false;

#if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
    if (!rho_wmimpl_get_is_version2())
        bRE1App = true;
#endif

    RHODESAPP().setJSApplication(bRE1App || _AtlModule.isJSApplication());

#if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
    if ((!rho_wmimpl_get_is_version2()) && (rho_wmimpl_get_startpage()[0] != 0)) {
        String spath = convertToStringA(rho_wmimpl_get_startpage());
        RHOCONF().setString("start_path", spath, false);
    }
#endif // APP_BUILD_CAPABILITY_SHARED_RUNTIME

    DWORD dwStyle = m_bMinimized ? 0 : WS_VISIBLE;

#ifdef OS_WINCE
    m_appWindow.getTabbar().SetStartTabName(m_strTabName);
#else
    m_appWindow.setStartTabName(m_strTabName);
#endif

#if !defined(_WIN32_WCE)
    dwStyle |= WS_OVERLAPPEDWINDOW;
#endif
    // Create the main application window
    String strTitle = RHODESAPP().getAppTitle();
    m_appWindow.Create(NULL, CWindow::rcDefault, convertToStringW(strTitle).c_str(), dwStyle);

    if (NULL == m_appWindow.m_hWnd)
    {
        return S_FALSE;
    }

    m_appWindow.InvalidateRect(NULL, TRUE);
    m_appWindow.UpdateWindow();

    m_appWindow.initBrowserWindow();

    if (m_bMinimized)
        m_appWindow.ShowWindow(SW_MINIMIZE);

/*
    if (bRE1App)
    {
#if defined(APP_BUILD_CAPABILITY_MOTOROLA)
        registerRhoExtension();
#endif

#if !defined( APP_BUILD_CAPABILITY_WEBKIT_BROWSER ) && defined(OS_WINCE)
	    m_appWindow.Navigate2(_T("about:blank"), -1 );
#endif //!APP_BUILD_CAPABILITY_WEBKIT_BROWSER

        rho_webview_navigate(RHOCONF().getString("start_path").c_str(), 0 );
    }
    else
    { */
        RHODESAPP().startApp();

#if !defined( APP_BUILD_CAPABILITY_WEBKIT_BROWSER ) && defined(OS_WINCE)
        // Navigate to the "loading..." page
	    m_appWindow.Navigate2(_T("about:blank"), -1 );
#endif //APP_BUILD_CAPABILITY_WEBKIT_BROWSER
    //}

#if defined(_WIN32_WCE)&& !defined( OS_PLATFORM_MOTCE )

    DWORD dwConnCount = 0;
    hr = RegistryGetDWORD( SN_CONNECTIONSNETWORKCOUNT_ROOT,
		SN_CONNECTIONSNETWORKCOUNT_PATH, 
		SN_CONNECTIONSNETWORKCOUNT_VALUE, 
        &dwConnCount
    );
    rho_sysimpl_sethas_network((dwConnCount > 1) ? 1 : 0);

    DWORD dwCellConnected = 0;
    hr = RegistryGetDWORD( SN_CONNECTIONSNETWORKCOUNT_ROOT,
		SN_CELLSYSTEMCONNECTED_PATH, 
		SN_CELLSYSTEMCONNECTED_VALUE, 
        &dwCellConnected
    );
    rho_sysimpl_sethas_cellnetwork(dwCellConnected);

	// Register for changes in the number of network connections
	hr = RegistryNotifyWindow(SN_CONNECTIONSNETWORKCOUNT_ROOT,
		SN_CONNECTIONSNETWORKCOUNT_PATH, 
		SN_CONNECTIONSNETWORKCOUNT_VALUE, 
		m_appWindow.m_hWnd, 
		WM_CONNECTIONSNETWORKCOUNT, 
		0, 
		NULL, 
		&g_hNotify);

	hr = RegistryNotifyWindow(SN_CONNECTIONSNETWORKCOUNT_ROOT,
		SN_CELLSYSTEMCONNECTED_PATH, 
		SN_CELLSYSTEMCONNECTED_VALUE, 
		m_appWindow.m_hWnd, 
		WM_CONNECTIONSNETWORKCELL, 
		0, 
		NULL, 
		&g_hNotifyCell);

#endif

    return S_OK;
}

void CRhodesModule::RunMessageLoop( ) throw( )
{
    m_appWindow.getWebKitEngine()->RunMessageLoop(m_appWindow);

#if defined(OS_WINCE)&& !defined( OS_PLATFORM_MOTCE )
    if (g_hNotify)
        RegistryCloseNotification(g_hNotify);

    if ( g_hNotifyCell )
        RegistryCloseNotification(g_hNotifyCell);

    CGPSController* pGPS = CGPSController::Instance();
    pGPS->DeleteInstance();
#endif
    rho_ringtone_manager_stop();

    rho::common::CRhodesApp::Destroy();

//	ReleaseMutex(m_hMutex);

    rho_platform_check_restart_application();
}

const rho::String& CRhodesModule::getRhoRootPath()
{
    if ( m_strRootPath.length() == 0 )
        m_strRootPath = getRhoRuntimePath();

    return m_strRootPath;
}

const rho::String& CRhodesModule::getAppName()
{
    if ( m_strAppName.length() == 0 )
    {
#if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
        bool bRE1App = false;
        if (!rho_wmimpl_get_is_version2())
            bRE1App = true;
        if ( bRE1App )
            m_strAppName = convertToStringA( rho_wmimpl_sharedconfig_getvalue( L"General\\Name" ) );
        else
        {
            String path = getRhoRootPath();
            String_replace(path, '/', '\\');

            int nEnd = path.find_last_of('\\');
            nEnd = path.find_last_of('\\', nEnd-1)-1;

            int nStart = path.find_last_of('\\', nEnd) +1;
            m_strAppName = path.substr( nStart, nEnd-nStart+1);
        }
#else
        m_strAppName = get_app_build_config_item("name");
#endif
    }

    return m_strAppName;
}

const rho::String& CRhodesModule::getRhoRuntimePath()
{
    if ( m_strRuntimePath.length() == 0 )
    {
        char rootpath[MAX_PATH];
        int len;
        if ( (len = GetModuleFileNameA(NULL,rootpath,MAX_PATH)) == 0 )
            strcpy(rootpath,".");
        else
        {
            while( !(rootpath[len] == '\\'  || rootpath[len] == '/') )
              len--;
            rootpath[len+1]=0;
        }

        m_strRuntimePath = rootpath;
        m_strRuntimePath += "rho\\";

        for(unsigned int i = 0; i < m_strRuntimePath.length(); i++ )
            if ( m_strRuntimePath.at(i) == '\\' )
                m_strRuntimePath[i] = '/';
    }

    return m_strRuntimePath; 
}

void CRhodesModule::createAutoStartShortcut()
{
#ifdef OS_WINCE
    StringW strLnk = L"\\Windows\\StartUp\\";
    strLnk += convertToStringW(getAppName());
    strLnk += L".lnk";

    StringW strAppPath = L"\"";
    char rootpath[MAX_PATH];
    GetModuleFileNameA(NULL,rootpath,MAX_PATH);
    strAppPath += convertToStringW(rootpath);
    strAppPath += L"\" -minimized";

    DWORD dwRes = SHCreateShortcut( (LPTSTR)strLnk.c_str(), (LPTSTR)strAppPath.c_str());
#endif

}

extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                                LPTSTR lpCmdLine, int nShowCmd)
{
	INITCOMMONCONTROLSEX ctrl;


	//Required to use datetime picker controls.
	ctrl.dwSize = sizeof(ctrl);
	ctrl.dwICC = ICC_DATE_CLASSES|ICC_BAR_CLASSES;
	InitCommonControlsEx(&ctrl);

    g_strCmdLine = convertToStringA(lpCmdLine);
    _AtlModule.SetModuleInstance(hInstance);

	return _AtlModule.WinMain(nShowCmd);
}

extern "C" void rho_wm_impl_performOnUiThread(rho::common::IRhoRunnable* pTask) 
{
    CMainWindow* mainWnd = _AtlModule.GetMainWindowObject();
    mainWnd->performOnUiThread(pTask);    
}

extern "C" HWND getMainWnd() {
	return _AtlModule.GetMainWindow();
}

CMainWindow& getAppWindow() 
{
	return _AtlModule.GetAppWindow();
}

extern "C" HWND getWebViewWnd(int index) {
	return _AtlModule.GetWebViewWindow(index);
}

CMainWindow* Rhodes_getMainWindow() {
	return _AtlModule.GetMainWindowObject();
}

extern "C" void rho_wmsys_run_app(const char* szPath, const char* szParams );
static bool g_bIsRestartApplication = false;
void rho_platform_restart_application() 
{
    g_bIsRestartApplication = true;
}

static void rho_platform_check_restart_application() 
{
    if (!g_bIsRestartApplication)
        return;

	char module[MAX_PATH];
    ::GetModuleFileNameA(NULL,module,MAX_PATH);
                                       
    rho_wmsys_run_app(module, (g_strCmdLine + " -restarting").c_str());
}

typedef bool (WINAPI *PCSD)();

#ifdef APP_BUILD_CAPABILITY_MOTOROLA
extern "C" void rho_wm_impl_CheckLicenseWithBarcode(HWND hParent, HINSTANCE hLicenseInstance);
#endif
extern "C" void rho_wm_impl_SetApplicationLicenseObj(void* pAppLicenseObj);

typedef LPCWSTR (WINAPI *PCL)(HWND, LPCWSTR, LPCWSTR, LPCWSTR);
typedef int (WINAPI *FUNC_IsLicenseOK)();
typedef void* (WINAPI *FUNC_GetAppLicenseObj)();

extern "C" void rho_wm_impl_CheckLicense()
{   
    int nRes = 0;
    LOG(INFO) + "Start license_rc.dll";
    HINSTANCE hLicenseInstance = LoadLibrary(L"license_rc.dll");
    LOG(INFO) + "Stop license_rc.dll";
    

    if(hLicenseInstance)
    {
        PCL pCheckLicense = (PCL) GetProcAddress(hLicenseInstance, L"CheckLicense");
        FUNC_IsLicenseOK pIsOK = (FUNC_IsLicenseOK) GetProcAddress(hLicenseInstance, L"IsLicenseOK");
        LPCWSTR szLogText = 0;
        if(pCheckLicense)
        {
            StringW strLicenseW;
            common::convertToStringW( get_app_build_config_item("motorola_license"), strLicenseW );

            StringW strCompanyW;
            common::convertToStringW( get_app_build_config_item("motorola_license_company"), strCompanyW );

        #if defined(APP_BUILD_CAPABILITY_SHARED_RUNTIME)
            LPCTSTR szLicense = rho_wmimpl_sharedconfig_getvalue( L"LicenseKey" );
            if ( szLicense )
                strLicenseW = szLicense;

            LPCTSTR szLicenseComp = rho_wmimpl_sharedconfig_getvalue( L"LicenseKeyCompany" );
            if ( szLicenseComp )
                strCompanyW = szLicenseComp;
        #endif

            StringW strAppNameW;
            strAppNameW = RHODESAPP().getAppNameW();
            szLogText = pCheckLicense( getMainWnd(), strAppNameW.c_str(), strLicenseW.c_str(), strCompanyW.c_str() );
        }

        if ( szLogText && *szLogText )
            LOGC(INFO, "License") + szLogText;

        nRes = pIsOK ? pIsOK() : 0;
    }

#ifdef APP_BUILD_CAPABILITY_MOTOROLA
    if ( nRes == 0 )
    {
        rho_wm_impl_CheckLicenseWithBarcode(getMainWnd(),hLicenseInstance);
        return;
    }
#endif

#ifdef APP_BUILD_CAPABILITY_WEBKIT_BROWSER
    if ( nRes )
    {
        FUNC_GetAppLicenseObj pGetAppLicenseObj = (FUNC_GetAppLicenseObj) GetProcAddress(hLicenseInstance, L"GetAppLicenseObj");
        if ( pGetAppLicenseObj )
            rho_wm_impl_SetApplicationLicenseObj( pGetAppLicenseObj() );
    }
#endif

    if ( !nRes )
        ::PostMessage( getMainWnd(), WM_SHOW_LICENSE_WARNING, 0, 0);
}

static inline char *
translate_char(char *p, int from, int to)
{
    while (*p) {
	if ((unsigned char)*p == from)
	    *p = to;
	p = CharNextA(p);
    }
    return p;
}

extern "C" const char* rho_native_get_appname()
{
    return _AtlModule.getAppName().c_str();
}

extern "C" const char* rho_native_rhopath() 
{
    return _AtlModule.getRhoRootPath().c_str();
}

extern "C" const char* rho_native_reruntimepath() 
{
    return _AtlModule.getRhoRuntimePath().c_str();
}

extern "C" HINSTANCE rho_wmimpl_get_appinstance()
{
    return _AtlModule.GetModuleInstance();
}

extern "C" HWND rho_wmimpl_get_mainwnd() 
{
	return _AtlModule.GetMainWindow();
}

extern "C" void rho_conf_show_log()
{
    ::PostMessage(getMainWnd(),WM_COMMAND,IDM_LOG,0);
}

extern "C" void rho_title_change(const int tabIndex, const char* strTitle)
{
    PostMessage( rho_wmimpl_get_mainwnd(),WM_COMMAND, ID_TITLECHANGE, (LPARAM)_tcsdup(convertToStringW(strTitle).c_str()) );
}

extern "C" void rho_win32_unset_window_proxy()
{
}

extern "C" void rho_win32_set_window_proxy(const char* host, const char* port, const char* login, const char* password)
{
}

//Hook for ruby call to refresh web view

extern "C" void rho_net_impl_network_indicator(int active)
{
    //TODO: rho_net_impl_network_indicator
}

extern "C" void rho_map_location(char* query)
{
}

extern "C" void rho_appmanager_load( void* httpContext, char* szQuery)
{
}

extern "C" void Init_fcntl(void)
{
}

//parseToken will allocate extra byte at the end of the 
//returned token value
LPTSTR parseToken (LPCTSTR start, LPCTSTR* next_token) {
    int nNameLen = 0;
    while(*start==' '){ start++; }

    int i = 0;
    while( start[i] != L'\0' )
	{
        if (( start[i] == L'=' ) || ( start[i] == L':' ) || ( start[i] == L' ' )) {
            if ( i > 0 ){
                int s = i-1;
                for(; s >= 0 && start[s]==' '; s-- );
                nNameLen = s+1;
                break;
            }else 
                break;
        }
		++i;
    }

	if ( nNameLen == 0 )
        return NULL;

    LPCTSTR szValue = start + i+1;
    int nValueLen = 0;

	// skip leading spaces and optional delimiter (that is either ' or ") and keep the delimiter for further use
	TCHAR delimiter = L' ';
	while ((*szValue==delimiter) || ((delimiter==L' ') && ((*szValue==L'\'') || (*szValue==L'"')))) {
		if ((delimiter==L' ') && ((*szValue==L'\'') || (*szValue==L'"'))) {
			// delimiter found
			delimiter = *szValue;
			szValue++;
			break;
		}
		szValue++;
	}
	// search for value end -- that is either end of line or delimiter
	while ((szValue[nValueLen] != L'\0') && (szValue[nValueLen] != delimiter)) { nValueLen++; }
	// next token begins with the character next to delimiter (if it's not the end of the line)
	if (*next_token == NULL)
		*next_token = szValue + nValueLen + (szValue[nValueLen] != L'\0' ? 1 : 0 );

	TCHAR* value = (TCHAR*) malloc((nValueLen+2)*sizeof(TCHAR));
	wcsncpy(value, szValue, nValueLen);
	value[nValueLen] = L'\0';

	return value;
}

#if defined( OS_PLATFORM_MOTCE )

#include <Imaging.h>

HBITMAP SHLoadImageFile(  LPCTSTR pszFileName )
{
    if ( !pszFileName || !*pszFileName )
        return 0;

    String strFileName = convertToStringA(pszFileName);
    /*if ( String_endsWith(strFileName, ".bmp") )
    {
        return (HBITMAP)::LoadImage(NULL, pszFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }*/

    if ( !String_endsWith(strFileName, ".png") && !String_endsWith(strFileName, ".bmp") )
        return 0;

	IImagingFactory *pImgFactory = NULL;
    IImage *pImage = NULL;
    //CoInitializeEx(NULL, COINIT_MULTITHREADED);
	HBITMAP hResult = 0;
    if (SUCCEEDED(CoCreateInstance (CLSID_ImagingFactory,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IImagingFactory,
                                    (void **)&pImgFactory)))
    {
		ImageInfo imageInfo;
		if (SUCCEEDED(pImgFactory->CreateImageFromFile(CA2W(strFileName.c_str()), &pImage))
			&& SUCCEEDED(pImage->GetImageInfo(&imageInfo)))
        {
			CWindowDC dc(getMainWnd());
			CDC dcBitmap;
			dcBitmap.CreateCompatibleDC(dc.m_hDC);
			hResult = CreateCompatibleBitmap(dc.m_hDC, imageInfo.Width, imageInfo.Height);
			if (hResult) 
			{
				HBITMAP hOldBitmap = dcBitmap.SelectBitmap(hResult);
                //dcBitmap.FillSolidRect( 0,0, imageInfo.Width, imageInfo.Height, RGB(255,255,255));

                CRect rc(0, 0, imageInfo.Width, imageInfo.Height);
	            COLORREF clrOld = ::SetBkColor(dcBitmap.m_hDC, RGB(255,255,255));
	            ::ExtTextOut(dcBitmap.m_hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	            ::SetBkColor(dcBitmap.m_hDC, clrOld);

				pImage->Draw(dcBitmap.m_hDC, rc, NULL);
				dcBitmap.SelectBitmap(hOldBitmap);
			}
			pImage->Release();
       }
       pImgFactory->Release();
    }
    //CoUninitialize();

	return hResult;
}

#endif

#if !defined(_WIN32_WCE)
#include <gdiplus.h>
#include <Gdiplusinit.h>
using namespace Gdiplus;

#define   SelectBitmap(hdc, hbm)  ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))
HBITMAP SHLoadImageFile(  LPCTSTR pszFileName )
{
    if ( !pszFileName || !*pszFileName )
        return 0;

    String strFileName = convertToStringA(pszFileName);
    if ( String_endsWith(strFileName, ".bmp") )
    {
        return (HBITMAP)::LoadImage(NULL, pszFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }

    if ( !String_endsWith(strFileName, ".png") )
        return 0;

    static bool s_GDIInit = false;
    if ( !s_GDIInit)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        s_GDIInit = true;
    }

    Gdiplus::Image* image = new Gdiplus::Image(convertToStringW(strFileName).c_str());
    SizeF sizePng;
    Status res = image->GetPhysicalDimension(&sizePng);

    HDC hDC = GetDC(getMainWnd());

    HDC hdcMem = CreateCompatibleDC(hDC);
    HBITMAP hBitmap  = ::CreateCompatibleBitmap(hDC, (int)sizePng.Width, (int)sizePng.Height);
    HBITMAP hbmOld = SelectBitmap(hdcMem, hBitmap);

    CRect rc(0,0,(int)sizePng.Width, (int)sizePng.Height);
	COLORREF clrOld = ::SetBkColor(hdcMem, RGB(255,255,255));
	::ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	::SetBkColor(hdcMem, clrOld);

    Gdiplus::Graphics grpx(hdcMem);
    res = grpx.DrawImage(image, 0, 0, (int)sizePng.Width, (int)sizePng.Height);

    SelectBitmap(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    DeleteDC(hDC);

    delete image;
    return hBitmap;
}

#endif
