#include <stdio.h>
#include <windows.h>
#include <fstream>
#include <ctime>
#include <iostream>
#include <iomanip>
using namespace std;

#define counter_num 6    //柜台数
#define max_customer 200 //最大顾客数量

int cus_input[max_customer][3] = {};  //输入信息
int cus_output[max_customer][6] = {}; //输出信息
int cus_arrive_num = 0;               //到达银行的顾客数
int cus_cntr_num = 0;
int cnt = 0; //被服务过的顾客数

HANDLE counter_thrd[counter_num];   //柜台线程
HANDLE customer_thrd[max_customer]; //顾客线程-----------能不能挪到int main里变成customer_num大小的
HANDLE customer_mutex = CreateMutex(NULL, FALSE, NULL);
HANDLE counter_mutex = CreateMutex(NULL, FALSE, NULL);
HANDLE CUS = CreateSemaphore(NULL, 0, max_customer, NULL);

time_t init_time = time(NULL);

DWORD WINAPI CUSTOMER(PVOID pv)
{
    Sleep(1000 * cus_input[(int)pv][1]);                   //还没到
    WaitForSingleObject(customer_mutex, INFINITE);         //取号
    cus_output[cus_arrive_num][0] = cus_input[(int)pv][0]; //输出第一列为顾客序号
    cus_output[cus_arrive_num][1] = (int)pv + 1;           //输出第二列为顾客取到的号
    cus_output[cus_arrive_num][2] = cus_input[(int)pv][1]; //输出第三列为顾客到达银行的时间
    cus_arrive_num++;
    ReleaseMutex(customer_mutex);
    ReleaseSemaphore(CUS, 1, NULL);
    return 0;
}

DWORD WINAPI COUNTER(PVOID pv)
{
    while (true)
    {
        time_t start_time, end_time;
        WaitForSingleObject(CUS, INFINITE);
        WaitForSingleObject(counter_mutex, INFINITE); //叫号
        int num_copy = cus_cntr_num;
        cus_cntr_num++;
        ReleaseMutex(counter_mutex);
        int sleep_time = cus_input[cus_output[num_copy][0] - 1][2]; //服务时长
        start_time = time(NULL);
        Sleep(1000 * sleep_time); //服务中
        end_time = time(NULL);
        cus_output[num_copy][3] = start_time - init_time; //输出第四列为顾客开始接受服务的时间
        cus_output[num_copy][4] = end_time - init_time;   //输出第五列为顾客离开银行的时间
        cus_output[num_copy][5] = (int)pv + 1;            //输出第六列为柜台编号
        cnt++;
    }
}

int main()
{
    //读入顾客信息
    ifstream infile;
    infile.open("input.txt");
    int customer_num = 0; //总顾客数
    while (infile.good())
    {
        for (int i = 0; i < 3; i++)
        {
            infile >> cus_input[customer_num][i];
            //cout << cus_in[customer_num][i] << ' ';
        }
        customer_num++;
    }
    infile.close();

    //创建进程
    for (int i = 0; i < counter_num; i++)
        counter_thrd[i] = CreateThread(NULL, 0, COUNTER, LPVOID(i), 0, NULL); //创建柜台线程
    for (int i = 0; i < customer_num; i++)
        customer_thrd[i] = CreateThread(NULL, 0, CUSTOMER, LPVOID(i), 0, NULL); //创建顾客线程

    //等待所有线程运行完
    //WaitForMultipleObjects(customer_num, customer_thrd, TRUE, INFINITE);
    while (cnt != customer_num)
        ;

    //输出
    for (int i = 0; i < customer_num; i++)
    {
        //cout << "序号" << endl;
        for (int j = 0; j < 6; j++)
        {
            cout << cus_output[i][j] << ' ';
        }
        cout << endl;
    }
    system("pause");
    return 0;
}