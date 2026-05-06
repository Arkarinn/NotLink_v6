#ifndef __ULOG_CONFIG_H__
#define __ULOG_CONFIG_H__

/* 打印颜色 */
#define ULOG_COLOR_ENABLE 0U

/* 打印序号 */
#define ULOG_INDEX_ENABLE 0U

/* 编译时的最低允许等级 */
#ifndef ULOG_COMPILE_LEVEL
#define ULOG_COMPILE_LEVEL ULOG_LEVEL_DEBUG
#endif

#endif // !__ULOG_CONFIG_H__
