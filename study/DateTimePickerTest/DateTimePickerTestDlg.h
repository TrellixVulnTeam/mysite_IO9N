
// DateTimePickerTestDlg.h : ͷ�ļ�
//

#pragma once
#include "afxdtctl.h"
#include "afxwin.h"
#include "explorer1.h"


// CDateTimePickerTestDlg �Ի���
class CDateTimePickerTestDlg : public CDialogEx
{
// ����
public:
	CDateTimePickerTestDlg(CWnd* pParent = NULL);	// ��׼���캯��
    virtual ~CDateTimePickerTestDlg();

// �Ի�������
	enum { IDD = IDD_DATETIMEPICKERTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
    virtual BOOL OnInitDialog();
    virtual BOOL DestroyWindow() override;
    virtual void OnOK() override;

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    CDateTimeCtrl m_DateTimeCtrl;
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    CButton btn_ok;
    CButton btn_cancel;
    CExplorer1 m_ieCtrl;
};
