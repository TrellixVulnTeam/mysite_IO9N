
// lua_gui_demo.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Clua_gui_demoApp: 
// �йش����ʵ�֣������ lua_gui_demo.cpp
//

class Clua_gui_demoApp : public CWinApp
{
public:
	Clua_gui_demoApp();

// ��д
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Clua_gui_demoApp theApp;