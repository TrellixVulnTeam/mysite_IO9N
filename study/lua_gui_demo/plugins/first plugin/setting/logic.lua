--������һ��lua�ļ��ķ�ʽdofile��require
--[[��dofile��ʽ�����á���Ҫ�õ�Ŀ���ļ��ľ���·�������һ�ִ��һ��ű���Ϊ�˱���ִ�в���Ҫ�Ĳ�����
��Ŀ���ļ�����Ӧ�ý���ֻ�Ƕ������/��������Ӧ���κκ������ò���
]]--
local info = debug.getinfo(1, "S"); -- �ڶ������� "S" ��ʾ������ source,short_src���ֶΣ� ���������� "n", "f", "I", "L"�� ���ز�ͬ���ֶ���Ϣ 
local path = info.source;
path = string.sub(path, 2, -1) -- ȥ����ͷ��"@"    
path = string.match(path, "^.*\\") -- �������һ�� "/" ֮ǰ�Ĳ��� ������������Ҫ��Ŀ¼����
dofile(path .. "layout.lua");  -- ��Ȼ���ԡ����á�������Ҫ���ز�ִ�����ݽű�

--[[��require��ʽ�����������ļ�������ִ��һ�顣������lua�ļ���˳����Ӧlua�ļ����������������·���£�
����ļ�������һ����㼶��Ŀ¼��������Ŀ¼��������������Ŀ¼�£�����lua�ļ��ڲ���Ҫȷ��֪���Լ�������
������������·�������������΢�Ķ�һ��Ŀ¼�ṹ�������һһ�޸����ڲ�ȫ����lua�ļ�����
]]--
--require("plugins\\test plugin\\layout");   -- ֱ������lua�ļ���Ҫȷ��֪��/��ȡĿ���ļ�������exe�����·��������ִ�нű�


gui_proxy = require("lua_gui_proxy");
local widget = nil;

local cort = coroutine.create(function (str)

        widget:Edit_AppendText(IDC_EDIT_INPUT, string.format("cort start '%s'\r\n", str));

        local rs = coroutine.yield("yield first time");
        widget:Edit_AppendText(IDC_EDIT_INPUT, string.format("yield param = '%s'\r\n", rs));
        
        return "cort end"
    end)

local function resumecort(str)
    local state, ret = coroutine.resume(cort, str);
    widget:Edit_AppendText(IDC_EDIT_INPUT, string.format("resume state = %s, ret = '%s'\r\n", 
        state and "true" or "false", ret));
end

function main(lua)
    widget = gui_proxy:CreateWidget(lua);
    print("lua lua lua lua lua");

    --ret = widget:DoModal(lua);
    --widget = nil;
    --collectgarbage("collect");

    ret = widget:Create();
    
    return ret;
end

function OnKickIdle()
    if widget ~= nil then
        if widget:IsDlgButtonChecked(IDC_CHECK_ENABLE) == true then
            widget:SetDlgItemEnable(IDC_RADIO_FIRST, true);
            widget:SetDlgItemEnable(IDC_RADIO_SECOND, true);
        else
            widget:SetDlgItemEnable(IDC_RADIO_FIRST, false);
            widget:SetDlgItemEnable(IDC_RADIO_SECOND, false);
        end
    end
end

function OnCommand(id, code)
    if widget ~= nil then
        if id == IDC_BUTTON_OK then
                    
            local cb = widget:IsDlgButtonChecked(IDC_CHECK_ENABLE);
            local r1 = widget:IsDlgButtonChecked(IDC_RADIO_FIRST);
            local r2 = widget:IsDlgButtonChecked(IDC_RADIO_SECOND);
            local edit_text = widget:Edit_GetText(IDC_EDIT_INPUT);

            --gui_proxy:MessageBox(string.format("checkbox=%s, radiofirst=%s, radiosecond=%s, edittext='%s'", 
            --    cb and "true" or "false", 
            --    r1 and "true" or "false", 
            --    r2 and "true" or "false", 
            --    edit_text));

            resumecort("resume");

        elseif id == IDC_BUTTON_CANCEL then
            --widget:EndDialog(2);
            widget:DestroyWindow();
        elseif id == IDCANCEL then
            gui_proxy:MessageBox("cancel button click");
        else
            --gui_proxy:MessageBox(string.format("lua OnCommand(%d, %d)", id, code));
        end
    end
end

function OnMouseMove(x, y)
    if widget ~= nil then
        widget:SetDlgItemText(IDC_STATIC_STATUE, string.format("%d, %d", x, y));
    end
end

function OnMouseExit()
    if widget ~= nil then
        widget:SetDlgItemText(IDC_STATIC_STATUE, "mouse leave");
    end
end

