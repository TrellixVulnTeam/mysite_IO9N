#pragma once


// ��static��const����ı�������˽�б�������Щ������������ֻ�ڶ����Դ�ļ���
static initializer s_init_val;
const initializer c_init_val;

// ���������ռ��ﶨ��ı�����Ҳ��˽�б�����������ֻ�ڸ���Դ�ļ���
namespace {
    initializer p_init_val;
}

// û�м�static��const���ζ���ı�������ȫ�ֱ��������Ա�����Դ�ļ���ͬʹ�á��������Դ�ļ�����ʱ���������ӽ׶γ����ظ������������ҵ�һ���������ض���ķ��ţ������ӽ׶�ʧ��
//initializer g_init_val;

// extern����������ȫ�ֱ�����ֻ��Ҫ��һ��Դ�ļ��ж���һ�Σ�����Դ�ļ���ʹ�õ�ͬһ������
extern initializer e_init_val;