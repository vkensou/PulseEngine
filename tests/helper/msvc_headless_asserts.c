// 让 MSVC Debug 下的 assert / abort 失败时不弹窗，改为输出到 stderr。
#if defined(_WIN32) && defined(_DEBUG)

#include <crtdbg.h>
#include <stdlib.h>

static int setup_headless_asserts(void)
{
    _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    return 0;
}

#pragma section(".CRT$XCU", read)
typedef int (*_PIFV)(void);

__declspec(allocate(".CRT$XCU"))
static _PIFV headless_asserts_initializer = setup_headless_asserts;

#endif