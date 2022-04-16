
// lua_gui_demoDlg.h : ͷ�ļ�
//

#pragma once
#include "lua.hpp"
#include <memory>

class Clua_gui_demoDlgAutoProxy;

namespace
{
    class ConsoleObject;
}

// Clua_gui_demoDlg �Ի���
class Clua_gui_demoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(Clua_gui_demoDlg);
	friend class Clua_gui_demoDlgAutoProxy;

// ����
public:
	Clua_gui_demoDlg(CWnd* pParent = NULL);	// ��׼���캯��
	virtual ~Clua_gui_demoDlg();

// �Ի�������
	enum { IDD = IDD_LUA_GUI_DEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	Clua_gui_demoDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()

private:
    BOOL m_bTracking;
    lua_State *m_lua;
    std::unique_ptr<ConsoleObject> console_;

    void ReloadPlugins();
public:
    virtual BOOL DestroyWindow();
    afx_msg void OnCommand(UINT uId);
    afx_msg void OnUpdateCommandUIRange(CCmdUI* uId);
    afx_msg void OnUpdateUIState(UINT /*nAction*/, UINT /*nUIElement*/);
};
