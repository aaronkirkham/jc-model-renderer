#include <ShObjIdl.h>
#include <shellapi.h>
#include <shlobj.h>

#include "drop_target.h"
#include "window.h"

DropTarget::DropTarget(HWND hwnd)
    : m_Hwnd(hwnd)
{
    OleInitialize(0);
    auto hr = RegisterDragDrop(m_Hwnd, this);
    assert(SUCCEEDED(hr));
}

DropTarget::~DropTarget()
{
    RevokeDragDrop(m_Hwnd);
    OleUninitialize();
}

HRESULT DropTarget::DragEnter(IDataObject* data_object, DWORD key_stae, POINTL cursor_position, DWORD* effect)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgm;
    if (SUCCEEDED(data_object->GetData(&fmte, &stgm))) {
        const auto drop      = reinterpret_cast<HDROP>(stgm.hGlobal);
        const auto num_files = DragQueryFile(drop, -1, NULL, NULL);

        std::vector<std::filesystem::path> files;
        files.reserve(num_files);

        for (uint32_t i = 0; i < num_files; ++i) {
            char file_path[MAX_PATH] = {0};
            DragQueryFile(drop, i, file_path, MAX_PATH);

            files.emplace_back(std::move(file_path));
        }

        m_TimeSinceDragEnter = clock::now();
        m_BringToFront       = true;

        Window::Get()->Events().DragEnter(std::move(files));
        ReleaseStgMedium(&stgm);
    }

    return S_OK;
}

HRESULT DropTarget::DragLeave()
{
    m_TimeSinceDragEnter = {};
    m_BringToFront       = false;

    Window::Get()->Events().DragLeave();
    return S_OK;
}

HRESULT DropTarget::DragOver(DWORD key_state, POINTL cursor_position, DWORD* effect)
{
    // bring the window to the front
    if (m_BringToFront
        && std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_TimeSinceDragEnter).count() > 500) {
        SetForegroundWindow(m_Hwnd);
        SetFocus(m_Hwnd);
        m_BringToFront = false;
    }

    return S_OK;
}

HRESULT DropTarget::Drop(IDataObject* data_object, DWORD key_state, POINTL cursor_position, DWORD* effect)
{
    m_TimeSinceDragEnter = {};
    m_BringToFront       = false;
    SetForegroundWindow(m_Hwnd);
    SetFocus(m_Hwnd);

    Window::Get()->Events().DragDropped();
    return S_OK;
}

HRESULT DropTarget::QueryInterface(const IID& iid, void** object)
{
    *object = NULL;
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget)) {
        *object = this;
    } else {
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
