#include <iostream>
#include <Windows.h>
/* 线程函数 */
#include <thread>

using namespace std;

/* 计算程序的CRC值 */
DWORD CRC32(BYTE* ptr, DWORD Size);
// 检查内存中CRC32特征值
DWORD CheckMemory(DWORD va_base, DWORD sec_len);
/* 单独启动一个线程来做CRC校验 */
void CheckMemory_CRC(pair<DWORD, DWORD> params);

int main()
{
	DWORD begin_addr, end_addr, size;
	// 获取到两个位置的偏移地址
	__asm mov begin_addr, offset start;
	__asm mov end_addr, offset exit;

	// 计算出 两者内存差值
	size = end_addr - begin_addr;

	thread check(CheckMemory_CRC, make_pair(begin_addr, size));
	check.detach();
start:
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_SECTION_HEADER pSecHeader = NULL;
	DWORD ImageBase;

	ImageBase = (DWORD)GetModuleHandle(NULL);
	if (NULL == ImageBase) {
		printf("GetModuleHandle() Error: %d", GetLastError());
		return 1;
	}

	printf("ImageBase: %08x\n", ImageBase);

	pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	pNtHeader = (PIMAGE_NT_HEADERS32)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pSecHeader = IMAGE_FIRST_SECTION(pNtHeader);
	// 取出节内偏移与节表长度
	DWORD va_base = ImageBase + pSecHeader->VirtualAddress;   // 定位代码节va基地址
	DWORD sec_len = pSecHeader->Misc.VirtualSize;             // 获取代码节长度

	printf("镜像基址(.text): %x --> 镜像大小: %x \n", va_base, sec_len);
exit:
	system("pause");
	return 0;
}

void CheckMemory_CRC(pair<DWORD, DWORD> params) {
	DWORD va_base = params.first;
	DWORD sec_len = params.second;

	DWORD OriginalCRC32 = 0;
	OriginalCRC32 = CheckMemory(va_base,sec_len);

	while (1)
	{
		Sleep(3000);
		DWORD NewCRC32 = CheckMemory(va_base, sec_len);
		if (OriginalCRC32 == NewCRC32)
			printf("程序没有被打补丁. \n");
		else {
			printf("程序被打补丁 \n");
			exit(1);
		}

	}


}
DWORD CRC32(BYTE* ptr, DWORD Size)
{
	DWORD crcTable[256], crcTmp1;

	// 动态生成CRC-32表
	for (int i = 0; i < 256; i++)
	{
		crcTmp1 = i;
		for (int j = 8; j > 0; j--)
		{
			if (crcTmp1 & 1) crcTmp1 = (crcTmp1 >> 1) ^ 0xEDB88320L;
			else crcTmp1 >>= 1;
		}
		crcTable[i] = crcTmp1;
	}
	// 计算CRC32值
	DWORD crcTmp2 = 0xFFFFFFFF;
	while (Size--)
	{
		crcTmp2 = ((crcTmp2 >> 8) & 0x00FFFFFF) ^ crcTable[(crcTmp2 ^ (*ptr)) & 0xFF];
		ptr++;
	}
	return (crcTmp2 ^ 0xFFFFFFFF);
}

DWORD CheckMemory(DWORD va_base, DWORD sec_len) {
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_SECTION_HEADER pSecHeader = NULL;
	DWORD ImageBase;

	// 获取基地址
	ImageBase = (DWORD)GetModuleHandle(NULL);

	// 定位到PE头结构
	pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	pNtHeader = (PIMAGE_NT_HEADERS32)((DWORD)pDosHeader + pDosHeader->e_lfanew);

	pSecHeader = IMAGE_FIRST_SECTION(pNtHeader);
	/* 使用标签来定义保护位置 */
	//DWORD va_base = ImageBase + pSecHeader->VirtualAddress;   // 定位代码节va基地址
	//DWORD sec_len = pSecHeader->Misc.VirtualSize;             // 获取代码节长度

	DWORD CheckCRC32 = CRC32((BYTE*)(va_base), sec_len);
	// printf(".text节CRC32 = %x \n", CheckCRC32);
	return CheckCRC32;
}