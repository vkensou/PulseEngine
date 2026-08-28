#pragma once

#ifndef PULSE_ANNOTATION_HEADER_GUARD
#define PULSE_ANNOTATION_HEADER_GUARD

#include <stdint.h>

// 仅仅为类型增加注解，不改变编译/运行时行为

// 可空（仅指针）
#define __pulse_nullable

// 所有权转移：调用者获得所有权，负责释放
#define __pulse_owned

// 借用：源对象（传入的父句柄/被调方内部对象）存活期间有效；
// 源被销毁或变更时失效。不要释放。
#define __pulse_borrow

// 异步借用：关联异步操作完成前有效，需外界保活
#define __pulse_borrow_async

/* ============================================================================
 * 容器
 * ============================================================================ */

// C 数组：用于字段声明
#define Pulse_Array(T, field) T* field; uint32_t field##_count

// C 数组：用于函数参数声明
#define Pulse_Array_Param(T, param) T* param, uint32_t param##_count

// 输出参数：函数将结果写入调用方提供的存储，与内层类型组合：
//   Pulse_Out(EPulseRetainErrorCode)  写出一个值
//   Pulse_Out(Pulse_Borrow(void))           写出借用指针（= void**）
//   Pulse_Out(Pulse_Owned(T))               函数分配并转移所有权
#define __pulse_out

// 借用 C 字符串；拥有所有权的字符串写 Pulse_Owned(char)
#define Pulse_CString const char*

#endif // PULSE_ANNOTATION_HEADER_GUARD
