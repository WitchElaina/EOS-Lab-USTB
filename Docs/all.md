# EOS实验文档合集

---

## Lab1 操作系统启动

参考word文档

---

## Lab2 线程的状态和转换

### 预备知识

#### 线程都有哪些状态

- 就绪
- 运行
- 阻塞
- 结束

#### EOS是如何定义这些状态

EOS线程的状态由**线程控制块TCB**中的`State域`保存，在文件`ps/psp.h`中定义的线程状态如下

```cpp
 typedef enum _THREAD_STATE {
    Zero,       // 线程状态转换过程中的中间状态
    Ready,      // 就绪
    Running,    // 运行
    Waiting,    // 等待(阻塞)
    Terminated  // 结束
 } THREAD_STATE;
 ```

 #### 阅读相关源代码

 EOS中的线程转换最终都是由`ps/sched.c`中的`Psp*`函数执行的，可自行查阅对应源代码

#### 挂起状态、Resume原语、Suspend原语

为线程添加挂起状态，这就不可避免地涉及到了活动状态与挂起状态的转化，因此引入了两个原语：Resume与Suspend

Resume 负责将挂起状态的线程重新恢复到活动状态

Suspend 负责将活动状态的线程转入挂起状态

#### 临界

**临界资源** 指多个并发进程可以同时访问的硬件或软件资源

**临界区（Critical Section）** 访问临界资源的代码

**互斥体（Mutex）** 为保证各进程互斥进入临界区而引入的信号量

```
Lock Mutex
(Critical Section)
Relaese Mutex 
```

#### EOS内核同步

EOS中的线程属于内核级，所有线程并发执行，不区分所属进程

EOS提供了三种同步对象：

- 互斥对象（Mutex）
- 信号量对象（Semaphore）
- 事件对象（Event）

这三种同步对象都涉及到两个状态`signaled`和`nosignaled`，进程间同步本质上是对这两个状态进行切换，不同对象有不同的切换逻辑，具体可参考源代码

#### 线程调度

EOS中线程是调度的基本单位，算法为**基于优先级的抢先式调度**

EOS使用双向链表存储同一个优先级的队列，用数组存放32个这种双向链表，并用下标表示不同优先级大小，同时设置一个32位就绪位图表示索引为n的队列中是否有线程

对于不同优先级的线程，优先级越大越优先被调度，同一优先级中的线程则采用FCFS的方式“先来先服务”。

线程调度利用到了专门为调度设计的48号软中断，并最终由`PspSelectNextThread`函数决定是让被中断的线程继续执行，还是从所有“就绪”线程中选择一个来执行，这个函数就是**调度程序**

#### 控制台派遣线程

EOS启动后就会被创建，当发生键盘事件时才会被唤醒，将键盘事件派遣到对应控制台后继续阻塞状态等待下一次唤醒

### EOS线程状态转换

#### 核心源代码分析

##### 线程状态

对线程状态的定义如下，共包含5种状态。其中有四种有效状态，分别为：就绪 (Ready)、运行 (Running)、等待 (Waiting) 和结束 (Terminated)。Zero状态为线程状态转换中间态。代码中定义如下

```cpp
typedef enum _THREAD_STATE {
	Zero,		// 0
	Ready,		// 1
	Running,	// 2
	Waiting,	// 3
	Terminated	// 4
} THREAD_STATE;

#define TS_CREATE 	0	// 创建
#define TS_READY	1	// 就绪态
#define TS_RUNNING	2	// 运行态
#define TS_WAIT		3	// 阻塞态
#define TS_STOPPED	4	// 结束
```

##### 线程状态转换

**PspReadyThread** 将指定线程插入其优先级对应的就绪队列的队尾，并修改其状态码为`Ready`。

```c
VOID
PspReadyThread(
	PTHREAD Thread
	)
{
    // 确保线程非空
	ASSERT(NULL != Thread);
    // 确保线程处于游离态Zero或者运行态Running
	ASSERT(Zero == Thread->State || Running == Thread->State);

	// 将线程插入其优先级对应的就绪队列的队尾
	ListInsertTail(&PspReadyListHeads[Thread->Priority], &Thread->StateListEntry);
    // 设置就绪位图
    BIT_SET(PspReadyBitmap, Thread->Priority);
    // 将线程的状态修改为就绪状态
	Thread->State = Ready;

#ifdef _DEBUG
    // Debug Outputs
	RECORD_TASK_STATE(ObGetObjectId(Thread) , TS_READY, Tick);
#endif
}
```

**PspUnreadyThread** 将指定线程从就绪队列中移除，并修改其状态码为`Zero`。

```c
VOID
PspUnreadyThread(
	PTHREAD Thread
	)
{
    // 确保操作线程非空且处于就绪态
	ASSERT(NULL != Thread && Ready == Thread->State);
	// 将线程从所在的就绪队列中取出
	ListRemoveEntry(&Thread->StateListEntry);

    // 判断取出后线程优先级对应的就绪队列是否为空
	if(ListIsEmpty(&PspReadyListHeads[Thread->Priority])) {
        // 清除就绪位图中对应位
		BIT_CLEAR(PspReadyBitmap, Thread->Priority);
	}
    // 设置线程状态为Zero
	Thread->State = Zero;
}
```

**PspWait** 将当前运行线程插入指定等待队列的队尾，并修改状态码为 Waiting，然后执行线程调度，让出处理器。

```c
STATUS
PspWait(
	IN PLIST_ENTRY WaitListHead,
	IN ULONG Milliseconds
	)

{
    // 确保当前执行在非中断环境中
	ASSERT(0 == KeGetIntNesting());
    // 确保线程为运行态
	ASSERT(Running == PspCurrentThread->State);
    // 确保就绪位图非空
	ASSERT(0 != PspReadyBitmap);

    // 有限等待时间用尽，超时退出
	if(0 == Milliseconds) {
		return STATUS_TIMEOUT;
	}

    // 将当前线程插入等待队列的队尾
	ListInsertTail(WaitListHead, &PspCurrentThread->StateListEntry);
    // 修改线程状态为Waiting
	PspCurrentThread->State = Waiting;
	
#ifdef _DEBUG
    // Debug Outputs
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_WAIT, Tick);
#endif

	if (INFINITE != Milliseconds) {
        // 注册一个用于超时唤醒线程的等待计时器
		KeInitializeTimer( &PspCurrentThread->WaitTimer,
						   Milliseconds,
						   PspOnWaitTimeout,
						   (ULONG_PTR)PspCurrentThread );
		KeRegisterTimer(&PspCurrentThread->WaitTimer);
		PspCurrentThread->WaitStatus = STATUS_TIMEOUT;
	} else {
        // 永久等待
		PspCurrentThread->WaitStatus = STATUS_SUCCESS;
	}


	// 当前线程进入等待状态后执行线程调度。
	PspThreadSchedule();

	// 线程被唤醒，返回等待结果状态码。
	return PspCurrentThread->WaitStatus;
}
```

**PspUnwaitThread** 将阻塞线程从其所在的等待队列中移除，并修改其状态码为`Zero`。

```c
VOID
PspUnwaitThread(
	IN PTHREAD Thread
	)
{
    // 确保线程处于等待状态
	ASSERT(Waiting == Thread->State);

	// 将线程从所在等待队列中移除
	ListRemoveEntry(&Thread->StateListEntry);
    // 修改线程状态为Zero
	Thread->State = Zero;

	if (STATUS_TIMEOUT == Thread->WaitStatus) {
        // 注销等待计时器
		KeUnregisterTimer(&Thread->WaitTimer);
	}
}
```

**PspWakeThread** 该函数会先调用`PspUnwaitThread`函数使线程脱离阻塞状态，然后再调用`PspReadyThread`函数使线程进入就绪状态，从而唤醒被阻塞的线程。

```c
PTHREAD
PspWakeThread(
	IN PLIST_ENTRY WaitListHead,
	IN STATUS WaitStatus
	)

{
	PTHREAD Thread;

    // 等待队列非空
	if (!ListIsEmpty(WaitListHead)) {
		// 选择队首线程
		Thread = CONTAINING_RECORD(WaitListHead->Next, THREAD, StateListEntry);
		// 唤醒线程
        PspUnwaitThread(Thread);
		PspReadyThread(Thread);

		// 设置线程返回值
		Thread->WaitStatus = WaitStatus;

	} else {
		Thread = NULL;
	}
	return Thread;
}
```

**PspSelectNextThread** 线程调度程序函数，使被抢先的线程从运行状态进入就绪状态，并决定哪个就绪线程应该进入运行状态。

```c
PCONTEXT
PspSelectNextThread(
	VOID
	)
{
	ULONG HighestPriority;
	SIZE_T StackSize;

	// 扫描就绪位图，获得当前最高优先级。
	BitScanReverse(&HighestPriority, PspReadyBitmap);

    // 当前线程不为空且处于运行态
	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {

        // 就绪位图非空
		if (0 != PspReadyBitmap && HighestPriority > PspCurrentThread->Priority) {
			// 将当前线程插入其对应优先级就绪队列的队首
			ListInsertHead( &PspReadyListHeads[PspCurrentThread->Priority],
							&PspCurrentThread->StateListEntry );
			BIT_SET(PspReadyBitmap, PspCurrentThread->Priority);
			PspCurrentThread->State = Ready;

// Debug info
#ifdef _DEBUG
			RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_READY, Tick);
#endif

		} else {
			// 继续当前线程
			MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
			return &PspCurrentThread->KernelContext;
		}

	} else if(0 == PspReadyBitmap) {

		//
		// 被中断运行线程处于非运行状态，必须存在一个可运行的就绪线程。
		//
		ASSERT(FALSE);
		KeBugCheck("No ready thread to run!");
	}

    // 当前线程不为空
	if (NULL != PspCurrentThread) {
		// 当前线程结束
		if (Terminated == PspCurrentThread->State) {
            // 释放线程占用的栈
			StackSize = 0;
			MmFreeVirtualMemory( &PspCurrentThread->KernelStack,
								 &StackSize,
								 MEM_RELEASE,
								 TRUE );
		}
		// 取消指针引用
		ObDerefObject(PspCurrentThread);
	}


	// 选择优先级最高的非空就绪队列的队首线程作为当前运行线程
	PspCurrentThread = CONTAINING_RECORD(PspReadyListHeads[HighestPriority].Next, THREAD, StateListEntry);
	ObRefObject(PspCurrentThread);

    // 从就绪队列移除，设为Zero态
	PspUnreadyThread(PspCurrentThread);
    // 设为运行态
	PspCurrentThread->State = Running;
	
// Debug Info
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_RUNNING, Tick);
#endif

	// 换入线程绑定运行的地址空间。
	MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
	// 返回线程的上下文
	return &PspCurrentThread->KernelContext;
}
```

#### 总结

EOS线程状态转换中设计到的核心数据结构为表示同优先级的队列，用双向链表存储

涉及到的算法包含在`ps/sched.c`中，用以实现状态转换，具体细节已经在源代码分析中剖析。

#### 跟踪

在`ke/sysproc.c`文件的`LoopThreadFunction`中添加断点，启动调试并执行`loop`命令，停止后删除当前断点，并分别在PspReadyThread、PspUnreadyThread、PspWait、PspUnwaitThread、PspSelectNextThread中添加断点，以便在线程切换状态时Debug。完成后开始跟踪

##### 阻塞->就绪

按下空格键触发KdbIsr调用，此时会发现loop函数停止，对表达式`*Thread`进行监视，这会监视所有以Thread为结尾的变量，通过对State的监视可以获得当前loop进程的状态码，通过调用堆栈可以查看相关函数调用，从而完成对`阻塞->就绪`过程的跟踪。

##### 运行->就绪

按F5继续执行，在断点PspSelectNextThread处停止，此时对表达式`*PspCurrentThread`进行监视，注意记录 Next 和 Prev 指针的值，随后单步调试直至函数结束，可以看到对应指针值从0变为其他值，并且线程状态码改变，从而完成对`运行->就绪`过程的跟踪。

##### 就绪->运行

按F5继续执行，在断点PspUnreadyThread处停止，此时对表达式`*Thread`进行监视，注意观察State的值，随后单步调试直至函数结束，发现State由0x1变成了0x2，说明线程由Ready态进入了Running态，从而完成对`就绪->运行`过程的跟踪。

##### 运行->阻塞

按F5继续执行，在断点PspUnreadyThread处停止，此时对表达式`*PspCurrentThread`进行监视，注意记录 Next 和 Prev 指针的值，随后单步调试直至State域的值为0x3，说明线程由运行态进入了阻塞态，从而完成对`运行->阻塞`过程的跟踪。


### 实现Resume原语

#### 简要描述

首先需要判断线程是否为NULL，随后按照参考步骤调用ListRemoveEntry函数将线程从挂起线程队列中移除、调用PspReadyThread函数将线程恢复为就绪状态，最后调用PspThreadSchedule宏函数执行线程调度，让刚刚恢复的线程有机会执行。

#### 源代码

在`ps/psspnd.c`文件第`119`行的`PsResumThread`函数中加入如下代码

```c
ASSERT(NULL != Thread)
// 1. 首先调用ListRemoveEntry函数将线程从挂起线程队列中移除。 
ListRemoveEntry(&Thread->StateListEntry);
// 2. 然后调用PspReadyThread函数将线程恢复为就绪状态。
PspReadyThread(Thread);
// 3. 最后调用PspThreadSchedule宏函数执行线程调度，让刚刚恢复的线程有机会执行。
PspThreadSchedule();
```

#### 测试

启用Bochs虚拟机，首先执行`loop`运行线程号为`24`的线程，随后切换至另一个Console先执行`Suspend 24`挂起线程，随后`Resume 24`，重复两次以确保Resume功能正常

### 结果分析

本实验首先通过断点调试，结合源代码观察了EOS线程状态的转换，随后自己编写了Resume原语，实现了Resume操作并加深了对EOS线程调度等的理解。

EOS的线程状态及转换特点是，相比其他OS，EOS的线程不属于进程，每个线程都是内核级，因此对进程的调度相当于线程的调度。EOS线程有32个优先级，不同优先级按优先级顺序调度，同一优先级队列当中按时间片轮转与FCFS的原则进行调度。EOS这种设计的不足可能是不方便管理进程，可以尝试将线程归属于进程，并对进程进行调度。

为EOS增加的Resume与Suspend是有效的，通过测试可以验证。不足之处是当线程被挂起时当前控制台会被阻塞，对用户交互不友好，可以改进。


---

## Lab3 进程同步

### 实验目的

- 使用EOS的信号量，编程解决生产者—消费者问题，理解进程同步的意义。
- 调试跟踪EOS信号量的工作过程，理解进程同步的原理。
- 修改EOS的信号量算法，使之支持等待超时唤醒功能(有限等待)，加深理解进程同步的原理。

### 预备知识

#### EOS内核的三种同步对象

- 互斥对象（Mutex）
- 信号量对象（Semaphore）
- 事件对象（Event）

这三种同步对象都涉及到两个状态`signaled`和`nosignaled`，进程间同步本质上是对这两个状态进行切换，不同对象有不同的切换逻辑，具体可参考源代码

EOS创建了如下调用，当线程对同步对象进行这种调用时，处于`nosignaled`的对象会变成`signaled`

```c
ULONG WaitForSingleObject (
	IN HANDLE Handle,
	IN ULONG Milliseconds
)
```

##### Mutex

```
Lock Mutex
(Critical Section)
Relaese Mutex 
```

EOS中定义如下

```c
//
// 互斥信号量结构体
//
typedef struct _MUTEX {
	PVOID OwnerThread;			// 当前拥有 Mutex 的线程指针
	ULONG RecursionCount;		// 递归拥有 Mutex 的计数器
	LIST_ENTRY WaitListHead;	// 等待队列
}MUTEX, *PMUTEX;
```

`OwnerThread==NULL`时`Mutex`处于`singaled`态，此时对Mutex对象进行Wait调用会将该指针指向调用的线程地址，如果调用时Mutex处于处于`nosingaled`态，则进入等待队列

##### Semaphore

> 以下解释摘自[维基百科](https://zh.m.wikipedia.org/zh-hans/%E4%BF%A1%E5%8F%B7%E9%87%8F)

信号量（英语：semaphore）又称为信号标，是一个同步对象，用于保持在0至指定最大值之间的一个计数值。当线程完成一次对该semaphore对象的等待（wait）时，该计数值减一；当线程完成一次对semaphore对象的释放（release）时，计数值加一。当计数值为0，则线程等待该semaphore对象不再能成功直至该semaphore对象变成signaled状态。semaphore对象的计数值大于0，为signaled状态；计数值等于0，为nonsignaled状态。

EOS中定义如下

```c
//
// 记录型信号量结构体
//
typedef struct _SEMAPHORE {
	LONG Count;					// 信号量的整形值
	LONG MaximumCount;			// 允许最大值
	LIST_ENTRY WaitListHead;	// 等待队列
}SEMAPHORE, *PSEMAPHORE;
```

##### Event

> **省流**：Event对象实验中未涉及，不感兴趣可以跳过本部分

```c
//
// 事件结构体
//
typedef struct _EVENT {
	BOOL IsManual;				// 是否手动类型事件
	BOOL IsSignaled;			// 是否处于 Signaled 状态
	LIST_ENTRY WaitListHead;	// 等待队列
}EVENT, *PEVENT;
```

Event对象在同步对象中最为灵活，因为其`signaled`和`nosignaled`完全由程序控制，程序可以自行调用。

Event对象有手动和自动，区别就是调用完成后事件是否会自动恢复（Reset）为`nosignaled`状态。


#### 生产者——消费者问题

生产者-消费者问题是一个著名的进程同步问题。它描述的是:有一群生产者进程在生产某种产品，并将此产品提供给一群消费者进程去消费。为使生产者进程和消费者进程能并发执行，在他们之间设置了一个具有n个缓冲区的缓冲池，生产者进程可以将它生产的一个产品放入一个缓冲区中，消费者进程可以从一个缓冲区中取得一个产品消费。尽管所有的生产者进程和消费者进程都是以异步方式运行的，但它们之间必须保持同步，即不允许消费者进程到一个空缓冲区去取产品，也不允许生产者进程向一个已经装有产品的缓冲区中放入产品

#### CreateThread调用

该方法流程大致为：首先是创建一个空白的线程控制块，然后为线程分配栈，并初始化线程 的上下文环境，最后使线程进入就绪状态

方法定义如下

```c
HANDLE CreateThread(
	// 用户模式线程栈的大小，如果当前进程是系统进程则忽略之。目前所有线程都执行在内核栈中，参数暂时无用。
	IN SIZE_T StackSize,
	// 线程开始执行的函数的指针
	IN PTHREAD_START_ROUTINE StartAddr,
	// 传递给线程函数的参数
	IN PVOID ThreadParam,
	// 指向用于保存线程句柄的变量
	IN ULONG CreateFlags,
	// 指向用于保存线程ID的变量
	OUT PULONG ThreadId
);
```

在调用`CreateThread`函数创建线程之前，要首先按照线程入口函数类型的定义，编写一个线程入口函数。

```c
typedef ULONG (*PTHREAD_START_ROUTINE)( PVOID ThreadParameter );
```

### 实验步骤

#### 使用EOS的信号量实现生产者-消费者问题

> 给出使用EOS的信号量解决生产者-消费者问题的实现方法，包括实现方法的简要描述、源代码、测试及结果等


##### 描述及源代码

首先定义缓冲池大小及产品数量，并用创建对应大小的缓冲池

```c
#define BUFFER_SIZE		10
int Buffer[BUFFER_SIZE];

#define PRODUCT_COUNT	30
```

随后创建生产者和消费者用于同步的Handle

```c
HANDLE MutexHandle;
HANDLE EmptySemaphoreHandle;
HANDLE FullSemaphoreHandle;
```

接下来创建生产者与消费者线程，根据EOS特性，需要首先编写各自的线程入口函数

生产者线程函数中定义一个下标并移动表示生产，生产时需要访问临界资源Buffer等，同时设置一个生产间隔，具体源代码如下

```c
ULONG Producer(PVOID Param) 
{
	int i;
	int InIndex = 0;

	for (i = 0; i < PRODUCT_COUNT; i++) {

		WaitForSingleObject(EmptySemaphoreHandle, INFINITE);
		WaitForSingleObject(MutexHandle, INFINITE);

		printf("Produce a %d\n", i);
		Buffer[InIndex] = i;
		InIndex = (InIndex + 1) % BUFFER_SIZE;

		ReleaseMutex(MutexHandle);
		ReleaseSemaphore(FullSemaphoreHandle, 1, NULL);


		Sleep(500);
	}
	
	return 0;
}
```

消费者线程函数中定义一个下标并移动表示消费，消费时需要访问临界资源Buffer等，同时设置一个消费间隔，并令前10次消费慢与后面的消费，具体源代码如下


```c
ULONG Consumer(PVOID Param)
{
	int i;
	int OutIndex = 0;

	for (i = 0; i < PRODUCT_COUNT; i++) {

		WaitForSingleObject(FullSemaphoreHandle, INFINITE);
		WaitForSingleObject(MutexHandle, INFINITE);

		printf("\t\t\tConsume a %d\n", Buffer[OutIndex]);
		OutIndex = (OutIndex + 1) % BUFFER_SIZE;

		ReleaseMutex(MutexHandle);
		ReleaseSemaphore(EmptySemaphoreHandle, 1, NULL);

		if (i < 10) {
			Sleep(2000);
		} else {
			Sleep(100);
		}
	}
	return 0;
}
```

主函数中创建Mutex确保生产者与消费者互斥地访问临界资源——缓冲池，同时创建full与empty两个信号量表示缓冲池中空/满缓冲区的数量，随后使用之前编写的入口函数创建线程，并等待线程结束，关闭句柄并返回0。另外的，如果Mutex创建失败将返回1，Empty创建失败返回2，Full创建失败将返回3，生产者线程创建失败将返回4，消费者线程创建失败将返回5。

```c
int main(int argc, char* argv[])
{
	HANDLE ProducerHandle;
	HANDLE ConsumerHandle;

	// 创建用于互斥访问缓冲池的 Mutex 对象。
	MutexHandle = CreateMutex(FALSE, NULL);
	if (NULL == MutexHandle) {
		return 1;
	}

	// 创建 Empty 信号量，表示缓冲池中空缓冲区数量。初始计数和最大计数都为 BUFFER_SIZE。
	EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);
	if (NULL == EmptySemaphoreHandle) {
		return 2;
	}

	// 创建 Full 信号量，表示缓冲池中满缓冲区数量。初始计数为 0，最大计数为 BUFFER_SIZE。
	FullSemaphoreHandle = CreateSemaphore(0, BUFFER_SIZE, NULL);
	if (NULL == FullSemaphoreHandle) {
		return 3;
	}


	ProducerHandle = CreateThread( 0,			// 默认堆栈大小
								   Producer,	// 线程函数入口地址
								   NULL,		// 线程函数参数
								   0,			// 创建标志
								   NULL );		// 线程 ID

	if (NULL == ProducerHandle) {
		return 4;
	}


	ConsumerHandle = CreateThread( 0,
								   Consumer,
								   NULL,
								   0,
								   NULL );

	if (NULL == ConsumerHandle) {
		return 5;
	}


	// 等待生产者线程和消费者线程结束。
	WaitForSingleObject(ProducerHandle, INFINITE);
	WaitForSingleObject(ConsumerHandle, INFINITE);
	// 关闭句柄
	CloseHandle(MutexHandle);
	CloseHandle(EmptySemaphoreHandle);
	CloseHandle(FullSemaphoreHandle);
	CloseHandle(ProducerHandle);
	CloseHandle(ConsumerHandle);
	return 0;
}
```


##### 测试结果

参考word文档


#### EOS信号量工作过程的跟踪与源代码分析

##### 调试跟踪

首先对EOS内核进行生成，分别生成Debug和Release版后将得到的sdk文件夹复制到当前实验环境并替换。

添加断点在`EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);`处，开始调试，运行到断点处时单步调试进入`semaphore.c`中，在`PsInitializeSemaphore`中添加一个断点，继续调试，在断点处打开记录型信号量窗口，得到如下内容

<!-- Pic -->

删除之前的断点，在`WaitForSingleObject(EmptySemaphoreHandle, INFINITE);`处添加断点，继续调试，随后在`PsWaitForSemaphore函数`添加一个断点，单步调试观察到Empty信号量计数减少了1（10->9）:

<!-- Pic -->

删除之前的断点，在`ReleaseSemaphore(FullSemaphoreHandle, 1, NULL);`中添加断点，随后单步调试观察到记录型信号量窗口中Full信号量的值增加了1

<!-- pic -->

重开调试，在`PspWait(&Semaphore->WaitListHead, INFINITE);`处添加断点，查看Empty计数值为-1，表示为0xffffffff，在调用堆栈中找到Producer函数，再查看i的值，得到0xe，即十进制的14。

<!-- Pic*2 -->

删除所有断点，并在`ReleaseSemaphore(EmptySemaphoreHandle, 1, NULL);`处添加断点，继续调试，查看记录型信号量窗口，观察到阻塞。随后查看Consumer函数中i的值，为0x4，说明四号已被消费。进入`PsReleaseSemaphore`观察到Empty计数为-1，单步调试至`PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);`，观察到刚刚的值变为0，然后继续，生产者线程继续执行。

<!-- pic*3 -->

在`PsWaitForSemaphore`函数的最后打上断点，继续调试，观察到Empty计数为0，Producer中i=14。

<!-- pic*2 -->

##### 源代码分析

参考预备知识部分

#### 支持等待超时唤醒和批量释放功能的信号量实现

##### 简要描述与源代码

修改后的`PsWaitForSemaphore`函数及注释如下

```c
// 返回值
STATUS Ret;

ASSERT(KeGetIntNesting() == 0); // 中断环境下不能调用此函数。

IntState = KeEnableInterrupts(FALSE); // 开始原子操作，禁止中断。

Semaphore->Count--;
if (Semaphore->Count <= 0) {
	// 计数值等于0时，调用PspWait函数阻塞线程的执行
	Ret = PspWait(&Semaphore->WaitListHead, Milliseconds);
} 
else if (Semaphore->Count > 0) {
	// 计数值大于 0 时，将计数值减 1 后直接返回成功
	Semaphore->Count -= 1;
	Ret = STATUS_SUCCESS;
}

KeEnableInterrupts(IntState); // 原子操作完成，恢复中断。

return Ret;
```


修改后的`PsReleaseSemaphore`函数及注释如下

```c
// 循环释放ReleaseCount个进程
while (ReleaseCount>0) {
	// 被阻塞进程数小于Release时，释放完全部后跳出循环
	if(ListIsEmpty(&Semaphore->WaitListHead))
		break;
	// 每次循环释放一个线程，并更新ReleaseCount
	PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);
	ReleaseCount--;
}

// 更新信号量的计数器
Semaphore->Count += ReleaseCount;
```

##### 测试及结果

重新生成内核，将sdk文件夹复制到EOSApp项目目录中，重新生成EOSApp项目，首先启动调试，观察到程序正常运行。

<!-- pic -->

随后为Producer函数的等待Empty和Consumer函数的等待Full分别增加输出语句，打印信息，重新调试，结果如下。

<!-- pic -->

将消费者替换为`NewConsumer`中的函数，再次验证，结果正确

<!-- pic -->

### 结果分析

<!--  -->


#### EOS信号量

EOS信号量是EOS的一个同步对象，用于保持在0至指定最大值之间的一个计数值。当线程完成一次对该semaphore对象的等待（wait）时，该计数值减一；当线程完成一次对semaphore对象的释放（release）时，计数值加一。当计数值为0，则线程等待该semaphore对象不再能成功直至该semaphore对象变成signaled状态。semaphore对象的计数值大于0，为signaled状态；计数值等于0，为nonsignaled状态。

#### 超时唤醒和批量释放

超时唤醒引入了计数器，当计数值达标后自动唤醒，批量释放通过循环每次最多可释放ReleaseCount个线程。经过上文验证其实现均有效。


---

## Lab4 线程调度

### 实验目的

- 调试EOS的线程调度程序，熟悉基于优先级的抢先式调度。
- 为EOS添加时间片轮转调度，了解其它常用的调度算法。

### 预备知识

#### 基于优先级的抢先式调度

EOS使用双向链表存储同一个优先级的队列，用数组存放32个这种双向链表，并用下标表示不同优先级大小，同时设置一个32位就绪位图表示索引为n的队列中是否有线程

对于不同优先级的线程，优先级越大越优先被调度，同一优先级中的线程则采用FCFS的方式“先来先服务”。


#### 调度程序执行的时机和流程

> 线程调度运用到了中断，要了解调度的流程首先要了解中断处理的流程。
> 
> EOS中有中断处理函数Interrupt，中断时首先调用InterEnter保存中断时CPU现场到KernelContext中，随后调用KiDispatchInterrupt派遣中断至中断处理服务程序，完成调用IntExit退出中断，恢复现场

调度时首先正常执行中断处理流程，在IntExit中调用`PspSelectNextThread`函数，决定要恢复的CPU现场，即选择一个线程获得CPU，这个函数就是**调度程序**

中断不仅可以由外部设备产生，也可以由线程主动调用48号软中断产生，这样就实现了线程调度

调度程序的流程可以由伪代码表示如下

```
PspSelectNextThread
if 被中断线程State == Running then:
	if 存在更高优先级线程 then:
		被中断线程State = Ready
		return 最高优先级就绪队首线程
	else
		return 被中断线程
else
	return 最高优先级就绪队首线程
```

#### 时间片轮转调度

当线程调度执行时，把CPU分配给队首线程，待线程的时间片用完后，会重新为它分配一个时间片，并将它移动到就绪队列的末尾，从而让新的队首线程开始执行。

### 实验步骤

#### EOS基于优先级的抢占式调度工作过程的跟踪与源代码分析

> 分析EOS基于优先级的抢占式调度的核心源代码，阐述其实现方法，包括相关数据结构和算法等；简要说明在本部分实验过程中完成的主要工作，包括对EOS调度程序的跟踪等


##### 源代码分析

EOS使用双向链表存储同一个优先级的队列，用数组存放32个这种双向链表，并用下标表示不同优先级大小，同时设置一个32位就绪位图表示索引为n的队列中是否有线程


##### 过程跟踪

首先在rr中fprintf处添加断点，随后启动调试，输入rr，运行到断点处时查看进程线程窗口，观察到了rr创建的线程如下
 
绘制24-33的线程，得到如下结果
 
刷新就绪线程队列，得到如下结果
 
同时观察到Y的值为0
 
多按几次F5，仍然得到相同的结果
接下来对BitScanReverse打上断点，继续执行，刷新就绪线程队列，得到如下结果
 
对*PspCurrentThread进行监视，结果如下
 
继续单步调试至在PspSelectNextThread函数返回前 
在ps/sched.c文件的PspSelectNextThread函数的第402行添加一个断点，继续调试，观察到优先级为24的线程，结果如下
 
单步调试至408行，观察到新建的第0个线程已经挂接在了优先级为8的就绪队列的队首，继续调试至455行，观察到正在执行的第0个新建的线程已经进入了就绪状态，让出了CPU，继续执行到473行，观察到优先级为24的控制台派遣线程已经进入了运行状态。

#### 为EOS添加时间片轮转调度

> 给出实现方法的简要描述、源代码、测试及结果等

##### 简要描述及源代码

参考流程图5-11编写即可

```c
// 被中断的线程处于运行态且不为空
if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
	// 减少时间片
	PspCurrentThread->RemainderTicks--;
	if(PspCurrentThread->RemainderTicks == 0) {
		// 如果时间片为0重置时间片
		PspCurrentThread->RemainderTicks=TICKS_OF_TIME_SLICE;
		// 存在和被中断线程优先级相同的就绪线程
		if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			// 将被中断线程转入就绪状态，并插入队尾
			PspReadyThread(PspCurrentThread);
		}
	}
}
```

##### 测试及结果

运行内核输入rr，观察到10个线程都获得了时间片

<!-- pic -->

### 结果分析

#### EOS线程调度的特点、不足及改进意见

EOS线程调度采用的算法是基于优先级的抢先式调度，对于不同优先级的线程，优先级越大越优先被调度，同一优先级中的线程则采用FCFS的方式“先来先服务”。缺点为如果同优先级有长时间运行的线程，则其他线程分不到CPU，不能满足实时操作系统的要求。，改进意见为添加时间片轮转法。

#### 线程调度的执行时机和过程

调度时首先正常执行中断处理流程，在IntExit中调用`PspSelectNextThread`函数，决定要恢复的CPU现场，即选择一个线程获得CPU，这个函数就是**调度程序**，中断不仅可以由外部设备产生，也可以由线程主动调用48号软中断产生，这样就实现了线程调度。调度程序的流程可以由伪代码表示如下

```
PspSelectNextThread
if 被中断线程State == Running then:
	if 存在更高优先级线程 then:
		被中断线程State = Ready
		return 最高优先级就绪队首线程
	else
		return 被中断线程
else
	return 最高优先级就绪队首线程
```

#### 为EOS添加时间片轮转调度

为EOS添加的时间片轮转法经过上文检验，证明了其有效性。

---

## Lab5 物理存储器与进程逻辑地址空间管理

### 实验目的

- 通过查看物理存储器的使用情况，并练习分配和回收物理内存，从而掌握物理存储器的管理方法。
- 通过查看进程逻辑地址空间的使用情况，并练习分配和回收虚拟内存，从而掌握进程逻辑地址空间的管理方法。

### 预备知识

#### 物理存储器的管理方式

EOS使用分页式存储管理方式，由页框号数据库PFN Database进行管理。其实质是一个数组，长度与页数量一致。定义如下

```c
typedef struct _MMPFN
{
	ULONG Unused : 9;		// 未使用
	ULONG PageState : 3;	// 页状态
	ULONG Next : 20;		// 下一个框号
}MMPFN, *PMMPFN;
```

物理页目前有三种状态，定义如下

```c
typedef enum _PAGE_STATE {
	ZEROED_PAGE,	// 0 零页，空闲，已0初始化
	FREE_PAGE,		// 1 自由页，空闲，未0初始化
	BUSY_PAGE,		// 2 占用页，被占用
} PAGE_STATE;
```

`ZEROED_PAGE`和`FREE_PAGE`的数据库构成链表，分配时只需要移除首部即可

`MiAllocateAnyPages`和`MiAllocateZeroedPages`两个函数分别负责优先从空闲分配空间和优先从零页分配空间。

同时还可以为EOS增加零页线程进行零初始化

#### 进程逻辑地址空间的管理方式

进程申请时才会为进程分配物理页，虚拟地址描述符VAD记录一段虚拟地址区域，定义如下

```c
typedef struct _MMVAD{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListEntry;
}MMVAD, *PMMVAD;
```

同时使用链表链接所有VAD，便于管理

```c
typedef struct _MMVAD_LIST{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListHead;
}MMVAD_LIST, *PMMVAD_LIST;
```

调用API函数`VirtualAlloc`申请一段虚拟地址并映射到物理内存，调用API函数`VirtualFree`释放已分配地址区域与映射的物理页。

进程地址空间至少需要一个页目录和一个`VAD`链表，EOS中使用`PAS（ProcessAddressSpace）`结构体定义

```c
typedef struct _MMPAS {
	// 可以给分配给进程使用的地址空间。
	LIST VadList;

	// 页目录和PTE计数器数据库的页框号。
	ULONG_PTR PfnOfPageDirectory;
	ULONG_PTR PfnOfPteCounter;
}MMPAS;
```

### 实验过程

#### EOS物理内存分配和回收的练习以及源代码分析

> 练习物理内存的分配和回收；分析相关源代码，阐述EOS中物理存储器的管理方法，包括数据结构和算法等；简要说明在本部分实验过程中完成的主要工作

##### EOS物理内存分配和回收的练习

在sysproc的第1076行打上断点，开始调试，输入pm，查看物理内存窗口得到如下结果

<!-- pic -->

按F5继续调试，观察到输出结果与上述结果一致

<!-- pic -->

 
使用pm.c文件中ConsoleCmdPhysicalMemory函数替换sysproc.c文件中ConsoleCmdPhysicalMemory函数的函数体，重新生成并调试，输入pm，得到如下结果
 
<!-- pic -->

分别给MiFreePages(1, PfnArray)和MiAllocateAnyPages(1, PfnArray)添加断点，重新调试，刷新物理内存窗口观察到和第一次一致
 
<!-- pic -->

进入MiAllocateAnyPages内部逐语句调试，函数末尾处刷新物理内存窗口，得到如下结果，观察到物理页框号为0x409的物理页由原先的FREE变为BUSY，且空闲物理页数量减少1，已使用物理页数量增加1，该物理页是从空闲列表中分配的。
 
<!-- pic -->

继续调试至MiFreePages结束，再次刷新物理内存窗口，发现刚刚申请的页已被回收，结果如下

<!-- pic -->

##### 源代码分析

上述过程涉及到了`MiAllocateAnyPages`与`MiFreePages`两个函数。

首先分析`MiAllocateAnyPages`函数，其功能是分配物理页。函数运行时首先定义必要变量，随后关中断，接着函数会首先从空闲页链表中分配，如果空闲页链表不足则再从零页链表分配，完成分配后修改对应变量的值，开中断，返回，完成调用。

分析`MiFreePages`函数，其功能是释放物理页面。函数运行时会检查待回收的物理页是否有效，若有效则修改器状态为Free，并将其插入空闲链表头部，随后返回，完成调用。



#### EOS进程逻辑地址空间分配和回收的练习以及源代码分析

> 练习虚拟内存的分配和回收；分析相关源代码，阐述EOS中进程逻辑地址空间的管理方法，包括数据结构和算法等；给出在应用进程中分配虚拟页和释放虚拟页的实现方法简要描述、源代码、测试及结果等；简要说明在本部分实验过程中完成的主要工作

##### 练习虚拟内存的分配和回收

首先调试vm命令，在ConsoleCmdVM函数的最后一行代码中添加一个断点，随后启动调试，首先输入pt查看当前线程的pid，得到1，随后输入vm 1查看1号线程的虚拟（逻辑）空间，得到如下结果
 
<!-- pic  -->

接下来调试分配虚拟页，使用vm.c文件中ConsoleCmdVM函数的函数体替换sysproc.c文件中ConsoleCmdVM函数的函数体，生成运行，输入vm 1，得到如下结果
 
<!-- pic  -->
 
分别在MmAllocateVirtualMemory函数的代码行和 MmFreeVirtualMemory函数的代码行添加断点，随后开始调试，输入vm 1
 
<!-- pic  -->
 
进入MmAllocateVirtualMemory函数并单步调试，观察结果。
 
<!-- pic  -->
 
按F5继续调试释放虚拟页，进入MmFreeVirtualMemory函数并单步调试，观察结果

 
<!-- pic  -->

##### 源代码分析

首先分析`MmAllocateVirtualMemory`函数，其功能为在当前进程地址空间或系统地址空间中分配虚拟内存，如果成功则返回STATUS_SUCCESS，否则表示失败。

函数运行时首先会确保参数有效，随后关中断，根据传入参数进行申请，完成后设置返回值，开中断，返回，完成调用。

再分析`MmFreeVirtualMemory`函数，其功能为在当前进程地址空间或系统地址空间中释放虚拟内存，如果成功则返回STATUS_SUCCESS。

函数运行时首先会确保参数有效，随后关中断，根据传入参数进行释放，完成后设置返回值，开中断，返回，完成调用。

#### 在应用程序进程中分配和释放虚拟页

> 此部分报告中没有涉及，但是需要提交代码

创建一个 EOS 应用程序，然后编写代码完成下列功能:
1. 调用API函数`VirtualAlloc`，分配一个整型变量所需的空间，并使用一个整型变量的指针指向这 个空间。
2. 修改整型变量的值为`0xFFFFFFFF`。在修改前输出整型变量的值，在修改后再输出整型变量的值。
3. 调用API函数`Sleep`，等待10秒钟。
4. 调用API函数`VirtualFree`，释放之前分配的整型变量的空间。
5. 进入死循环，这样应用程序就不会结束。

```c
#include "EOSApp.h"
int main(int argc, char* argv[])
{
	// 1. 调用API函数`VirtualAlloc`，分配一个整型变量所需的空间，并使用一个整型变量的指针指向这个空间。
	PINT space = VirtualAlloc(0, sizeof(INT), MEM_RESERVE|MEM_COMMIT);
	// 2. 修改整型变量的值为`0xFFFFFFFF`。在修改前输出整型变量的值，在修改后再输出整型变量的值。
	printf("Before: 0x%X\n", *space);
	*space = 0xFFFFFFFF;
	printf("After: 0x%X", *space);
	// 3. 调用API函数`Sleep`，等待10秒钟。
	Sleep(1000);
	// 4. 调用API函数`VirtualFree`，释放之前分配的整型变量的空间。
	VirtualFree(space, 0, MEM_RELEASE);
	// 5. 进入死循环，这样应用程序就不会结束。
	while(1);
	return 0;
}
```

### 结果分析

参考word文档与预备知识

---

## Lab6 FAT12文件系统

### 实验目的

- 了解在EOS应用程序中读文件和写文件的基本方法。
- 通过为FAT12文件系统添加写文件功能，加深对FAT12文件系统和磁盘存储器管理原理的理解。

### 预备知识

#### 文件系统驱动程序的作用

用户对文件的读写请求转换为 对磁盘扇区的读写请求，并负责对磁盘扇区进行管理。

#### FAT12



### 实验步骤

#### EOS中FAT12文件系统相关源代码分析

> 分析EOS中FAT12文件系统的相关源代码，简要说明EOS实现FAT12文件系统的方法，包括主要数据结构与文件基本操作的实现等

文件系统是建立在磁盘等块设备之上的一个逻辑层，读写文件时，文件系统驱动会将文件读写请求转换为对磁盘扇区的读写请求，最终由磁盘驱动程序完成对磁盘扇区的读写。

EOS为应用程序提供了一组可以操作文件的API函数，包括打开文件(CreateFile)、关闭文件(CloseHandle)、读文件(ReadFile)和写文件(WriteFile)

打开文件时使用的CreateFile定义如下

```c
 EOSAPI HANDLE CreateFile(
 IN PCSTR FileName,
 IN ULONG DesiredAccess,
 IN ULONG ShareMode,
 IN ULONG CreationDisposition,
 IN ULONG FlagsAndAttributes
 )
```

该函数被调用后会依次调用FatCreate、FatOpenExistingFile、FatOpenFile和FatOpenFileDirectory。

CloseHandle函数定义如下

```c
 EOSAPI BOOL CloseHandle(
 IN HANDLE Handle)
```

该函数被调用后会依次调用FatClose、FatCloseFile。

读文件函数ReadFile定义如下

```c
 EOSAPI BOOL ReadFile(
 IN HANDLE Handle,
 OUT PVOID Buffer,
 IN ULONG NumberOfBytesToRead,
 OUT PULONG NumberOfBytesRead)
```

该函数被调用后会依次调用ObRead、IopReadFileObject、FatRead和FatReadFile。

写文件函数ReadFile定义如下

```c
 EOSAPI BOOL WriteFile(
 IN HANDLE Handle,
 IN PVOID Buffer,
 IN ULONG NumberOfBytesToWrite,
 OUT PULONG NumberOfBytesWritten)
```

该函数被调用后会依次调用ObWrite、IopWriteFileObject、FatWrite和FatWriteFile。


#### EOS中FAT12文件系统读文件过程的跟踪

> 简要说明在本部分实验过程中完成的主要工作，包括对读文件的跟踪等，总结EOS中读文件的实现方法

替换EOSApp.c为学生包中的FileApp.c的内容，随后将a.txt复制到img镜像中，生成项目并调试输入A:\eosapp.exe A:\a.txt，得到结果如下
 
结束调试，将fat12.c拖入IDE窗口，在EOSApp.c文件中的ReadFile函数代码行添加断点，开始调试，输入A:\EOSApp.exe A:\a.txt，在fat12.c文件中FatReadFile函数的开始处添加一个断点。随后继续运行调试，停下后对*Vcb和*File进行监视，得到以下结果
 
 
同时观察到offset值为0，BytesToRead为256。
  
打开调用堆栈监视流程
 
继续单步调试观察相关变量变化，最后观察到虚拟机正常输出


#### 为EOS的FAT12文件系统添加写文件功能

> 给出实现方法的简要描述、源代码、测试及结果等

首先导入学生包中的代码并测试
 

 
观察到能正常实现文件读写

修改代码使其支持跨扇区写文件，实现思路为将写入数的数据分为三部分：头扇区、中间扇区和尾扇区，在写入头扇区和尾扇区的中间，插入一个循环，用于写入中间跨越的多个中间扇区，其特点为均是写入满扇区，写入的数据均为512个字节，在此之前计算出跨越的扇区个数即可。

代码及测试如下


<!-- 本段代码参考了18级大佬的[实现](https://github.com/lyfcsdo2011/OS-Work/blob/master/mission14/fat12.c) -->

```c
STATUS
FatWriteFile(
	IN PVCB Vcb,
	IN PFCB File,
	IN ULONG Offset,
	IN ULONG BytesToWrite,
	IN PVOID Buffer,
	OUT PULONG BytesWriten
	)
	{
	STATUS Status;

	// 由于在将新分配的簇插入簇链尾部时，必须知道前一个簇的簇号，
	// 所以定义了“前一个簇号”和“当前簇号”两个变量。
	USHORT PrevClusterNum, CurrentClusterNum;
	USHORT NewClusterNum;
	ULONG ClusterIndex;
	ULONG FirstSectorOfCluster;
	ULONG OffsetInSector;
	
	ULONG i;

	// 写入的起始位置不能超出文件大小（并不影响增加文件大小或增加簇，想想原因？）
	if (Offset > File->FileSize)
		return STATUS_SUCCESS;

	// 根据簇的大小，计算写入的起始位置在簇链的第几个簇中（从 0 开始计数）
	ClusterIndex = Offset / FatBytesPerCluster(&Vcb->Bpb);
	
	//
	// 计算起始地址在簇中的第几个字节
	ULONG BytesNumOfCluster = Offset - ClusterIndex * FatBytesPerCluster(&Vcb->Bpb);
	
	ULONG NeedSplit;							// 分割标志
	ULONG ByteFommer = BytesToWrite;			// 前一半需要写入的字节
	ULONGByteLatter = 0;						// 后一半需要写入的字节
	ULONG SpanOfSector = 0;						// 跨过的扇区数
	
	PCHAR FrontBuffer;							// 		 前
	PCHAR MidBuffer;							// 缓冲区 中
	PCHAR LatterBuffer;							//		 后
	
	// 如果写入的内容没有跨扇区，则无需切分
	if((512 - BytesNumOfCluster)>= BytesToWrite){
		NeedSplit = 0;
	}
	//否则要分成两半
	else{
		NeedSplit = 1;
		ByteFommer = 512 - BytesNumOfCluster;
		NumOfLatterBuffer = (BytesToWrite - ByteFommer) % 512;
		SpanOfSector = (BytesToWrite - ByteFommer)/512;
	}	
	

	if(NeedSplit == 0){
		FrontBuffer=Buffer;
	}
	else{
		//划分缓存区
		FrontBuffer = Buffer;
		MidBuffer = &Buffer[ByteFommer];
		LatterBuffer = &Buffer[ByteFommer + SpanOfSector * 512];
	}
	
	// 分为三个部分写入，先写入头一个扇区，然后循环写入中间满扇区（均是512字节），最后写入尾扇区
	// 写入头扇区
	// 顺着簇链向后查找写入的起始位置所在簇的簇号。
	PrevClusterNum = 0;
	CurrentClusterNum = File->FirstCluster;
	for (i = ClusterIndex; i > 0; i--) {
		PrevClusterNum = CurrentClusterNum;
		CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);	
	}

	// 如果写入的起始位置还没有对应的簇，就增加簇
	if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {

		// 为文件分配一个空闲簇
		FatAllocateOneCluster(Vcb, &NewClusterNum);

		// 将新分配的簇安装到簇链中
		if (0 == File->FirstCluster)
			File->FirstCluster = NewClusterNum;
		else
			FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
		
		CurrentClusterNum = NewClusterNum;
	}

	// 计算当前簇的第一个扇区的扇区号。簇从 2 开始计数。
	FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
	
	// 计算写位置在扇区内的字节偏移。
	OffsetInSector = Offset % Vcb->Bpb.BytesPerSector;

	// 为了简单，暂时只处理一个簇包含一个扇区的情况。
	// 并且只处理写入的数据在一个扇区范围内的情况。
	Status = IopReadWriteSector( Vcb->DiskDevice,
									FirstSectorOfCluster,
									OffsetInSector,
									(PCHAR)FrontBuffer,
									ByteFommer,
									FALSE );

	if (!EOS_SUCCESS(Status))
		return Status;

	// 如果文件长度增加了则必须修改文件的长度。
	if (Offset + BytesToWrite > File->FileSize) {
		File->FileSize = Offset + BytesToWrite;
		
		// 如果是数据文件则需要同步修改文件在磁盘上对应的 DIRENT 结构
		// 体。目录文件的 DIRENT 结构体中的 FileSize 永远为 0，无需修改。
		if (!File->AttrDirectory)
			FatWriteDirEntry(Vcb, File);
	}

	// 循环写入中间满扇区
	Offset += ByteFommer;
	
	ULONG k = SpanOfSector;
	while(k > 0){
		// 顺着簇链向后查找写入的起始位置所在簇的簇号。
		PrevClusterNum = 0;
		CurrentClusterNum = File->FirstCluster;
		// 根据簇的大小，计算写入的起始位置在簇链的第几个簇中（从 0 开始计数）
		ClusterIndex = (Offset) / FatBytesPerCluster(&Vcb->Bpb);
		for (i = ClusterIndex; i > 0; i--) {
			PrevClusterNum = CurrentClusterNum;
			CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);
			//CurrentClusterNum = FatGetFatEntryValue(Vcb,CurrentClusterNum);	
		}
		
	
		// 如果写入的起始位置还没有对应的簇，就增加簇
		if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {
	
			// 为文件分配一个空闲簇
			FatAllocateOneCluster(Vcb, &NewClusterNum);
	
			// 将新分配的簇安装到簇链中
			if (0 == File->FirstCluster)
				File->FirstCluster = NewClusterNum;
			else
				FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
			
			CurrentClusterNum = NewClusterNum;
		}
		// 计算当前簇的第一个扇区的扇区号。簇从 2 开始计数。
		FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
		
		// 计算写位置在扇区内的字节偏移。
		OffsetInSector = (Offset) % Vcb->Bpb.BytesPerSector;
		
		// 并且只处理写入的数据在一个扇区范围内的情况。
		Status = IopReadWriteSector( Vcb->DiskDevice,
										FirstSectorOfCluster,//当前簇的第一个扇区的扇区号
										OffsetInSector,//写位置在扇区内的字节偏移
										(PCHAR)MidBuffer,//缓存区
										512,//要写入多少字节
										FALSE );
	
		if (!EOS_SUCCESS(Status))
			return Status;
	
		// 如果文件长度增加了则必须修改文件的长度。
		if (Offset + 512 > File->FileSize) {
			File->FileSize = Offset + BytesToWrite;
			
			// 如果是数据文件则需要同步修改文件在磁盘上对应的 DIRENT 结构
			// 体。目录文件的 DIRENT 结构体中的 FileSize 永远为 0，无需修改。
			if (!File->AttrDirectory)
				FatWriteDirEntry(Vcb, File);
		}
		MidBuffer = &MidBuffer[512];
		Offset += 512;
		k--;
	}

	
	// 写入尾扇区
	if(NeedSplit == 1 &&ByteLatter > 0){
		// 顺着簇链向后查找写入的起始位置所在簇的簇号。
		PrevClusterNum = 0;
		CurrentClusterNum = File->FirstCluster;
		// 根据簇的大小，计算写入的起始位置在簇链的第几个簇中（从 0 开始计数）
		ClusterIndex = (Offset) / FatBytesPerCluster(&Vcb->Bpb);
		for (i = ClusterIndex; i > 0; i--) {
			PrevClusterNum = CurrentClusterNum;
			CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);
		//CurrentClusterNum = FatGetFatEntryValue(Vcb,CurrentClusterNum);	
		}
	
		// 如果写入的起始位置还没有对应的簇，就增加簇
		if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {
	
			// 为文件分配一个空闲簇
			FatAllocateOneCluster(Vcb, &NewClusterNum);
	
			// 将新分配的簇安装到簇链中
			if (0 == File->FirstCluster)
				File->FirstCluster = NewClusterNum;
			else
				FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
			
			CurrentClusterNum = NewClusterNum;
		}
		// 计算当前簇的第一个扇区的扇区号。簇从 2 开始计数。
		FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
		
		// 计算写位置在扇区内的字节偏移。
		OffsetInSector = (Offset) % Vcb->Bpb.BytesPerSector;
	
		// 为了简单，暂时只处理一个簇包含一个扇区的情况。
		// 并且只处理写入的数据在一个扇区范围内的情况。
		Status = IopReadWriteSector( Vcb->DiskDevice,
										FirstSectorOfCluster,//当前簇的第一个扇区的扇区号
										OffsetInSector,//写位置在扇区内的字节偏移
										(PCHAR)LatterBuffer,//缓存区
										NumOfLatterBuffer,//要写入多少字节
										FALSE );
	
		if (!EOS_SUCCESS(Status))
			return Status;
	
		// 如果文件长度增加了则必须修改文件的长度。
		if (Offset +ByteLatter > File->FileSize) {
			File->FileSize = Offset + BytesToWrite;
			
			// 如果是数据文件则需要同步修改文件在磁盘上对应的 DIRENT 结构
			// 体。目录文件的 DIRENT 结构体中的 FileSize 永远为 0，无需修改。
			if (!File->AttrDirectory)
				FatWriteDirEntry(Vcb, File);
		}
	}
	
	// 返回实际写入的字节数量
	*BytesWriten = BytesToWrite;

	return STATUS_SUCCESS;

}
```


### 结果分析

EOS利用FAT12文件系统实现了文件读写，但是实验中观察到其写入文件过程比较慢，性能不足，有待改善。

---

## Lab Ex 1

EX1有七道题，clone、修改、提交、推送代码十分繁琐，不过**无所谓，我会批处理**，写个脚本~~重拳出击~~

首先领取任务，建议命名为`ex1-$`，将$替换成序号，eg.`ex1-7`

将本仓库中所有shell脚本下载到你的实验根目录，修改脚本中的仓库地址为你自己的地址，然后执行

```sh
chmod +x ./autoDoEx1.sh && ./autoDoEx1.sh
```

即可

---

## Lab Ex 3

1. 实现时间片轮转调度算法。

```c
// 被中断的线程处于运行态且不为空
if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
	// 减少时间片
	PspCurrentThread->RemainderTicks--;
	if(PspCurrentThread->RemainderTicks == 0) {
		// 如果时间片为0重置时间片
		PspCurrentThread->RemainderTicks=TICKS_OF_TIME_SLICE;
		// 存在和被中断线程优先级相同的就绪线程
		if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			// 将被中断线程转入就绪状态，并插入队尾
			PspReadyThread(PspCurrentThread);
		}
	}
}

return;
```

2. 修改时间片的大小TICKS_OF_TIME_SLICE为100，方便观察执行后的效果。

```c
//
// 用于时间片轮转调度的时间片大小（即时间片包含的时钟滴答数）
//
#define TICKS_OF_TIME_SLICE		100
```

3. 在控制台命令“rr”的处理函数中，将Sleep时间更改为200*1000，这样可以有充足的时间查看优先级降低后的效果。

```c
// 当前线程等待一段时间。由于当前线程优先级 24 高于新建线程的优先级 8，
// 所以只有在当前线程进入“阻塞”状态后，新建的线程才能执行。
Sleep(200 * 1000);
```

4. 修改线程控制块(TCB)结构体，在其中新增两个成员，一个是线程整个生命周期中合计使用的时间片数量，另一个是线程的初始时间片数量。

```c
ULONG TTLTickNum;					// 线程整个生命周期中合计使用的时间片数量
ULONG InitTickNum;					// 线程的初始时间片数量
```

```c
// create.c
// line 607:
NewThread->InitTickNum = TICKS_OF_TIME_SLICE;
NewThread->RemainderTicks = TICKS_OF_TIME_SLICE;
NewThread->TTLTickNum = 0;
```

5. 修改“rr”命令在控制台输出的内容和格式，不再显示线程计数，而是显示线程初始化时间片的大小，已使用时间片的合计数量，剩余时间片的数量。注意，在调用fprintf函数格式化字符时，需要在字符串的末尾增加一个空格，否则会导致输出异常。

```c
// ThreadFunction
PspCurrentThread->InitTickNum = TICKS_OF_TIME_SLICE;
PspCurrentThread->TTLTickNum = TICKS_OF_TIME_SLICE - PspCurrentThread->RemainderTicks;

// ...

fprintf(
	pThreadParameter->StdHandle, 
	"Thread ID:%d, Priority:%d, InitTicks:%d, UsedTicks:%d, RemainderTicks:%d ",
	ObGetObjectId(PspCurrentThread),
	Priority,
	PspCurrentThread->InitTickNum,
	PspCurrentThread->TTLTickNum,
	PspCurrentThread->InitTickNum - PspCurrentThread->TTLTickNum
);
```

6. 实现多级反馈队列调度算法

```c
// 被中断的线程处于运行态且不为空
	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
		// 减少时间片
		PspCurrentThread->RemainderTicks--;
		PspCurrentThread->TTLTickNum = PspCurrentThread->InitTickNum - PspCurrentThread->RemainderTicks;

		// 时间片耗尽
		if(PspCurrentThread->RemainderTicks == 0) {
			// 如果不处于最低优先级
			if(PspCurrentThread->Priority > 0) {
				// 降低优先级
				PspCurrentThread->Priority;
				// 增加初始时间片
				PspCurrentThread->InitTickNum += TICKS_OF_TIME_SLICE;
			}
				
			// 重置时间片为初始时间片
			PspCurrentThread->RemainderTicks = PspCurrentThread->InitTickNum;

			// 存在和被中断线程优先级相同的就绪线程
			if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
				// 将被中断线程转入就绪状态，并插入队尾
				PspReadyThread(PspCurrentThread);
			}
		}
	}

	return;
```

7. 由于EOS没有提供鼠标，可以使用键盘事件或者控制台命令使线程优先级提升。

```c
if(KeyEventRecord.VirtualKeyValue == VK_SPACE && PspCurrentThread->State == Running && PspCurrentThread->Priority > 0 && PspCurrentThread->Priority < 8 )
		PspCurrentThread->Priority = 8;
```

8. 使用控制台命令提升线程优先级，在EOS操作系统中实现一个“upt”命令,通过输入的线程ID来提升对应线程的优先级。

eg. `upt 24`

```c
PRIVATE
VOID
ConsoleUpThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	);

// ...

PRIVATE
VOID
ConsoleUpThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
{
	BOOL IntState;
	ULONG ThreadID;
	PTHREAD pThreadCtrlBlock;
	STATUS Status;
	//
	// 从命令参数字符串中获得线程 ID。
	//
	ThreadID = atoi(Arg);
	
	
	//
	// 由进程 ID 获得进程控制块
	//
	Status = ObRefObjectById(ThreadID, PspThreadType, (PVOID*)&pThreadCtrlBlock);
	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	// 如果是处于运行状态或阻塞状态,直接设置优先级。
	if(pThreadCtrlBlock->State == Running || pThreadCtrlBlock->State == Waiting){
		pThreadCtrlBlock->Priority = 8;
	}
	// 如果是处于就绪状态的线程，需要先将该线程移出队列，
	// 然后设置该线程的优先级为8，恢复线程的初始时间片大小和剩余时间片大小
	else if(pThreadCtrlBlock->State == Ready){
	 	ListRemoveEntry(&pThreadCtrlBlock->StateListEntry);
	 	
	 	pThreadCtrlBlock->State = Zero;								// 设置为游离状态
	 	pThreadCtrlBlock->Priority = 8;								// 设置优先级为8
	 	pThreadCtrlBlock->InitTickNum = 8 * TICKS_OF_TIME_SLICE;		// 设置时间片剩余大小
	 	pThreadCtrlBlock->RemainderTicks = pThreadCtrlBlock->InitTickNum;			// 设置初始化时间片大小
	 	
	 	PspReadyThread(pThreadCtrlBlock);
	 }
	
	PspThreadSchedule();//线程调度
	KeEnableInterrupts(IntState);	// 开中断
	
	return;
}
```

```c
// ...
else if (0 == stricmp(Line, "upt)) {
	ConsoleUpThread(StdHandle, Arg);
	continue;
} 
```



