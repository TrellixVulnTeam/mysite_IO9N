// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#include <iostream>

class initializer
{
public:
    initializer()
    {
        if (s_counter_++ == 0) init();
    }

    ~initializer()
    {
        if (--s_counter_ == 0) clean();
    }

    void print()
    {
        printf("s_counter_=%d \n", s_counter_);
    }

    int s_counter_ = 0;

private:
    void init() {}
    void clean() {}
};
