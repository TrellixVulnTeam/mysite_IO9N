// lua_gui_proxy.h : lua_gui_proxy DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Clua_gui_proxyApp
// �йش���ʵ�ֵ���Ϣ������� lua_gui_proxy.cpp
//

class Clua_gui_proxyApp : public CWinApp
{
public:
	Clua_gui_proxyApp();

// ��д
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern Clua_gui_proxyApp theApp;