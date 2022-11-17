# EOSʵ���ĵ��ϼ�

---

## Lab1 ����ϵͳ����

�ο�word�ĵ�

---

## Lab2 �̵߳�״̬��ת��

### Ԥ��֪ʶ

#### �̶߳�����Щ״̬

- ����
- ����
- ����
- ����

#### EOS����ζ�����Щ״̬

EOS�̵߳�״̬��**�߳̿��ƿ�TCB**�е�`State��`���棬���ļ�`ps/psp.h`�ж�����߳�״̬����

```cpp
 typedef enum _THREAD_STATE {
    Zero,       // �߳�״̬ת�������е��м�״̬
    Ready,      // ����
    Running,    // ����
    Waiting,    // �ȴ�(����)
    Terminated  // ����
 } THREAD_STATE;
 ```

 #### �Ķ����Դ����

 EOS�е��߳�ת�����ն�����`ps/sched.c`�е�`Psp*`����ִ�еģ������в��Ķ�ӦԴ����

#### ����״̬��Resumeԭ�Suspendԭ��

Ϊ�߳���ӹ���״̬����Ͳ��ɱ�����漰���˻״̬�����״̬��ת�����������������ԭ�Resume��Suspend

Resume ���𽫹���״̬���߳����»ָ����״̬

Suspend ���𽫻״̬���߳�ת�����״̬

#### �ٽ�

**�ٽ���Դ** ָ����������̿���ͬʱ���ʵ�Ӳ���������Դ

**�ٽ�����Critical Section��** �����ٽ���Դ�Ĵ���

**�����壨Mutex��** Ϊ��֤�����̻�������ٽ�����������ź���

```
Lock Mutex
(Critical Section)
Relaese Mutex 
```

#### EOS�ں�ͬ��

EOS�е��߳������ں˼��������̲߳���ִ�У���������������

EOS�ṩ������ͬ������

- �������Mutex��
- �ź�������Semaphore��
- �¼�����Event��

������ͬ�������漰������״̬`signaled`��`nosignaled`�����̼�ͬ���������Ƕ�������״̬�����л�����ͬ�����в�ͬ���л��߼�������ɲο�Դ����

#### �̵߳���

EOS���߳��ǵ��ȵĻ�����λ���㷨Ϊ**�������ȼ�������ʽ����**

EOSʹ��˫������洢ͬһ�����ȼ��Ķ��У���������32������˫�����������±��ʾ��ͬ���ȼ���С��ͬʱ����һ��32λ����λͼ��ʾ����Ϊn�Ķ������Ƿ����߳�

���ڲ�ͬ���ȼ����̣߳����ȼ�Խ��Խ���ȱ����ȣ�ͬһ���ȼ��е��߳������FCFS�ķ�ʽ�������ȷ��񡱡�

�̵߳������õ���ר��Ϊ������Ƶ�48�����жϣ���������`PspSelectNextThread`�����������ñ��жϵ��̼߳���ִ�У����Ǵ����С��������߳���ѡ��һ����ִ�У������������**���ȳ���**

#### ����̨��ǲ�߳�

EOS������ͻᱻ�����������������¼�ʱ�Żᱻ���ѣ��������¼���ǲ����Ӧ����̨���������״̬�ȴ���һ�λ���

### EOS�߳�״̬ת��

#### ����Դ�������

##### �߳�״̬

���߳�״̬�Ķ������£�������5��״̬��������������Ч״̬���ֱ�Ϊ������ (Ready)������ (Running)���ȴ� (Waiting) �ͽ��� (Terminated)��Zero״̬Ϊ�߳�״̬ת���м�̬�������ж�������

```cpp
typedef enum _THREAD_STATE {
	Zero,		// 0
	Ready,		// 1
	Running,	// 2
	Waiting,	// 3
	Terminated	// 4
} THREAD_STATE;

#define TS_CREATE 	0	// ����
#define TS_READY	1	// ����̬
#define TS_RUNNING	2	// ����̬
#define TS_WAIT		3	// ����̬
#define TS_STOPPED	4	// ����
```

##### �߳�״̬ת��

**PspReadyThread** ��ָ���̲߳��������ȼ���Ӧ�ľ������еĶ�β�����޸���״̬��Ϊ`Ready`��

```c
VOID
PspReadyThread(
	PTHREAD Thread
	)
{
    // ȷ���̷߳ǿ�
	ASSERT(NULL != Thread);
    // ȷ���̴߳�������̬Zero��������̬Running
	ASSERT(Zero == Thread->State || Running == Thread->State);

	// ���̲߳��������ȼ���Ӧ�ľ������еĶ�β
	ListInsertTail(&PspReadyListHeads[Thread->Priority], &Thread->StateListEntry);
    // ���þ���λͼ
    BIT_SET(PspReadyBitmap, Thread->Priority);
    // ���̵߳�״̬�޸�Ϊ����״̬
	Thread->State = Ready;

#ifdef _DEBUG
    // Debug Outputs
	RECORD_TASK_STATE(ObGetObjectId(Thread) , TS_READY, Tick);
#endif
}
```

**PspUnreadyThread** ��ָ���̴߳Ӿ����������Ƴ������޸���״̬��Ϊ`Zero`��

```c
VOID
PspUnreadyThread(
	PTHREAD Thread
	)
{
    // ȷ�������̷߳ǿ��Ҵ��ھ���̬
	ASSERT(NULL != Thread && Ready == Thread->State);
	// ���̴߳����ڵľ���������ȡ��
	ListRemoveEntry(&Thread->StateListEntry);

    // �ж�ȡ�����߳����ȼ���Ӧ�ľ��������Ƿ�Ϊ��
	if(ListIsEmpty(&PspReadyListHeads[Thread->Priority])) {
        // �������λͼ�ж�Ӧλ
		BIT_CLEAR(PspReadyBitmap, Thread->Priority);
	}
    // �����߳�״̬ΪZero
	Thread->State = Zero;
}
```

**PspWait** ����ǰ�����̲߳���ָ���ȴ����еĶ�β�����޸�״̬��Ϊ Waiting��Ȼ��ִ���̵߳��ȣ��ó���������

```c
STATUS
PspWait(
	IN PLIST_ENTRY WaitListHead,
	IN ULONG Milliseconds
	)

{
    // ȷ����ǰִ���ڷ��жϻ�����
	ASSERT(0 == KeGetIntNesting());
    // ȷ���߳�Ϊ����̬
	ASSERT(Running == PspCurrentThread->State);
    // ȷ������λͼ�ǿ�
	ASSERT(0 != PspReadyBitmap);

    // ���޵ȴ�ʱ���þ�����ʱ�˳�
	if(0 == Milliseconds) {
		return STATUS_TIMEOUT;
	}

    // ����ǰ�̲߳���ȴ����еĶ�β
	ListInsertTail(WaitListHead, &PspCurrentThread->StateListEntry);
    // �޸��߳�״̬ΪWaiting
	PspCurrentThread->State = Waiting;
	
#ifdef _DEBUG
    // Debug Outputs
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_WAIT, Tick);
#endif

	if (INFINITE != Milliseconds) {
        // ע��һ�����ڳ�ʱ�����̵߳ĵȴ���ʱ��
		KeInitializeTimer( &PspCurrentThread->WaitTimer,
						   Milliseconds,
						   PspOnWaitTimeout,
						   (ULONG_PTR)PspCurrentThread );
		KeRegisterTimer(&PspCurrentThread->WaitTimer);
		PspCurrentThread->WaitStatus = STATUS_TIMEOUT;
	} else {
        // ���õȴ�
		PspCurrentThread->WaitStatus = STATUS_SUCCESS;
	}


	// ��ǰ�߳̽���ȴ�״̬��ִ���̵߳��ȡ�
	PspThreadSchedule();

	// �̱߳����ѣ����صȴ����״̬�롣
	return PspCurrentThread->WaitStatus;
}
```

**PspUnwaitThread** �������̴߳������ڵĵȴ��������Ƴ������޸���״̬��Ϊ`Zero`��

```c
VOID
PspUnwaitThread(
	IN PTHREAD Thread
	)
{
    // ȷ���̴߳��ڵȴ�״̬
	ASSERT(Waiting == Thread->State);

	// ���̴߳����ڵȴ��������Ƴ�
	ListRemoveEntry(&Thread->StateListEntry);
    // �޸��߳�״̬ΪZero
	Thread->State = Zero;

	if (STATUS_TIMEOUT == Thread->WaitStatus) {
        // ע���ȴ���ʱ��
		KeUnregisterTimer(&Thread->WaitTimer);
	}
}
```

**PspWakeThread** �ú������ȵ���`PspUnwaitThread`����ʹ�߳���������״̬��Ȼ���ٵ���`PspReadyThread`����ʹ�߳̽������״̬���Ӷ����ѱ��������̡߳�

```c
PTHREAD
PspWakeThread(
	IN PLIST_ENTRY WaitListHead,
	IN STATUS WaitStatus
	)

{
	PTHREAD Thread;

    // �ȴ����зǿ�
	if (!ListIsEmpty(WaitListHead)) {
		// ѡ������߳�
		Thread = CONTAINING_RECORD(WaitListHead->Next, THREAD, StateListEntry);
		// �����߳�
        PspUnwaitThread(Thread);
		PspReadyThread(Thread);

		// �����̷߳���ֵ
		Thread->WaitStatus = WaitStatus;

	} else {
		Thread = NULL;
	}
	return Thread;
}
```

**PspSelectNextThread** �̵߳��ȳ�������ʹ�����ȵ��̴߳�����״̬�������״̬���������ĸ������߳�Ӧ�ý�������״̬��

```c
PCONTEXT
PspSelectNextThread(
	VOID
	)
{
	ULONG HighestPriority;
	SIZE_T StackSize;

	// ɨ�����λͼ����õ�ǰ������ȼ���
	BitScanReverse(&HighestPriority, PspReadyBitmap);

    // ��ǰ�̲߳�Ϊ���Ҵ�������̬
	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {

        // ����λͼ�ǿ�
		if (0 != PspReadyBitmap && HighestPriority > PspCurrentThread->Priority) {
			// ����ǰ�̲߳������Ӧ���ȼ��������еĶ���
			ListInsertHead( &PspReadyListHeads[PspCurrentThread->Priority],
							&PspCurrentThread->StateListEntry );
			BIT_SET(PspReadyBitmap, PspCurrentThread->Priority);
			PspCurrentThread->State = Ready;

// Debug info
#ifdef _DEBUG
			RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_READY, Tick);
#endif

		} else {
			// ������ǰ�߳�
			MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
			return &PspCurrentThread->KernelContext;
		}

	} else if(0 == PspReadyBitmap) {

		//
		// ���ж������̴߳��ڷ�����״̬���������һ�������еľ����̡߳�
		//
		ASSERT(FALSE);
		KeBugCheck("No ready thread to run!");
	}

    // ��ǰ�̲߳�Ϊ��
	if (NULL != PspCurrentThread) {
		// ��ǰ�߳̽���
		if (Terminated == PspCurrentThread->State) {
            // �ͷ��߳�ռ�õ�ջ
			StackSize = 0;
			MmFreeVirtualMemory( &PspCurrentThread->KernelStack,
								 &StackSize,
								 MEM_RELEASE,
								 TRUE );
		}
		// ȡ��ָ������
		ObDerefObject(PspCurrentThread);
	}


	// ѡ�����ȼ���ߵķǿվ������еĶ����߳���Ϊ��ǰ�����߳�
	PspCurrentThread = CONTAINING_RECORD(PspReadyListHeads[HighestPriority].Next, THREAD, StateListEntry);
	ObRefObject(PspCurrentThread);

    // �Ӿ��������Ƴ�����ΪZero̬
	PspUnreadyThread(PspCurrentThread);
    // ��Ϊ����̬
	PspCurrentThread->State = Running;
	
// Debug Info
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_RUNNING, Tick);
#endif

	// �����̰߳����еĵ�ַ�ռ䡣
	MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
	// �����̵߳�������
	return &PspCurrentThread->KernelContext;
}
```

#### �ܽ�

EOS�߳�״̬ת������Ƶ��ĺ������ݽṹΪ��ʾͬ���ȼ��Ķ��У���˫������洢

�漰�����㷨������`ps/sched.c`�У�����ʵ��״̬ת��������ϸ���Ѿ���Դ���������������

#### ����

��`ke/sysproc.c`�ļ���`LoopThreadFunction`����Ӷϵ㣬�������Բ�ִ��`loop`���ֹͣ��ɾ����ǰ�ϵ㣬���ֱ���PspReadyThread��PspUnreadyThread��PspWait��PspUnwaitThread��PspSelectNextThread����Ӷϵ㣬�Ա����߳��л�״̬ʱDebug����ɺ�ʼ����

##### ����->����

���¿ո������KdbIsr���ã���ʱ�ᷢ��loop����ֹͣ���Ա��ʽ`*Thread`���м��ӣ�������������ThreadΪ��β�ı�����ͨ����State�ļ��ӿ��Ի�õ�ǰloop���̵�״̬�룬ͨ�����ö�ջ���Բ鿴��غ������ã��Ӷ���ɶ�`����->����`���̵ĸ��١�

##### ����->����

��F5����ִ�У��ڶϵ�PspSelectNextThread��ֹͣ����ʱ�Ա��ʽ`*PspCurrentThread`���м��ӣ�ע���¼ Next �� Prev ָ���ֵ����󵥲�����ֱ���������������Կ�����Ӧָ��ֵ��0��Ϊ����ֵ�������߳�״̬��ı䣬�Ӷ���ɶ�`����->����`���̵ĸ��١�

##### ����->����

��F5����ִ�У��ڶϵ�PspUnreadyThread��ֹͣ����ʱ�Ա��ʽ`*Thread`���м��ӣ�ע��۲�State��ֵ����󵥲�����ֱ����������������State��0x1�����0x2��˵���߳���Ready̬������Running̬���Ӷ���ɶ�`����->����`���̵ĸ��١�

##### ����->����

��F5����ִ�У��ڶϵ�PspUnreadyThread��ֹͣ����ʱ�Ա��ʽ`*PspCurrentThread`���м��ӣ�ע���¼ Next �� Prev ָ���ֵ����󵥲�����ֱ��State���ֵΪ0x3��˵���߳�������̬����������̬���Ӷ���ɶ�`����->����`���̵ĸ��١�


### ʵ��Resumeԭ��

#### ��Ҫ����

������Ҫ�ж��߳��Ƿ�ΪNULL������ղο��������ListRemoveEntry�������̴߳ӹ����̶߳������Ƴ�������PspReadyThread�������ָ̻߳�Ϊ����״̬��������PspThreadSchedule�꺯��ִ���̵߳��ȣ��øոջָ����߳��л���ִ�С�

#### Դ����

��`ps/psspnd.c`�ļ���`119`�е�`PsResumThread`�����м������´���

```c
ASSERT(NULL != Thread)
// 1. ���ȵ���ListRemoveEntry�������̴߳ӹ����̶߳������Ƴ��� 
ListRemoveEntry(&Thread->StateListEntry);
// 2. Ȼ�����PspReadyThread�������ָ̻߳�Ϊ����״̬��
PspReadyThread(Thread);
// 3. ������PspThreadSchedule�꺯��ִ���̵߳��ȣ��øոջָ����߳��л���ִ�С�
PspThreadSchedule();
```

#### ����

����Bochs�����������ִ��`loop`�����̺߳�Ϊ`24`���̣߳�����л�����һ��Console��ִ��`Suspend 24`�����̣߳����`Resume 24`���ظ�������ȷ��Resume��������

### �������

��ʵ������ͨ���ϵ���ԣ����Դ����۲���EOS�߳�״̬��ת��������Լ���д��Resumeԭ�ʵ����Resume�����������˶�EOS�̵߳��ȵȵ���⡣

EOS���߳�״̬��ת���ص��ǣ��������OS��EOS���̲߳����ڽ��̣�ÿ���̶߳����ں˼�����˶Խ��̵ĵ����൱���̵߳ĵ��ȡ�EOS�߳���32�����ȼ�����ͬ���ȼ������ȼ�˳����ȣ�ͬһ���ȼ����е��а�ʱ��Ƭ��ת��FCFS��ԭ����е��ȡ�EOS������ƵĲ�������ǲ����������̣����Գ��Խ��̹߳����ڽ��̣����Խ��̽��е��ȡ�

ΪEOS���ӵ�Resume��Suspend����Ч�ģ�ͨ�����Կ�����֤������֮���ǵ��̱߳�����ʱ��ǰ����̨�ᱻ���������û��������Ѻã����ԸĽ���


---

## Lab3 ����ͬ��

### ʵ��Ŀ��

- ʹ��EOS���ź�������̽�������ߡ����������⣬������ͬ�������塣
- ���Ը���EOS�ź����Ĺ������̣�������ͬ����ԭ��
- �޸�EOS���ź����㷨��ʹ֧֮�ֵȴ���ʱ���ѹ���(���޵ȴ�)������������ͬ����ԭ��

### Ԥ��֪ʶ

#### EOS�ں˵�����ͬ������

- �������Mutex��
- �ź�������Semaphore��
- �¼�����Event��

������ͬ�������漰������״̬`signaled`��`nosignaled`�����̼�ͬ���������Ƕ�������״̬�����л�����ͬ�����в�ͬ���л��߼�������ɲο�Դ����

EOS���������µ��ã����̶߳�ͬ������������ֵ���ʱ������`nosignaled`�Ķ������`signaled`

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

EOS�ж�������

```c
//
// �����ź����ṹ��
//
typedef struct _MUTEX {
	PVOID OwnerThread;			// ��ǰӵ�� Mutex ���߳�ָ��
	ULONG RecursionCount;		// �ݹ�ӵ�� Mutex �ļ�����
	LIST_ENTRY WaitListHead;	// �ȴ�����
}MUTEX, *PMUTEX;
```

`OwnerThread==NULL`ʱ`Mutex`����`singaled`̬����ʱ��Mutex�������Wait���ûὫ��ָ��ָ����õ��̵߳�ַ���������ʱMutex���ڴ���`nosingaled`̬�������ȴ�����

##### Semaphore

> ���½���ժ��[ά���ٿ�](https://zh.m.wikipedia.org/zh-hans/%E4%BF%A1%E5%8F%B7%E9%87%8F)

�ź�����Ӣ�semaphore���ֳ�Ϊ�źű꣬��һ��ͬ���������ڱ�����0��ָ�����ֵ֮���һ������ֵ�����߳����һ�ζԸ�semaphore����ĵȴ���wait��ʱ���ü���ֵ��һ�����߳����һ�ζ�semaphore������ͷţ�release��ʱ������ֵ��һ��������ֵΪ0�����̵߳ȴ���semaphore�������ܳɹ�ֱ����semaphore������signaled״̬��semaphore����ļ���ֵ����0��Ϊsignaled״̬������ֵ����0��Ϊnonsignaled״̬��

EOS�ж�������

```c
//
// ��¼���ź����ṹ��
//
typedef struct _SEMAPHORE {
	LONG Count;					// �ź���������ֵ
	LONG MaximumCount;			// �������ֵ
	LIST_ENTRY WaitListHead;	// �ȴ�����
}SEMAPHORE, *PSEMAPHORE;
```

##### Event

> **ʡ��**��Event����ʵ����δ�漰��������Ȥ��������������

```c
//
// �¼��ṹ��
//
typedef struct _EVENT {
	BOOL IsManual;				// �Ƿ��ֶ������¼�
	BOOL IsSignaled;			// �Ƿ��� Signaled ״̬
	LIST_ENTRY WaitListHead;	// �ȴ�����
}EVENT, *PEVENT;
```

Event������ͬ����������Ϊ����Ϊ��`signaled`��`nosignaled`��ȫ�ɳ�����ƣ�����������е��á�

Event�������ֶ����Զ���������ǵ�����ɺ��¼��Ƿ���Զ��ָ���Reset��Ϊ`nosignaled`״̬��


#### �����ߡ�������������

������-������������һ�������Ľ���ͬ�����⡣����������:��һȺ�����߽���������ĳ�ֲ�Ʒ�������˲�Ʒ�ṩ��һȺ�����߽���ȥ���ѡ�Ϊʹ�����߽��̺������߽����ܲ���ִ�У�������֮��������һ������n���������Ļ���أ������߽��̿��Խ���������һ����Ʒ����һ���������У������߽��̿��Դ�һ����������ȡ��һ����Ʒ���ѡ��������е������߽��̺������߽��̶������첽��ʽ���еģ�������֮����뱣��ͬ�����������������߽��̵�һ���ջ�����ȥȡ��Ʒ��Ҳ�����������߽�����һ���Ѿ�װ�в�Ʒ�Ļ������з����Ʒ

#### CreateThread����

�÷������̴���Ϊ�������Ǵ���һ���հ׵��߳̿��ƿ飬Ȼ��Ϊ�̷߳���ջ������ʼ���߳� �������Ļ��������ʹ�߳̽������״̬

������������

```c
HANDLE CreateThread(
	// �û�ģʽ�߳�ջ�Ĵ�С�������ǰ������ϵͳ���������֮��Ŀǰ�����̶߳�ִ�����ں�ջ�У�������ʱ���á�
	IN SIZE_T StackSize,
	// �߳̿�ʼִ�еĺ�����ָ��
	IN PTHREAD_START_ROUTINE StartAddr,
	// ���ݸ��̺߳����Ĳ���
	IN PVOID ThreadParam,
	// ָ�����ڱ����߳̾���ı���
	IN ULONG CreateFlags,
	// ָ�����ڱ����߳�ID�ı���
	OUT PULONG ThreadId
);
```

�ڵ���`CreateThread`���������߳�֮ǰ��Ҫ���Ȱ����߳���ں������͵Ķ��壬��дһ���߳���ں�����

```c
typedef ULONG (*PTHREAD_START_ROUTINE)( PVOID ThreadParameter );
```

### ʵ�鲽��

#### ʹ��EOS���ź���ʵ��������-����������

> ����ʹ��EOS���ź������������-�����������ʵ�ַ���������ʵ�ַ����ļ�Ҫ������Դ���롢���Լ������


##### ������Դ����

���ȶ��建��ش�С����Ʒ���������ô�����Ӧ��С�Ļ����

```c
#define BUFFER_SIZE		10
int Buffer[BUFFER_SIZE];

#define PRODUCT_COUNT	30
```

��󴴽������ߺ�����������ͬ����Handle

```c
HANDLE MutexHandle;
HANDLE EmptySemaphoreHandle;
HANDLE FullSemaphoreHandle;
```

�������������������������̣߳�����EOS���ԣ���Ҫ���ȱ�д���Ե��߳���ں���

�������̺߳����ж���һ���±겢�ƶ���ʾ����������ʱ��Ҫ�����ٽ���ԴBuffer�ȣ�ͬʱ����һ���������������Դ��������

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

�������̺߳����ж���һ���±겢�ƶ���ʾ���ѣ�����ʱ��Ҫ�����ٽ���ԴBuffer�ȣ�ͬʱ����һ�����Ѽ��������ǰ10�����������������ѣ�����Դ��������


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

�������д���Mutexȷ���������������߻���ط����ٽ���Դ��������أ�ͬʱ����full��empty�����ź�����ʾ������п�/�������������������ʹ��֮ǰ��д����ں��������̣߳����ȴ��߳̽������رվ��������0������ģ����Mutex����ʧ�ܽ�����1��Empty����ʧ�ܷ���2��Full����ʧ�ܽ�����3���������̴߳���ʧ�ܽ�����4���������̴߳���ʧ�ܽ�����5��

```c
int main(int argc, char* argv[])
{
	HANDLE ProducerHandle;
	HANDLE ConsumerHandle;

	// �������ڻ�����ʻ���ص� Mutex ����
	MutexHandle = CreateMutex(FALSE, NULL);
	if (NULL == MutexHandle) {
		return 1;
	}

	// ���� Empty �ź�������ʾ������пջ�������������ʼ��������������Ϊ BUFFER_SIZE��
	EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);
	if (NULL == EmptySemaphoreHandle) {
		return 2;
	}

	// ���� Full �ź�������ʾ�����������������������ʼ����Ϊ 0��������Ϊ BUFFER_SIZE��
	FullSemaphoreHandle = CreateSemaphore(0, BUFFER_SIZE, NULL);
	if (NULL == FullSemaphoreHandle) {
		return 3;
	}


	ProducerHandle = CreateThread( 0,			// Ĭ�϶�ջ��С
								   Producer,	// �̺߳�����ڵ�ַ
								   NULL,		// �̺߳�������
								   0,			// ������־
								   NULL );		// �߳� ID

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


	// �ȴ��������̺߳��������߳̽�����
	WaitForSingleObject(ProducerHandle, INFINITE);
	WaitForSingleObject(ConsumerHandle, INFINITE);
	// �رվ��
	CloseHandle(MutexHandle);
	CloseHandle(EmptySemaphoreHandle);
	CloseHandle(FullSemaphoreHandle);
	CloseHandle(ProducerHandle);
	CloseHandle(ConsumerHandle);
	return 0;
}
```


##### ���Խ��

�ο�word�ĵ�


#### EOS�ź����������̵ĸ�����Դ�������

##### ���Ը���

���ȶ�EOS�ں˽������ɣ��ֱ�����Debug��Release��󽫵õ���sdk�ļ��и��Ƶ���ǰʵ�黷�����滻��

��Ӷϵ���`EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);`������ʼ���ԣ����е��ϵ㴦ʱ�������Խ���`semaphore.c`�У���`PsInitializeSemaphore`�����һ���ϵ㣬�������ԣ��ڶϵ㴦�򿪼�¼���ź������ڣ��õ���������

<!-- Pic -->

ɾ��֮ǰ�Ķϵ㣬��`WaitForSingleObject(EmptySemaphoreHandle, INFINITE);`����Ӷϵ㣬�������ԣ������`PsWaitForSemaphore����`���һ���ϵ㣬�������Թ۲쵽Empty�ź�������������1��10->9��:

<!-- Pic -->

ɾ��֮ǰ�Ķϵ㣬��`ReleaseSemaphore(FullSemaphoreHandle, 1, NULL);`����Ӷϵ㣬��󵥲����Թ۲쵽��¼���ź���������Full�ź�����ֵ������1

<!-- pic -->

�ؿ����ԣ���`PspWait(&Semaphore->WaitListHead, INFINITE);`����Ӷϵ㣬�鿴Empty����ֵΪ-1����ʾΪ0xffffffff���ڵ��ö�ջ���ҵ�Producer�������ٲ鿴i��ֵ���õ�0xe����ʮ���Ƶ�14��

<!-- Pic*2 -->

ɾ�����жϵ㣬����`ReleaseSemaphore(EmptySemaphoreHandle, 1, NULL);`����Ӷϵ㣬�������ԣ��鿴��¼���ź������ڣ��۲쵽���������鿴Consumer������i��ֵ��Ϊ0x4��˵���ĺ��ѱ����ѡ�����`PsReleaseSemaphore`�۲쵽Empty����Ϊ-1������������`PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);`���۲쵽�ոյ�ֵ��Ϊ0��Ȼ��������������̼߳���ִ�С�

<!-- pic*3 -->

��`PsWaitForSemaphore`�����������϶ϵ㣬�������ԣ��۲쵽Empty����Ϊ0��Producer��i=14��

<!-- pic*2 -->

##### Դ�������

�ο�Ԥ��֪ʶ����

#### ֧�ֵȴ���ʱ���Ѻ������ͷŹ��ܵ��ź���ʵ��

##### ��Ҫ������Դ����

�޸ĺ��`PsWaitForSemaphore`������ע������

```c
// ����ֵ
STATUS Ret;

ASSERT(KeGetIntNesting() == 0); // �жϻ����²��ܵ��ô˺�����

IntState = KeEnableInterrupts(FALSE); // ��ʼԭ�Ӳ�������ֹ�жϡ�

Semaphore->Count--;
if (Semaphore->Count <= 0) {
	// ����ֵ����0ʱ������PspWait���������̵߳�ִ��
	Ret = PspWait(&Semaphore->WaitListHead, Milliseconds);
} 
else if (Semaphore->Count > 0) {
	// ����ֵ���� 0 ʱ��������ֵ�� 1 ��ֱ�ӷ��سɹ�
	Semaphore->Count -= 1;
	Ret = STATUS_SUCCESS;
}

KeEnableInterrupts(IntState); // ԭ�Ӳ�����ɣ��ָ��жϡ�

return Ret;
```


�޸ĺ��`PsReleaseSemaphore`������ע������

```c
// ѭ���ͷ�ReleaseCount������
while (ReleaseCount>0) {
	// ������������С��Releaseʱ���ͷ���ȫ��������ѭ��
	if(ListIsEmpty(&Semaphore->WaitListHead))
		break;
	// ÿ��ѭ���ͷ�һ���̣߳�������ReleaseCount
	PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);
	ReleaseCount--;
}

// �����ź����ļ�����
Semaphore->Count += ReleaseCount;
```

##### ���Լ����

���������ںˣ���sdk�ļ��и��Ƶ�EOSApp��ĿĿ¼�У���������EOSApp��Ŀ�������������ԣ��۲쵽�����������С�

<!-- pic -->

���ΪProducer�����ĵȴ�Empty��Consumer�����ĵȴ�Full�ֱ����������䣬��ӡ��Ϣ�����µ��ԣ�������¡�

<!-- pic -->

���������滻Ϊ`NewConsumer`�еĺ������ٴ���֤�������ȷ

<!-- pic -->

### �������

<!--  -->


#### EOS�ź���

EOS�ź�����EOS��һ��ͬ���������ڱ�����0��ָ�����ֵ֮���һ������ֵ�����߳����һ�ζԸ�semaphore����ĵȴ���wait��ʱ���ü���ֵ��һ�����߳����һ�ζ�semaphore������ͷţ�release��ʱ������ֵ��һ��������ֵΪ0�����̵߳ȴ���semaphore�������ܳɹ�ֱ����semaphore������signaled״̬��semaphore����ļ���ֵ����0��Ϊsignaled״̬������ֵ����0��Ϊnonsignaled״̬��

#### ��ʱ���Ѻ������ͷ�

��ʱ���������˼�������������ֵ�����Զ����ѣ������ͷ�ͨ��ѭ��ÿ�������ͷ�ReleaseCount���̡߳�����������֤��ʵ�־���Ч��


---

## Lab4 �̵߳���

### ʵ��Ŀ��

- ����EOS���̵߳��ȳ�����Ϥ�������ȼ�������ʽ���ȡ�
- ΪEOS���ʱ��Ƭ��ת���ȣ��˽��������õĵ����㷨��

### Ԥ��֪ʶ

#### �������ȼ�������ʽ����

EOSʹ��˫������洢ͬһ�����ȼ��Ķ��У���������32������˫�����������±��ʾ��ͬ���ȼ���С��ͬʱ����һ��32λ����λͼ��ʾ����Ϊn�Ķ������Ƿ����߳�

���ڲ�ͬ���ȼ����̣߳����ȼ�Խ��Խ���ȱ����ȣ�ͬһ���ȼ��е��߳������FCFS�ķ�ʽ�������ȷ��񡱡�


#### ���ȳ���ִ�е�ʱ��������

> �̵߳������õ����жϣ�Ҫ�˽���ȵ���������Ҫ�˽��жϴ�������̡�
> 
> EOS�����жϴ�����Interrupt���ж�ʱ���ȵ���InterEnter�����ж�ʱCPU�ֳ���KernelContext�У�������KiDispatchInterrupt��ǲ�ж����жϴ�����������ɵ���IntExit�˳��жϣ��ָ��ֳ�

����ʱ��������ִ���жϴ������̣���IntExit�е���`PspSelectNextThread`����������Ҫ�ָ���CPU�ֳ�����ѡ��һ���̻߳��CPU�������������**���ȳ���**

�жϲ����������ⲿ�豸������Ҳ�������߳���������48�����жϲ�����������ʵ�����̵߳���

���ȳ�������̿�����α�����ʾ����

```
PspSelectNextThread
if ���ж��߳�State == Running then:
	if ���ڸ������ȼ��߳� then:
		���ж��߳�State = Ready
		return ������ȼ����������߳�
	else
		return ���ж��߳�
else
	return ������ȼ����������߳�
```

#### ʱ��Ƭ��ת����

���̵߳���ִ��ʱ����CPU����������̣߳����̵߳�ʱ��Ƭ����󣬻�����Ϊ������һ��ʱ��Ƭ���������ƶ����������е�ĩβ���Ӷ����µĶ����߳̿�ʼִ�С�

### ʵ�鲽��

#### EOS�������ȼ�����ռʽ���ȹ������̵ĸ�����Դ�������

> ����EOS�������ȼ�����ռʽ���ȵĺ���Դ���룬������ʵ�ַ���������������ݽṹ���㷨�ȣ���Ҫ˵���ڱ�����ʵ���������ɵ���Ҫ������������EOS���ȳ���ĸ��ٵ�


##### Դ�������

EOSʹ��˫������洢ͬһ�����ȼ��Ķ��У���������32������˫�����������±��ʾ��ͬ���ȼ���С��ͬʱ����һ��32λ����λͼ��ʾ����Ϊn�Ķ������Ƿ����߳�


##### ���̸���

������rr��fprintf����Ӷϵ㣬����������ԣ�����rr�����е��ϵ㴦ʱ�鿴�����̴߳��ڣ��۲쵽��rr�������߳�����
 
����24-33���̣߳��õ����½��
 
ˢ�¾����̶߳��У��õ����½��
 
ͬʱ�۲쵽Y��ֵΪ0
 
�ఴ����F5����Ȼ�õ���ͬ�Ľ��
��������BitScanReverse���϶ϵ㣬����ִ�У�ˢ�¾����̶߳��У��õ����½��
 
��*PspCurrentThread���м��ӣ��������
 
����������������PspSelectNextThread��������ǰ 
��ps/sched.c�ļ���PspSelectNextThread�����ĵ�402�����һ���ϵ㣬�������ԣ��۲쵽���ȼ�Ϊ24���̣߳��������
 
����������408�У��۲쵽�½��ĵ�0���߳��Ѿ��ҽ��������ȼ�Ϊ8�ľ������еĶ��ף�����������455�У��۲쵽����ִ�еĵ�0���½����߳��Ѿ������˾���״̬���ó���CPU������ִ�е�473�У��۲쵽���ȼ�Ϊ24�Ŀ���̨��ǲ�߳��Ѿ�����������״̬��

#### ΪEOS���ʱ��Ƭ��ת����

> ����ʵ�ַ����ļ�Ҫ������Դ���롢���Լ������

##### ��Ҫ������Դ����

�ο�����ͼ5-11��д����

```c
// ���жϵ��̴߳�������̬�Ҳ�Ϊ��
if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
	// ����ʱ��Ƭ
	PspCurrentThread->RemainderTicks--;
	if(PspCurrentThread->RemainderTicks == 0) {
		// ���ʱ��ƬΪ0����ʱ��Ƭ
		PspCurrentThread->RemainderTicks=TICKS_OF_TIME_SLICE;
		// ���ںͱ��ж��߳����ȼ���ͬ�ľ����߳�
		if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			// �����ж��߳�ת�����״̬���������β
			PspReadyThread(PspCurrentThread);
		}
	}
}
```

##### ���Լ����

�����ں�����rr���۲쵽10���̶߳������ʱ��Ƭ

<!-- pic -->

### �������

#### EOS�̵߳��ȵ��ص㡢���㼰�Ľ����

EOS�̵߳��Ȳ��õ��㷨�ǻ������ȼ�������ʽ���ȣ����ڲ�ͬ���ȼ����̣߳����ȼ�Խ��Խ���ȱ����ȣ�ͬһ���ȼ��е��߳������FCFS�ķ�ʽ�������ȷ��񡱡�ȱ��Ϊ���ͬ���ȼ��г�ʱ�����е��̣߳��������̷ֲ߳���CPU����������ʵʱ����ϵͳ��Ҫ�󡣣��Ľ����Ϊ���ʱ��Ƭ��ת����

#### �̵߳��ȵ�ִ��ʱ���͹���

����ʱ��������ִ���жϴ������̣���IntExit�е���`PspSelectNextThread`����������Ҫ�ָ���CPU�ֳ�����ѡ��һ���̻߳��CPU�������������**���ȳ���**���жϲ����������ⲿ�豸������Ҳ�������߳���������48�����жϲ�����������ʵ�����̵߳��ȡ����ȳ�������̿�����α�����ʾ����

```
PspSelectNextThread
if ���ж��߳�State == Running then:
	if ���ڸ������ȼ��߳� then:
		���ж��߳�State = Ready
		return ������ȼ����������߳�
	else
		return ���ж��߳�
else
	return ������ȼ����������߳�
```

#### ΪEOS���ʱ��Ƭ��ת����

ΪEOS��ӵ�ʱ��Ƭ��ת���������ļ��飬֤��������Ч�ԡ�

---

## Lab5 ����洢��������߼���ַ�ռ����

### ʵ��Ŀ��

- ͨ���鿴����洢����ʹ�����������ϰ����ͻ��������ڴ棬�Ӷ���������洢���Ĺ�������
- ͨ���鿴�����߼���ַ�ռ��ʹ�����������ϰ����ͻ��������ڴ棬�Ӷ����ս����߼���ַ�ռ�Ĺ�������

### Ԥ��֪ʶ

#### ����洢���Ĺ���ʽ

EOSʹ�÷�ҳʽ�洢����ʽ����ҳ������ݿ�PFN Database���й�����ʵ����һ�����飬������ҳ����һ�¡���������

```c
typedef struct _MMPFN
{
	ULONG Unused : 9;		// δʹ��
	ULONG PageState : 3;	// ҳ״̬
	ULONG Next : 20;		// ��һ�����
}MMPFN, *PMMPFN;
```

����ҳĿǰ������״̬����������

```c
typedef enum _PAGE_STATE {
	ZEROED_PAGE,	// 0 ��ҳ�����У���0��ʼ��
	FREE_PAGE,		// 1 ����ҳ�����У�δ0��ʼ��
	BUSY_PAGE,		// 2 ռ��ҳ����ռ��
} PAGE_STATE;
```

`ZEROED_PAGE`��`FREE_PAGE`�����ݿ⹹����������ʱֻ��Ҫ�Ƴ��ײ�����

`MiAllocateAnyPages`��`MiAllocateZeroedPages`���������ֱ������ȴӿ��з���ռ�����ȴ���ҳ����ռ䡣

ͬʱ������ΪEOS������ҳ�߳̽������ʼ��

#### �����߼���ַ�ռ�Ĺ���ʽ

��������ʱ�Ż�Ϊ���̷�������ҳ�������ַ������VAD��¼һ�������ַ���򣬶�������

```c
typedef struct _MMVAD{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListEntry;
}MMVAD, *PMMVAD;
```

ͬʱʹ��������������VAD�����ڹ���

```c
typedef struct _MMVAD_LIST{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListHead;
}MMVAD_LIST, *PMMVAD_LIST;
```

����API����`VirtualAlloc`����һ�������ַ��ӳ�䵽�����ڴ棬����API����`VirtualFree`�ͷ��ѷ����ַ������ӳ�������ҳ��

���̵�ַ�ռ�������Ҫһ��ҳĿ¼��һ��`VAD`����EOS��ʹ��`PAS��ProcessAddressSpace��`�ṹ�嶨��

```c
typedef struct _MMPAS {
	// ���Ը����������ʹ�õĵ�ַ�ռ䡣
	LIST VadList;

	// ҳĿ¼��PTE���������ݿ��ҳ��š�
	ULONG_PTR PfnOfPageDirectory;
	ULONG_PTR PfnOfPteCounter;
}MMPAS;
```

### ʵ�����

#### EOS�����ڴ����ͻ��յ���ϰ�Լ�Դ�������

> ��ϰ�����ڴ�ķ���ͻ��գ��������Դ���룬����EOS������洢���Ĺ��������������ݽṹ���㷨�ȣ���Ҫ˵���ڱ�����ʵ���������ɵ���Ҫ����

##### EOS�����ڴ����ͻ��յ���ϰ

��sysproc�ĵ�1076�д��϶ϵ㣬��ʼ���ԣ�����pm���鿴�����ڴ洰�ڵõ����½��

<!-- pic -->

��F5�������ԣ��۲쵽���������������һ��

<!-- pic -->

 
ʹ��pm.c�ļ���ConsoleCmdPhysicalMemory�����滻sysproc.c�ļ���ConsoleCmdPhysicalMemory�����ĺ����壬�������ɲ����ԣ�����pm���õ����½��
 
<!-- pic -->

�ֱ��MiFreePages(1, PfnArray)��MiAllocateAnyPages(1, PfnArray)��Ӷϵ㣬���µ��ԣ�ˢ�������ڴ洰�ڹ۲쵽�͵�һ��һ��
 
<!-- pic -->

����MiAllocateAnyPages�ڲ��������ԣ�����ĩβ��ˢ�������ڴ洰�ڣ��õ����½�����۲쵽����ҳ���Ϊ0x409������ҳ��ԭ�ȵ�FREE��ΪBUSY���ҿ�������ҳ��������1����ʹ������ҳ��������1��������ҳ�Ǵӿ����б��з���ġ�
 
<!-- pic -->

����������MiFreePages�������ٴ�ˢ�������ڴ洰�ڣ����ָո������ҳ�ѱ����գ��������

<!-- pic -->

##### Դ�������

���������漰����`MiAllocateAnyPages`��`MiFreePages`����������

���ȷ���`MiAllocateAnyPages`�������书���Ƿ�������ҳ����������ʱ���ȶ����Ҫ�����������жϣ����ź��������ȴӿ���ҳ�����з��䣬�������ҳ���������ٴ���ҳ������䣬��ɷ�����޸Ķ�Ӧ������ֵ�����жϣ����أ���ɵ��á�

����`MiFreePages`�������书�����ͷ�����ҳ�档��������ʱ��������յ�����ҳ�Ƿ���Ч������Ч���޸���״̬ΪFree������������������ͷ������󷵻أ���ɵ��á�



#### EOS�����߼���ַ�ռ����ͻ��յ���ϰ�Լ�Դ�������

> ��ϰ�����ڴ�ķ���ͻ��գ��������Դ���룬����EOS�н����߼���ַ�ռ�Ĺ��������������ݽṹ���㷨�ȣ�������Ӧ�ý����з�������ҳ���ͷ�����ҳ��ʵ�ַ�����Ҫ������Դ���롢���Լ�����ȣ���Ҫ˵���ڱ�����ʵ���������ɵ���Ҫ����

##### ��ϰ�����ڴ�ķ���ͻ���

���ȵ���vm�����ConsoleCmdVM���������һ�д��������һ���ϵ㣬����������ԣ���������pt�鿴��ǰ�̵߳�pid���õ�1���������vm 1�鿴1���̵߳����⣨�߼����ռ䣬�õ����½��
 
<!-- pic  -->

���������Է�������ҳ��ʹ��vm.c�ļ���ConsoleCmdVM�����ĺ������滻sysproc.c�ļ���ConsoleCmdVM�����ĺ����壬�������У�����vm 1���õ����½��
 
<!-- pic  -->
 
�ֱ���MmAllocateVirtualMemory�����Ĵ����к� MmFreeVirtualMemory�����Ĵ�������Ӷϵ㣬���ʼ���ԣ�����vm 1
 
<!-- pic  -->
 
����MmAllocateVirtualMemory�������������ԣ��۲�����
 
<!-- pic  -->
 
��F5���������ͷ�����ҳ������MmFreeVirtualMemory�������������ԣ��۲���

 
<!-- pic  -->

##### Դ�������

���ȷ���`MmAllocateVirtualMemory`�������书��Ϊ�ڵ�ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ��з��������ڴ棬����ɹ��򷵻�STATUS_SUCCESS�������ʾʧ�ܡ�

��������ʱ���Ȼ�ȷ��������Ч�������жϣ����ݴ�������������룬��ɺ����÷���ֵ�����жϣ����أ���ɵ��á�

�ٷ���`MmFreeVirtualMemory`�������书��Ϊ�ڵ�ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ����ͷ������ڴ棬����ɹ��򷵻�STATUS_SUCCESS��

��������ʱ���Ȼ�ȷ��������Ч�������жϣ����ݴ�����������ͷţ���ɺ����÷���ֵ�����жϣ����أ���ɵ��á�

#### ��Ӧ�ó�������з�����ͷ�����ҳ

> �˲��ֱ�����û���漰��������Ҫ�ύ����

����һ�� EOS Ӧ�ó���Ȼ���д����������й���:
1. ����API����`VirtualAlloc`������һ�����ͱ�������Ŀռ䣬��ʹ��һ�����ͱ�����ָ��ָ���� ���ռ䡣
2. �޸����ͱ�����ֵΪ`0xFFFFFFFF`�����޸�ǰ������ͱ�����ֵ�����޸ĺ���������ͱ�����ֵ��
3. ����API����`Sleep`���ȴ�10���ӡ�
4. ����API����`VirtualFree`���ͷ�֮ǰ��������ͱ����Ŀռ䡣
5. ������ѭ��������Ӧ�ó���Ͳ��������

```c
#include "EOSApp.h"
int main(int argc, char* argv[])
{
	// 1. ����API����`VirtualAlloc`������һ�����ͱ�������Ŀռ䣬��ʹ��һ�����ͱ�����ָ��ָ������ռ䡣
	PINT space = VirtualAlloc(0, sizeof(INT), MEM_RESERVE|MEM_COMMIT);
	// 2. �޸����ͱ�����ֵΪ`0xFFFFFFFF`�����޸�ǰ������ͱ�����ֵ�����޸ĺ���������ͱ�����ֵ��
	printf("Before: 0x%X\n", *space);
	*space = 0xFFFFFFFF;
	printf("After: 0x%X", *space);
	// 3. ����API����`Sleep`���ȴ�10���ӡ�
	Sleep(1000);
	// 4. ����API����`VirtualFree`���ͷ�֮ǰ��������ͱ����Ŀռ䡣
	VirtualFree(space, 0, MEM_RELEASE);
	// 5. ������ѭ��������Ӧ�ó���Ͳ��������
	while(1);
	return 0;
}
```

### �������

�ο�word�ĵ���Ԥ��֪ʶ

---

## Lab6 FAT12�ļ�ϵͳ

### ʵ��Ŀ��

- �˽���EOSӦ�ó����ж��ļ���д�ļ��Ļ���������
- ͨ��ΪFAT12�ļ�ϵͳ���д�ļ����ܣ������FAT12�ļ�ϵͳ�ʹ��̴洢������ԭ�����⡣

### Ԥ��֪ʶ

#### �ļ�ϵͳ�������������

�û����ļ��Ķ�д����ת��Ϊ �Դ��������Ķ�д���󣬲�����Դ����������й���

#### FAT12



### ʵ�鲽��

#### EOS��FAT12�ļ�ϵͳ���Դ�������

> ����EOS��FAT12�ļ�ϵͳ�����Դ���룬��Ҫ˵��EOSʵ��FAT12�ļ�ϵͳ�ķ�����������Ҫ���ݽṹ���ļ�����������ʵ�ֵ�

�ļ�ϵͳ�ǽ����ڴ��̵ȿ��豸֮�ϵ�һ���߼��㣬��д�ļ�ʱ���ļ�ϵͳ�����Ὣ�ļ���д����ת��Ϊ�Դ��������Ķ�д���������ɴ�������������ɶԴ��������Ķ�д��

EOSΪӦ�ó����ṩ��һ����Բ����ļ���API�������������ļ�(CreateFile)���ر��ļ�(CloseHandle)�����ļ�(ReadFile)��д�ļ�(WriteFile)

���ļ�ʱʹ�õ�CreateFile��������

```c
 EOSAPI HANDLE CreateFile(
 IN PCSTR FileName,
 IN ULONG DesiredAccess,
 IN ULONG ShareMode,
 IN ULONG CreationDisposition,
 IN ULONG FlagsAndAttributes
 )
```

�ú��������ú�����ε���FatCreate��FatOpenExistingFile��FatOpenFile��FatOpenFileDirectory��

CloseHandle������������

```c
 EOSAPI BOOL CloseHandle(
 IN HANDLE Handle)
```

�ú��������ú�����ε���FatClose��FatCloseFile��

���ļ�����ReadFile��������

```c
 EOSAPI BOOL ReadFile(
 IN HANDLE Handle,
 OUT PVOID Buffer,
 IN ULONG NumberOfBytesToRead,
 OUT PULONG NumberOfBytesRead)
```

�ú��������ú�����ε���ObRead��IopReadFileObject��FatRead��FatReadFile��

д�ļ�����ReadFile��������

```c
 EOSAPI BOOL WriteFile(
 IN HANDLE Handle,
 IN PVOID Buffer,
 IN ULONG NumberOfBytesToWrite,
 OUT PULONG NumberOfBytesWritten)
```

�ú��������ú�����ε���ObWrite��IopWriteFileObject��FatWrite��FatWriteFile��


#### EOS��FAT12�ļ�ϵͳ���ļ����̵ĸ���

> ��Ҫ˵���ڱ�����ʵ���������ɵ���Ҫ�����������Զ��ļ��ĸ��ٵȣ��ܽ�EOS�ж��ļ���ʵ�ַ���

�滻EOSApp.cΪѧ�����е�FileApp.c�����ݣ����a.txt���Ƶ�img�����У�������Ŀ����������A:\eosapp.exe A:\a.txt���õ��������
 
�������ԣ���fat12.c����IDE���ڣ���EOSApp.c�ļ��е�ReadFile������������Ӷϵ㣬��ʼ���ԣ�����A:\EOSApp.exe A:\a.txt����fat12.c�ļ���FatReadFile�����Ŀ�ʼ�����һ���ϵ㡣���������е��ԣ�ͣ�º��*Vcb��*File���м��ӣ��õ����½��
 
 
ͬʱ�۲쵽offsetֵΪ0��BytesToReadΪ256��
  
�򿪵��ö�ջ��������
 
�����������Թ۲���ر����仯�����۲쵽������������


#### ΪEOS��FAT12�ļ�ϵͳ���д�ļ�����

> ����ʵ�ַ����ļ�Ҫ������Դ���롢���Լ������

���ȵ���ѧ�����еĴ��벢����
 

 
�۲쵽������ʵ���ļ���д

�޸Ĵ���ʹ��֧�ֿ�����д�ļ���ʵ��˼·Ϊ��д���������ݷ�Ϊ�����֣�ͷ�������м�������β��������д��ͷ������β�������м䣬����һ��ѭ��������д���м��Խ�Ķ���м����������ص�Ϊ����д����������д������ݾ�Ϊ512���ֽڣ��ڴ�֮ǰ�������Խ�������������ɡ�

���뼰��������


<!-- ���δ���ο���18�����е�[ʵ��](https://github.com/lyfcsdo2011/OS-Work/blob/master/mission14/fat12.c) -->

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

	// �����ڽ��·���Ĵز������β��ʱ������֪��ǰһ���صĴغţ�
	// ���Զ����ˡ�ǰһ���غš��͡���ǰ�غš�����������
	USHORT PrevClusterNum, CurrentClusterNum;
	USHORT NewClusterNum;
	ULONG ClusterIndex;
	ULONG FirstSectorOfCluster;
	ULONG OffsetInSector;
	
	ULONG i;

	// д�����ʼλ�ò��ܳ����ļ���С������Ӱ�������ļ���С�����Ӵأ�����ԭ�򣿣�
	if (Offset > File->FileSize)
		return STATUS_SUCCESS;

	// ���ݴصĴ�С������д�����ʼλ���ڴ����ĵڼ������У��� 0 ��ʼ������
	ClusterIndex = Offset / FatBytesPerCluster(&Vcb->Bpb);
	
	//
	// ������ʼ��ַ�ڴ��еĵڼ����ֽ�
	ULONG BytesNumOfCluster = Offset - ClusterIndex * FatBytesPerCluster(&Vcb->Bpb);
	
	ULONG NeedSplit;							// �ָ��־
	ULONG ByteFommer = BytesToWrite;			// ǰһ����Ҫд����ֽ�
	ULONGByteLatter = 0;						// ��һ����Ҫд����ֽ�
	ULONG SpanOfSector = 0;						// �����������
	
	PCHAR FrontBuffer;							// 		 ǰ
	PCHAR MidBuffer;							// ������ ��
	PCHAR LatterBuffer;							//		 ��
	
	// ���д�������û�п��������������з�
	if((512 - BytesNumOfCluster)>= BytesToWrite){
		NeedSplit = 0;
	}
	//����Ҫ�ֳ�����
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
		//���ֻ�����
		FrontBuffer = Buffer;
		MidBuffer = &Buffer[ByteFommer];
		LatterBuffer = &Buffer[ByteFommer + SpanOfSector * 512];
	}
	
	// ��Ϊ��������д�룬��д��ͷһ��������Ȼ��ѭ��д���м�������������512�ֽڣ������д��β����
	// д��ͷ����
	// ˳�Ŵ���������д�����ʼλ�����ڴصĴغš�
	PrevClusterNum = 0;
	CurrentClusterNum = File->FirstCluster;
	for (i = ClusterIndex; i > 0; i--) {
		PrevClusterNum = CurrentClusterNum;
		CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);	
	}

	// ���д�����ʼλ�û�û�ж�Ӧ�Ĵأ������Ӵ�
	if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {

		// Ϊ�ļ�����һ�����д�
		FatAllocateOneCluster(Vcb, &NewClusterNum);

		// ���·���Ĵذ�װ��������
		if (0 == File->FirstCluster)
			File->FirstCluster = NewClusterNum;
		else
			FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
		
		CurrentClusterNum = NewClusterNum;
	}

	// ���㵱ǰ�صĵ�һ�������������š��ش� 2 ��ʼ������
	FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
	
	// ����дλ���������ڵ��ֽ�ƫ�ơ�
	OffsetInSector = Offset % Vcb->Bpb.BytesPerSector;

	// Ϊ�˼򵥣���ʱֻ����һ���ذ���һ�������������
	// ����ֻ����д���������һ��������Χ�ڵ������
	Status = IopReadWriteSector( Vcb->DiskDevice,
									FirstSectorOfCluster,
									OffsetInSector,
									(PCHAR)FrontBuffer,
									ByteFommer,
									FALSE );

	if (!EOS_SUCCESS(Status))
		return Status;

	// ����ļ�����������������޸��ļ��ĳ��ȡ�
	if (Offset + BytesToWrite > File->FileSize) {
		File->FileSize = Offset + BytesToWrite;
		
		// ����������ļ�����Ҫͬ���޸��ļ��ڴ����϶�Ӧ�� DIRENT �ṹ
		// �塣Ŀ¼�ļ��� DIRENT �ṹ���е� FileSize ��ԶΪ 0�������޸ġ�
		if (!File->AttrDirectory)
			FatWriteDirEntry(Vcb, File);
	}

	// ѭ��д���м�������
	Offset += ByteFommer;
	
	ULONG k = SpanOfSector;
	while(k > 0){
		// ˳�Ŵ���������д�����ʼλ�����ڴصĴغš�
		PrevClusterNum = 0;
		CurrentClusterNum = File->FirstCluster;
		// ���ݴصĴ�С������д�����ʼλ���ڴ����ĵڼ������У��� 0 ��ʼ������
		ClusterIndex = (Offset) / FatBytesPerCluster(&Vcb->Bpb);
		for (i = ClusterIndex; i > 0; i--) {
			PrevClusterNum = CurrentClusterNum;
			CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);
			//CurrentClusterNum = FatGetFatEntryValue(Vcb,CurrentClusterNum);	
		}
		
	
		// ���д�����ʼλ�û�û�ж�Ӧ�Ĵأ������Ӵ�
		if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {
	
			// Ϊ�ļ�����һ�����д�
			FatAllocateOneCluster(Vcb, &NewClusterNum);
	
			// ���·���Ĵذ�װ��������
			if (0 == File->FirstCluster)
				File->FirstCluster = NewClusterNum;
			else
				FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
			
			CurrentClusterNum = NewClusterNum;
		}
		// ���㵱ǰ�صĵ�һ�������������š��ش� 2 ��ʼ������
		FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
		
		// ����дλ���������ڵ��ֽ�ƫ�ơ�
		OffsetInSector = (Offset) % Vcb->Bpb.BytesPerSector;
		
		// ����ֻ����д���������һ��������Χ�ڵ������
		Status = IopReadWriteSector( Vcb->DiskDevice,
										FirstSectorOfCluster,//��ǰ�صĵ�һ��������������
										OffsetInSector,//дλ���������ڵ��ֽ�ƫ��
										(PCHAR)MidBuffer,//������
										512,//Ҫд������ֽ�
										FALSE );
	
		if (!EOS_SUCCESS(Status))
			return Status;
	
		// ����ļ�����������������޸��ļ��ĳ��ȡ�
		if (Offset + 512 > File->FileSize) {
			File->FileSize = Offset + BytesToWrite;
			
			// ����������ļ�����Ҫͬ���޸��ļ��ڴ����϶�Ӧ�� DIRENT �ṹ
			// �塣Ŀ¼�ļ��� DIRENT �ṹ���е� FileSize ��ԶΪ 0�������޸ġ�
			if (!File->AttrDirectory)
				FatWriteDirEntry(Vcb, File);
		}
		MidBuffer = &MidBuffer[512];
		Offset += 512;
		k--;
	}

	
	// д��β����
	if(NeedSplit == 1 &&ByteLatter > 0){
		// ˳�Ŵ���������д�����ʼλ�����ڴصĴغš�
		PrevClusterNum = 0;
		CurrentClusterNum = File->FirstCluster;
		// ���ݴصĴ�С������д�����ʼλ���ڴ����ĵڼ������У��� 0 ��ʼ������
		ClusterIndex = (Offset) / FatBytesPerCluster(&Vcb->Bpb);
		for (i = ClusterIndex; i > 0; i--) {
			PrevClusterNum = CurrentClusterNum;
			CurrentClusterNum = FatGetFatEntryValue(Vcb, PrevClusterNum);
		//CurrentClusterNum = FatGetFatEntryValue(Vcb,CurrentClusterNum);	
		}
	
		// ���д�����ʼλ�û�û�ж�Ӧ�Ĵأ������Ӵ�
		if (0 == CurrentClusterNum || CurrentClusterNum >= 0xFF8) {
	
			// Ϊ�ļ�����һ�����д�
			FatAllocateOneCluster(Vcb, &NewClusterNum);
	
			// ���·���Ĵذ�װ��������
			if (0 == File->FirstCluster)
				File->FirstCluster = NewClusterNum;
			else
				FatSetFatEntryValue(Vcb, PrevClusterNum, NewClusterNum);
			
			CurrentClusterNum = NewClusterNum;
		}
		// ���㵱ǰ�صĵ�һ�������������š��ش� 2 ��ʼ������
		FirstSectorOfCluster = Vcb->FirstDataSector + (CurrentClusterNum - 2) * Vcb->Bpb.SectorsPerCluster;
		
		// ����дλ���������ڵ��ֽ�ƫ�ơ�
		OffsetInSector = (Offset) % Vcb->Bpb.BytesPerSector;
	
		// Ϊ�˼򵥣���ʱֻ����һ���ذ���һ�������������
		// ����ֻ����д���������һ��������Χ�ڵ������
		Status = IopReadWriteSector( Vcb->DiskDevice,
										FirstSectorOfCluster,//��ǰ�صĵ�һ��������������
										OffsetInSector,//дλ���������ڵ��ֽ�ƫ��
										(PCHAR)LatterBuffer,//������
										NumOfLatterBuffer,//Ҫд������ֽ�
										FALSE );
	
		if (!EOS_SUCCESS(Status))
			return Status;
	
		// ����ļ�����������������޸��ļ��ĳ��ȡ�
		if (Offset +ByteLatter > File->FileSize) {
			File->FileSize = Offset + BytesToWrite;
			
			// ����������ļ�����Ҫͬ���޸��ļ��ڴ����϶�Ӧ�� DIRENT �ṹ
			// �塣Ŀ¼�ļ��� DIRENT �ṹ���е� FileSize ��ԶΪ 0�������޸ġ�
			if (!File->AttrDirectory)
				FatWriteDirEntry(Vcb, File);
		}
	}
	
	// ����ʵ��д����ֽ�����
	*BytesWriten = BytesToWrite;

	return STATUS_SUCCESS;

}
```


### �������

EOS����FAT12�ļ�ϵͳʵ�����ļ���д������ʵ���й۲쵽��д���ļ����̱Ƚ��������ܲ��㣬�д����ơ�

---

## Lab Ex 1

EX1���ߵ��⣬clone���޸ġ��ύ�����ʹ���ʮ�ַ���������**����ν���һ�������**��д���ű�~~��ȭ����~~

������ȡ���񣬽�������Ϊ`ex1-$`����$�滻����ţ�eg.`ex1-7`

�����ֿ�������shell�ű����ص����ʵ���Ŀ¼���޸Ľű��еĲֿ��ַΪ���Լ��ĵ�ַ��Ȼ��ִ��

```sh
chmod +x ./autoDoEx1.sh && ./autoDoEx1.sh
```

����

---

## Lab Ex 3

1. ʵ��ʱ��Ƭ��ת�����㷨��

```c
// ���жϵ��̴߳�������̬�Ҳ�Ϊ��
if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
	// ����ʱ��Ƭ
	PspCurrentThread->RemainderTicks--;
	if(PspCurrentThread->RemainderTicks == 0) {
		// ���ʱ��ƬΪ0����ʱ��Ƭ
		PspCurrentThread->RemainderTicks=TICKS_OF_TIME_SLICE;
		// ���ںͱ��ж��߳����ȼ���ͬ�ľ����߳�
		if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			// �����ж��߳�ת�����״̬���������β
			PspReadyThread(PspCurrentThread);
		}
	}
}

return;
```

2. �޸�ʱ��Ƭ�Ĵ�СTICKS_OF_TIME_SLICEΪ100������۲�ִ�к��Ч����

```c
//
// ����ʱ��Ƭ��ת���ȵ�ʱ��Ƭ��С����ʱ��Ƭ������ʱ�ӵδ�����
//
#define TICKS_OF_TIME_SLICE		100
```

3. �ڿ���̨���rr���Ĵ������У���Sleepʱ�����Ϊ200*1000�����������г����ʱ��鿴���ȼ����ͺ��Ч����

```c
// ��ǰ�̵߳ȴ�һ��ʱ�䡣���ڵ�ǰ�߳����ȼ� 24 �����½��̵߳����ȼ� 8��
// ����ֻ���ڵ�ǰ�߳̽��롰������״̬���½����̲߳���ִ�С�
Sleep(200 * 1000);
```

4. �޸��߳̿��ƿ�(TCB)�ṹ�壬����������������Ա��һ�����߳��������������кϼ�ʹ�õ�ʱ��Ƭ��������һ�����̵߳ĳ�ʼʱ��Ƭ������

```c
ULONG TTLTickNum;					// �߳��������������кϼ�ʹ�õ�ʱ��Ƭ����
ULONG InitTickNum;					// �̵߳ĳ�ʼʱ��Ƭ����
```

```c
// create.c
// line 607:
NewThread->InitTickNum = TICKS_OF_TIME_SLICE;
NewThread->RemainderTicks = TICKS_OF_TIME_SLICE;
NewThread->TTLTickNum = 0;
```

5. �޸ġ�rr�������ڿ���̨��������ݺ͸�ʽ��������ʾ�̼߳�����������ʾ�̳߳�ʼ��ʱ��Ƭ�Ĵ�С����ʹ��ʱ��Ƭ�ĺϼ�������ʣ��ʱ��Ƭ��������ע�⣬�ڵ���fprintf������ʽ���ַ�ʱ����Ҫ���ַ�����ĩβ����һ���ո񣬷���ᵼ������쳣��

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

6. ʵ�ֶ༶�������е����㷨

```c
// ���жϵ��̴߳�������̬�Ҳ�Ϊ��
	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {
		// ����ʱ��Ƭ
		PspCurrentThread->RemainderTicks--;
		PspCurrentThread->TTLTickNum = PspCurrentThread->InitTickNum - PspCurrentThread->RemainderTicks;

		// ʱ��Ƭ�ľ�
		if(PspCurrentThread->RemainderTicks == 0) {
			// ���������������ȼ�
			if(PspCurrentThread->Priority > 0) {
				// �������ȼ�
				PspCurrentThread->Priority;
				// ���ӳ�ʼʱ��Ƭ
				PspCurrentThread->InitTickNum += TICKS_OF_TIME_SLICE;
			}
				
			// ����ʱ��ƬΪ��ʼʱ��Ƭ
			PspCurrentThread->RemainderTicks = PspCurrentThread->InitTickNum;

			// ���ںͱ��ж��߳����ȼ���ͬ�ľ����߳�
			if(BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
				// �����ж��߳�ת�����״̬���������β
				PspReadyThread(PspCurrentThread);
			}
		}
	}

	return;
```

7. ����EOSû���ṩ��꣬����ʹ�ü����¼����߿���̨����ʹ�߳����ȼ�������

```c
if(KeyEventRecord.VirtualKeyValue == VK_SPACE && PspCurrentThread->State == Running && PspCurrentThread->Priority > 0 && PspCurrentThread->Priority < 8 )
		PspCurrentThread->Priority = 8;
```

8. ʹ�ÿ���̨���������߳����ȼ�����EOS����ϵͳ��ʵ��һ����upt������,ͨ��������߳�ID��������Ӧ�̵߳����ȼ���

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
	// ����������ַ����л���߳� ID��
	//
	ThreadID = atoi(Arg);
	
	
	//
	// �ɽ��� ID ��ý��̿��ƿ�
	//
	Status = ObRefObjectById(ThreadID, PspThreadType, (PVOID*)&pThreadCtrlBlock);
	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	// ����Ǵ�������״̬������״̬,ֱ���������ȼ���
	if(pThreadCtrlBlock->State == Running || pThreadCtrlBlock->State == Waiting){
		pThreadCtrlBlock->Priority = 8;
	}
	// ����Ǵ��ھ���״̬���̣߳���Ҫ�Ƚ����߳��Ƴ����У�
	// Ȼ�����ø��̵߳����ȼ�Ϊ8���ָ��̵߳ĳ�ʼʱ��Ƭ��С��ʣ��ʱ��Ƭ��С
	else if(pThreadCtrlBlock->State == Ready){
	 	ListRemoveEntry(&pThreadCtrlBlock->StateListEntry);
	 	
	 	pThreadCtrlBlock->State = Zero;								// ����Ϊ����״̬
	 	pThreadCtrlBlock->Priority = 8;								// �������ȼ�Ϊ8
	 	pThreadCtrlBlock->InitTickNum = 8 * TICKS_OF_TIME_SLICE;		// ����ʱ��Ƭʣ���С
	 	pThreadCtrlBlock->RemainderTicks = pThreadCtrlBlock->InitTickNum;			// ���ó�ʼ��ʱ��Ƭ��С
	 	
	 	PspReadyThread(pThreadCtrlBlock);
	 }
	
	PspThreadSchedule();//�̵߳���
	KeEnableInterrupts(IntState);	// ���ж�
	
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



