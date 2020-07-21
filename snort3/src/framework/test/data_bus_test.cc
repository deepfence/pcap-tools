//--------------------------------------------------------------------------
// Copyright (C) 2019-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// data_bus_test.cc author Steven Baigal <sbaigal@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "framework/data_bus.h"
#include "main/snort_config.h"


#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using namespace snort;

//--------------------------------------------------------------------------
// mocks
//--------------------------------------------------------------------------
InspectionPolicy::InspectionPolicy(unsigned int) {}
InspectionPolicy::~InspectionPolicy() {}
namespace snort
{
SnortConfig::SnortConfig(snort::SnortConfig const*)
{ global_dbus = new DataBus(); }

THREAD_LOCAL const SnortConfig* snort_conf = nullptr;

const SnortConfig* SnortConfig::get_conf()
{ return snort_conf; }

SnortConfig* SnortConfig::get_main_conf()
{ return const_cast<SnortConfig*>(snort_conf); }

SnortConfig::~SnortConfig()
{ delete global_dbus; }

static  InspectionPolicy* my_inspection_policy = nullptr;

InspectionPolicy* get_inspection_policy()
{ return my_inspection_policy; }
}
//--------------------------------------------------------------------------
class UTestEvent : public DataEvent
{
public:
    UTestEvent(int m) : msg(m) { }

    int get_message()
    { return msg; }

private:
    int msg;
};

class UTestHandler : public DataHandler
{
public:
    UTestHandler() : DataHandler("unit_test")
    { }

    void handle(DataEvent&, Flow*) override;

    int evt_msg = 0;
};

void UTestHandler::handle(DataEvent& event, Flow*)
{
    UTestEvent* evt = (UTestEvent*)&event;
    evt_msg = evt->get_message();
}
#define DB_UTEST_EVENT "unit.test.event"

//--------------------------------------------------------------------------
// data bus unit tests
//--------------------------------------------------------------------------

TEST_GROUP(data_bus)
{
   void setup() override
   {
        snort_conf = new SnortConfig();
        my_inspection_policy = new InspectionPolicy();
   }

   void teardown() override
   {
        delete my_inspection_policy;
        delete snort_conf;
   }
};

TEST(data_bus, subscribe_global)
{
    SnortConfig* sc = SnortConfig::get_main_conf();
    UTestHandler* h = new UTestHandler();
    DataBus::subscribe_global(DB_UTEST_EVENT, h, sc);

    UTestEvent event(100);
    DataBus::publish(DB_UTEST_EVENT, event);
    CHECK(100 == h->evt_msg);

    UTestEvent event1(200);
    DataBus::publish(DB_UTEST_EVENT, event1);
    CHECK(200 == h->evt_msg);

    DataBus::unsubscribe_global(DB_UTEST_EVENT, h, sc);

    UTestEvent event2(300);
    DataBus::publish(DB_UTEST_EVENT, event2);
    CHECK(200 == h->evt_msg); // unsubscribed!

    delete h;
}

TEST(data_bus, subscribe)
{
    UTestHandler* h = new UTestHandler();
    DataBus::subscribe(DB_UTEST_EVENT, h);

    UTestEvent event(100);
    DataBus::publish(DB_UTEST_EVENT, event);
    CHECK(100 == h->evt_msg);

    UTestEvent event1(200);
    DataBus::publish(DB_UTEST_EVENT, event1);
    CHECK(200 == h->evt_msg);

    DataBus::unsubscribe(DB_UTEST_EVENT, h);

    UTestEvent event2(300);
    DataBus::publish(DB_UTEST_EVENT, event2);
    CHECK(200 == h->evt_msg); // unsubscribed!

    delete h;
}

//-------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------

int main(int argc, char** argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

