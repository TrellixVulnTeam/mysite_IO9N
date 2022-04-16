#pragma once
#include <windows.h>

#include "data_encapsulation/smart_pointer.h"

namespace mctm
{
    class IOCP
    {
    public:
        struct IOItem
        {
            ULONG_PTR key = 0;
            OVERLAPPED* overlapped = nullptr;
            DWORD bytes_transfered = 0;
            DWORD error = 0;
        };

    public:
        explicit IOCP(unsigned int thread_count);
        ~IOCP();

        /************************************
        * Method:    RegisterIOHandler
        * FullName:  mctm::IOCP::RegisterIOHandler
        * Access:    public 
        * Returns:   bool
        * Parameter: HANDLE pipe_or_file
        * Parameter: IOHandler * handler
        * Remarks: �ϲ�Ӧ���Լ���֤HANDLE��IOHandler��Ψһ��Ӧ����Ϊ�ú�������handler��Ϊpipe_or_file
        *          �󶨵�iocp�ı�ʶ���������Ψһ��Ӧ����ô��OnIOCompleted�ص�ʱ�ϲ��û���ж����I/O���
        *          �����������ĸ�HANDLE�ġ�
        *          ���͵��÷�Ӧ���ǣ�����һ��IPCChannelʵ����װ��ά��һ��hPipeHandle��IPCChannel�̳�IOHandler��
        *          ���ڸ�IPCChannel::OnIOCompleted��������hPipeHandle���첽�ص�
        ************************************/
        bool RegisterIOHandle(HANDLE handle, ULONG_PTR key);

        bool GetIOItem(DWORD timeout, IOItem* item);

        operator HANDLE() const
        {
            return port_;
        }

    private:
        ScopedHandle port_;
    };
}

