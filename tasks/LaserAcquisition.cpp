#include "LaserAcquisition.hpp"
#include <hokuyo.hh>

#include <memory>
#include <iostream>

using namespace hokuyo;
using namespace std;
using Orocos::Logger;

LaserAcquisition::LaserAcquisition(std::string const& name)
    : LaserAcquisitionBase(name)
    , m_driver(0)
{
}

LaserAcquisition::~LaserAcquisition()
{
    delete m_driver;
}

bool LaserAcquisition::configureHook()
{
    auto_ptr<URG> driver(new URG());
    if (_rate.value() && !handle_error(*driver, "setBaudRate", driver->setBaudrate(_rate.value())))
        return false;
    if (!handle_error(*driver, "open", driver->open(_port.value())))
        return false;

    Logger::log(Logger::Info) << driver->getInfo() << Logger::endl;
    m_driver = driver.release();
    return true;
}

bool LaserAcquisition::startHook()
{
    return handle_error(*m_driver, "start",
                m_driver->startAcquisition(0, _start_step.value(), 
                    _end_step.value(), _scan_skip.value(), _merge_count.value())
            );
}

void LaserAcquisition::updateHook()
{
    DFKI::LaserReadings reading;
    if (handle_error(*m_driver, "reading", m_driver->readRanges(reading)))
        _scans.Push(reading);
    else
        error();
}
void LaserAcquisition::errorHook()
{
    DFKI::LaserReadings reading;
    if (handle_error(*m_driver, "error", m_driver->fullReset()))
        if (handle_error(*m_driver, "reading", m_driver->readRanges(reading)))
            return recovered();

    fatal();
}

void LaserAcquisition::stopHook()
{ m_driver->stopAcquisition(); }

bool LaserAcquisition::handle_error(URG& driver, string const& phase, bool retval)
{
    if (!retval)
    {
        cerr << "Error during " << phase << ": " << driver.errorString() << endl;
        return false;
    }
    return true;
}

