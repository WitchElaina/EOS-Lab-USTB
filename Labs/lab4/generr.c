/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: generr.c

描述: 内部状态码到错误码的转换。



*******************************************************************************/

#include "rtl.h"

//
// 此一维数组描述了内部状态码和错误码的对应关系。内部状态码
// 在奇数下标处，错误码在偶数下标处。多个内部状态码可以对应
// 同一个错误码。
//
CONST ULONG CodePairs[] = {

	STATUS_SUCCESS, NO_ERROR,
	STATUS_TIMEOUT, ERROR_TIMEOUT,
	STATUS_NOTHING_TO_TERMINATE, ERROR_PROC_NOT_FOUND,
	STATUS_PENDING, ERROR_IO_PENDING,

	STATUS_OBJECT_NAME_EXISTS, ERROR_ALREADY_EXISTS,
	STATUS_FILE_ALLREADY_EXISTS, ERROR_ALREADY_EXISTS,

	STATUS_NOT_SUPPORTED, ERROR_NOT_SUPPORTED,
	STATUS_ACCESS_VIOLATION, ERROR_NOACCESS,
	STATUS_ACCESS_DENIED, ERROR_ACCESS_DENIED,
	STATUS_INVALID_PARAMETER, ERROR_INVALID_PARAMETER,

	STATUS_OBJECT_NAME_NOT_FOUND, ERROR_FILE_NOT_FOUND,
	STATUS_OBJECT_ID_NOT_FOUND, ERROR_FILE_NOT_FOUND,
	STATUS_OBJECT_NAME_COLLISION, ERROR_ALREADY_EXISTS,

	STATUS_MAX_HANDLE_EXCEEDED, ERROR_TOO_MANY_POSTS,
	STATUS_OBJECT_TYPE_MISMATCH, ERROR_INVALID_HANDLE,
	STATUS_INVALID_HANDLE, ERROR_INVALID_HANDLE,

	STATUS_NO_MEMORY, ERROR_NOT_ENOUGH_MEMORY,

	STATUS_FREE_VM_NOT_AT_BASE, ERROR_INVALID_ADDRESS,
	STATUS_MEMORY_NOT_ALLOCATED, ERROR_INVALID_ADDRESS,
	STATUS_INVALID_ADDRESS, ERROR_INVALID_ADDRESS,
	STATUS_INVALID_DESTINATION_ADDRESS, ERROR_INVALID_ADDRESS,
	STATUS_INVALID_SOURCE_ADDRESS, ERROR_INVALID_ADDRESS,

	STATUS_MUTEX_NOT_OWNED, ERROR_NOT_OWNER,
	STATUS_SEMAPHORE_LIMIT_EXCEEDED, ERROR_TOO_MANY_POSTS,
	STATUS_SUSPEND_COUNT_EXCEEDED, ERROR_SIGNAL_REFUSED,

	STATUS_FILE_CORRUPT_ERROR, ERROR_FILE_CORRUPT,
	STATUS_PATH_NOT_FOUND, ERROR_PATH_NOT_FOUND,
	STATUS_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND,
	STATUS_PATH_SYNTAX_BAD, ERROR_BAD_PATHNAME,
	STATUS_PATH_TOO_LONG, ERROR_FILENAME_EXCED_RANGE,
	STATUS_FILE_NAME_COLLISION, ERROR_ALREADY_EXISTS,

	STATUS_FLOPPY_UNKNOWN_ERROR, ERROR_FLOPPY_UNKNOWN_ERROR,
	STATUS_WRONG_VOLUME, ERROR_WRONG_DISK,

	STATUS_PROCESS_IS_TERMINATING, ERROR_ACCESS_DENIED,
	STATUS_SHARING_VIOLATION, ERROR_SHARING_VIOLATION,
	STATUS_INVALID_APP_IMAGE, ERROR_BAD_EXE_FORMAT,
	STATUS_SYMBOL_NOT_FOUND, ERROR_NOT_FOUND,
	STATUS_INVALID_COMMAND_LINE, ERROR_INVALID_COMMAND_LINE,

	0xffffffff, 0
};

ULONG
TranslateStatusToError(
	CONST STATUS status
	)
/*++

功能描述：
	将内部状态码转换为错误码。

参数：
	status -- 输入内部状态码。

返回值：
	返回错误码。

--*/
{
	LONG Index;
	
	ASSERT(0 == sizeof(CodePairs) % 2);

	for(Index = 0; CodePairs[Index] != 0xffffffff; Index += 2) {
		if(status == CodePairs[Index])
			return CodePairs[Index + 1];
	}

	ASSERT(FALSE);
	return ERROR_UNKNOWN;
}
