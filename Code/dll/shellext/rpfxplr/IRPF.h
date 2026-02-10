#pragma once

class RPFReader;

class IRPF : public IUnknown
{
public:
	STDMETHOD (GetRPFReader) (_Out_ RPFReader** ppOutReader) PURE;
};
