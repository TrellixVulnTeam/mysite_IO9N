
// DateTimePickerTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CDateTimePickerTestApp: 
// �йش����ʵ�֣������ DateTimePickerTest.cpp
//

class CDateTimePickerTestApp : public CWinApp
{
public:
	CDateTimePickerTestApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CDateTimePickerTestApp theApp;