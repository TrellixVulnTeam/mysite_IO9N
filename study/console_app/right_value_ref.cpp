
#include "stdafx.h"
#pragma warning(disable:4996)
#include <list>  
#include <vector>  
#include <cstring>  
#include <iostream>  
#include <iostream>
#include <algorithm>
#include <xutility>
#include <type_traits>
using namespace std;

namespace
{
    template <typename T>
    struct Name;

    template <>
    struct Name<string>
    {
        static const char * get()
        {
            return "string";
        }
    };

    template <>
    struct Name<const string>
    {
        static const char * get()
        {
            return "const string";
        }
    };

    template <>
    struct Name<string&>
    {
        static const char * get()
        {
            return "string&";
        }
    };

    template <>
    struct Name<const string&>
    {
        static const char * get()
        {
            return "const string&";
        }
    };

    template <>
    struct Name<string&&>
    {
        static const char * get()
        {
            return "string&&";
        }
    };

    template <>
    struct Name<const string&&>
    {
        static const char * get()
        {
            return "const string&&";
        }
    };

    template <typename T>
    void quark(T&& t)
    {
        cout << "**********************************" << endl;
        cout << "t: " << t << endl;
        cout << "T: " << Name<T>::get() << endl;  // -->A  
        cout << "T&&: " << Name<T&&>::get() << endl;  // -->B  
        cout << endl;
    }

    string strange()
    {
        return "strange()";
    }

    const string charm()
    {
        return "charm()";
    }


    class bigobj
    {
    public:
        bigobj() { cout << ">> create obj " << endl; }
        bigobj(const bigobj& other) { cout << ">> copy create obj " << endl; }
        bigobj(bigobj&& other)
        {
            if (this != &other)
            {
                cout << ">> move create obj " << endl;
            }
        }

        bigobj& operator = (bigobj&& other)
        {
            cout << ">> operator = copy obj " << endl;
            return *this;
        }

    };


    class MemoryBlock
    {
    public:
        // Simple constructor that initializes the resource.  
        explicit MemoryBlock(size_t length)
            : _length(length)
            , _data(new int[length])
        {
            static int inc = 1000;
            id_ = ++inc;

            std::cout << "In MemoryBlock(size_t). id = " << id_ << ", length = "
                << _length << "." << std::endl;
        }

        // Destructor.  
        ~MemoryBlock()
        {
            std::cout << "In ~MemoryBlock(). id = " << id_ << ", length = "
                << _length << ".";

            if (_data != nullptr)
            {
                std::cout << " Deleting resource.";
                // Delete the resource.  
                delete[] _data;
            }

            std::cout << std::endl;
        }

        // Copy constructor.  
        MemoryBlock(const MemoryBlock& other)
            : _length(other._length)
            , _data(new int[other._length])
        {
            static int inc = -1000;
            id_ = --inc;

            std::cout << "In MemoryBlock(const MemoryBlock&). id = " << id_ << ", length = "
                << other._length << ". Copying resource." << std::endl;

            //std::copy(other._data, other._data + _length, _data);
        }

        // Copy assignment operator.  
        MemoryBlock& operator=(const MemoryBlock& other)
        {
            if (this != &other)
            {
                std::cout << "In operator=(const MemoryBlock&). id = " << id_ << ", length = "
                    << other._length << ". Copying resource. other.id = " << other.id_ << "." << std::endl;

                // Free the existing resource.  
                delete[] _data;

                _length = other._length;
                _data = new int[_length];
                //std::copy(other._data, other._data + _length, _data);
            }
            return *this;
        }

        // Retrieves the length of the data resource.  
        size_t Length() const
        {
            return _length;
        }

        MemoryBlock(MemoryBlock&& other)
            : _data(nullptr)
            , _length(0)
        {
            static int inc = 0;
            id_ = --inc;

            std::cout << "In MemoryBlock(MemoryBlock&&). id = " << id_ << ", length = "
                << other._length << ". Moving resource. other.id = " << other.id_ << "." << std::endl;

            //// Copy the data pointer and its length from the   
            //// source object.  
            //_data = other._data;
            //_length = other._length;

            //// Release the data pointer from the source object so that  
            //// the destructor does not free the memory multiple times.  
            //other._data = nullptr;
            //other._length = 0;


            // �����MemoryBlock& operator=(MemoryBlock&& other)�����ٴ�������
            *this = std::move(other);
        }

        // Move assignment operator.  
        MemoryBlock& operator=(MemoryBlock&& other)
        {
            if (this != &other)
            {
                std::cout << "In operator=(MemoryBlock&&). id = " << id_ << ", length = "
                    << other._length << "." << "other.id = " << other.id_ << "." << std::endl;

                // Free the existing resource.  
                delete[] _data;

                // Copy the data pointer and its length from the   
                // source object.  
                _data = other._data;
                _length = other._length;

                // Release the data pointer from the source object so that  
                // the destructor does not free the memory multiple times.  
                other._data = nullptr;
                other._length = 0;
            }
            return *this;
        }

    private:
        size_t _length; // The length of the resource.  
        int* _data; // The resource.  

        int id_;
    };
}

// https://blog.csdn.net/ink_cherry/article/details/72876767
// https://msdn.microsoft.com/zh-cn/library/dd293665.aspx?f=255&MSPPError=-2147217396
// https://www.cnblogs.com/boydfd/p/5182743.html
// https://blog.csdn.net/linwh8/article/details/51569807
//��Ϊ����ת������ָ�ں���ģ���У���ȫ����ģ��Ĳ������ͣ����������ݸ�����ģ���е�������һ��������
//������ת����ô�϶�Ҳ�в�����ת��������ڲ������ݵĹ����в����˶������ʱ���󿽱�����ô��ת��Ҳ���㲻������ת����
//Ϊ�˱���������������Ҫ�����������Է�ֹ�������ʱ����Ŀ�����
void right_ref_study()
{
    //string up("up");
    //const string down("down");
    //quark(up);  // -->1  
    //quark(down);    // -->2  
    //quark(strange());   // -->3  
    //quark(charm());     // -->4  

    /*list<bigobj> list;
    for (int i = 0; i < 3; i++)
    {
    bigobj obj;
    list.push_back(obj);
    }
    cout << "&&----------------------" << endl;
    for (int i = 0; i < 3; i++)
    {
    bigobj obj;
    list.push_back(std::move(obj));
    }*/

    // Create a vector object and add a few elements to it.  
    vector<MemoryBlock> v;
    v.push_back(MemoryBlock(25));

    std::cout << std::endl;
    v.push_back(MemoryBlock(75));

    // Insert a new element into the second position of the vector.  
    std::cout << std::endl;
    v.insert(v.begin() + 1, MemoryBlock(50));

    // std::move�������ذ����Ĳ���ת����һ����ֵ����std::forwardֻ���ض���������������ִ�����ת����
    std::string str0 = "asd";
    std::string str1 = std::move(str0);
    std::string str2 = std::forward<std::string>(str1);
    std::string str3 = "str3";
    str3 = std::forward<std::string>(str2);
    std::string str4 = "str4";
    str3 = std::move(str4);

    MemoryBlock mb = MemoryBlock(100);
    MemoryBlock cb = std::move(mb);

    std::cout << "copy vct -----------------------" << std::endl;
    vector<MemoryBlock> k = std::move(v);
    std::swap(v, k);
    std::cout << std::endl;
}
