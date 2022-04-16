#include "stdafx.h"
#include <vector>
#include <utility>

using namespace std;

const int asize = 10;

void PrintArray(const int* a, int len)
{
    for (int i = 0; i < len; i++)
    {
        cout << *(a + i) << ",";
    }
    cout << endl;
}

// ����
void Quicksort(int a[], int left, int right) {// l ��������ʼλ�ã�e ����������ֹλ�á�
    if (left >= right) return;// ��֤��ʼλ������ֹλ��֮ǰ��
    int i = left, j = right;// ����ʼĩλ�ö����ı�ԭֵ���������������±�����
    int key = left;// key����ָ�룬��ʼʱָ���ʼλ�ã��������һ����������ġ�
    cout << "�ؼ����� " << a[key] << " ��" << endl;
    while (j > i)
    {
        // while (j > i) ��ÿһ��ѭ����Ϊ��ȷ����
        // �������ҵķ����ϣ���һ���Ȳ�����С������λ�ã�
        // ��������ķ����ϣ���һ���Ȳ������������λ�ã�

        while (j > i && a[j] >= a[key])
        {
            j--;// ������ұߵ���ǰ��
        }

        while (j > i && a[i] <= a[key])
        {
            i++;// ��ǰ���֪��key == i�������������ǰ�棬��һ������ִ������������������´�������ĶԱȻ�����һ����
        }

        if (i != j)
        {
            cout << "���ν��� " << a[i] << "[" << i << "]" << "-" << a[j] << "[" << j << "]" << " ��";
            swap(a[i], a[j]);// swap��һ���򵥵Ľ���������͵����qaq��

            PrintArray(a, asize);
        }
    }// i == j ʱ����whileѭ��

     // ���������⽨�����Լ��ֻ�ģ��һ��
    if (key != i)
    {
        swap(a[key], a[i]);// �Դ�ʱ��a[i] Ϊ�ֽ��ߣ�ǰ�����һ����a[i]С�������һ����a[i]��

        cout << "���η��� " << a[i] << "[" << i << "]" << " ��";
        PrintArray(a, asize);
    }

    // �����õݹ齫a[i]ǰ��ĺͺ���ķֱ��ź���
    Quicksort(a, left, i - 1);
    Quicksort(a, i + 1, right);
}

// ���ֲ���
int BinarySearch1(int a[], int value, int n)
{
    int low, high, mid;
    low = 0;
    high = n - 1;
    while (low <= high)
    {
        mid = (low + high) / 2;
        if (a[mid] == value)
            return mid;
        if (a[mid] > value)
            high = mid - 1;
        if (a[mid] < value)
            low = mid + 1;
    }
    return -1;
}

//���ֲ��ң��ݹ�汾
int BinarySearch2(int a[], int value, int low, int high)
{
    int mid = low + (high - low) / 2;
    if (a[mid] == value)
        return mid;
    if (a[mid] > value)
        return BinarySearch2(a, value, low, mid - 1);
    if (a[mid] < value)
        return BinarySearch2(a, value, mid + 1, high);
    return -1;
}

// ��ȡ��������λ���ϵ�ֵ���Ӹ�λ��ʼ����
std::vector<int> GetIntegerDigit(int i)
{
    if (0 >= i)
    {
        return {};
    }

    std::vector<int> vct;
    int num = 0;// ������λ
    int n = i;
    int c = 0;
    while (n > 0)
    {
        c = n % 10;

        vct.push_back(c);

        n = n / 10;
    }
    return vct;
}

void TestSort()
{
    int value;
    std::srand((unsigned int)time(0));
    int a[asize] = { 0 };
    for (int i = 0; i < asize; ++i)
    {
        a[i] = std::rand() % 100 + 1;

        if (i == 5)
        {
            value = a[i];
        }
    }

    cout << "��ʼ���飺" << endl;
    PrintArray(a, asize);

    cout << "���ţ�" << endl;
    Quicksort(a, 0, asize - 1);
    cout << "������ϣ�" << endl;
    PrintArray(a, asize);


    int idx = BinarySearch1(a, value, asize);
    cout << "���ֲ��ң�" << value << "[" << idx << "]" << endl;


    int i = 23456;
    auto vct = GetIntegerDigit(i);
    cout << "������λ��" << i << endl;
    for (auto& n : vct)
    {
        cout << n << ",";
    }
    cout << endl;
}
