/*
  Q Light Controller
  mastertimer.cpp

  Copyright (C) Heikki Junnila

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <QDebug>

#ifdef WIN32
#   include "mastertimer-win32.h"
#else
#   include <unistd.h>
#   include "mastertimer-unix.h"
#endif

#include "universearray.h"
#include "genericfader.h"
#include "mastertimer.h"
#include "outputmap.h"
#include "dmxsource.h"
#include "qlcmacros.h"
#include "function.h"
#include "doc.h"

/** The timer tick frequency in Hertz */
const uint MasterTimer::s_frequency = 50;

/*****************************************************************************
 * Initialization
 *****************************************************************************/

MasterTimer::MasterTimer(Doc* doc)
    : QObject(doc)
    , m_stopAllFunctions(false)
    , m_fader(new GenericFader(doc))
    , d_ptr(new MasterTimerPrivate(this))
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(d_ptr != NULL);
}

MasterTimer::~MasterTimer()
{
    if (d_ptr->isRunning() == true)
        stop();

    delete d_ptr;
    d_ptr = NULL;
}

void MasterTimer::start()
{
    Q_ASSERT(d_ptr != NULL);
    d_ptr->start();
}

void MasterTimer::stop()
{
    Q_ASSERT(d_ptr != NULL);
    stopAllFunctions();
    d_ptr->stop();
}

void MasterTimer::timerTick()
{
    Doc* doc = qobject_cast<Doc*> (parent());
    Q_ASSERT(doc != NULL);

    UniverseArray* universes = doc->outputMap()->claimUniverses();
    universes->zeroIntensityChannels();

    timerTickFunctions(universes);
    timerTickDMXSources(universes);
    timerTickFader(universes);

    doc->outputMap()->releaseUniverses();
    doc->outputMap()->dumpUniverses();
}

uint MasterTimer::frequency()
{
    return s_frequency;
}

uint MasterTimer::tick()
{
    return uint(double(1000) / double(s_frequency));
}

/*****************************************************************************
 * Functions
 *****************************************************************************/

void MasterTimer::startFunction(Function* function)
{
    if (function == NULL)
        return;

    m_functionListMutex.lock();
    if (m_startQueue.contains(function) == false)
        m_startQueue.append(function);
    m_functionListMutex.unlock();
}

void MasterTimer::stopAllFunctions()
{
    m_stopAllFunctions = true;

    /* Wait until all functions have been stopped */
    while (runningFunctions() > 0)
    {
#ifdef WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    /* Remove all generic fader's channels */
    m_functionListMutex.lock();
    m_dmxSourceListMutex.lock();
    fader()->removeAll();
    m_dmxSourceListMutex.unlock();
    m_functionListMutex.unlock();

    m_stopAllFunctions = false;
}

int MasterTimer::runningFunctions() const
{
    return m_functionList.size();
}

void MasterTimer::timerTickFunctions(UniverseArray* universes)
{
    // List of m_functionList indices that should be removed at the end of this
    // function. The functions at the indices have been stopped.
    QList <int> removeList;

    /* Lock before accessing the running functions list. */
    m_functionListMutex.lock();
    for (int i = 0; i < m_functionList.size(); i++)
    {
        Function* function = m_functionList.at(i);

        /* No need to access function list on this round anymore */
        m_functionListMutex.unlock();

        if (function != NULL)
        {
            /* Run the function unless it's supposed to be stopped */
            if (function->stopped() == false && m_stopAllFunctions == false)
                function->write(this, universes);

            if (function->stopped() == true || m_stopAllFunctions == true)
            {
                /* Function should be stopped instead */
                m_functionListMutex.lock();
                function->postRun(this, universes);
                //qDebug() << "[MasterTimer] Add function (ID: " << function->id() << ") to remove list ";
                removeList << i; // Don't remove the item from the list just yet.
                m_functionListMutex.unlock();
                emit functionListChanged();
            }
        }

        /* Lock function list for the next round. */
        m_functionListMutex.lock();
    }

    // Remove functions that need to be removed AFTER all functions have been run
    // for this round. This is done separately to prevent a case when a function
    // is first removed and then another is added (chaser, for example), keeping the
    // list's size the same, thus preventing the last added function from being run
    // on this round. The indices in removeList are automatically sorted because the
    // list is iterated with an int above from 0 to size, so iterating the removeList
    // backwards here will always remove the correct indices.
    QListIterator <int> it(removeList);
    it.toBack();
    while (it.hasPrevious() == true)
        m_functionList.removeAt(it.previous());

    /* No more functions. Get out and wait for next timer event. */
    m_functionListMutex.unlock();

    foreach (Function* f, m_startQueue)
    {
        //qDebug() << "[MasterTimer] Processing ID: " << f->id();
        if (m_functionList.contains(f) == false)
        {
            m_functionListMutex.lock();
            m_functionList.append(f);
            m_functionListMutex.unlock();
            //qDebug() << "[MasterTimer] Starting up ID: " << f->id();
            f->preRun(this);
            f->write(this, universes);
            emit functionListChanged();
        }
        m_startQueue.removeOne(f);
    }
}

/****************************************************************************
 * DMX Sources
 ****************************************************************************/

void MasterTimer::registerDMXSource(DMXSource* source)
{
    Q_ASSERT(source != NULL);

    m_dmxSourceListMutex.lock();
    if (m_dmxSourceList.contains(source) == false)
        m_dmxSourceList.append(source);
    m_dmxSourceListMutex.unlock();
}

void MasterTimer::unregisterDMXSource(DMXSource* source)
{
    Q_ASSERT(source != NULL);

    m_dmxSourceListMutex.lock();
    m_dmxSourceList.removeAll(source);
    m_dmxSourceListMutex.unlock();
}

void MasterTimer::timerTickDMXSources(UniverseArray* universes)
{
    /* Lock before accessing the running functions list. */
    m_dmxSourceListMutex.lock();
    for (int i = 0; i < m_dmxSourceList.size(); i++)
    {
        DMXSource* source = m_dmxSourceList.at(i);
        Q_ASSERT(source != NULL);

        /* No need to access the list on this round anymore. */
        m_dmxSourceListMutex.unlock();

        /* Get DMX data from the source */
        source->writeDMX(this, universes);

        /* Lock for the next round. */
        m_dmxSourceListMutex.lock();
    }

    /* No more sources. Get out and wait for next timer event. */
    m_dmxSourceListMutex.unlock();
}

/****************************************************************************
 * Generic Fader
 ****************************************************************************/

GenericFader* MasterTimer::fader() const
{
    return m_fader;
}

void MasterTimer::timerTickFader(UniverseArray* universes)
{
    m_functionListMutex.lock();
    m_dmxSourceListMutex.lock();

    fader()->write(universes);

    m_dmxSourceListMutex.unlock();
    m_functionListMutex.unlock();
}
