#include "config_dialog_listview_tooltip.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

#include <ovarray.h>
#include <ovbase.h>

enum {
  subclass_id_listview = 100,
  tooltip_text_buffer_size = 1024,
};

struct config_dialog_listview_tooltip {
  HWND tooltip_window;
  HWND parent;
  HWND listview;
  int hover_item;
  int hover_subitem;
  wchar_t *tooltip_text;
};

static void update_tooltip(struct config_dialog_listview_tooltip *state, int item, int subitem) {
  if (!state || !state->tooltip_window || !state->listview) {
    return;
  }

  // Hide tooltip if no valid item
  if (item < 0) {
    if (state->hover_item >= 0) {
      SendMessageW(state->tooltip_window,
                   TTM_TRACKACTIVATE,
                   FALSE,
                   (LPARAM) & (TOOLINFOW){
                                  .cbSize = sizeof(TOOLINFOW),
                                  .hwnd = state->listview,
                                  .uId = (UINT_PTR)GetDlgCtrlID(state->listview),
                              });
      state->hover_item = -1;
      state->hover_subitem = -1;
    }
    return;
  }

  // Same item - nothing to do
  if (state->hover_item == item && state->hover_subitem == subitem) {
    return;
  }

  state->hover_item = item;
  state->hover_subitem = subitem;

  // Get the full text of the cell
  wchar_t text[tooltip_text_buffer_size] = {0};
  LVITEMW lvi = {0};
  lvi.mask = LVIF_TEXT;
  lvi.iItem = item;
  lvi.iSubItem = subitem;
  lvi.pszText = text;
  lvi.cchTextMax = tooltip_text_buffer_size;
  SendMessageW(state->listview, LVM_GETITEMTEXTW, (WPARAM)item, (LPARAM)&lvi);

  if (text[0] == L'\0') {
    // No text, hide tooltip
    update_tooltip(state, -1, -1);
    return;
  }

  // Check if text is truncated by comparing with the column width
  RECT cell_rect = {0};
  cell_rect.top = subitem;
  cell_rect.left = LVIR_LABEL;
  SendMessageW(state->listview, LVM_GETSUBITEMRECT, (WPARAM)item, (LPARAM)&cell_rect);

  // Get text width
  HDC hdc = GetDC(state->listview);
  LRESULT font_result = SendMessageW(state->listview, WM_GETFONT, 0, 0);
  HFONT font = (HFONT)(uintptr_t)font_result;
  HFONT old_font = NULL;
  if (font) {
    old_font = (HFONT)SelectObject(hdc, font);
  }

  SIZE text_size = {0};
  GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &text_size);

  if (old_font) {
    SelectObject(hdc, old_font);
  }
  ReleaseDC(state->listview, hdc);

  // Add some padding
  int const cell_width = cell_rect.right - cell_rect.left - 6;

  // Only show tooltip if text is truncated
  if (text_size.cx <= cell_width) {
    update_tooltip(state, -1, -1);
    return;
  }

  // Update tooltip text
  if (!OV_ARRAY_GROW(&state->tooltip_text, wcslen(text) + 1)) {
    return;
  }
  wcscpy(state->tooltip_text, text);

  SendMessageW(state->tooltip_window,
               TTM_UPDATETIPTEXTW,
               0,
               (LPARAM) & (TOOLINFOW){
                              .cbSize = sizeof(TOOLINFOW),
                              .hwnd = state->listview,
                              .uId = (UINT_PTR)GetDlgCtrlID(state->listview),
                              .lpszText = state->tooltip_text,
                          });

  // Calculate position (below the cell)
  RECT item_rect = {0};
  item_rect.top = subitem;
  item_rect.left = LVIR_LABEL;
  SendMessageW(state->listview, LVM_GETSUBITEMRECT, (WPARAM)item, (LPARAM)&item_rect);

  POINT screen_pt = {item_rect.left, item_rect.bottom};
  ClientToScreen(state->listview, &screen_pt);

  SendMessageW(state->tooltip_window, TTM_TRACKPOSITION, 0, MAKELPARAM(screen_pt.x, screen_pt.y + 4));

  SendMessageW(state->tooltip_window,
               TTM_TRACKACTIVATE,
               TRUE,
               (LPARAM) & (TOOLINFOW){
                              .cbSize = sizeof(TOOLINFOW),
                              .hwnd = state->listview,
                              .uId = (UINT_PTR)GetDlgCtrlID(state->listview),
                          });
}

static LRESULT CALLBACK listview_subclass_proc(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  struct config_dialog_listview_tooltip *state = (struct config_dialog_listview_tooltip *)dwRefData;

  switch (message) {
  case WM_MOUSEMOVE: {
    TrackMouseEvent(&(TRACKMOUSEEVENT){
        .cbSize = sizeof(TRACKMOUSEEVENT),
        .dwFlags = TME_LEAVE,
        .hwndTrack = hWnd,
    });

    POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};

    LVHITTESTINFO hti = {0};
    hti.pt = pt;
    int const item = (int)SendMessageW(hWnd, LVM_SUBITEMHITTEST, 0, (LPARAM)&hti);

    if (item < 0 || (hti.flags & LVHT_NOWHERE)) {
      update_tooltip(state, -1, -1);
    } else {
      update_tooltip(state, item, hti.iSubItem);
    }
    break;
  }

  case WM_MOUSELEAVE:
    update_tooltip(state, -1, -1);
    break;

  case WM_NCDESTROY:
    RemoveWindowSubclass(hWnd, listview_subclass_proc, uIdSubclass);
    break;
  }

  return DefSubclassProc(hWnd, message, wParam, lParam);
}

struct config_dialog_listview_tooltip *
config_dialog_listview_tooltip_create(void *parent, void *listview, struct ov_error *const err) {
  if (!parent || !listview) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return NULL;
  }

  HWND const hParent = (HWND)parent;
  HWND const hListview = (HWND)listview;

  struct config_dialog_listview_tooltip *tt = NULL;
  bool result = false;

  {
    if (!OV_REALLOC(&tt, 1, sizeof(struct config_dialog_listview_tooltip))) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }

    *tt = (struct config_dialog_listview_tooltip){
        .parent = hParent,
        .listview = hListview,
        .hover_item = -1,
        .hover_subitem = -1,
    };

    tt->tooltip_window = CreateWindowExW(WS_EX_TOPMOST,
                                         TOOLTIPS_CLASSW,
                                         NULL,
                                         WS_POPUP | TTS_NOPREFIX,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         hParent,
                                         NULL,
                                         NULL,
                                         NULL);
    if (!tt->tooltip_window) {
      OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
      goto cleanup;
    }
    SendMessageW(tt->tooltip_window, TTM_SETMAXTIPWIDTH, 0, 600);

    // Register listview with tooltip
    SendMessageW(tt->tooltip_window,
                 TTM_ADDTOOLW,
                 0,
                 (LPARAM) & (TOOLINFOW){
                                .cbSize = sizeof(TOOLINFOW),
                                .uFlags = TTF_ABSOLUTE | TTF_TRACK,
                                .hwnd = hListview,
                                .uId = (UINT_PTR)GetDlgCtrlID(hListview),
                                .lpszText = L"",
                            });

    // Subclass the listview
    if (!SetWindowSubclass(hListview, listview_subclass_proc, subclass_id_listview, (DWORD_PTR)tt)) {
      OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
      goto cleanup;
    }
  }

  result = true;

cleanup:
  if (!result) {
    if (tt) {
      if (tt->listview) {
        RemoveWindowSubclass(tt->listview, listview_subclass_proc, subclass_id_listview);
      }
      if (tt->tooltip_window) {
        DestroyWindow(tt->tooltip_window);
      }
      if (tt->tooltip_text) {
        OV_ARRAY_DESTROY(&tt->tooltip_text);
      }
      OV_FREE(&tt);
    }
    return NULL;
  }
  return tt;
}

void config_dialog_listview_tooltip_destroy(struct config_dialog_listview_tooltip **ttpp) {
  if (!ttpp || !*ttpp) {
    return;
  }
  struct config_dialog_listview_tooltip *ttp = *ttpp;

  if (ttp->listview) {
    RemoveWindowSubclass(ttp->listview, listview_subclass_proc, subclass_id_listview);
    ttp->listview = NULL;
  }

  if (ttp->tooltip_window) {
    DestroyWindow(ttp->tooltip_window);
    ttp->tooltip_window = NULL;
  }

  if (ttp->tooltip_text) {
    OV_ARRAY_DESTROY(&ttp->tooltip_text);
  }

  OV_FREE(ttpp);
}
