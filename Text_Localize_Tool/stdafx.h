// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_WINNT	0x0501
#define _WIN32_IE	0x0501
#define _RICHEDIT_VER	0x0200

#define WIDTHBUFLEN 8
#define MINWINWIDTH 550
#define OUTCTRLLEN 18

//默认选项值
//宽度
#define DEFAULT_WIDTH_ID _T("125")
#define DEFAULT_WIDTH_JP _T("280")
#define DEFAULT_WIDTH_CN _T("280")
#define DEFAULT_WIDTH_COM _T("280")
//高级
#define DEFAULT_BYTE_PER_CHAR 2
#define DEFAULT_TEXT_BUF_LEN 300
#define DEFAULT_ITEM_PER_PAGE 10000
#define DEFAULT_WRITE_FILE_PER_TIME 10
#define DEFAULT_EDIT_CTRLCHAR 0
#define DEFAULT_HALF_TO_FULL 1
#define DEFAULT_SAVE_SPLITER_POS 0
#define DEFAULT_SPLITER_PERCENT 30
#define DEFAULT_JP_SHOW_CNFONT 0
#define DEFAULT_CN_SHOW_DIFF 1
#define DEFAULT_ONE_PAGE_SLIDER 1
#define DEFAULT_RE2_FONT_SIZE 9
#define DEFAULT_CN_COM_NULL 0
#define DEFAULT_OUT_CTRL_LEN 0
//颜色
#define DEFAULT_NOR_TEXT _T("#000000")
#define DEFAULT_CTRL_TEXT _T("#0000ff")
#define DEFAULT_DIFF_TEXT _T("#ff0000")
#define DEFAULT_OVER_LEN_BG _T("#ffff80")
#define DEFAULT_NOR_TR _T("#f1f7fb")
#define DEFAULT_SEL_TR _T("#b5ddf2")
#define DEFAULT_TAB_BORDER _T("#a0a0a0")
#define DEFAULT_BODY_BG _T("#f0f0f0")
#define DEFAULT_HILIRE_BG _T("#ffffc0")

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlsplit.h>

#include <atlmisc.h>
#include <io.h>

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
