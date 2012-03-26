#include <stdio.h>
#include <stdarg.h>

void _vlog_write(char *prefix, const char *format, va_list arg)
{
	printf("%s", prefix);
	vprintf(format, arg);
}

void _log_error(const char* format, ...)
{
	va_list arg;
	va_start(arg, format);
	_vlog_write("error: ", format, arg);
	va_end(arg);
	exit(1);
}

void _log_warning(const char* format, ...)
{
	va_list arg;
	va_start(arg, format);
	_vlog_write("warning: ", format, arg);
	va_end(arg);
}

void _log_debug(const char* format, ...)
{
	va_list arg;
	va_start(arg, format);
	_vlog_write("debug: ", format, arg);
	va_end(arg);
}

void _log_info(const char* format, ...)
{
	va_list arg;
	va_start(arg, format);
	_vlog_write("info: ", format, arg);
	va_end(arg);
}
