// EventSink.h
#pragma once

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <vector>
#include <string>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

class EventSink : public IWbemObjectSink
{
	LONG m_lRef;
	bool bDone;

public:
	struct procentry {
		std::string name;
		bool is_on = false;

		bool operator==(const std::string& b) const {
			return name == b;
		}
	};


	std::vector<procentry> process_blacklist;

	bool is_any_on() const {
		for(const auto& b : process_blacklist) {
			if(b.is_on) {
				return true;
			}
		}

		return false;
	}

	EventSink() { m_lRef = 0; }
	~EventSink() { bDone = true; }

	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();        
	virtual HRESULT 
		STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

	virtual HRESULT STDMETHODCALLTYPE Indicate( 
		LONG lObjectCount,
		IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
	);

	virtual HRESULT STDMETHODCALLTYPE SetStatus( 
		/* [in] */ LONG lFlags,
		/* [in] */ HRESULT hResult,
		/* [in] */ BSTR strParam,
		/* [in] */ IWbemClassObject __RPC_FAR *pObjParam
	);
};

extern EventSink* pSink;

int makeeventsink(std::vector<std::string> blacklist_processes);
