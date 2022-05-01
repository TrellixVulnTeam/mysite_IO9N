#include "stdafx.h"
#include <string>

#include "def.h"


namespace
{
    class BaseClass
    {
    public:
        explicit BaseClass(const char* str)
            : str_(str)
        {
        }

        virtual ~BaseClass() = default;

        void print() const
        {
            printf("%s \n", str_.c_str());
        }

        std::string* operator->()
        {
            return &str_;
        }

        std::string& operator*()
        {
            return str_;
        }

        // ��ʽת��������
        operator std::string()
        {
            return str_;
        }

        virtual void fly() = 0
        {
            printf("BaseClass::fly() \n");
        }
        virtual void fly(int dst)
        {
            printf("fly to dst \n");
        }

    private:
        std::string str_;
    };

    void test_BaseClass_func(const BaseClass &base)
    {
        base.print();
    }

    void test_string_func(const std::string &str)
    {
        printf("%s \n", str.c_str());
    }

    class DervClass : public BaseClass
    {
    public:
        using BaseClass::fly;

        explicit DervClass(const char* str)
            : BaseClass(str)
        {
        }

        virtual void fly() override
        {
            BaseClass::fly();

            printf("DervClass::fly() \n");
        }

    };

    class Mem
    {
    public:
        Mem()
        {
            mm = 1;
        }
        ~Mem()
        {
        }

        int mm;
    };

    class Mem1
    {
    public:
        Mem1()
        {
            mm = 1;
        }
        ~Mem1()
        {
        }

        int mm;
    };

    class Base
    {
    public:
        Base()
        {
            func1();
        }

        virtual ~Base()
        {
            func1();
        }

        virtual void func1() {
            printf("Base::func1 \n");
        }

        virtual void func2() {
            printf("Base::func2 \n");
        }

        Mem bb;
    };

    class Base1
    {
    public:
        Base1()
        {
        }

        ~Base1()
        {

        }

        Mem1 bb1;
    };

    class Dev : public Base1, public Base
    {
    public:
        Dev()
        {
            func1();
        }

        ~Dev()
        {
            func1();
        }

        virtual void func1() {
            printf("Dev::func1 \n");
        }

        Mem1 m1;
        Mem m;
    };
    
    class CtrlModel {
    public:

        virtual void func() = 0;
        virtual ~CtrlModel() = default;
    };
    class CtrlView {
    public:
        CtrlView(CtrlModel* model)
            : model_(model) {}

        ~CtrlView()
        {
            model_->func();	// _purecall
        }
    private:
        CtrlModel* model_;
    };
    class View : public CtrlModel
    {
    public:
        View()
        {
            child_.reset(new CtrlView(this));
        }
        ~View()
        {
        }

        void func() override {}
    private:
        std::unique_ptr<CtrlView> child_;
    };

    //��ӻ���A
    class A {
    public:
        virtual void print() {
            printf("A::print\n");
        }

        void printNoVir() {
            printf("A::printNoVir\n");
        }
    protected:
        int m_a;
        int m_aa;
    };
    //ֱ�ӻ���B
    class B : virtual public A {  //��̳�
    public:
        void print() override {
            printf("B::print\n");
        }

        virtual void pout() {
            printf("B::pout\n");
        }

        void printNoVir() {
            printf("B::printNoVir\n");
        }
    protected:
        int m_a;
        int m_b = 22;
    };
    //ֱ�ӻ���C
    class C : virtual public A {  //��̳�
    public:
        void print() override {
            printf("C::print\n");
        }

        virtual void pout() {
            printf("C::pout\n");
        }

        void printNoVir() {
            printf("C::printNoVir\n");
        }
    protected:
        int m_a;
        int m_c = 33;
    };
    //������D
    class D : public B, public C {
    public:
        D()
        {
            B::m_a = 1;
            C::m_a = 2;
            m_aa = 1122;
            A::m_a = -3;
        }
        void seta(int a) {
            B::m_a = a;
        }  //��ȷ
        void setb(int b) {
            m_b = b;
        }  //��ȷ
        void setc(int c) {
            m_c = c;
        }  //��ȷ
        void setd(int d) {
            m_d = d;
        }  //��ȷ

        void print() override {
            printf("D::print\n");
        }

        /*void printNoVir() {
            printf("D::printNoVir\n");
        }*/
    private:
        int m_d;
        //int m_a = -1;
    };
}

const int c_int = 1;
static int s_int = 1;
//int g_int = 1;

extern Mem gm;
extern Mem1 gm1;

Mem1 gm1;
Mem gm;

void effective_example()
{
    /*p_init_val.s_counter_--;
    printf("effective_example print p_init_val\n");
    p_init_val.print();
    s_init_val.s_counter_--;
    printf("effective_example print s_init_val\n");
    s_init_val.print();
    printf("effective_example print c_init_val address %p \n", &c_init_val);
    printf("effective_example print e_init_val address %p \n", &e_init_val);*/

    //test_BaseClass_func("effect"); // error, BaseClass's contruct with "explicit" flag

    //DervClass basec("test_string_func");
    //basec.fly();
    //basec.fly(1);
    //basec.print();
    //test_string_func(basec); // operator std::string()
    //printf("%s \n", basec->c_str()); // std::string* operator->()

    /*int ss = sizeof(Mem);
    Dev* dev = new Dev;
    Mem* bb = &dev->bb;
    Mem1* bb1 = &dev->bb1;
    Mem* m = &dev->m;
    Mem1* m1 = &dev->m1;
    delete dev;*/

    /*View* view = new View();
    delete view;*/

    // һ����ͨ���μ̳�
    // ����һ����̳С�һ����ͨ�̳�
    //   1����Ա������ʹ��
    //      D��Ҳ�л���Aͬ�������ģ�ֱ��ͨ��D���õĻ�����D��ı�����
    //      D�ౣ�л���A���������ݣ�������DҪͨ����ʽָ���������B/C�ķ�ʽ����A�ı������磺B::m_a = 1;��C::m_a = 2;
    // 
    //   2����ͨ��Ա������ʹ��
    //      D��Ҳ�л���Aͬ�������ģ�����D�ĺ������ȼ���ߣ�ֱ��ͨ��D���õľ���D��汾�ĺ�����
    //      D��û�л���Aͬ�������ģ�B/C������֮һ�еģ���ͨ��D���õ���B/C��ĺ�����
    //      D��û�л���Aͬ�������ģ�B/C����߶��еģ���������Ժ����ķ��ʲ���ȷ���ı������
    //      ������ʽ��ָ�����͵���ָ�����͵ĺ������磺d.A::printNoVir();��d.B::printNoVir();��d.C::printNoVir();
    //
    //   3���麯����ʹ��
    //      D����д��A���麯���ģ�ֱ��ͨ��D���õĻ�����D����д�İ汾��
    //      D��û����дA���麯����B/C������֮һ��д��A���麯������ͨ��D���õĻ��õ���B/C����д�İ汾;
    //      D��û����дA���麯����B/C����߶���д��A���麯������������Ժ����ķ��ʲ���ȷ���ı������
    //      ������ʽ��ָ�����͵���ָ�����͵ĺ������磺d.A::print();��d.B::print();��d.C::print();

    // ����������̳�
    //  1����Ա������ʹ��
    //      D��Ҳ�л���Aͬ�������ģ�ֱ��ͨ��D���õĻ�����D��ı�����
    //      D��ֻ���л���A��һ�����ݣ���ʹ������Dͨ����ʽָ���������B/C�ķ�ʽ����A�ı��������õ��Ķ���ͬһ����ַ���磺B::m_a = 1;��C::m_a = 2;������m_a=2;
    // 
    // �ܽ�
    // ������˫��̳� ���� ˫��ͨ�̳� ���� һ����ͨ�̳�һ����̳У�
    // ��Ժ������������麯����д������ͨ�������ǣ���������3�ֹ��ԣ�
    //    1���������и����������ĺ����ģ�����������ȼ���ߣ�ͨ����������õĺ��������������Լ��İ汾��
    //    2��������û�и����������ĺ����ģ������м����еĻ������������
    //       a.����м��඼�У������ʧ�ܣ����и���ʧ�����Ϳ��Է�Ϊ�̳й�ϵ����ȷ�ͺ������ò���ȷ��
    //          �� ����Ƕ���м��඼��д�˻����麯��������������������������������Ի���Ĳ���ȷ�̳С��ı������
    //          �� ����Ƕ���м��඼��ͬ����ͨ��������ôͨ����������к����ĵ��ô���������Ժ����ķ��ʲ���ȷ���ı������
    //       b.ֻ��һ���м������У���ô���õľ�������м���ĺ����汾��
    //    3��������ͨ����ʽָ���м����͵����м����͵ĺ����汾���� d.A::print();��d.B::print();��d.C::print()��
    // 
    // ��Գ�Ա������
    //    1���������и�����ͬ�������ģ�����������ȼ���ߣ�ֱ��ͨ�����������õľ����������Լ��ı�������������ĳ�Ա�ͻ���ĳ�Ա�ǻ�������ı�����
    //    2��������û�и�����ͬ�������ģ������м����еĻ����м���ı����ͻ���ı���Ҳ�ǻ�������ı�����Ҫ������Щ������Ҫ��ʽָ�����ͣ��磺B::m_a = 1;��A::m_a = 2;��
    //    3����������м��඼û�и�����ͬ���ı����ģ������������
    //       a.���м�����ֻ��һ��������̳л�����һ������ͨ�̳л��ࡱ�͡������м��඼����ͨ�̳л��ࡱ�����һ����
    //         �������໹�ǻᱣ�л�����������ݻ������ݣ����������ͨ����ʽָ���м����������������ݻ������ݣ��磺B::m_a = 1;��C::m_a = 2;
    //       b.�����м��඼����̳л��࣬��������ֻ����һ�ݻ������ݣ���ʹ������ͨ����ʽָ��������͵ķ�ʽ���û���ı��������õ��Ķ���ͬһ����ַ���磺B::m_a = 1;��C::m_a = 2;������m_a=2;

    D d;
    d.seta('c');
    d.A::print();
    d.B::print();
    d.C::print();

    d.A::printNoVir();
    d.B::printNoVir();
    d.C::printNoVir();

    //d.pout();
    d.print();
    //d.printNoVir();

}