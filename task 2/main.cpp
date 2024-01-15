#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string.h>
using namespace std;
#define totalnum 1000000
#define BAD_POS 0xFFFFFFFF
int d[1000000];

// quick sort
void exch(int &e1, int &e2)
{
    int tmp;
    tmp = e1;
    e1 = e2;
    e2 = tmp;
}

int partition(int *a, int left, int right)
{
    int i = left - 1;
    int j = right;
    int key = a[right]; //划分元素
    while (1)
    {
        while (a[++i] <= key)
            if (i == right)
                break; //向右寻找
        while (a[--j] >= key)
            if (j == left)
                break; //向左寻找
        if (i >= j)
            break; //划分位置
        exch(a[i], a[j]);
    }
    exch(a[i], a[right]); //划分元素就位
    return i;
}

void quick_sort(int *a, int left, int right)
{
    if (right <= left)
        return;
    int pos = partition(a, left, right);
    quick_sort(a, left, pos - 1);
    quick_sort(a, pos + 1, right);
}

// 数据分割
struct Range
{
    int left, right;
};
int *seq;                                               // 存排序结果数列
HANDLE threadnum = CreateSemaphore(NULL, 19, 21, NULL); // 限制产生的线程数不超过21个，初始时19个

DWORD WINAPI sort(LPVOID lpParam)
{
    Range *pN = (Range *)lpParam;
    if (pN->right - pN->left + 1 >= 1000)
    {
        int m = partition(seq, pN->left, pN->right);

        Range *subseq_1 = new Range;
        subseq_1->left = pN->left;
        subseq_1->right = m - 1;
        Range *subseq_2 = new Range;
        subseq_2->left = m + 1;
        subseq_2->right = pN->right;

        // threadnum大于0，则threadnum-1；否则等候到threadnum大于0再继续
        WaitForSingleObject(threadnum, INFINITE);
        HANDLE subthread_1 = CreateThread(NULL, 0, sort, subseq_1, 0, NULL); // 左序列新子线程
        WaitForSingleObject(subthread_1, INFINITE);
        CloseHandle(subthread_1);

        WaitForSingleObject(threadnum, INFINITE);
        HANDLE subthread_2 = CreateThread(NULL, 0, sort, subseq_2, 0, NULL); // 右序列新子线程
        WaitForSingleObject(subthread_2, INFINITE);
        CloseHandle(subthread_2);

        delete subseq_1;
        delete subseq_2;
    }
    else // 小于1000个的序列执行快排并释放线程，threadnum+1
    {
        quick_sort(seq, pN->left, pN->right);
        //cout << "1 ";
        ReleaseSemaphore(threadnum, 1, NULL);
    }
    ReleaseSemaphore(threadnum, 1, NULL);
    return 0;
}

int main()
{
    // 生成随机数
    ofstream fdata("seq.dat", ios_base::binary);
    for (int i = 0; i < totalnum; i++)
    {
        d[i] = rand();
        fdata.write((char *)&d[i], sizeof(int));
    }
    fdata.close();

    // 文件映射方式线程间数据共享
    HANDLE mmf = CreateFile(TEXT("seq.dat"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD error_code;
    if (mmf == INVALID_HANDLE_VALUE)
    {
        cout << "mmf error!" << endl;
    }
    else
    {
        DWORD high_size;
        DWORD file_size = GetFileSize(mmf, &high_size);
        error_code = GetLastError();
        if (file_size == BAD_POS && error_code != 0)
        {
            CloseHandle(mmf);
            cout << "error:" << error_code << endl;
        }
        cout << "mmf success" << endl;
        HANDLE mmfm = CreateFileMapping(mmf, NULL, PAGE_READWRITE, 0, file_size, 0);
        error_code = GetLastError();
        if (error_code != 0)
        {
            cout << "CreateFileMapping error!" << endl;
        }
        else
        {
            cout << "CreateFileMapping success" << endl;
            if (mmfm == NULL)
            {
                if (mmf != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(mmf);
                }
            }
            else
            {
                size_t view_size = file_size;
                DWORD view_access = FILE_MAP_WRITE;
                seq = (int *)MapViewOfFile(mmfm, view_access, 0, 0, view_size);
                if (seq == NULL)
                {
                    error_code = GetLastError();
                    if (error_code != 0)
                    {
                        cout << "MapViewOfFile error!" << endl;
                    }
                }
                else
                {
                    cout << "MapViewOfFile success" << endl;
                    // 构造第一个线程
                    Range *s = new Range;
                    s->left = 0;
                    s->right = totalnum - 1;
                    HANDLE thread_1 = CreateThread(NULL, 0, sort, s, 0, NULL);
                    WaitForSingleObject(thread_1, INFINITE);
                    CloseHandle(threadnum);
                    CloseHandle(thread_1);
                    delete s;

                    // 排序结果写到文件中
                    ofstream f_res("sort_result.txt", ios_base::out);
                    for (int i = 0; i < totalnum; i++)
                    {
                        f_res << seq[i] << " ";
                        if (i % 20 == 19)
                            f_res << endl;
                    }
                    f_res.close();
                    cout << "Finish writing sort_result.txt" << endl;
                    UnmapViewOfFile(seq);
                    CloseHandle(mmfm);
                    CloseHandle(mmf);
                }
            }
        }
    }
    system("pause");
    return 0;
}