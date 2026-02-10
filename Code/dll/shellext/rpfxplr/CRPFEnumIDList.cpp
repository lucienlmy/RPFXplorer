#include "pch.h"

#include <IRPF.h>
#include "RPFPIDL.h"
#include "CRAGEFolder.h"

class CRPFEnumIDList :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
	IRPF* m_pRPF = NULL;
	RPFReader* m_pReader = NULL;
	std::vector<RPFEntry> m_vEntries;
	size_t m_nCurrentIndex = 0;

public:
	HRESULT Initialize(IRPF* pRPF)
	{
		m_pRPF = pRPF;
		
		// Get the RPF reader
		HRESULT hr = m_pRPF->GetRPFReader(&m_pReader);
		if (FAILED(hr) || !m_pReader)
			return E_FAIL;

		// Get the current directory from the folder
		CRAGEFolder* pFolder = static_cast<CRAGEFolder*>(pRPF);
		RPFEntry* pCurrentDir = pFolder->GetCurrentDirectory();

		if (pCurrentDir)
		{
			// Enumerate subdirectory
			m_vEntries = m_pReader->GetSubEntries(pCurrentDir);
		}
		else
		{
			// Enumerate root directory
			RPFEntry& rootEntry = m_pReader->GetRootEntry();
			m_vEntries = m_pReader->GetSubEntries(&rootEntry);
		}

		m_nCurrentIndex = 0;

		return S_OK;
	}

	//
	// IEnumIDList
	//
	STDMETHODIMP Next(_In_ ULONG celt, _Out_ PITEMID_CHILD* rgelt, _Out_ ULONG* pceltFetched) override
	{
		if (!rgelt)
			return E_POINTER;

		if (!m_pReader)
			return E_FAIL;

		ULONG fetched = 0;

		// Enumerate up to celt items
		while (fetched < celt && m_nCurrentIndex < m_vEntries.size())
		{
			const RPFEntry& entry = m_vEntries[m_nCurrentIndex];
			
			// Create a PIDL for this entry
			LPITEMIDLIST pidl = _CreateRPFPIDL(entry, m_pReader);
			if (pidl)
			{
				rgelt[fetched] = pidl;
				fetched++;
			}

			m_nCurrentIndex++;
		}

		if (pceltFetched)
			*pceltFetched = fetched;

		// Return S_OK if we fetched the requested amount, S_FALSE otherwise
		return (fetched == celt) ? S_OK : S_FALSE;
	}

	STDMETHODIMP Skip(_In_ ULONG celt) override
	{
		m_nCurrentIndex += celt;
		
		// Return S_FALSE if we skipped past the end
		if (m_nCurrentIndex >= m_vEntries.size())
		{
			m_nCurrentIndex = m_vEntries.size();
			return S_FALSE;
		}

		return S_OK;
	}

	STDMETHODIMP Reset(void) override
	{
		m_nCurrentIndex = 0;
		return S_OK;
	}

	STDMETHODIMP Clone(_Out_opt_ IEnumIDList** ppenum) override
	{
		if (!ppenum)
			return E_POINTER;

		*ppenum = NULL;

		// Create a new enumerator
		HRESULT hr;
		CComObject<CRPFEnumIDList>* pNewEnum;

		hr = CComObject<CRPFEnumIDList>::CreateInstance(&pNewEnum);
		if (FAILED(hr))
			return hr;

		pNewEnum->AddRef();

		// Initialize with the same RPF
		hr = pNewEnum->Initialize(m_pRPF);
		if (FAILED(hr))
		{
			pNewEnum->Release();
			return hr;
		}

		// Set the same position
		pNewEnum->m_nCurrentIndex = m_nCurrentIndex;

		hr = pNewEnum->QueryInterface(IID_IEnumIDList, reinterpret_cast<void**>(ppenum));
		pNewEnum->Release();

		return hr;
	}

public:
	DECLARE_NOT_AGGREGATABLE(CRPFEnumIDList)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CRPFEnumIDList)
		COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
	END_COM_MAP()
};

HRESULT CreateRPFEnumIDL(IRPF* pRPF, REFIID riid, LPVOID* ppvOut)
{
	HRESULT hr;
	CComObject<CRPFEnumIDList>* pList;

	hr = CComObject<CRPFEnumIDList>::CreateInstance(&pList);
	if (FAILED(hr))
		return hr;

	pList->AddRef();

	hr = pList->Initialize(pRPF);
	if (SUCCEEDED(hr))
		hr = pList->QueryInterface(riid, reinterpret_cast<void**>(ppvOut));

	pList->Release();

	return hr;
}
