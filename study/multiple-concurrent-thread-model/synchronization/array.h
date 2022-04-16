#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace mctm
{
    // ѭ������
    // 
    // ��������ߵ�����£�д����Ӧ����ͬ������������������ʽ�������ǻᷢ��ͬʱд�������
    // ��Ϊ���������ͨ��CAS����������ֻ��ԭ�ӵĵõ�Ҫ�������ڴ��ַ�����������ѭ�����飬��ô����û���⣬
    // ��������ѭ�����飬���������write�������ڴ������ص����������ʱ���ǻᷢ��д��ͻ��
    // 
    // ��������߿��Բ�������������Ӧ��Ҫ����ѭ����������ڴ������ص�Ҳû��ϵ
    template<class T>
    class CycleArray
    {
    public:
        explicit CycleArray(size_t capacity)
            : capacity_(capacity),
            vector_(capacity_)
        {
        }

        void write(const T* elem, size_t len)
        {
            if (len <= 0)
            {
                return;
            }

            // Ҫд�����ݳ��������ȵĻ���ֻд�������ݵ������һ���ĳ��ȣ���������һ��write����ȴ����������ݸ�д�����
            if (len > capacity_)
            {
                elem = elem + len - capacity_;
                len = capacity_;
            }

            std::lock_guard<std::mutex> lock(vct_mutex_);

            unsigned long long wc;
            do 
            {
                wc = write_total_;
            } while (!write_total_.compare_exchange_strong(wc, wc + len));

            size_t woft = wc % capacity_;    // �õ���ʼдƫ��

            // ����len������Ҫ�������ڴ��
            wc = len;
            while (wc > 0)
            {
                unsigned long long cc = capacity_ - woft;
                cc = std::min(cc, wc);

                for (size_t i = 0; i < cc; ++i)
                {
                    vector_[i + woft] = *(elem + i);
                }

                wc -= cc;

                woft += (size_t)cc;
                if (woft >= capacity_)
                {
                    woft = 0;
                }
            }
        }

        T read()
        {
            unsigned long long rc;
            do
            {
                rc = read_total_;
            } while (!read_total_.compare_exchange_strong(rc, rc + 1));

            size_t oft = rc % capacity_;    // �õ���ʼ��ƫ��

            return vector_[oft];
        }

        std::vector<T> read(size_t len)
        {
            if (len <= 0)
            {
                return {};
            }

            std::vector<T> vct;
            vct.resize(len);
            unsigned long long oc;
            do
            {
                oc = read_total_;
            } while (!read_total_.compare_exchange_strong(oc, oc + len));

            size_t oft = oc % capacity_;    // �õ���ʼ��ƫ��

            // ����len������Ҫ�������ڴ��
            oc = len;
            size_t idx = 0;
            while (oc > 0)
            {
                unsigned long long cc = capacity_ - oft;
                cc = std::min(cc, oc);

                for (size_t i = 0; i < cc; ++i)
                {
                    vct[idx + i] = vector_[oft + i];
                }

                oc -= cc;
                idx += cc;
                oft += (size_t)cc;
                if (oft >= capacity_)
                {
                    oft = 0;
                }
            }

            return vct;
        }

    private:
        unsigned long capacity_ = 0;
        std::atomic_ullong read_total_ = 0;
        std::atomic_ullong write_total_ = 0;    // ֻ������������д��������ÿ��д���ȶ�capacity_��ģ�õ������дƫ��
        std::vector<T> vector_;
        std::mutex vct_mutex_;
    };


    //template<class T>
    //class Array
    //{
    //    explicit CycleArray(size_t capacity)
    //        : capacity_(capacity),
    //        vector_(capacity_)
    //    {
    //    }

    //    bool push_back(const T& elem)
    //    {
    //        size_t cs = size_;

    //        do 
    //        {
    //            cs = size_;
    //            if (cs >= capacity_)
    //            {
    //                // the queue is full
    //                return false;
    //            }
    //        } while (!size_.compare_exchange_strong(cs, cs + 1));

    //        cs = cs + 1;
    //        vector_[cs] = elem;
    //    }

    //    void pop()
    //    {
    //        size_t cs = size_;

    //        do
    //        {
    //            cs = size_;
    //            if (cs <= 0)
    //            {
    //                // the queue is empty
    //                return;
    //            }
    //        } while (!size_.compare_exchange_strong(cs, cs - 1));

    //        cs = cs - 1;
    //        vector_[cs] = elem;
    //    }

    //    T at(size_t index)
    //    {
    //        if (index >= size_)
    //        {
    //            throw std::exception("invalid param");
    //        }
    //    }

    //private:
    //    std::atomic_uint size_ = 0;
    //    size_t capacity_ = 0;
    //    std::vector<T> vector_;
    //};
}


void TestCycleArray();