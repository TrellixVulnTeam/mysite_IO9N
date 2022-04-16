
// lua_gui_demoDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "lua_gui_demo.h"
#include "lua_gui_demoDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"
#include <io.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <set>
#include "..\lua_gui_proxy\lua_object.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace
{
    class ConsoleObject
    {
    public:
        ConsoleObject()
            : m_sout(nullptr)
            , m_serr(nullptr)
        {
            int nCrt = 0;
            FILE* fp;
            AllocConsole();
            nCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
            fp = _fdopen(nCrt, "w"); //ON_COMMAND; WM_COMMAND
            *stdout = *fp;
            setvbuf(stdout, NULL, _IONBF, 0);

            /*AllocConsole();
            errno_t err;
            err = freopen_s(&m_sout, "CONOUT$", "w", stdout);
            err = freopen_s(&m_serr, "CONOUT$", "w", stderr);*/
        }

        ~ConsoleObject()
        {
            if (m_sout)
            {
                fclose(m_sout);
            }
            if (m_serr)
            {
                fclose(m_serr);
            }
        }

    protected:
    private:
        FILE* m_sout;
        FILE* m_serr;
    };

    enum
    {
        DYNAMIC_CTRL_ID_BEGIN = 1000,

        MENU_ID_BEGIN,
        MENU_ID_END = MENU_ID_BEGIN + 10000,

        DYNAMIC_CTRL_ID_END
    };
    
    const char* kInfosLua = "infos.lua";
    const char* kLogicLua = "logic.lua";
    const char* kLogicMainFunc = "main";

    struct plugin_info
    {
        plugin_info()
        {
            id = 0;
        }

        plugin_info(const plugin_info& right)
        {
            id = 1;

            plugin_folder = right.plugin_folder;
            menu_item_id_map = right.menu_item_id_map;
        }

        int id;
        std::string plugin_folder;
        std::map<UINT, int> menu_item_id_map;// ����в˵����������˵��е�ID -> ������Լ�����Ĳ˵�ID
    };

    UINT g_menu_id = MENU_ID_BEGIN;
    std::vector<plugin_info> g_plugins_vector;
}

// Clua_gui_demoDlg �Ի���


IMPLEMENT_DYNAMIC(Clua_gui_demoDlg, CDialogEx);

Clua_gui_demoDlg::Clua_gui_demoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Clua_gui_demoDlg::IDD, pParent)
    , m_bTracking(FALSE)
    , m_lua(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
}

Clua_gui_demoDlg::~Clua_gui_demoDlg()
{
	// ����öԻ������Զ���������
	//  ���˴���ָ��öԻ���ĺ���ָ������Ϊ NULL���Ա�
	//  �˴���֪���öԻ����ѱ�ɾ����
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;

    if (m_lua)
    {
        lua_close(m_lua);
        m_lua = nullptr;
    }
}

void Clua_gui_demoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Clua_gui_demoDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_COMMAND_RANGE(DYNAMIC_CTRL_ID_BEGIN, DYNAMIC_CTRL_ID_END, OnCommand)
    ON_WM_UPDATEUISTATE()
    ON_UPDATE_COMMAND_UI_RANGE(MENU_ID_BEGIN, MENU_ID_END, OnUpdateCommandUIRange)
END_MESSAGE_MAP()


// Clua_gui_demoDlg ��Ϣ�������

BOOL Clua_gui_demoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������

    //console_.reset(new ConsoleObject());

    // ��һ�ַ�ʽ����UI�Ĳ������ú���Ϣ������luaȥ����
    // ����������Ĵ��������ƣ����ַ�ʽӦ��ֻ�����ڽ��߼��벼�ַ������
    // �������ڳ���������
    m_lua = luaL_newstate();
    if (m_lua)
    {
        // ����Lua������
        luaL_openlibs(m_lua);
    }
    //ListBox_AddItemData;
    ReloadPlugins();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

BOOL Clua_gui_demoDlg::DestroyWindow()
{
    // TODO:  �ڴ����ר�ô����/����û���

    return CDialogEx::DestroyWindow();
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Clua_gui_demoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Clua_gui_demoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ���û��ر� UI ʱ������������Ա���������ĳ��
//  �������Զ�����������Ӧ�˳���  ��Щ
//  ��Ϣ�������ȷ����������: �����������ʹ�ã�
//  ������ UI�������ڹرնԻ���ʱ��
//  �Ի�����Ȼ�ᱣ�������

void Clua_gui_demoDlg::OnClose()
{
	if (CanExit())
		CDialogEx::OnClose();
}

void Clua_gui_demoDlg::OnOK()
{
	/*if (CanExit())
		CDialogEx::OnOK();*/
    //RestoreGuiLua();
    //int ret = 0;
    //std::string str;
    //if (m_lua)
    //{
    //    if (LUA_TFUNCTION == lua_getglobal(m_lua, "domodal"))
    //    {
    //        lua_pushlightuserdata(m_lua, m_hWnd);
    //        ret = lua_pcall(m_lua, 1, 1, 0);
    //        ret = lua_tointeger(m_lua, -1);//IDOK
    //    }
    //}
    ReloadPlugins();
}

void Clua_gui_demoDlg::OnCancel()
{
	if (CanExit())
		CDialogEx::OnCancel();
}

BOOL Clua_gui_demoDlg::CanExit()
{
	// �����������Ա�����������Զ���
	//  �������Իᱣ�ִ�Ӧ�ó���
	//  ʹ�Ի���������������� UI ����������
	if (m_pAutoProxy != NULL)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

void Clua_gui_demoDlg::ReloadPlugins()
{
    if (!m_lua)
        return;

    lua_settop(m_lua, 0);
    g_menu_id = MENU_ID_BEGIN;
    g_plugins_vector.clear();

    HMENU hMenu = CreateMenu();

    TCHAR szFilePath[MAX_PATH] = { 0 }; // MAX_PATH
    //GetModuleFileName(NULL, szFilePath, MAX_PATH);
    //(_tcsrchr(szFilePath, _T('\\')))[1] = 0;//ɾ���ļ�����ֻ���·��
    CString plugins_folder(szFilePath);

    CFileFind finder;
    BOOL bWorking = finder.FindFile(plugins_folder + L"plugins\\*.*");
    {
        while (bWorking)
        {
            bWorking = finder.FindNextFile();
            CString plugin_path = finder.GetFilePath();
            if (!finder.IsDots() && finder.IsDirectory())
            {
                if (m_lua)
                {
                    CStringA lua_path = CStringA(plugin_path + L"\\") + kInfosLua;
                    if (LUA_OK == luaL_dofile(m_lua, lua_path))
                    {
                        int type = lua_getglobal(m_lua, "name");
                        if (LUA_TSTRING == type)
                        {
                            plugin_info plugin;
                            plugin.plugin_folder = CStringA(plugin_path);

                            std::string name = lua_tostring(m_lua, -1);
                            HMENU hPluginMenu = CreateMenu();
                            ::AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPluginMenu, CString(name.c_str()));

                            type = lua_getglobal(m_lua, "menus");
                            if (LUA_TTABLE == type)
                            {
                                // ������������
                                lua_Integer num = luaL_len(m_lua, -1);
                                for (lua_Integer item = 1; item <= num; ++item)
                                {
                                    type = lua_rawgeti(m_lua, -1, item);
                                    if (type == LUA_TTABLE)
                                    {
                                        bool separator = true;
                                        int id;
                                        std::string text;
                                        bool enable = true;
                                        type = lua_getfield(m_lua, -1, "id");
                                        if (LUA_TNUMBER == type)
                                        {
                                            id = lua_tointeger(m_lua, -1);
                                            separator = false;
                                        }
                                        lua_pop(m_lua, 1);

                                        type = lua_getfield(m_lua, -1, "text");
                                        if (LUA_TSTRING == type)
                                        {
                                            text = lua_tostring(m_lua, -1);
                                        }
                                        lua_pop(m_lua, 1);

                                        type = lua_getfield(m_lua, -1, "enable");
                                        if (LUA_TBOOLEAN == type)
                                        {
                                            enable = lua_toboolean(m_lua, -1);
                                        }
                                        lua_pop(m_lua, 1);

                                        if (!separator)
                                        {
                                            int mid = g_menu_id++;
                                            plugin.menu_item_id_map[mid] = id;

                                            ::AppendMenu(hPluginMenu, MF_STRING | enable ? MF_ENABLED : MF_DISABLED,
                                                mid, CString(text.c_str()));
                                        }
                                        else
                                        {
                                            ::AppendMenu(hPluginMenu, MF_SEPARATOR, 0, nullptr);
                                        }
                                    }
                                    lua_pop(m_lua, 1);
                                }
                            }
                            lua_pop(m_lua, 1);

                            g_plugins_vector.push_back(plugin);
                        }
                        lua_pop(m_lua, 1);
                    }
                    else
                    {
                        std::string str = lua_tostring(m_lua, -1);
                        TRACE(lua_tostring(m_lua, -1));
                    }
                }
            }
        }
    }

    if (hMenu)
    {
        ::SetMenu(m_hWnd, hMenu);
    }
}

void Clua_gui_demoDlg::OnCommand(UINT uId)
{
    if (MENU_ID_BEGIN <= uId && MENU_ID_END >= uId)
    {
        for (plugin_info &var : g_plugins_vector)
        {
            if (var.menu_item_id_map.find(uId) != var.menu_item_id_map.end())
            {
                if (m_lua)
                {
                    lua_settop(m_lua, 1);
                    if (LUA_OK == luaL_dofile(m_lua, (var.plugin_folder + "\\" + kInfosLua).c_str()))
                    {
                        if (LUA_TFUNCTION == lua_getglobal(m_lua, "OnMenuSelected"))
                        {
                            lua_pushnumber(m_lua, (LUA_NUMBER)var.menu_item_id_map[uId]);
                            lua_pcall(m_lua, 1, 1, 0);

                            if (lua_isstring(m_lua, -1))
                            {
                                std::string dst_view = lua_tostring(m_lua, -1);
                                if (!dst_view.empty())
                                {
                                    RefLuaState lua = make_shared_lua_State();
                                    if (lua)
                                    {
                                        luaL_openlibs(lua.get());

                                        std::string path = var.plugin_folder + "\\" + dst_view + "\\" + kLogicLua;

                                        if (LUA_OK == luaL_dofile(lua.get(), path.c_str()))
                                        {
                                            if (LUA_TFUNCTION == lua_getglobal(lua.get(), kLogicMainFunc))
                                            {
                                                lua_pushlightuserdata(lua.get(), &lua);
                                                lua_pcall(lua.get(), 1, 1, 0);
                                                if (lua_isnumber(lua.get(), -1))
                                                {
                                                    if (-1 == lua_tointeger(lua.get(), -1))
                                                    {
                                                        AfxMessageBox(L"open plugin view faild!");
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            TRACE(lua_tostring(lua.get(), -1));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}

void Clua_gui_demoDlg::OnUpdateCommandUIRange(CCmdUI* uId)
{
    int id = uId->m_nID;
}

void Clua_gui_demoDlg::OnUpdateUIState(UINT /*nAction*/, UINT /*nUIElement*/)
{
    // �ù���Ҫ��ʹ�� Windows 2000 ����߰汾��
    // ���� _WIN32_WINNT �� WINVER ���� >= 0x0500��
    // TODO:  �ڴ˴������Ϣ����������
}
