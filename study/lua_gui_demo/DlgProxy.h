
// DlgProxy.h: ͷ�ļ�
//

#pragma once

class Clua_gui_demoDlg;


// Clua_gui_demoDlgAutoProxy ����Ŀ��

class Clua_gui_demoDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(Clua_gui_demoDlgAutoProxy)

	Clua_gui_demoDlgAutoProxy();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��

// ����
public:
	Clua_gui_demoDlg* m_pDialog;

// ����
public:

// ��д
	public:
	virtual void OnFinalRelease();

// ʵ��
protected:
	virtual ~Clua_gui_demoDlgAutoProxy();

	// ���ɵ���Ϣӳ�亯��

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(Clua_gui_demoDlgAutoProxy)

	// ���ɵ� OLE ����ӳ�亯��

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

