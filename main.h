﻿#pragma once

#include <Windows.h>
#include "WindowList.h"
#include "WindowStatusBar.h"
#include "AppConfig.h"

struct MAINWINDOW__TOGGLE
{
	bool ime_ui;
	bool cursor_shadow;
	bool hot_tracking;
	bool window_shadow;
	bool monitor_power;
	bool monitor_power_low;
	bool prevent_power_save;
	bool cursor_vanish;
	bool pause_update;
	bool left_menu;
};

class MainWindow
{
private:
	HINSTANCE p_instance;
	int p_show_window;
	HWND p_window;
	POINT p_pos;
	SIZE p_size;
	POINT p_dragcurpos;
	RECT p_rect_start;
	WindowList * p_list;
	WindowStatusBar * p_status;
	HMENU p_menu;
	HMENU p_window_menu;
	POINT p_min_client_area; //MINMAXINFO.ptMinTrackSizeに代入するためPOINTとする
	ULONG p_timer_res;
	unsigned int p_item_count;
	struct MAINWINDOW__TOGGLE p_toggle;
	ApplicationConfig * config;

public:
	MainWindow(HINSTANCE);
	~MainWindow();
	HWND ShowWindow2(int);
	static ATOM RegisterClass2(const WNDCLASSEX *);

private:
	bool on_create();
	void on_destroy();
	void on_move(unsigned int, unsigned int);
	void on_size(unsigned int, unsigned int);
	void on_close();
	void on_winini_change();
	void on_get_minmax_info(MINMAXINFO * const);
	bool on_contextmenu(HWND, unsigned int, unsigned int);
	void on_command(unsigned int, unsigned int, HWND);
	void on_timer(UINT_PTR);
	void on_enter_menu_loop();
	void on_moving(RECT * const);
	void on_enter_size_move();
	static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
	static const WNDCLASSEX window_class_t;
};
