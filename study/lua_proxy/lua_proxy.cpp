// lua_proxy.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"

#include "lua.hpp"
#include "LuaProxy.h"


// �ⲿģ��
//extern template class MyClass<int>;
//extern template int MyClass<int>::static_Add(int,int);
//extern template int sub(int t1, int t2);
//extern template void static_func<int>(int t1);// error C2129: ��̬������void static_func<int>(int)����������δ���塣

void stackDump(lua_State* L)
{
    printf("\nbegin dump lua stack \n");
    int i = 0;
    int top = lua_gettop(L);
    printf("total elem %d \n", top);
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
            printf(lua_toboolean(L, i) ? "true " : "false ");
        }
                           break;
        case LUA_TNUMBER: {
            printf("%g \n", lua_tonumber(L, i));
        }
                          break;
        /*case LUA_TTABLE:
        {
            while (lua_next(L, i) != 0)
            {

            }
        }
            break;*/
        default: {
            printf("%s \n", lua_typename(L, t));
        }
                 break;
        }
    }
    printf("end dump lua stack \n");
}

extern "C"
{
    int CLuaProxy_NotMenberFn(lua_State *lua)
    {
        printf("CLuaProxy_NotMenberFn \n");
        return 0;
    }

    int CLuaProxy_SayHello(lua_State *lua)
    {
        //�õ���һ������Ķ����������stack��ײ���
        ProxyWrapper **wrapper = (ProxyWrapper**)luaL_checkudata(lua, 1, "CLuaProxy");
        luaL_argcheck(lua, wrapper != NULL, 1, "invalid data");
        
        (*wrapper)->get()->SayHello();

        //���stack
        //lua_settop(lua, 0);

        //�����ݷ���stack�У���Luaʹ��
        //lua_pushstring(lua, "proxy say sth");

        return 0;
    }

    int CLuaProxy_gc(lua_State *lua)
    {
        //�õ���һ������Ķ����������stack��ײ���
        ProxyWrapper **wrapper = (ProxyWrapper**)luaL_checkudata(lua, 1, "CLuaProxy");
        if (wrapper)
        {
            delete (*wrapper);
        }
        return 0;
    }

    int CLuaProxy_tostring(lua_State *lua)
    {
        ProxyWrapper **wrapper = (ProxyWrapper**)luaL_checkudata(lua, 1, "CLuaProxy");
        luaL_argcheck(lua, wrapper != NULL, 1, "invalid data");

        lua_pushfstring(lua, "this is CLuaProxy info %d!", (*wrapper)->get()->ct());

        return 1;
    }

    luaL_Reg kProxyMemberFuncs[] =
    {
        { "NotMenberFn", CLuaProxy_NotMenberFn },
        { "SayHello", CLuaProxy_SayHello },
        { "__gc", CLuaProxy_gc },
        { "__tostring", CLuaProxy_tostring },
        { nullptr, nullptr }
    };

    int NewCLuaProxy(lua_State* lua)
    {
        // һ
        CLuaProxy **proxy = (CLuaProxy**)lua_newuserdata(lua, sizeof(CLuaProxy*));
        *proxy = new CLuaProxy();

        // ��
        //lua_pushlightuserdata(lua, new CLuaProxy());

        return 1;
    }

    int CreateCLuaProxy(lua_State *lua)
    {
        //����һ������ָ��ŵ�stack����ظ�Lua��ʹ�ã�userdata��λ��-1
        ProxyWrapper **wrapper = (ProxyWrapper**)lua_newuserdata(lua, sizeof(ProxyWrapper*));
        *wrapper = new ProxyWrapper(new CLuaProxy());
        /*CLuaProxy **proxy = (CLuaProxy**)lua_newuserdata(lua, sizeof(CLuaProxy*));
        *proxy = new CLuaProxy();*/

        //Lua->stack���õ�ȫ��Ԫ��λ��-1,userdata(proxy)λ��-2
        luaL_getmetatable(lua, "CLuaProxy");
        //��Ԫ��ֵ��λ��-2��userdata(proxy)��������-1��Ԫ��
        lua_setmetatable(lua, -2);

        return 1;
    }

    int AttchCLuaProxy(lua_State *lua)
    {
        //����һ������ָ��ŵ�stack����ظ�Lua��ʹ�ã�userdata��λ��-1
        if (lua_islightuserdata(lua, -1))
        {
            // light userdata����lua ��gc����ֻ��newuserdata������full userdata�Ź�lua gc����
            CLuaProxy *proxy = (CLuaProxy*)lua_touserdata(lua, -1);
            
            ProxyWrapper **wrapper = (ProxyWrapper**)lua_newuserdata(lua, sizeof(ProxyWrapper*));
            *wrapper = new ProxyWrapper(proxy, 1);

            //Lua->stack���õ�ȫ��Ԫ��λ��-1,userdata(proxy)λ��-2
            luaL_getmetatable(lua, "CLuaProxy");
            //��Ԫ��ֵ��λ��-2��userdata(proxy)��������-1��Ԫ��
            lua_setmetatable(lua, -2);

            return 1;
        }
        return 0;
    }

    int average(lua_State *lua)
    {
        double sum = 0;
        int num = lua_gettop(lua);//��ȡ�����ĸ���
        int actual_num = 0;
        for (int i = 1; i <= num; i++)
        {
            if (lua_isnumber(lua, i))
            {
                actual_num++;
                sum += lua_tonumber(lua, i);
            }
        }
        //���λ�ȡ���в���ֵ�����
        lua_pushnumber(lua, sum / actual_num);//��ƽ����ѹ��ջ����lua��ȡ

        printf("c++ average funtion\n");

        return 1;//���ط���ֵ������֪ͨluaӦ����ջ��ȡ����ֵ��Ϊ���ؽ��
    }

    int print_str(lua_State *lua)
    {
        if (lua_isstring(lua, -1))
        {
            const char *str = lua_tostring(lua, -1);
            printf("c++ print_str %s\n", str);
        }
        else if (lua_istable(lua, -1))
        {
            printf("c++ print_str table\n");
        }

        lua_pushstring(lua, "c++ pushstring");
        return 1;
    }

    luaL_Reg cFuntions[] =
    {
        { "CreateCLuaProxy", CreateCLuaProxy },
        { "AttchCLuaProxy", AttchCLuaProxy },
        { "NewCLuaProxy", NewCLuaProxy },
        { "average", average },
        { "print_str", print_str },
        { nullptr, nullptr }
    };

    __declspec(dllexport) int luaopen_lua_proxy(lua_State *lua)
    {
        //����ȫ��Ԫ����������˶�LUA_REGISTRYINDEX�Ĳ�������Ԫ���λ��Ϊ-1
        luaL_newmetatable(lua, "CLuaProxy");
        
        //��Ԫ����Ϊһ������ѹջ��λ��-1��ԭԪ��λ��-2
        lua_pushvalue(lua, -1);
        
        //����-2λ��Ԫ���__index������ֵΪλ��-1��Ԫ��������λ��-1��Ԫ��ԭԪ���λ��Ϊ-1
        lua_setfield(lua, -2, "__index");
        
        //����Ա����ע�ᵽԪ���У������ȫ��Ԫ������þ�ȫ������ˣ�
        luaL_setfuncs(lua, kProxyMemberFuncs, 0);

        // ע��ÿ�ĺ���
        luaL_newlib(lua, cFuntions);

        MyClass<int> mc;
        int a = mc.add(5, 2);
        int aa = MyClass<int>::static_Add(5, 2);

        int b = sub<int>(2, 1);

        static_func(3);

        return 1;
    }
}


void test()
{
    return;
}