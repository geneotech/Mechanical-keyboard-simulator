
#include "eventsink.h"

int makeeventsink(std::vector<std::string> blacklist_processes)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x" 
			<< hex << hres << endl;
		return 1;                  // Program has failed.
	}

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres =  CoInitializeSecurity(
		NULL, 
		-1,                          // COM negotiates service
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);


	if (FAILED(hres))
	{
		cout << "Failed to initialize security. Error code = 0x" 
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                      // Program has failed.
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,             
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator, (LPVOID *) &pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object. "
			<< "Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Step 4: ---------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices *pSvc = NULL;

	// Connect to the local root\cimv2 namespace
	// and obtain pointer pSvc to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), 
		NULL,
		NULL, 
		0, 
		NULL, 
		0, 
		0, 
		&pSvc
	);

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x" 
			<< hex << hres << endl;
		pLoc->Release();     
		CoUninitialize();
		return 1;                // Program has failed.
	}

	cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket. Error code = 0x" 
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();     
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 6: -------------------------------------------------
	// Receive event notifications -----------------------------

	// Use an unsecured apartment for security
	IUnsecuredApartment* pUnsecApp = NULL;

	hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, 
		CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, 
		(void**)&pUnsecApp);

	pSink = new EventSink;
	
	for(const auto& p : blacklist_processes) {
		pSink->process_blacklist.push_back({ p, false });
	}
	
	pSink->AddRef();


	IUnknown* pStubUnk = NULL; 
	pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

	IWbemObjectSink* pStubSink = NULL;
	pStubUnk->QueryInterface(IID_IWbemObjectSink,
		(void **) &pStubSink);

	// The ExecNotificationQueryAsync method will call
	// The EventQuery::Indicate method when an event occurs
	hres = pSvc->ExecNotificationQueryAsync(
		_bstr_t("WQL"), 
		_bstr_t("SELECT * " 
			"FROM __InstanceCreationEvent WITHIN 1 "
			"WHERE TargetInstance ISA 'Win32_Process'"), 
		WBEM_FLAG_SEND_STATUS, 
		NULL, 
		pStubSink);

	// Check for errors.
	if (FAILED(hres))
	{
		printf("ExecNotificationQueryAsync failed "
			"with = 0x%X\n", hres);
		pSvc->Release();
		pLoc->Release();
		pUnsecApp->Release();
		pStubUnk->Release();
		pSink->Release();
		pStubSink->Release();
		CoUninitialize();    
		return 1;
	}

	//// Wait for the event
	//Sleep(10000);
	//
	//hres = pSvc->CancelAsyncCall(pStubSink);
	//
	//// Cleanup
	//// ========
	//
	//pSvc->Release();
	//pLoc->Release();
	//pUnsecApp->Release();
	//pStubUnk->Release();
	//pSink->Release();
	//pStubSink->Release();
	//CoUninitialize();

	return 0;   // Program successfully completed.

}