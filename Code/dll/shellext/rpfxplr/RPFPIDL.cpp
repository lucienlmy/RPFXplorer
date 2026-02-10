#include "pch.h"
#include "RPFPIDL.h"

LPITEMIDLIST _CreateRPFPIDL(_In_ const RPFEntry& rpfEntry, RPFReader* prpfReader)
{
    auto name = prpfReader->GetName(&rpfEntry);
    int nameLen = (name.GetLength() + 1) * sizeof(WCHAR); // +1 for null terminator
    int size = sizeof(RPFPidlData) - sizeof(WCHAR) + nameLen + sizeof(USHORT); // +sizeof(USHORT) for terminator

    RPFPidlData* pidl = (RPFPidlData*)CoTaskMemAlloc(size);
    if (!pidl)
        return NULL;

    ZeroMemory(pidl, size);

    pidl->cb = (USHORT)(size - sizeof(USHORT)); // Size excluding the terminator
    pidl->uMagicValue = RPF_PIDL_MAGIC;
    pidl->rpfEntry = rpfEntry;

    wcscpy_s(pidl->szName, name.GetLength() + 1, name.GetString());

    // Add terminator
    USHORT* terminator = (USHORT*)((BYTE*)pidl + pidl->cb + sizeof(USHORT));
    *terminator = 0;

    return (LPITEMIDLIST)pidl;
}

const RPFPidlData* _PidlToRPFEntry(_In_ LPCITEMIDLIST pidl)
{
    RPFPidlData* data = (RPFPidlData*)pidl;

    return data->uMagicValue == RPF_PIDL_MAGIC ? data : NULL;
}
