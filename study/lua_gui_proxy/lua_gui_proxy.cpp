// lua_gui_proxy.cpp : ���� DLL �ĳ�ʼ�����̡�
//

#include "stdafx.h"
#include "lua_gui_proxy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO:  ����� DLL ����� MFC DLL �Ƕ�̬���ӵģ�
//		��Ӵ� DLL �������κε���
//		MFC �ĺ������뽫 AFX_MANAGE_STATE ����ӵ�
//		�ú�������ǰ�档
//
//		����: 
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// �˴�Ϊ��ͨ������
//		}
//
//		�˺������κ� MFC ����
//		������ÿ��������ʮ����Ҫ��  ����ζ��
//		��������Ϊ�����еĵ�һ�����
//		���֣������������ж������������
//		������Ϊ���ǵĹ��캯���������� MFC
//		DLL ���á�
//
//		�й�������ϸ��Ϣ��
//		����� MFC ����˵�� 33 �� 58��
//

// Clua_gui_proxyApp

BEGIN_MESSAGE_MAP(Clua_gui_proxyApp, CWinApp)
END_MESSAGE_MAP()


// Clua_gui_proxyApp ����

Clua_gui_proxyApp::Clua_gui_proxyApp()
{
	// TODO:  �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� Clua_gui_proxyApp ����

Clua_gui_proxyApp theApp;


// Clua_gui_proxyApp ��ʼ��

BOOL Clua_gui_proxyApp::InitInstance()
{
    // ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
    // ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
    //����Ҫ InitCommonControlsEx()��  ���򣬽��޷��������ڡ�
    //INITCOMMONCONTROLSEX InitCtrls;
    //InitCtrls.dwSize = sizeof(InitCtrls);
    //// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
    //// �����ؼ��ࡣ
    //InitCtrls.dwICC = ICC_WIN95_CLASSES;
    //InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	return TRUE;
}
