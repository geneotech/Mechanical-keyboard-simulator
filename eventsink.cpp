// EventSink.cpp
#include "eventsink.h"
#include <sstream>
#include <string>
#include "augs/log.h"

ULONG EventSink::AddRef()
{
	return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
	LONG lRef = InterlockedDecrement(&m_lRef);
	if(lRef == 0)
		delete this;
	return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
	{
		*ppv = (IWbemObjectSink *) this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else return E_NOINTERFACE;
}

#include <thread>
#include "augs/misc/typesafe_sscanf.h"

extern EventSink* pSink = nullptr;

VOID CALLBACK WaitOrTimerCallback(
	_In_  PVOID lpParameter,
	_In_  BOOLEAN TimerOrWaitFired
)
{
	std::size_t index = reinterpret_cast<std::size_t>(lpParameter);
	auto& procentry = pSink->process_blacklist[index];

	procentry.is_on = false;

	// LOG(procentry.name);
	return;
}

HRESULT EventSink::Indicate(long lObjectCount,
	IWbemClassObject **apObjArray)
{
	HRESULT hr = S_OK;
	_variant_t vtProp;

	for (int i = 0; i < lObjectCount; i++)
	{
		std::wstring procname;
		DWORD handleid;

		hr = apObjArray[i]->Get(_bstr_t(L"TargetInstance"), 0, &vtProp, 0, 0);
		if (!FAILED(hr))
		{
			IUnknown* str = vtProp;
			hr = str->QueryInterface(IID_IWbemClassObject, reinterpret_cast< void** >(&apObjArray[i]));
			if (SUCCEEDED(hr))
			{
				_variant_t cn;
				hr = apObjArray[i]->Get(L"Caption", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY)) {
						//	wwcout << "Caption : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << std::endl;
					}
					else
						if ((cn.vt & VT_ARRAY)) {
							//	wwcout << "Caption : " << "Array types not supported (yet)" << endl;
						}
						else{
							procname = cn.bstrVal;
						}
				}
				VariantClear(&cn);

				hr = apObjArray[i]->Get(L"CommandLine", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					//if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY)) {
					//	//wwcout << "CommandLine : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
					//}
					//else
					//	if ((cn.vt & VT_ARRAY))
					//		wwcout << "CommandLine : " << "Array types not supported (yet)" << endl;
					//	else
					//		wwcout << "CommandLine : " << cn.bstrVal << endl;
				}
				VariantClear(&cn);

				hr = apObjArray[i]->Get(L"Handle", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY)) {
						//wwcout << "Handle : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
					}
					else
						if ((cn.vt & VT_ARRAY)) {
							//wwcout << "Handle : " << "Array types not supported (yet)" << endl;
						}
						else
						{
							typesafe_sscanf(std::wstring(cn.bstrVal), L"%x", handleid);
						}
				}
				VariantClear(&cn);


			}
		}
		VariantClear(&vtProp);


		std::string procnamestr(procname.begin(), procname.end());
		
		if(procnamestr.empty()){
			continue;
		}
		//LOG_NVPS(procnamestr);
		
		const auto it = std::find(process_blacklist.begin(), process_blacklist.end(), procnamestr);

		if(it != process_blacklist.end()) {
			HANDLE hProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, handleid);
			//LOG_NVPS(procnamestr, handleid);

			std::size_t index = it - process_blacklist.begin();
			process_blacklist[index].is_on = true;
			static_assert(sizeof(void*) == sizeof(index), "wrong");

			HANDLE hNewHandle;
			RegisterWaitForSingleObject(&hNewHandle, hProcHandle , WaitOrTimerCallback, reinterpret_cast<void*>(index), INFINITE, WT_EXECUTEONLYONCE);
		}
	}

	return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
	/* [in] */ LONG lFlags,
	/* [in] */ HRESULT hResult,
	/* [in] */ BSTR strParam,
	/* [in] */ IWbemClassObject __RPC_FAR *pObjParam
)
{
	if(lFlags == WBEM_STATUS_COMPLETE)
	{
		printf("Call complete. hResult = 0x%X\n", hResult);
	}
	else if(lFlags == WBEM_STATUS_PROGRESS)
	{
		printf("Call in progress.\n");
	}

	return WBEM_S_NO_ERROR;
}    // end of EventSink.cpp

