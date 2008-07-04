#ifndef LASERACQUISITION_TASK_HPP
#define LASERACQUISITION_TASK_HPP

#include "hokuyo/LaserAcquisitionBase.hpp"
#include <rtt/FileDescriptorActivity.hpp>

class URG;

namespace hokuyo {
    class LaserAcquisition : public LaserAcquisitionBase, public RTT::FileDescriptorActivity::Provider
    {
	friend class LaserAcquisitionBase;
    protected:
    
        URG* m_driver;
        bool handle_error(URG& driver, std::string const& phase, bool retval);

        uint64_t m_max_period;
        uint64_t m_sample_count;
        uint64_t m_period_sum;
        uint64_t m_period_sum2;
        DFKI::Timestamp m_last_stamp;
    
    public:
        LaserAcquisition(std::string const& name = "LaserAcquisition");
        ~LaserAcquisition();

        bool configureHook();
        bool startHook();
        void updateHook();
        void errorHook();
        void stopHook();

        int getFileDescriptor() const;
    };
}

#endif

