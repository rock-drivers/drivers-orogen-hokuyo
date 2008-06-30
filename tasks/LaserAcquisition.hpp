#ifndef LASERACQUISITION_TASK_HPP
#define LASERACQUISITION_TASK_HPP

#include "tasks/LaserAcquisitionBase.hpp"

class URG;

namespace hokuyo {
    class LaserAcquisition : public LaserAcquisitionBase
    {
	friend class LaserAcquisitionBase;
    protected:
    
        URG* m_driver;
        bool handle_error(URG& driver, std::string const& phase, bool retval);
    
    public:
        LaserAcquisition(std::string const& name = "LaserAcquisition");
        ~LaserAcquisition();

        bool configureHook();
        bool startHook();
        void updateHook();
        void errorHook();
        void stopHook();
    };
}

#endif

