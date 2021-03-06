﻿#include "manifests.h"
#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <ShlObj.h>
#include "misc.h"
#include "powsavt.h"
#include "main.h"
#include "WindowList.h"
#include "WindowStatusBar.h"
#include "resource.h"

struct THREAD_PARAM
{
	HINSTANCE instance;
	int show;
};
typedef struct THREAD_PARAM THREAD_PARAM;


bool MainWindow::on_create()
{
	::SetLastError(0);
	if(::SetWindowLongPtr(this->p_window, 0, reinterpret_cast<LONG_PTR>(this)) == 0)
	{
		if(::GetLastError() != ERROR_SUCCESS)
		{
			return false;
		}
	}

	this->p_menu = ::GetMenu(this->p_window);
	this->p_window_menu = ::LoadMenu(this->p_instance, MAKEINTRESOURCE(IDM_OP));

	this->p_list = new WindowList(this->p_window, 1, nullptr);
	this->p_status = new WindowStatusBar(this->p_window, 2);

	::SetTimer(this->p_window, 398, 1000, nullptr);
	::SetTimer(this->p_window, 37564, 1000, nullptr);

	this->p_list->Update();

	return true;
}

void MainWindow::on_destroy()
{
	::KillTimer(this->p_window, 37564);
	::KillTimer(this->p_window, 398);

	if(this->p_window_menu)
	{
		::DestroyMenu(this->p_window_menu);
	}
	if(this->p_status)
	{
		delete this->p_status;
	}
	if(this->p_list)
	{
		delete this->p_list;
	}
	::PostQuitMessage(0);
}

void MainWindow::on_move(unsigned int x, unsigned int y)
{
	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);

	RECT r;
	::GetWindowRect(this->p_window, &r);
	this->p_pos.x = r.left;
	this->p_pos.y = r.top;
}

void MainWindow::on_size(unsigned int width, unsigned int height)
{
	HWND listview;
	RECT r;

	if(!(::IsZoomed(this->p_window) || ::IsIconic(this->p_window)))
	{
		::GetWindowRect(this->p_window, &r);
		this->p_size.cx = r.right - r.left;
		this->p_size.cy = r.bottom - r.top;
	}

	::SendMessage(this->p_status->GetWindowHandle(), WM_SIZE, MAKELONG(width, height), 0);

	listview = this->p_list->GetWindowHandle();
	::SetWindowPos(listview, nullptr, 0, 0, width, height - this->p_status->GetBarHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	ListView_Arrange(listview, LVA_DEFAULT);
}

void MainWindow::on_close()
{
	::ShowWindow(this->p_window, SW_HIDE);
	::ShowWindow(this->p_window, SW_RESTORE);
	::DestroyWindow(this->p_window);
}

void MainWindow::on_winini_change()
{
	RECT r[2];

	::GetWindowRect(this->p_window, &r[0]);
	::GetClientRect(this->p_window, &r[1]);
	::OffsetRect(&r[0], -(r[0].left), -(r[0].top));

	this->p_min_client_area.x = 256 + (r[0].right - r[1].right);
	this->p_min_client_area.y = 256 + (r[0].bottom - r[1].bottom);

	this->p_list->HandleThemeChange();
}

void MainWindow::on_get_minmax_info(MINMAXINFO *m)
{
	m->ptMinTrackSize = this->p_min_client_area;
}

bool MainWindow::on_contextmenu(HWND wnd, unsigned int x, unsigned int y)
{
	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	HMENU context_menu;

	if(wnd == this->p_list->GetWindowHandle())
	{
		context_menu = ::GetSubMenu(this->p_window_menu, 0);
		if(context_menu)
		{
			this->p_list->HandleContextMenu(context_menu);
		}
		return true;
	}

	return false;
}

void MainWindow::on_command(unsigned int id, unsigned int type, HWND cwindow)
{
	UNREFERENCED_PARAMETER(cwindow);
	DWORD exstyle;
	bool is_menu;

	is_menu = (type == 0);

	if(is_menu)
	{
		switch(id)
		{
		case IDC_VIEW_NORMALICON:
			this->p_list->SetViewMode(LVS_ICON);
			break;

		case IDC_VIEW_SMALLICON:
			this->p_list->SetViewMode(LVS_SMALLICON);
			break;

		case IDC_VIEW_LIST:
			this->p_list->SetViewMode(LVS_LIST);
			break;

		case IDC_VIEW_REPORT:
			this->p_list->SetViewMode(LVS_REPORT);
			break;

		case IDC_VIEW_TILE:
			this->p_list->SetViewMode(LV_VIEW_TILE);
			break;

		case IDC_VIEW_TOGGLE_INVISIBLE:
			this->p_list->ToggleStylesMask(WS_VISIBLE);
			this->p_list->Update();
			break;

		case IDC_VIEW_TOGGLE_TOOLBARS:
			this->p_list->ToggleStylesMask(WS_POPUP);
			this->p_list->ToggleExtendStylesMask(WS_EX_TOOLWINDOW);
			this->p_list->Update();
			break;

		case IDC_VIEW_TOGGLE_NOTITLE:
			this->p_list->ShowNoTitleWindow();
			this->p_list->Update();
			break;

		case IDC_OPTION_TOPMOST:
			exstyle = static_cast<DWORD>(::GetWindowLongPtr(this->p_window, GWL_EXSTYLE));
			::SetWindowPos(this->p_window, (exstyle & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			break;

		case IDC_OPTION_TOGGLEIMEBAR:
			::SystemParametersInfo(SPI_SETSHOWIMEUI, this->p_toggle.ime_ui ? FALSE : TRUE, nullptr, 0);
			break;

		case IDC_OPTION_TOGGLECURSHADOW:
			::SystemParametersInfo(SPI_SETCURSORSHADOW, 0, reinterpret_cast<PVOID>(static_cast<intptr_t>(this->p_toggle.cursor_shadow ? FALSE : TRUE)), 0);
			break;

		case IDC_OPTION_TOGGLEHOTTRACK:
			::SystemParametersInfo(SPI_SETHOTTRACKING, 0, reinterpret_cast<PVOID>(static_cast<intptr_t>(this->p_toggle.hot_tracking ? FALSE : TRUE)), SPIF_SENDWININICHANGE);
			break;

		case IDC_OPTION_TOGGLEWINDOWSHADOW:
			::SystemParametersInfo(SPI_SETDROPSHADOW, 0, reinterpret_cast<PVOID>(static_cast<intptr_t>(this->p_toggle.window_shadow ? FALSE : TRUE)), SPIF_SENDWININICHANGE);
			break;

		case IDC_OPTION_TOGGLEMONITORPOWER:
			::SystemParametersInfo(SPI_SETPOWEROFFACTIVE, this->p_toggle.monitor_power ? 0 : 1, nullptr, 0);
			break;

		case IDC_OPTION_TOGGLELOWMONITORPOWER:
			::SystemParametersInfo(SPI_SETLOWPOWERACTIVE, this->p_toggle.monitor_power_low ? 0 : 1, nullptr, 0);
			break;

		case IDC_OPTION_POWERSAVE_TIMER:
			::ShowPowerSaveTimerDialog(this->p_window, p_instance);
			break;

		case IDC_OPTION_PREVENT_POWERSAVE:
			if(this->p_toggle.prevent_power_save)
			{
				::KillTimer(this->p_window, 36115);
			}
			else
			{
				::SetTimer(this->p_window, 36115, 10000, nullptr);
			}
			this->p_toggle.prevent_power_save = !this->p_toggle.prevent_power_save;
			break;

		case IDC_OPTION_DISABLE_RESET:
			if(!::SetThreadExecutionState(ES_CONTINUOUS))
			{
				::MessageBox(this->p_window, TEXT("省電力タイマーのリセットの無効化に失敗しました。"), nullptr, MB_ICONWARNING);
			}
			break;

		case IDC_OPTION_TOGGLEVANISH:
			::SystemParametersInfo(SPI_SETMOUSEVANISH, 0, reinterpret_cast<PVOID>(static_cast<intptr_t>(this->p_toggle.cursor_vanish ? FALSE : TRUE)), SPIF_SENDWININICHANGE);
			break;

		case IDC_OPTION_PAUSE:
			if(!this->p_toggle.pause_update)
			{
				::KillTimer(this->p_window, 37564);
			}
			else
			{
				::SetTimer(this->p_window, 37564, 1000, nullptr);
			}
			this->p_toggle.pause_update = !this->p_toggle.pause_update;
			break;

		case IDC_HELP_ABOUT:
			::DialogBoxParam(this->p_instance, MAKEINTRESOURCE(IDD_ABOUT), this->p_window, AboutDialogProc, 0);
			break;

		case IDC_OPTION_TOGGLELEFTMENU:
			::SystemParametersInfo(SPI_SETMENUDROPALIGNMENT, 0, reinterpret_cast<PVOID>(static_cast<intptr_t>(this->p_toggle.left_menu ? FALSE : TRUE)), SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
			break;

		case IDC_VIEW_TOGGLE_NOACTIVE:
			this->p_list->ToggleExtendStylesMask(WS_EX_NOACTIVATE);
			this->p_list->Update();
			break;
		}
	}
}

void MainWindow::on_timer(UINT_PTR id)
{
	ULONG res;
	unsigned int c;
	TCHAR timer_res_text[32];

	switch(id)
	{
	case 398:
		if(!::QueryTimerResolutions(nullptr, nullptr, &res))
		{
			if(this->p_timer_res != res)
			{
				this->p_timer_res = res;
				::_stprintf_s(timer_res_text, 32, _T("タイマーの解像度:%u00 ns"), res);
				this->p_status->SetText(0, timer_res_text);
			}
		}
		break;

	case 36115:
		::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		break;

	case 37564:
		if(!::IsIconic(this->p_window))
		{
			this->p_list->Update();
			c = this->p_list->GetItemCount();
			if(c != this->p_item_count)
			{
				this->p_item_count = c;
				::_stprintf_s(timer_res_text, 32, _T("%u のウィンドウ"), this->p_item_count);
				this->p_status->SetText(1, timer_res_text);
			}
		}
		break;
	}
}

void MainWindow::on_enter_menu_loop()
{
	MENUITEMINFO item_info;
	DWORD style,exstyle;

	::memset(&item_info, 0, sizeof(MENUITEMINFO));

	item_info.cbSize = sizeof(MENUITEMINFO);
	item_info.fMask = MIIM_STATE;

	style = this->p_list->GetStylesMask();
	exstyle = this->p_list->GetExtendStylesMask();

	item_info.fState = (style & WS_VISIBLE) ? MFS_UNCHECKED : MFS_CHECKED;
	::SetMenuItemInfo(this->p_menu, IDC_VIEW_TOGGLE_INVISIBLE, FALSE, &item_info);

	item_info.fState = ((style & WS_POPUP) && (exstyle & WS_EX_TOOLWINDOW)) ? MFS_UNCHECKED : MFS_CHECKED;
	::SetMenuItemInfo(this->p_menu, IDC_VIEW_TOGGLE_TOOLBARS, FALSE, &item_info);

	item_info.fState = p_list->IsShowNoTitleWindow() ? MFS_CHECKED : MFS_UNCHECKED;
	::SetMenuItemInfo(this->p_menu, IDC_VIEW_TOGGLE_NOTITLE, FALSE, &item_info);

	item_info.fState = (exstyle & WS_EX_NOACTIVATE) ? MFS_UNCHECKED : MFS_CHECKED;
	::SetMenuItemInfo(this->p_menu, IDC_VIEW_TOGGLE_NOACTIVE, FALSE, &item_info);

	item_info.fState = MFS_GRAYED;
	::SetMenuItemInfo(this->p_menu, IDC_VIEW_TILE, FALSE, &item_info);

	this->p_toggle.ime_ui = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETSHOWIMEUI));
	this->p_toggle.cursor_shadow = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETCURSORSHADOW));
	this->p_toggle.hot_tracking = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETHOTTRACKING));
	this->p_toggle.window_shadow = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETDROPSHADOW));
	this->p_toggle.monitor_power = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETPOWEROFFACTIVE));
	this->p_toggle.monitor_power_low = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETLOWPOWERACTIVE));
	this->p_toggle.cursor_vanish = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETMOUSEVANISH));
	this->p_toggle.left_menu = TO_BOOLEAN(::GetSysParametersBoolean(SPI_GETMENUDROPALIGNMENT));

	exstyle = static_cast<DWORD>(::GetWindowLongPtr(this->p_window, GWL_EXSTYLE));

	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOPMOST, exstyle & WS_EX_TOPMOST);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLEIMEBAR, this->p_toggle.ime_ui);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLECURSHADOW, this->p_toggle.cursor_shadow);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLEHOTTRACK, this->p_toggle.hot_tracking);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLEWINDOWSHADOW, this->p_toggle.window_shadow);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLEMONITORPOWER, this->p_toggle.monitor_power);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLELOWMONITORPOWER, this->p_toggle.monitor_power_low);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_PREVENT_POWERSAVE, this->p_toggle.prevent_power_save);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLEVANISH, this->p_toggle.cursor_vanish);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_PAUSE, this->p_toggle.pause_update);
	::CheckMenuItem2(this->p_menu, IDC_OPTION_TOGGLELEFTMENU, this->p_toggle.left_menu);
}

void MainWindow::on_moving(RECT * r)
{
	POINT pt;

	::GetCursorPos(&pt);
	r->left = this->p_rect_start.left + (pt.x - this->p_dragcurpos.x);
	r->top = this->p_rect_start.top + (pt.y - this->p_dragcurpos.y);
	r->right = r->left + (this->p_rect_start.right - this->p_rect_start.left);
	r->bottom = r->top + (this->p_rect_start.bottom - this->p_rect_start.top);
	SnapWindow(this->p_window, r, 4, nullptr);
}

void MainWindow::on_enter_size_move()
{
	::GetCursorPos(&this->p_dragcurpos);
	::GetWindowRect(this->p_window, &this->p_rect_start);
}

LRESULT CALLBACK MainWindow::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MainWindow *_this;
	LRESULT r;

	_this = reinterpret_cast<MainWindow *>(::GetWindowLongPtr(hwnd, 0));
	r = 0;

	switch(uMsg)
	{
	case WM_CREATE: // 0x0001
		_this = reinterpret_cast<MainWindow *>(reinterpret_cast<const CREATESTRUCT *>(lParam)->lpCreateParams);
		_this->p_window = hwnd;
		if(!_this->on_create())
		{
			r = -1;
		}
		break;

	case WM_DESTROY: // 0x0002
		_this->on_destroy();
		break;

	case WM_MOVE: // 0x0003
		_this->on_move(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SIZE: // 0x0005
		_this->on_size(LOWORD(lParam), HIWORD(lParam));

		break;

	case WM_SETFOCUS: // 0x0007
		_this->p_list->GiveFocus();
		break;

	case WM_CLOSE: // 0x0010
		_this->on_close();
		break;

	case WM_SETTINGCHANGE: // 0x001A
		_this->on_winini_change();
		break;

	case WM_GETMINMAXINFO: // 0x0024
		if(_this)
		{
			_this->on_get_minmax_info(reinterpret_cast<MINMAXINFO *>(lParam));
		}
		break;

	case WM_NOTIFY: // 0x004E
		if(reinterpret_cast<const NMHDR *>(lParam)->idFrom == 1)
		{
			r = _this->p_list->Notify(reinterpret_cast<const NMHDR *>(lParam));
		}
		break;

	case WM_CONTEXTMENU: // 0x007B
		if(!_this->on_contextmenu(reinterpret_cast<HWND>(wParam), LOWORD(lParam), HIWORD(lParam)))
		{
			r = ::DefWindowProc(hwnd, WM_CONTEXTMENU, wParam, lParam);
		}
		break;

	case WM_COMMAND: // 0x0111
		_this->on_command(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
		break;

	case WM_TIMER: // 0x112
		_this->on_timer(static_cast<UINT_PTR>(wParam));
		break;

	case WM_LBUTTONUP: // 0x0202
	case WM_RBUTTONUP: // 0x0205
		_this->p_list->EndDragMode();
		break;

	case WM_ENTERMENULOOP: // 0x0211
		_this->on_enter_menu_loop();
		break;

	case WM_MOVING: // 0x0216
		_this->on_moving(reinterpret_cast<RECT *>(lParam));
		break;

	case WM_ENTERSIZEMOVE: // 0x0231
		_this->on_enter_size_move();
		break;

	case WM_THEMECHANGED: // 0x031A
		MessageBox(_this->p_window, TEXT("WM_THEMECHANGED"), TEXT("/"), MB_ICONASTERISK);
		_this->p_list->HandleThemeChange();
		break;

	default:
		r = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}

	return r;
}

MainWindow::MainWindow(HINSTANCE instance)
{
	TCHAR * file_path;
	HRESULT HResult;

	this->p_instance = instance;
	this->p_show_window = 0;

	this->p_list = nullptr;
	this->p_status = nullptr;
	this->p_menu = nullptr;
	this->p_window_menu = nullptr;

	this->p_min_client_area.x = 0;
	this->p_min_client_area.y = 0;

	this->p_timer_res = 0;
	this->p_item_count = 0;

	this->p_toggle.ime_ui = false;
	this->p_toggle.cursor_shadow = false;
	this->p_toggle.hot_tracking = false;
	this->p_toggle.window_shadow = false;
	this->p_toggle.monitor_power = false;
	this->p_toggle.monitor_power_low = false;
	this->p_toggle.prevent_power_save = false;
	this->p_toggle.cursor_vanish = false;
	this->p_toggle.pause_update = false;

	file_path = new TCHAR[32768];
	HResult = ::SHGetFolderPath(this->p_window, CSIDL_APPDATA, nullptr, 0, file_path);
	if(HResult != S_OK)
	{
		this->config = new ApplicationConfig();
	}
	else
	{
		::_tcscat_s(file_path, 32768, _T("\\WINTOOL.HIV"));
		this->config = new ApplicationConfig(file_path);
	}
	delete [] file_path;

	this->p_pos.x = CW_USEDEFAULT;
	this->p_size.cx = -1;

	this->config->GetWindowPosition(&this->p_pos);
	this->config->GetWindowSize(&this->p_size);
}

MainWindow::~MainWindow()
{
	this->config->SetWindowPosition(&this->p_pos);
	this->config->SetWindowSize(&this->p_size);

	delete this->config;
}

HWND MainWindow::ShowWindow2(int show_window)
{
	int _x, _y, _w, _h;

	_x = this->p_pos.x;
	_y = this->p_pos.y;
	_w = (this->p_size.cx > 0) ? this->p_size.cx : CW_USEDEFAULT;
	_h = this->p_size.cy;
	this->p_window = CreateWindowEx(0, TEXT("Window Tools"), TEXT("Window Tools"), WS_OVERLAPPEDWINDOW, _x, _y, _w, _h, nullptr, nullptr, p_instance, this);
	if(this->p_window)
	{
		::ShowWindow(this->p_window, show_window);
	}
	return this->p_window;
}

ATOM MainWindow::RegisterClass2(const WNDCLASSEX *wc)
{
	WNDCLASSEX window_class;

	window_class = window_class_t;
	window_class.hInstance = wc->hInstance;
	window_class.hIcon = wc->hIcon;
	window_class.hIconSm = wc->hIconSm;

	return RegisterClassEx(&window_class);
}

const WNDCLASSEX MainWindow::window_class_t = {
	/* cbSize        */ sizeof(WNDCLASSEX),
	/* style         */ 0,
	/* lpfnWndProc   */ MainWindow::window_proc,
	/* cbClsExtra    */ 0,
	/* cbWndExtra    */ sizeof(void*), /* ポインタを格納する領域を確保する */
	/* hInstance     */ nullptr,
	/* hIcon         */ nullptr,
	/* hCursor       */ nullptr,
	/* hbrBackground */ nullptr, //(HBRUSH)(1 + COLOR_3DFACE), // クライアント領域を子ウィンドウがすべて覆い尽くす場合はNULLで良い
	/* lpszMenuName  */ MAKEINTRESOURCE(IDM_VIEW),
	/* lpszClassName */ TEXT("Window Tools"),
	/* hIconSm       */ nullptr };

unsigned __int64 get_system_time()
{
	FILETIME current;

#if (NTDDI_VERSION >= NTDDI_WIN8)
	void (WINAPI * _GetSystemTimePreciseAsFileTime)(LPFILETIME);
	HMODULE kernel32;
	kernel32 = ::LoadLibrary(TEXT("Kernel32.dll"));
	_GetSystemTimePreciseAsFileTime = (void (WINAPI *)(LPFILETIME))::GetProcAddress(kernel32, "GetSystemTimePreciseAsFileTime");
	if(_GetSystemTimePreciseAsFileTime)
	{
		(*_GetSystemTimePreciseAsFileTime)(&current);
	}
	else
	{
		::GetSystemTimeAsFileTime(&current);
	}
	FreeLibrary(kernel32);
#else
	::GetSystemTimeAsFileTime(&current);
#endif
	return (static_cast<unsigned __int64>(current.dwHighDateTime) << 32) + static_cast<unsigned __int64>(current.dwLowDateTime);
}

LRESULT CALLBACK kbd_watchdog(int nCode, WPARAM wParam, LPARAM lParam)
{
	static unsigned __int64 last_exec;
	unsigned __int64 current_time;

	if(nCode == HC_ACTION)
	{
		current_time = get_system_time();

		if(current_time >= last_exec + 10000000ULL)
		{
			last_exec = current_time;
		}
		else
		{
			return ::CallNextHookEx(nullptr, HC_ACTION, wParam, lParam);
		}

		switch(wParam)
		{
		case VK_NONCONVERT:
			::LocalHeapDump();
			break;

		case VK_CONVERT:
			::DumpAllHeapsStatus();
			break;
		}
	}

	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

unsigned __stdcall main_window_thread(void * arg)
{
	auto rarg = new MainWindow(reinterpret_cast<const THREAD_PARAM *>(arg)->instance);
	auto window = rarg->ShowWindow2(reinterpret_cast<const THREAD_PARAM *>(arg)->show);
	unsigned rcode;
	if(window)
	{
		auto hook = ::SetWindowsHookEx(WH_KEYBOARD, kbd_watchdog, nullptr, GetCurrentThreadId());

		::SendMessage(window, WM_SETTINGCHANGE, 0, 0);

		MSG msg;
		// BOOLは符号付き整数なのでこのような比較でも問題はない
		while(::GetMessage(&msg, nullptr, 0, 0) > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		UnhookWindowsHookEx(hook);
		rcode = msg.wParam & 0xFF;
	}
	else
		rcode = 0;

	delete rarg;
	return rcode;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	WNDCLASSEX wc;
	THREAD_PARAM p;
	ATOM catom;
#if _WIN32_WINNT >= 0x600  && defined(ENABLE_THUMBNAIL)
	ATOM thumb_atom;
#endif
	int quit_code;
	bool loaded_oem_normal_icon;
	bool loaded_oem_small_icon;

	::LocalHeapInitialize();
	::InitComctl32();

	ntdll = ::LoadLibrary(TEXT("NTDLL.DLL"));
	if(ntdll)
	{
		u_NtQueryTimerResolution.FuncPointer = ::GetProcAddress(ntdll, "NtQueryTimerResolution");
	}

	// アイコンをリソースから読み込む
	// Windows95、WindowsNT4以降では小さいアイコンも読み込む必要がある
	wc.hInstance = hInstance;
	// LoadIconのアイコンサイズはSM_CXICON、SM_CYICON固定なので、LoadImage+GetSystemMetricsでサイズを指定して読み込む
	wc.hIcon = reinterpret_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXICON),   GetSystemMetrics(SM_CYICON),   0));
	if(wc.hIcon == NULL)
	{
		wc.hIcon = reinterpret_cast<HICON>(::LoadImage(NULL, MAKEINTRESOURCE(OIC_SAMPLE), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
		loaded_oem_normal_icon = (wc.hIcon != NULL);
	}
	else
	{
		loaded_oem_normal_icon = false;
	}
	wc.hIconSm = reinterpret_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
	if(wc.hIconSm == NULL)
	{
		wc.hIconSm = reinterpret_cast<HICON>(::LoadImage(NULL, MAKEINTRESOURCE(OIC_SAMPLE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
		loaded_oem_small_icon = (wc.hIconSm != NULL);
	}
	else
	{
		loaded_oem_small_icon = false;
	}

	catom = MainWindow::RegisterClass2(&wc);

#if _WIN32_WINNT >= 0x600 && defined(ENABLE_THUMBNAIL)
	thumb_atom = Thumbnail::RegisterClass2(&wc);
#endif

	p.instance = hInstance;
	p.show = nCmdShow;

	quit_code = static_cast<int>(main_window_thread(&p));

	// 後始末
#if _WIN32_WINNT >= 0x600 && defined(ENABLE_THUMBNAIL)
	UnregisterClass((LPCTSTR)thumb_atom,hInstance);
#endif

	::UnregisterClass(reinterpret_cast<LPCTSTR>(catom), hInstance);

	// アイコンの削除
	if(loaded_oem_small_icon)
	{
		::DestroyIcon(wc.hIconSm);
	}
	if(loaded_oem_normal_icon)
	{
		::DestroyIcon(wc.hIcon);
	}

	if(ntdll)
	{
		::FreeLibrary(ntdll);
	}

	::LocalHeapDestroy();

	return quit_code;
}
