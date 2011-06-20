#ifndef HOKUYO_TASK_TASK_HPP
#define HOKUYO_TASK_TASK_HPP

#include "hokuyo/TaskBase.hpp"

namespace aggregator
{
    template<class Item>
    class Timestamper;
}

class URG;

namespace hokuyo {
    class Task : public TaskBase
    {
	friend class TaskBase;
    protected:
    
        URG* m_driver;
    
        struct StatUpdater
        {
            uint64_t m_max;
            uint64_t m_count;
            uint64_t m_sum;
            uint64_t m_sum2;

            StatUpdater()
                : m_max(0), m_count(0), m_sum(0), m_sum2(0) {}

            Statistics update(unsigned int value)
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

        base::Time m_last_device;
        base::Time m_last_stamp;

	aggregator::Timestamper<base::samples::LaserScan>* timestamper;

    public:
        Task(std::string const& name = "hokuyo::Task");
        ~Task();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         *
         *   task_context "TaskName" do
         *     needs_configuration
         *     ...
         *   end
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called. See README.txt for different
         * triggering options.
         *
         * The warning(), error() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeWarning, RunTimeError and
         * FatalError states. 
         *
         * In the first case, updateHook() is still called, and recovered()
         * allows you to go back into the Running state.  In the second case,
         * the errorHook() will be called instead of updateHook() and in the
         * third case the component is stopped and resetError() needs to be
         * called before starting it again.
         *
         */
        void updateHook();
        

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recovered() to go back in the Runtime state.
         */
        //void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();
    };
}

#endif

