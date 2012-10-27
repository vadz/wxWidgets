///////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/tabartgtk.cpp
// Purpose:     implementation of the wxAuiGTKTabArt
// Author:      Jens Lody and Teodor Petrov
// Modified by: Malcolm MacLeod (mmacleod@webmail.co.za)
// Modified by: Jens Lody
// Created:     2012-03-23
// RCS-ID:      $Id$
// Copyright:   (c) 2012 Jens Lody <jens@codeblocks.org>
//                  and Teodor Petrov
//                  2012, Jens Lody for the code related to left and right
//                        positioning
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#ifndef WX_PRECOMP
    #include "wx/dc.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/image.h"
#endif

#include "wx/gtk/dc.h"
#include "wx/gtk/private.h"

#include <gtk/gtk.h>

#include "wx/aui/auibook.h"
#include "wx/aui/tabartgtk.h"
#include "wx/renderer.h"

namespace
{

static int s_CloseIconSize = 16; // default size

}

wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size);

wxAuiGtkTabArt::wxAuiGtkTabArt()
{
}

wxAuiTabArt* wxAuiGtkTabArt::Clone()
{
    wxAuiGtkTabArt* clone = new wxAuiGtkTabArt();

    clone->SetNormalFont(m_normalFont);
    clone->SetSelectedFont(m_normalFont);
    clone->SetMeasuringFont(m_normalFont);

    return clone;
}

void wxAuiGtkTabArt::DrawBackground(wxDC& dc, wxWindow* WXUNUSED(wnd), const wxRect& rect)
{
    wxGTKDCImpl* impldc = (wxGTKDCImpl*) dc.GetImpl();
    GdkWindow* window = impldc->GetGDKWindow();

    gtk_style_apply_default_background(gtk_widget_get_style(wxGTKPrivate::GetNotebookWidget()),
                                       window,
                                       true,
                                       GTK_STATE_NORMAL,
                                       NULL,
                                       rect.x, rect.y, rect.width, rect.height);
}

void ButtonStateAndShadow(int buttonState, GtkStateType& state, GtkShadowType& shadow)
{

    if (buttonState & wxAUI_BUTTON_STATE_DISABLED)
    {
        state = GTK_STATE_INSENSITIVE;
        shadow = GTK_SHADOW_ETCHED_IN;
    }
    else if (buttonState & wxAUI_BUTTON_STATE_HOVER)
    {
        state = GTK_STATE_PRELIGHT;
        shadow = GTK_SHADOW_OUT;
    }
    else if (buttonState & wxAUI_BUTTON_STATE_PRESSED)
    {
        state = GTK_STATE_ACTIVE;
        shadow = GTK_SHADOW_IN;
    }
    else
    {
        state = GTK_STATE_NORMAL;
        shadow = GTK_SHADOW_OUT;
    }
}

wxRect CalculateCloseButtonRect(wxRect const& inRect, int orientation)
{
    GtkStyle* styleButton = gtk_widget_get_style(wxGTKPrivate::GetButtonWidget());
    int xthickness = styleButton->xthickness;
    int ythickness = styleButton->ythickness;

    int buttonSize = s_CloseIconSize + 2 * xthickness;

    wxRect outRect;

    outRect.y = inRect.y + (inRect.height - buttonSize) / 2;

    if (orientation == wxLEFT)
        outRect.x = inRect.x - xthickness;
    else if (orientation == wxRIGHT)
        outRect.x = inRect.x + inRect.width - buttonSize;
    else if (orientation == wxUP)
    {
        outRect.x = inRect.x + inRect.width - buttonSize - xthickness;
        outRect.y = inRect.y + ythickness;
    }

    outRect.width = buttonSize;
    outRect.height = buttonSize;

    return outRect;
}

wxRect DrawCloseButton(wxDC& dc, GtkWidget* widget, int buttonState, wxRect const& inRect, int orientation, GdkRectangle* clipRect)
{
    GtkStyle* styleButton = gtk_widget_get_style(wxGTKPrivate::GetButtonWidget());
    int xthickness = styleButton->xthickness;
    int ythickness = styleButton->ythickness;

    wxBitmap bmp(gtk_widget_render_icon(widget, GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR, "tab"));

    if(bmp.GetWidth() != s_CloseIconSize || bmp.GetHeight() != s_CloseIconSize)
    {
        wxImage img = bmp.ConvertToImage();
        img.Rescale(s_CloseIconSize, s_CloseIconSize);
        bmp = img;
    }

    wxRect outRect = CalculateCloseButtonRect(inRect, orientation);

    wxGTKDCImpl* impldc = (wxGTKDCImpl*) dc.GetImpl();
    GdkWindow* window = impldc->GetGDKWindow();

    if (buttonState == wxAUI_BUTTON_STATE_HOVER)
    {
        gtk_paint_box(styleButton, window,
                      GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, clipRect, widget, "button",
                     outRect.x, outRect.y, outRect.width, outRect.height);
    }
    else if (buttonState == wxAUI_BUTTON_STATE_PRESSED)
    {
        gtk_paint_box(styleButton, window,
                      GTK_STATE_ACTIVE, GTK_SHADOW_IN, clipRect, widget, "button",
                      outRect.x, outRect.y, outRect.width, outRect.height);
    }


    dc.DrawBitmap(bmp, outRect.x + xthickness, outRect.y + ythickness, true);

    return outRect;
}

void wxAuiGtkTabArt::DrawTab(wxDC& dc, wxWindow* wnd, const wxAuiPaneInfo& page, const wxRect& inRect, int closeButtonState, bool haveFocus, wxRect* outTabRect, wxRect* outButtonRect, int* xExtent)
{
    GtkWidget* widget = wnd->GetHandle();
    GtkStyle* styleNotebook = gtk_widget_get_style(wxGTKPrivate::GetNotebookWidget());

    int focusWidth = 0;

    gtk_widget_style_get(wxGTKPrivate::GetNotebookWidget(),
                         "focus-line-width", &focusWidth,
                         NULL);

    // figure out the size of the tab
    wxSize tabSize = GetTabSize(dc, wnd, page.caption, page.GetBitmap(),
                                    page.HasFlag(wxAuiPaneInfo::optionActiveNotebook), closeButtonState, xExtent);

    wxRect tabRect = inRect;
    wxRect gapRect = wnd->GetRect();
    int gapStart = 0, gapWidth = 0;

    GtkPositionType gapBoxPos;
    GtkPositionType gapPos;

    if (IsHorizontal())
    {
        tabRect.width = tabSize.x;
        tabRect.height = tabSize.y;
        if (page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
            tabRect.height += 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
        // if no bitmap is set, we need a tiny correction
        if (! page.GetBitmap().IsOk())
            tabRect.height += 1;
        tabRect.y += GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder / 2;
        gapRect.height = 6 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
        gapRect.x = 1;
        gapStart = tabRect.x - GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder / 2;
        gapWidth = tabRect.width;

        if (HasFlag(wxAUI_NB_TOP))
        {
            if (!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                tabRect.y += 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
            gapRect.y = tabRect.y + tabRect.height;
            gapBoxPos = GTK_POS_TOP;
            gapPos = GTK_POS_BOTTOM;
        }
        else//wxAUI_NB_BOTTOM
        {
            tabRect.y += 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
            tabRect.height += 2;
            gapRect.y = tabRect.y - gapRect.height;
            gapRect.y += GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder / 2;
            gapBoxPos = GTK_POS_BOTTOM;
            gapPos = GTK_POS_TOP;
        }
    }
    else
    {
        tabRect.width -= 4 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
        tabRect.height = tabSize.y;
        gapRect.width = 6 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
        gapRect.y = 1;
        gapStart = tabRect.y - GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder / 2;
        gapWidth = tabRect.height;

        if (HasFlag(wxAUI_NB_LEFT))
        {
            if (!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
            {
                tabRect.x += 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
                tabRect.width -= 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
            }
            gapRect.x = tabRect.x + tabRect.width;
            gapBoxPos = GTK_POS_LEFT;
            gapPos = GTK_POS_RIGHT;
        }
        else//wxAUI_NB_RIGHT
        {
            tabRect.x = inRect.width - tabRect.width;
            if (!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                tabRect.width -= 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
            gapRect.x = tabRect.x - gapRect.width;
            gapBoxPos = GTK_POS_RIGHT;
            gapPos = GTK_POS_LEFT;
        }
    }

    int padding = focusWidth + GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;

    int clipWidth = tabRect.width;
    int clipHeight = tabRect.height;
    if (tabRect.x + tabRect.width > inRect.x + inRect.width)
        clipWidth = (inRect.x + inRect.width) - tabRect.x;
    if (tabRect.y + tabRect.height > inRect.y + inRect.height)
        clipHeight = (inRect.y + inRect.height) - tabRect.y;

    dc.SetClippingRegion(tabRect.x, tabRect.y, clipWidth, clipHeight);

    GdkRectangle area;
    area.x = tabRect.x - GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
    area.y = tabRect.y - 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
    area.width = clipWidth + GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
    area.height = clipHeight + 2 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;

    wxGTKDCImpl* impldc = (wxGTKDCImpl*) dc.GetImpl();
    GdkWindow* window = impldc->GetGDKWindow();

    // we draw a box here, because the gap-box is not drawn, if the active tab is not visible
    gtk_paint_box(styleNotebook, window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                  NULL, widget,
                  const_cast<char*>("notebook"),
                  gapRect.x, gapRect.y,
                  gapRect.width, gapRect.height);

    if (page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
    {
        gtk_paint_box_gap(styleNotebook, window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                          NULL, widget,
                          const_cast<char*>("notebook"),
                          gapRect.x, gapRect.y,
                          gapRect.width, gapRect.height,
                          gapBoxPos, gapStart , gapWidth);
    }
    gtk_paint_extension(styleNotebook, window,
                       page.HasFlag(wxAuiPaneInfo::optionActiveNotebook) ? GTK_STATE_NORMAL : GTK_STATE_ACTIVE, GTK_SHADOW_OUT,
                       &area, widget,
                       const_cast<char*>("tab"),
                       tabRect.x, tabRect.y,
                       tabRect.width, tabRect.height,
                       gapPos);

    wxCoord textX = tabRect.x + padding + styleNotebook->xthickness;

    int bitmapX = 0;
    if (page.GetBitmap().IsOk())
    {
        bitmapX = textX;

        // draw bitmap
        int bitmapY = tabRect.y +(tabRect.height - page.GetBitmap().GetHeight()) / 2;
        if(!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
        {
            if (HasFlag(wxAUI_NB_TOP))
                bitmapY += styleNotebook->ythickness / 2;
            else
                bitmapY -= styleNotebook->ythickness / 2;
        }
        dc.DrawBitmap(page.GetBitmap(),
                      bitmapX,
                      bitmapY,
                      true);

        textX += page.GetBitmap().GetWidth() + padding;
    }

    wxCoord textW, textH, textY;

    dc.SetFont(m_normalFont);
    dc.GetTextExtent(page.caption, &textW, &textH);
    textY = tabRect.y + (tabRect.height - textH) / 2;
    if(!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
    {
        if (HasFlag(wxAUI_NB_TOP))
            textY += styleNotebook->ythickness / 2;
        else
            textY -= styleNotebook->ythickness / 2;
    }

    // draw tab text
    GdkColor textColour = page.HasFlag(wxAuiPaneInfo::optionActiveNotebook) ? styleNotebook->fg[GTK_STATE_NORMAL] : styleNotebook->fg[GTK_STATE_ACTIVE];
    dc.SetTextForeground(wxColor(textColour));
    GdkRectangle focusArea;

    int paddingFocus = padding - focusWidth;
    focusArea.x = tabRect.x + paddingFocus;
    focusArea.y = textY - focusWidth;
    focusArea.width = tabRect.width - 2 * paddingFocus;
    focusArea.height = textH + 2 * focusWidth;

    if (page.HasFlag(wxAuiPaneInfo::optionActiveNotebook) && haveFocus && focusArea.x <= (area.x + area.width))
    {
        // clipping seems not to work here, so we we have to recalc the focus-area manually
        if((focusArea.x + focusArea.width) > (area.x + area.width))
            focusArea.width = area.x + area.width - focusArea.x + focusWidth - GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
        if((focusArea.y + focusArea.height) > (area.y + area.height))
            focusArea.height = area.y + area.height - focusArea.y + focusWidth - GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_hborder;
        gtk_paint_focus (styleNotebook, window,
                         GTK_STATE_ACTIVE, NULL, widget, "tab",
                         focusArea.x, focusArea.y, focusArea.width, focusArea.height);
    }

    int closeButtonWidth = (closeButtonState != wxAUI_BUTTON_STATE_HIDDEN) ? s_CloseIconSize + 3 * padding : 0;
    wxString draw_text = wxAuiChopText(dc,page.caption, tabRect.width - (textX - tabRect.x) - closeButtonWidth);

    dc.DrawText(draw_text, textX, textY);

    // draw close-button on tab (if enabled)
    if (closeButtonState != wxAUI_BUTTON_STATE_HIDDEN)
    {
        wxRect rect(tabRect.x, tabRect.y, tabRect.width - styleNotebook->xthickness, tabRect.height);
        if(!page.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
        {
            if (HasFlag(wxAUI_NB_TOP))
                rect.y += styleNotebook->ythickness / 2;
            else
                rect.y -= styleNotebook->ythickness / 2;
        }
        *outButtonRect = DrawCloseButton(dc, widget, closeButtonState, rect, wxRIGHT, &area);
    }

    tabRect.width = std::min(tabRect.width, clipWidth);
    *outTabRect = tabRect;

    dc.DestroyClippingRegion();
}

wxRect DrawSimpleArrow(wxDC& dc, GtkWidget* widget, int buttonState, wxRect const& inRect, int orientation, GtkArrowType arrowType)
{
    int scrollArrowHLength, scrollArrowVLength;
    gtk_widget_style_get(widget,
                         "scroll-arrow-hlength", &scrollArrowHLength,
                         "scroll-arrow-vlength", &scrollArrowVLength,
                         NULL);

    GtkStateType state;
    GtkShadowType shadow;
    ButtonStateAndShadow(buttonState, state, shadow);

    wxRect outRect;

    if (orientation == wxLEFT)
        outRect.x = inRect.x;
    else if (orientation == wxUP || orientation == wxDOWN)
        outRect.x = inRect.x + (inRect.width - scrollArrowHLength) / 2;
    else  if (orientation == wxRIGHT)
        outRect.x = inRect.x + inRect.width - scrollArrowHLength;
    if (orientation == wxDOWN)
        outRect.y = inRect.y + inRect.height -  scrollArrowVLength;
    else if (orientation == wxUP)
        outRect.y = inRect.y;
    else
         outRect.y = (inRect.y + inRect.height - 3 * gtk_widget_get_style(wxGTKPrivate::GetNotebookWidget())->ythickness - scrollArrowVLength) / 2;

    outRect.width = scrollArrowHLength;
    outRect.height = scrollArrowVLength;

    wxGTKDCImpl* impldc = (wxGTKDCImpl*) dc.GetImpl();
    GdkWindow* window = impldc->GetGDKWindow();
    gtk_paint_arrow (gtk_widget_get_style(wxGTKPrivate::GetButtonWidget()), window, state, shadow, NULL, widget, "notebook",
                     arrowType, TRUE, outRect.x, outRect.y, outRect.width, outRect.height);

    return outRect;
}

void wxAuiGtkTabArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxRect& inRect, int bitmapID, int buttonState, int orientation, wxRect* outRect)
{
    GtkWidget* widget = wnd->GetHandle();
    wxRect rect = inRect;
    if (m_flags &wxAUI_NB_BOTTOM)
        rect.y += 2 * gtk_widget_get_style(wxGTKPrivate::GetButtonWidget())->ythickness;

    switch (bitmapID)
    {
        case wxAUI_BUTTON_CLOSE:
            if (orientation != wxUP)
                rect.y -= 2 * gtk_widget_get_style(wxGTKPrivate::GetButtonWidget())->ythickness;
            else
                rect.y -= gtk_widget_get_style(wxGTKPrivate::GetButtonWidget())->ythickness;
            if (HasFlag(wxAUI_NB_LEFT))
                rect.x -= 4 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
            rect = DrawCloseButton(dc, widget, buttonState, rect, orientation, NULL);
            break;

        case wxAUI_BUTTON_LEFT:
            rect = DrawSimpleArrow(dc, widget, buttonState, rect, orientation, GTK_ARROW_LEFT);
            break;

        case wxAUI_BUTTON_RIGHT:
            if (HasFlag(wxAUI_NB_WINDOWLIST_BUTTON))
            {
                int scrollArrowHLength;
                gtk_widget_style_get(widget,
                                     "scroll-arrow-hlength", &scrollArrowHLength,
                                     NULL);
                rect.width -= 1.5 * scrollArrowHLength;
            }
            if (HasFlag(wxAUI_NB_CLOSE_BUTTON))
                rect.width -= CalculateCloseButtonRect(inRect, orientation).GetWidth();
            rect = DrawSimpleArrow(dc, widget, buttonState, rect, orientation, GTK_ARROW_RIGHT);
            break;

        case wxAUI_BUTTON_UP:
            rect = DrawSimpleArrow(dc, widget, buttonState, rect, orientation, GTK_ARROW_UP);
            break;

        case wxAUI_BUTTON_DOWN:
            rect = DrawSimpleArrow(dc, widget, buttonState, rect, orientation, GTK_ARROW_DOWN);
            break;

        case wxAUI_BUTTON_WINDOWLIST:
            {
                int scrollArrowHLength, scrollArrowVLength;
                gtk_widget_style_get(widget,
                                     "scroll-arrow-hlength", &scrollArrowHLength,
                                     "scroll-arrow-vlength", &scrollArrowVLength,
                                     NULL);
                rect.height = 1.5 * scrollArrowVLength;
                rect.width = 1.5 * scrollArrowHLength;
                rect.x = inRect.x + inRect.width - rect.width;
                if (HasFlag(wxAUI_NB_CLOSE_BUTTON))
                    rect.x -= CalculateCloseButtonRect(inRect, orientation).GetWidth();

                if (orientation == wxUP)
                {
                    if (HasFlag(wxAUI_NB_LEFT))
                        rect.x -= 4 * GTK_NOTEBOOK (wxGTKPrivate::GetNotebookWidget())->tab_vborder;
                }

                if (buttonState == wxAUI_BUTTON_STATE_HOVER)
                    wxRendererNative::Get().DrawComboBoxDropButton(wnd, dc, rect, wxCONTROL_CURRENT);
                else if (buttonState == wxAUI_BUTTON_STATE_PRESSED)
                    wxRendererNative::Get().DrawComboBoxDropButton(wnd, dc, rect, wxCONTROL_PRESSED);
                else
                    wxRendererNative::Get().DrawDropArrow(wnd, dc, rect);
            }
            break;
    }

    *outRect = rect;
}


wxSize wxAuiGtkTabArt::GetBestTabSize(wxWindow* wnd, const wxAuiPaneInfoPtrArray& pages, const wxSize& requiredBmpSize)
{
    SetMeasuringFont(m_normalFont);
    SetSelectedFont(m_normalFont);

    wxSize tabSize = wxAuiGenericTabArt::GetBestTabSize(wnd, pages, requiredBmpSize);
    tabSize.IncBy(3 * gtk_widget_get_style(wxGTKPrivate::GetNotebookWidget())->xthickness,
                   3 * gtk_widget_get_style(wxGTKPrivate::GetNotebookWidget())->ythickness);
    return tabSize;
}

wxSize wxAuiGtkTabArt::GetTabSize(wxDC& dc, wxWindow* wnd, const wxString& caption, const wxBitmap& bitmap, bool active, int closeButtonState, int* extent)
{
    wxSize tabSize = wxAuiGenericTabArt::GetTabSize(dc, wnd, caption, bitmap, active, closeButtonState, extent);

    int overlap = 0;
    gtk_widget_style_get (wnd->GetHandle(), "focus-line-width", &overlap, NULL);
    *extent -= overlap;
    return tabSize;
}
#endif  // wxUSE_AUI
