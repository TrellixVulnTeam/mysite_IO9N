
// DlgProxy.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "lua_gui_demo.h"
#include "DlgProxy.h"
#include "lua_gui_demoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Clua_gui_demoDlgAutoProxy

IMPLEMENT_DYNCREATE(Clua_gui_demoDlgAutoProxy, CCmdTarget)

Clua_gui_demoDlgAutoProxy::Clua_gui_demoDlgAutoProxy()
{
	EnableAutomation();
	
	// ΪʹӦ�ó������Զ��������ڻ״̬ʱһֱ���� 
	//	���У����캯������ AfxOleLockApp��
	AfxOleLockApp();

	// ͨ��Ӧ�ó����������ָ��
	//  �����ʶԻ���  ���ô�����ڲ�ָ��
	//  ָ��Ի��򣬲����öԻ���ĺ���ָ��ָ��
	//  �ô���
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(Clua_gui_demoDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(Clua_gui_demoDlg)))
		{
			m_pDialog = reinterpret_cast<Clua_gui_demoDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

Clua_gui_demoDlgAutoProxy::~Clua_gui_demoDlgAutoProxy()
{
	// Ϊ������ OLE �Զ����������ж������ֹӦ�ó���
	//	������������ AfxOleUnlockApp��
	//  ���������������⣬�⻹���������Ի���
	if (m_pDialog != NULL)
		m_pDialog->m_pAutoProxy = NULL;
	AfxOleUnlockApp();
}

void Clua_gui_demoDlgAutoProxy::OnFinalRelease()
{
	// �ͷ��˶��Զ�����������һ�����ú󣬽�����
	// OnFinalRelease��  ���ཫ�Զ�
	// ɾ���ö���  �ڵ��øû���֮ǰ�����������
	// ��������ĸ���������롣

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(Clua_gui_demoDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(Clua_gui_demoDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// ע��: ��������˶� IID_Ilua_gui_demo ��֧��
//  ��֧������ VBA �����Ͱ�ȫ�󶨡�  �� IID ����ͬ���ӵ� .IDL �ļ��е�
//  ���Ƚӿڵ� GUID ƥ�䡣

// {C720E8CB-F20E-427B-8D0F-50FBB9279154}
static const IID IID_Ilua_gui_demo =
{ 0xC720E8CB, 0xF20E, 0x427B, { 0x8D, 0xF, 0x50, 0xFB, 0xB9, 0x27, 0x91, 0x54 } };

BEGIN_INTERFACE_MAP(Clua_gui_demoDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(Clua_gui_demoDlgAutoProxy, IID_Ilua_gui_demo, Dispatch)
END_INTERFACE_MAP()

// IMPLEMENT_OLECREATE2 ���ڴ���Ŀ�� StdAfx.h �ж���
// {74EFA0ED-FA22-45B1-B6E5-F724269C1ED3}
IMPLEMENT_OLECREATE2(Clua_gui_demoDlgAutoProxy, "lua_gui_demo.Application", 0x74efa0ed, 0xfa22, 0x45b1, 0xb6, 0xe5, 0xf7, 0x24, 0x26, 0x9c, 0x1e, 0xd3)


// Clua_gui_demoDlgAutoProxy ��Ϣ�������
