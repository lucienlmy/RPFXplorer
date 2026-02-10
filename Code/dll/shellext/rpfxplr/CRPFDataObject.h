#pragma once

#include "RPFReader.h"
#include "RPFPIDL.h"

class ATL_NO_VTABLE CRPFDataObject :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IDataObject
{
private:
	RPFReader* m_pReader = NULL;
	std::vector<RPFEntry> m_vEntries;
	ATL::CString m_strTempPath;

public:
	CRPFDataObject()
	{
	}

	~CRPFDataObject()
	{
		// Clean up temporary files
		CleanupTempFiles();
	}

	HRESULT Initialize(RPFReader* pReader, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
	{
		if (!pReader || cidl == 0 || !apidl)
			return E_INVALIDARG;

		m_pReader = pReader;

		// Extract entries from PIDLs
		for (UINT i = 0; i < cidl; i++)
		{
			const RPFPidlData* pData = _PidlToRPFEntry(apidl[i]);
			if (pData)
			{
				m_vEntries.push_back(pData->rpfEntry);
			}
		}

		if (m_vEntries.empty())
			return E_FAIL;

		return S_OK;
	}

	//
	// IDataObject
	//
	STDMETHOD(GetData)(_In_ FORMATETC* pformatetcIn, _Out_ STGMEDIUM* pmedium) override;
	STDMETHOD(GetDataHere)(_In_ FORMATETC* pformatetc, _Inout_ STGMEDIUM* pmedium) override;
	STDMETHOD(QueryGetData)(_In_opt_ FORMATETC* pformatetc) override;
	STDMETHOD(GetCanonicalFormatEtc)(_In_opt_ FORMATETC* pformatectIn, _Out_ FORMATETC* pformatetcOut) override;
	STDMETHOD(SetData)(_In_ FORMATETC* pformatetc, _In_ STGMEDIUM* pmedium, _In_ BOOL fRelease) override;
	STDMETHOD(EnumFormatEtc)(_In_ DWORD dwDirection, _Out_opt_ IEnumFORMATETC** ppenumFormatEtc) override;
	STDMETHOD(DAdvise)(_In_ FORMATETC* pformatetc, _In_ DWORD advf, _In_opt_ IAdviseSink* pAdvSink, _Out_ DWORD* pdwConnection) override;
	STDMETHOD(DUnadvise)(_In_ DWORD dwConnection) override;
	STDMETHOD(EnumDAdvise)(_Out_opt_ IEnumSTATDATA** ppenumAdvise) override;

private:
	HRESULT CreateTempFiles();
	void CleanupTempFiles();
	ATL::CString GetTempPath();

public:
	DECLARE_NOT_AGGREGATABLE(CRPFDataObject)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CRPFDataObject)
		COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
	END_COM_MAP()
};
