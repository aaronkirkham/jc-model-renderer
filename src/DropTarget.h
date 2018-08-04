#pragma once

#include <StdInc.h>
#include <ShObjIdl.h>

class DropTarget : public IDropTarget
{
private:
    HWND m_Hwnd = nullptr;
    LONG m_RefCount = 0;

public:
    DropTarget(HWND hwnd);
    virtual ~DropTarget();

    HRESULT DragEnter(IDataObject* data_object, DWORD key_state, POINTL cursor_position, DWORD* effect);
    HRESULT DragLeave();
    HRESULT DragOver(DWORD key_state, POINTL cursor_position, DWORD* effect);
    HRESULT Drop(IDataObject* data_object, DWORD key_state, POINTL ptl, DWORD* effect);
    HRESULT QueryInterface(const IID& iid, void** object);
    ULONG AddRef();
    ULONG Release();
};