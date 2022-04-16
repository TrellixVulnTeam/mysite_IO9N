// lua_demo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <process.h>

#include "lua.hpp"

#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <winuser.rh>

#include "lua_proxy/LuaProxy.h"

void stackDump(lua_State* L)
{
    std::cout << "begin dump lua stack" << std::endl;
    int i = 0;
    int top = lua_gettop(L);
    for (i = 1; i <= top; ++i)
    {
        int t = lua_type(L, i);
        switch (t)
        {
        case LUA_TSTRING: {
            printf("'%s' \n", lua_tostring(L, i));
        }
                          break;
        case LUA_TBOOLEAN: {
            printf(lua_toboolean(L, i) ? "true \n" : "false \n");
        }
                           break;
        case LUA_TNUMBER: {
            printf("%g \n", lua_tonumber(L, i));
        }
                          break;
        default: {
            printf("%s \n", lua_typename(L, t));
        }
                 break;
        }
    }
    std::cout << "end dump lua stack" << std::endl;
}

int lua_call_cpp_fn(lua_State *lua)
{
    printf("c++ lua_call_cpp_fn param order\n");
    //stackDump(lua);

    lua_pushnumber(lua, 22);
    lua_pushnumber(lua, 33);
    return 2;
}

bool test_lua()
{
    bool ret = false;
    static lua_State *lua = luaL_newstate();
    if (lua)
    {
        // ����Lua������
        luaL_openlibs(lua);

        // lua���е��õ�����������Ҫ��ע�ắ��
        lua_register(lua, "lua_call_cpp_fn", lua_call_cpp_fn);

        // ���ز�ִ�нű�
        int code = luaL_dofile(lua, "lua_script/test.lua");
        if (LUA_OK == code)
        {
            std::cout << "--------------cpp begin----------------" << std::endl;

            // ��ȫ��Ԫ����һ
            //lua_getfield(lua, LUA_REGISTRYINDEX, "CLuaProxy");
            // ��ȫ��Ԫ������
            lua_pushstring(lua, "CLuaProxy");            
            lua_gettable(lua, LUA_REGISTRYINDEX);
            if (lua_istable(lua, -1))
            {
                lua_getfield(lua, -1, "NotMenberFn");
                if (lua_isfunction(lua, -1))
                {
                    lua_pcall(lua, 0, 0, 0); // ��Ч
                }
            }

            //��ȡ����-------------------------------------------------------------
            lua_getglobal(lua, "g_member");   //string to be indexed
            std::cout << "g_member = " << lua_tostring(lua, -1) << std::endl;

            lua_getglobal(lua, "proxy_raw_obj");
            if (lua_isuserdata(lua, -1))
            {
                // һ,lua full userdata
                CLuaProxy **proxy = (CLuaProxy**)lua_touserdata(lua, -1);

                // ����lua light userdata
                //CLuaProxy *proxy = (CLuaProxy*)lua_touserdata(lua, -1);

                std::cout << "proxy_raw_obj = " << (*proxy)->ct() << std::endl;

                (*proxy)->Release();
            }

            //��ȡ��key-value---------------------------------------------------
            lua_getglobal(lua, "g_table");  //table to be indexed
            if (lua_istable(lua, -1))
            {
                lua_Integer num = luaL_len(lua, -1);

                //������֪keyȡ����Ԫ��
                lua_getfield(lua, -1, "name");
                std::cout << "g_table->name = " << lua_tostring(lua, -1) << std::endl;
                lua_pop(lua, 1);

                // ����table��keyΪ������͵Ĺ�ϣ��
                lua_pushnil(lua); // ����һ��������keyѹջ
                //lua_pushstring(lua, "name");  // ��key='name'����һ��k-v��ֵ�Կ�ʼ
                //lua_pushinteger(lua, 10101);// ��key=10101����һ��k-v��ֵ�Կ�ʼ
                while (lua_next(lua, -2))   // ����λ��Ϊ-1��key������key����һ��key-value��ѹջ��key=-2��value=-1�������ط�0
                {
                    int k_type = lua_type(lua, -2);
                    switch (k_type)
                    {
                    case LUA_TNUMBER:
                        if (lua_isinteger(lua, -2))
                        {
                            printf("g_table key = %lld(integer), ", lua_tointeger(lua, -2));
                        }
                        else if (lua_isnumber(lua, -2))
                        {
                            printf("g_table key = %g(number), ", lua_tonumber(lua, -2));
                        }
                        break;
                    case LUA_TSTRING:
                        printf("g_table key = '%s'(string), ", lua_tostring(lua, -2));
                        break;
                    default:
                        printf("g_table key type %s, ", lua_typename(lua, k_type));
                        break;
                    }

                    int v_type = lua_type(lua, -1);
                    switch (v_type)
                    {
                    case LUA_TNUMBER:
                        if (lua_isinteger(lua, -1))
                        {
                            printf("value = %lld(integer)\n", lua_tointeger(lua, -1));
                        }
                        else if (lua_isnumber(lua, -1))
                        {
                            printf("value = %g(number)\n", lua_tonumber(lua, -1));
                        }
                        break;
                    case LUA_TSTRING:
                        printf("value = '%s'(string)\n", lua_tostring(lua, -1));
                        break;
                    default:
                        printf("value type %s \n", lua_typename(lua, v_type));
                        break;
                    }

                    lua_pop(lua, 1);// ��value��ջ����ʱ��ǰkeyλ��Ϊ-1����һ��lua_next�Ը�keyΪ��׼λ�����±���
                }

                // ���ڻ�����͵Ĺ�ϣtable����key���������Σ���c++�������ȡ����ֵһ����key��������������ȡ���ƺ�ûʲô���壩
                // ����Ϊָ��key=1�������������Ч��luaL_len���õ���key=1��ʼ��������ֵ������������table�е����ж��ټ�ֵ��
                int type = lua_rawgeti(lua, -1, 10101);
                if (lua_istable(lua, -1))
                {
                    std::cout << "g_table->10101 is table " << std::endl;
                }
                lua_pop(lua, 1);
            }

            //�޸ı���Ԫ��
            lua_pushstring(lua, "bean");
            lua_setfield(lua, -2, "name");
            lua_getfield(lua, -1, "name");
            std::cout << "new table->name = " << lua_tostring(lua, -1) << std::endl;
            lua_pop(lua, 1);

            // ����key-value��ϣtable
            lua_newtable(lua);
            lua_pushinteger(lua, 22);
            lua_setfield(lua, -2, "p1");

            lua_pushnumber(lua, 33.0f);
            lua_setfield(lua, -2, "p2");

            lua_pushstring(lua, "c++ string");
            lua_setfield(lua, -2, "p3");

            lua_setglobal(lua, "cpp_map");

            // ��ȡ����---------------------------------------------------------------
            lua_getglobal(lua, "g_vector");  //table to be indexed
            if (lua_istable(lua, -1))
            {
                // ������������
                lua_Integer num = luaL_len(lua, -1);
                for (lua_Integer i = 1; i <= num; ++i)
                {
                    int type = lua_rawgeti(lua, -1, i);
                    switch (type)
                    {
                    case LUA_TNUMBER:
                        if (lua_isinteger(lua, -1))
                        {
                            printf("g_vector[%lld] lua_isinteger %lld\n", i, lua_tointeger(lua, -1));
                        }
                        else if (lua_isnumber(lua, -1))
                        {
                            printf("g_vector[%lld] lua_isnumber %g\n", i, lua_tonumber(lua, -1));
                        }
                        break;
                    case LUA_TSTRING:
                        printf("g_vector[%lld] lua_isstring %s\n", i, lua_tostring(lua, -1));
                        break;
                    default:
                        printf("g_vector[%lld] type is %s \n", i, lua_typename(lua, type));
                        break;
                    }
                    lua_pop(lua, 1);
                }

                // ���ڴ����飬��ʵluaĬ����key typeΪinteger�����Ҵ�1����
                lua_pushnil(lua); // ����һ��������keyѹջ
                while (lua_next(lua, -2))   // ����λ��Ϊ-1��key������key����һ��key-value��ѹջ��key=-2��value=-1�������ط�0
                {
                    int k_type = lua_type(lua, -2);
                    switch (k_type)
                    {
                    case LUA_TNUMBER:
                        if (lua_isinteger(lua, -2))
                        {
                            printf("g_vector key = %lld(integer), ", lua_tointeger(lua, -2));
                        }
                        else if (lua_isnumber(lua, -2))
                        {
                            printf("g_vector key = %g(number), ", lua_tonumber(lua, -2));
                        }
                        break;
                    case LUA_TSTRING:
                        printf("g_vector key = '%s'(string), ", lua_tostring(lua, -2));
                        break;
                    default:
                        printf("g_vector key type %s, ", lua_typename(lua, k_type));
                        break;
                    }

                    int v_type = lua_type(lua, -1);
                    switch (v_type)
                    {
                    case LUA_TNUMBER:
                        if (lua_isinteger(lua, -1))
                        {
                            printf("value = %lld(integer)\n", lua_tointeger(lua, -1));
                        }
                        else if (lua_isnumber(lua, -1))
                        {
                            printf("value = %g(number)\n", lua_tonumber(lua, -1));
                        }
                        break;
                    case LUA_TSTRING:
                        printf("value = '%s'(string)\n", lua_tostring(lua, -1));
                        break;
                    default:
                        printf("value type %s \n", lua_typename(lua, v_type));
                        break;
                    }

                    lua_pop(lua, 1);// ��value��ջ����ʱ��ǰkeyλ��Ϊ-1����һ��lua_next�Ը�keyΪ��׼λ�����±���
                }
            }

            // ��������
            lua_newtable(lua);
            lua_pushinteger(lua, 22);
            lua_rawseti(lua, -2, 1);

            lua_pushnumber(lua, 33.0f);
            lua_rawseti(lua, -2, 2);

            lua_pushstring(lua, "c++ string");
            lua_rawseti(lua, -2, 3);

            lua_setglobal(lua, "cpp_vector");

            // ȡ����-----------------------------------------------------------
            std::cout << "c++ call lua check_cpp_global" << std::endl;
            lua_getglobal(lua, "check_cpp_global");
            lua_pcall(lua, 0, 0, 0);

            std::cout << "c++ call lua add" << std::endl;
            lua_getglobal(lua, "add");
            lua_pushnumber(lua, 22);
            lua_pushnumber(lua, 33);
            lua_pcall(lua, 2, 1, 0);//2-����������1-����ֵ���������ú���������ִ���꣬�Ὣ����ֵѹ��ջ��
            std::cout << "add fn result = " << lua_tonumber(lua, -1) << std::endl;

            std::cout << "c++ call lua attch_proxy" << std::endl;
            CLuaProxy *proxy = new CLuaProxy();
            lua_getglobal(lua, "attch_proxy");
            lua_pushlightuserdata(lua, proxy);
            lua_pcall(lua, 1, 0, 0);//2-����������1-����ֵ���������ú���������ִ���꣬�Ὣ����ֵѹ��ջ��

            //// ������ע��ĺ���ͨ��getglobal���ò�����
            //if(LUA_OK == lua_getglobal(lua, "lua_call_cpp_fn"))
            //{
            //    std::cout << "c++ call lua_call_cpp_fn" << std::endl;
            //    lua_pushnumber(lua, 11);
            //    lua_pushnumber(lua, 22);
            //    lua_pcall(lua, 0, 1, 0);//2-������ʽ��1-����ֵ���������ú���������ִ���꣬�Ὣ����ֵѹ��ջ��
            //}

            proxy->Release();

            std::cout << "--------------cpp end----------------" << std::endl;

            ////�鿴ջ
            //stackDump(lua);

            ////�鿴ջ
            //stackDump(lua);

            //lua_pcall(lua, 0, LUA_MULTRET, 0);

            ret = true;
        }
        else
        {
            std::cout << lua_tostring(lua, -1) << std::endl;
        }

        //�ر�state
        //lua_close(lua);
    }
    return ret;
}

int _tmain(int argc, _TCHAR* argv[])
{
    /*MyClass<int> mc;
    int a = mc.add(1, 2);
    int b = sub<int>(2, 1);
    int c = nor(2 , 2);*/

    do
    {
        test_lua();
    } while (/*getchar()*//*getc(stdin)*/_getche() != VK_ESCAPE);
	return 0;
}

