#pragma once


// CModalDialogTest �Ի���

class CModalDialogTest : public CDialogEx
{
	DECLARE_DYNAMIC(CModalDialogTest)

public:
	CModalDialogTest(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CModalDialogTest();

// �Ի�������
	enum { IDD = IDD_DATETIMEPICKERTEST_DIALOG };

    virtual void OnOK() override;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButton1();

private:
    static int inc_id_;
    int id_;
public:
    virtual BOOL OnInitDialog();
    virtual BOOL DestroyWindow() override;
};
