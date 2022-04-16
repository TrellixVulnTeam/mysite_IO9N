#include "time_util.h"

#include <mutex>

#include <windows.h>

namespace
{
    //std::lock_guard<std.recursive_mutex>
    std::recursive_mutex g_mutex;
    DWORD g_last_seen_now = 0;
    __int64 g_rollover_ms = 0;

    mctm::TimeDelta RolloverProtectedNow()
    {
        std::lock_guard<std::recursive_mutex> locked(g_mutex);
        // We should hold the lock while calling tick_function to make sure that
        // we keep last_seen_now stay correctly in sync.
        DWORD now = ::timeGetTime();
        // ��ǰ�ĺ�����С���ϴ��õ��ĺ�������˵��ϵͳ������ʱ���Ѿ�����timeGetTime����ֵ����Ч��Χ��
        // ��timeGetTimeһ���ֻش�Լ49.71�죬��ô������ʱ��������һ���ֻ�
        if (now < g_last_seen_now)
        {
            g_rollover_ms += 0x100000000I64;  // ~49.7 days.
        }
        g_last_seen_now = now;
        return mctm::TimeDelta::FromMilliseconds(now + g_rollover_ms);
    }
}

namespace mctm
{
    // TimeDelta
    TimeDelta::TimeDelta()
    {
    }

    TimeDelta::TimeDelta(__int64 ms)
        : delta_in_us_(ms)
    {
    }

    TimeDelta::~TimeDelta()
    {
    }

    TimeDelta TimeDelta::FromMilliseconds(__int64 ms)
    {
        return TimeDelta(ms * Time::kMicrosecondsPerMillisecond);
    }

    double TimeDelta::InMillisecondsF() const
    {
        return static_cast<double>(delta_in_us_) / Time::kMicrosecondsPerMillisecond;
    }

    __int64 TimeDelta::InMilliseconds() const
    {
        return delta_in_us_ / Time::kMicrosecondsPerMillisecond;
    }


    // TimeTicks
    TimeTicks::TimeTicks()
    {
    }

    TimeTicks::TimeTicks(__int64 us)
        : ticks_in_us_(us)
    {
    }

    TimeTicks::~TimeTicks()
    {
    }

    TimeTicks TimeTicks::Now()
    {
        return TimeTicks() + RolloverProtectedNow();
    }

    bool TimeTicks::is_null() const
    {
        return ticks_in_us_ == 0;
    }

}