#ifndef _WX_CURSOR_H_
#define _WX_CURSOR_H_

// Cursor
class WXDLLIMPEXP_CORE wxCursor //: public wxGDIObject
{
public:
    wxCursor();
    /*wxCursor(const wxImage& image);
    wxCursor(const wxString& name,
             wxBitmapType type = wxCURSOR_DEFAULT_TYPE,
             int hotSpotX = 0, int hotSpotY = 0);
    wxCursor(wxStockCursor id) { InitFromStock(id); }
#if WXWIN_COMPATIBILITY_2_8
    wxCursor(int id) { InitFromStock((wxStockCursor)id); }
#endif*/
    virtual ~wxCursor();

    virtual bool IsOk() const { return false; }
};

#endif  // _WX_CURSOR_H_