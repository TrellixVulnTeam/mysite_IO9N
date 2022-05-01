#include "stdafx.h"
#include <algorithm>
#include <vector>
#include <excpt.h>


struct PublishedFileInfo
{
    enum PublishedFileHashType
    {
        md5,
        sha1,
    };
    std::string relate_path;// ���·��
    int hash_type = PublishedFileHashType::md5;  // �ļ�hash����
    std::string hash;   // �ļ�hashֵ
    __int64 length = 0; // �ļ���С

    bool operator ==(const PublishedFileInfo& _Right) const
    {
        return relate_path == _Right.relate_path &&
            hash_type == _Right.hash_type &&
            hash == _Right.hash &&
            length == _Right.length;
    }
};

void catch_error()
{
    PublishedFileInfo* ptr = 0;
    ptr->hash_type = 9;
}

void try_except_test()
{
    __try
    {
        catch_error();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        printf("asdsa");
    }
}

void vector_operation_study()
{
    PublishedFileInfo* ptr = 0;
    try
    {
        ptr->hash_type = 9;
    }
    catch (...)
    {
        printf("asdsa");
    }
    

    std::vector<PublishedFileInfo> local_files{
        {"a.exe", 0, "hash", 1234},
        {"c.exe", 0, "hashC", 1234},
        {"g.exe", 0, "hashG", 1234},
        {"j.exe", 0, "hashJ", 1234},
        {"l.exe", 0, "hashL", 1234}
    };
    std::vector<PublishedFileInfo> remote_files{
        {"a.exe", 0, "hash", 1234},
        {"b.exe", 0, "hashB", 5678},
        {"e.exe", 0, "hashE", 5678},
        {"g.exe", 0, "hashG", 5678},
        {"h.exe", 0, "hashH", 5678},
        {"j.exe", 0, "hashJ", 5678},
        {"k.exe", 0, "hashK", 5678}
    };

    // ��ǰ�����Ƚ��������򣬰����·������
    std::sort(local_files.begin(), local_files.end(), 
        [](const PublishedFileInfo& v1, const PublishedFileInfo& v2)->bool
        {
            return v1.relate_path < v2.relate_path;
        }
    );
    std::sort(remote_files.begin(), remote_files.end(), 
        [](const PublishedFileInfo& v1, const PublishedFileInfo& v2)->bool
        {
            return v1.relate_path < v2.relate_path;
        }
    );

    // ֱ����Զ�˷����ļ��б�Ա����ļ��б�Ĳ������ͨ����ɣ�����Ҫ�ԳƲ��
    // ����Զ�����ڱ���û�е���ѵ�һ�����ϵ�����������ϣ�ȫ��������Զ�˵�Ϊ׼������remote_files������Ϊ��һ�����ϲ�����
    // �ó�����Զ������Ϊ�����Ĳ�Ǳ�����������Ҫ������ıȽ��ж���
    std::vector<PublishedFileInfo> diff_files;
    std::set_difference(remote_files.begin(), remote_files.end(), local_files.begin(), local_files.end(),
        std::back_inserter(diff_files),
        [](const PublishedFileInfo& r_file, const PublishedFileInfo& l_file)->bool
        {
            return r_file.relate_path < l_file.relate_path;
        }
    );

    // �󽻼�������ͬ����ѵ�һ�����ϵ�����������ϣ�ȫ��������Զ�˵�Ϊ׼������remote_files������Ϊ��һ�����ϲ���
    std::vector<PublishedFileInfo> itsct_files;
    std::set_intersection(remote_files.begin(), remote_files.end(), local_files.begin(), local_files.end(),
        std::back_inserter(itsct_files),
        [](const PublishedFileInfo& r_file, const PublishedFileInfo& l_file)->bool
        {
            return r_file.relate_path < l_file.relate_path;
        }
    );

    // ���������б����Ա����ļ��б���бȽϣ�HASH��һ�µı������
    std::vector<PublishedFileInfo> copy_files;
    for (auto& r_file : itsct_files)
    {
        auto find_ret = std::find_if(local_files.begin(), local_files.end(),
            [&](const PublishedFileInfo& l_file)->bool
            {
                return r_file == l_file;
            }
        );

        // ·����ͬ�Ľ������ļ���һ���Ǿ���Ҫ���£�
        // һ�µ�ֱ�ӿ���һ�ݼ���
        if (find_ret == local_files.end())
        {
            diff_files.push_back(r_file);
        }
        else
        {
            copy_files.push_back(r_file);
        }
    }

    // ���ˣ������ļ��б��Զ���ļ��б�ԱȽ������Աȵó����������:
    // diff_files����������¼������Ҫ���ظ��µ��Զ�������Ļ���Զ�����޸ĵģ���
    // copy_files����������¼���ǲ���Ҫ���ظ��µ��ֱ�ӿ���һ�ݵ��°汾Ŀ��Ŀ¼���ɣ�
}