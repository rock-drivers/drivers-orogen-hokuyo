#ifndef LASERACQUISITION_TASK_HPP
#define LASERACQUISITION_TASK_HPP

#include "hokuyo/LaserAcquisitionBase.hpp"


namespace RTT
{
    class FileDescriptorActivity;
}

class URG;

namespace hokuyo {
    class LaserAcquisition : public LaserAcquisitionBase
    {
	friend class LaserAcquisitionBase;
    protected:
    
        URG* m_driver;
        bool handle_error(URG& driver, std::string const& phase, bool retval);

        struct StatUpdater
        {
            uint64_t m_max;
            uint64_t m_count;
            uint64_t m_sum;
            uint64_t m_sum2;

            StatUpdater()
                : m_max(0), m_count(0), m_sum(0), m_sum2(0) {}

            Statistics update(int value)
            {
                m_sum  += value;
                m_sum2 += value * value;
                if (m_max < value)
                    m_max = value;
                m_count++;

                Statistics stats;
                stats.count = m_count;
                stats.min   = 0;
                stats.max   = m_max;
                stats.mean  = m_sum / m_count;
                stats.dev   = sqrt((m_sum2 / m_count) - (stats.mean * stats.mean));
                return stats;
            }
        };

        StatUpdater m_period_stats;
        StatUpdater m_latency_stats;

        DFKI::Time m_last_device;
        DFKI::Time m_last_stamp;
    
    public:
        LaserAcquisition(std::string const& name = "hokuyo::LaserAcquisition");
        ~LaserAcquisition();

        RTT::FileDescriptorActivity* getFileDescriptorActivity();

        bool configureHook();
        bool startHook();
        void updateHook();
        void errorHook();
        void stopHook();
    };
}

#endif

