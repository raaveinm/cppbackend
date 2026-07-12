# VARIADIC TEMPLATE LOGGER

> Logger signature:
> ```C++
> Logger::GetInstance().Log("User entered numbers "sv, x, " and "sv, y);
> ```

## Description

Logger get variable amount of arguments and each of it will be written to the log file. 

For each argument must exist operator `<<`.

Logs has the following format: `YYYY-MM-DD HH:MM:SS: arg1 arg2`

## Testing

Method `SetTimestamp(std::chrono::system_clock::time_point timestamp)` is accepting time stamp and use its time in `Log(..)`
instead of `system_clock::now()`

## Output

Logger provides its output to `/var/log/sample_log_$YYYY_$MM_$DD.log`. If during runtime date has been changed - Logger will
write following logs to the new file.