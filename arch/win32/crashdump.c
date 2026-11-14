// Contains handlers for automatically generating crash dumps on Windows.

#include <windows.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <string.h>
#include "dxxerror.h"
#include "console.h"

// Writes what crashed, and the game code found on the stack, into the gamelog, then flushes it so the
// buffered last lines survive. exe+ numbers are offsets into the exe with the printed build stamp.
static void win32_log_crash(EXCEPTION_POINTERS *ep)
{
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	ULONG_PTR base = (ULONG_PTR)GetModuleHandle(NULL);
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
	IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
	ULONG_PTR code_lo = 0, code_hi = 0, at = (ULONG_PTR)er->ExceptionAddress;
	int i;

	for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
		if (!strncmp((const char *)sec->Name, ".text", IMAGE_SIZEOF_SHORT_NAME))
		{
			code_lo = base + sec->VirtualAddress;
			code_hi = code_lo + sec->Misc.VirtualSize;
		}
	if (at >= code_lo && at < code_hi)
		con_printf(CON_URGENT, "CRASH: exception %08lx at exe+%llx (build %08lx)\n", (unsigned long)er->ExceptionCode, (unsigned long long)(at - base), (unsigned long)nt->FileHeader.TimeDateStamp);
	else
		con_printf(CON_URGENT, "CRASH: exception %08lx at %llx, outside the game code (build %08lx)\n", (unsigned long)er->ExceptionCode, (unsigned long long)at, (unsigned long)nt->FileHeader.TimeDateStamp);
	if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
		con_printf(CON_URGENT, "CRASH: bad address %s %llx\n", er->ExceptionInformation[0] == 0 ? "read" : (er->ExceptionInformation[0] == 8 ? "execute" : "write"), (unsigned long long)er->ExceptionInformation[1]);
#ifdef _WIN64
	{
		// return addresses left on the stack that point into game code, newest first
		ULONG_PTR *sp = (ULONG_PTR *)ep->ContextRecord->Rsp, *top = (ULONG_PTR *)((NT_TIB *)NtCurrentTeb())->StackBase;
		char line[512];
		int n = 0, len = snprintf(line, sizeof(line), "CRASH: stack");

		for (; sp < top && n < 24; sp++)
			if (*sp >= code_lo && *sp < code_hi)
			{
				len += snprintf(line + len, sizeof(line) - len, " exe+%llx", (unsigned long long)(*sp - base));
				n++;
			}
		con_printf(CON_URGENT, "%s\n", line);
	}
#endif
	con_flush_log();
}

void win32_create_dump(EXCEPTION_POINTERS* exceptionPointers)
{
	// Append the current date and time to the dump file name.
	char dumpFileName[MAX_PATH];
	SYSTEMTIME stLocalTime;
	GetLocalTime(&stLocalTime);
	wsprintf(dumpFileName, "d1x-redux_%04d%02d%02d_%02d%02d%02d.dmp",
			 stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
			 stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

	HANDLE hDumpFile = CreateFile(dumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
		exceptionInfo.ThreadId = GetCurrentThreadId();
		exceptionInfo.ExceptionPointers = exceptionPointers;
		exceptionInfo.ClientPointers = FALSE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, &exceptionInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}
}

LONG WINAPI win32_exception_handler(EXCEPTION_POINTERS* exceptionPointers)
{
	win32_log_crash(exceptionPointers);
	win32_create_dump(exceptionPointers);
	return EXCEPTION_CONTINUE_SEARCH;
}
