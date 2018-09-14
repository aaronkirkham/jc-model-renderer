#include <Window.h>
#include <graphics/Renderer.h>
#include <DropTarget.h>

#include <ShObjIdl.h>
#include <shlobj.h>
#include <shellapi.h>

DropTarget::DropTarget(HWND hwnd)
    : m_Hwnd(hwnd)
{
    OleInitialize(0);
    auto hr = RegisterDragDrop(m_Hwnd, this);
    assert(SUCCEEDED(hr));
}

DropTarget::~DropTarget()
{
    //
    RevokeDragDrop(m_Hwnd);
    OleUninitialize();
}

HRESULT DropTarget::DragEnter(IDataObject* data_object, DWORD key_stae, POINTL cursor_position, DWORD* effect)
{
    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;
    if (SUCCEEDED(data_object->GetData(&fmte, &stgm))) {
        const auto drop = reinterpret_cast<HDROP>(stgm.hGlobal);

        char file[MAX_PATH] = { 0 };
        DragQueryFile(drop, 0, file, MAX_PATH);

        SetForegroundWindow(m_Hwnd);
        SetFocus(m_Hwnd);
        Window::Get()->Events().DragEnter(file);

        ReleaseStgMedium(&stgm);
    }

    return S_OK;
}

HRESULT DropTarget::DragLeave()
{
    Window::Get()->Events().DragLeave();
    return S_OK;
}

HRESULT DropTarget::DragOver(DWORD key_state, POINTL cursor_position, DWORD* effect)
{
    return S_OK;
}

HRESULT DropTarget::Drop(IDataObject* data_object, DWORD key_state, POINTL cursor_position, DWORD* effect)
{
    Window::Get()->Events().DragDropped();
    return S_OK;
}

HRESULT DropTarget::QueryInterface(const IID& iid, void** object)
{
    *object = NULL;
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget)) {
        *object = this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG DropTarget::AddRef()
{
    return ++m_RefCount;
}

ULONG DropTarget::Release()
{
    if (--m_RefCount == 0) {
        delete this;
        return 0U;
    }
    return m_RefCount;
}