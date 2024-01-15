#include <iostream>
#include <Windows.h>
#include <iomanip>
using namespace std;

DWORD ThreadAllocatorID, ThreadTrackerID;        //线程Allocator、Tracker的地址
HANDLE allo = CreateSemaphore(NULL, 0, 1, NULL); //信号量alloc，跟踪Allocator
HANDLE trac = CreateSemaphore(NULL, 1, 1, NULL); //信号量trac，跟踪Tracker
SYSTEM_INFO sysinfo;                             //GetSystemInfo返回SYSTEM_INFO结构体
LPVOID base_addr;                                //基址

DWORD WINAPI Allocator(LPVOID lpParam)
{
    int block = 16; //设定申请的大小，页数为16
    GetSystemInfo(&sysinfo);

    //researve
    WaitForSingleObject(allo, INFINITE);
    base_addr = VirtualAlloc(NULL, block * sysinfo.dwPageSize, MEM_RESERVE, PAGE_NOACCESS);
    ReleaseSemaphore(trac, 1, NULL);

    //commit
    WaitForSingleObject(allo, INFINITE);
    VirtualAlloc(base_addr, block * sysinfo.dwPageSize, MEM_COMMIT, PAGE_READWRITE);
    ReleaseSemaphore(trac, 1, NULL);

    //lock
    WaitForSingleObject(allo, INFINITE);
    VirtualLock(base_addr, block * sysinfo.dwPageSize);
    ReleaseSemaphore(trac, 1, NULL);

    //Unlock
    WaitForSingleObject(allo, INFINITE);
    VirtualUnlock(base_addr, block * sysinfo.dwPageSize);
    ReleaseSemaphore(trac, 1, NULL);

    //reset
    WaitForSingleObject(allo, INFINITE);
    VirtualAlloc(base_addr, block * sysinfo.dwPageSize, MEM_RESET, PAGE_READWRITE);
    ReleaseSemaphore(trac, 1, NULL);

    //decommit
    WaitForSingleObject(allo, INFINITE);
    VirtualFree(base_addr, block * sysinfo.dwPageSize, MEM_DECOMMIT);
    ReleaseSemaphore(trac, 1, NULL);

    //release
    WaitForSingleObject(allo, INFINITE);
    VirtualFree(base_addr, 0, MEM_RELEASE);
    ReleaseSemaphore(trac, 1, NULL);

    WaitForSingleObject(allo, INFINITE);
    ReleaseSemaphore(trac, 1, NULL);
    WaitForSingleObject(allo, INFINITE);
    return 0;
}

void cout_sysinfo()
{
    GetSystemInfo(&sysinfo); //返回关于当前系统的信息并输出
    cout << "--------system info--------" << endl;
    cout << "PageSize: " << sysinfo.dwPageSize << endl;
    cout << "NumberOfProcessors: " << sysinfo.dwNumberOfProcessors << endl;               //个数
    cout << "ProcessorType: " << sysinfo.dwProcessorType << endl;                         //类型
    cout << "MaximumApplicationAddress: " << sysinfo.lpMaximumApplicationAddress << endl; //最大寻址单元
    cout << "MinimumApplicationAddress: " << sysinfo.lpMinimumApplicationAddress << endl; //最小寻址单元
    cout << "ActiveProcessorMask: " << sysinfo.dwActiveProcessorMask << endl;             //掩码
    cout << "AllocationGranularity: " << sysinfo.dwAllocationGranularity << endl;         //粒度？
    cout << "ProcessorArchitecture: " << sysinfo.wProcessorArchitecture << endl;          //架构？
    cout << "ProcessorLevel: " << sysinfo.wProcessorLevel << endl;                        //等级
    cout << "ProcessorRevision: " << sysinfo.wProcessorRevision << endl;                  //版本
    cout << endl;
}

DWORD WINAPI Tracker(LPVOID lpParam)
{
    WaitForSingleObject(trac, INFINITE); //trac初始为1 由tracker先开始
    ReleaseSemaphore(allo, 1, NULL);

    for (int i = 0; i <= 6; i++)
    {
        WaitForSingleObject(trac, INFINITE);
        MEMORY_BASIC_INFORMATION mem;
        VirtualQuery(base_addr, &mem, sizeof(MEMORY_BASIC_INFORMATION));
        cout << "--------Virtual mamory states--------" << endl;
        cout << "AllocationBase: " << mem.AllocationBase << endl;
        cout << "AllocationProtect: " << mem.AllocationProtect << endl;
        cout << "base_address: " << mem.BaseAddress << endl;
        cout << "Protect: " << mem.Protect << endl;
        cout << "RegionSize: " << mem.RegionSize << endl;
        cout << "State: " << ((mem.State == 0x1000) ? "Commit" : ((mem.State == 0x2000) ? "Reserve" : "Free")) << endl;
        cout << "Type: " << ((mem.Type == 0x20000) ? "Private" : ((mem.Type == 0x40000) ? "Mappped" : "Image")) << endl;
        cout << endl;
        ReleaseSemaphore(allo, 1, NULL);
    }

    WaitForSingleObject(trac, INFINITE); //检测最后状态
    cout_sysinfo();
    ReleaseSemaphore(allo, 1, NULL);

    return 0;
}

int main()
{
    cout_sysinfo();
    HANDLE Allocator_thrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Allocator, NULL, 0, &ThreadAllocatorID);
    HANDLE Tracker_thrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Tracker, NULL, 0, &ThreadTrackerID);
    WaitForSingleObject(Allocator_thrd, INFINITE);

    CloseHandle(Allocator_thrd);
    CloseHandle(Tracker_thrd);
    CloseHandle(trac);
    CloseHandle(allo);
    system("pause");
    return 0;
}