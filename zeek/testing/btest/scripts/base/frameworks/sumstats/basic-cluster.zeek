# @TEST-PORT: BROKER_PORT1
# @TEST-PORT: BROKER_PORT2
# @TEST-PORT: BROKER_PORT3
#
# @TEST-EXEC: btest-bg-run manager-1 ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=manager-1 zeek %INPUT
# @TEST-EXEC: btest-bg-run worker-1  ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=worker-1 zeek %INPUT
# @TEST-EXEC: btest-bg-run worker-2  ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=worker-2 zeek %INPUT
# @TEST-EXEC: btest-bg-wait 15

# @TEST-EXEC: btest-diff manager-1/.stdout

@TEST-START-FILE cluster-layout.zeek
redef Cluster::nodes = {
	["manager-1"] = [$node_type=Cluster::MANAGER, $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT1"))],
	["worker-1"]  = [$node_type=Cluster::WORKER,  $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT2")), $manager="manager-1", $interface="eth0"],
	["worker-2"]  = [$node_type=Cluster::WORKER,  $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT3")), $manager="manager-1", $interface="eth1"],
};
@TEST-END-FILE

redef Log::default_rotation_interval = 0secs;

global n = 0;

event zeek_init() &priority=5
	{
	local r1: SumStats::Reducer = [$stream="test", $apply=set(SumStats::SUM, SumStats::MIN, SumStats::MAX, SumStats::AVERAGE, SumStats::STD_DEV, SumStats::VARIANCE, SumStats::UNIQUE, SumStats::HLL_UNIQUE)];
	SumStats::create([$name="test",
	                  $epoch=5secs,
	                  $reducers=set(r1),
	                  $epoch_result(ts: time, key: SumStats::Key, result: SumStats::Result) =
	                  	{
	                  	local r = result["test"];
	                  	print fmt("Host: %s - num:%d - sum:%.1f - avg:%.1f - max:%.1f - min:%.1f - var:%.1f - std_dev:%.1f - unique:%d - hllunique:%d", key$host, r$num, r$sum, r$average, r$max, r$min, r$variance, r$std_dev, r$unique, r$hll_unique);
	                  	},
	                  $epoch_finished(ts: time) =
	                  	{
	                  	terminate();
	                  	}]);
	}

event Broker::peer_lost(endpoint: Broker::EndpointInfo, msg: string)
	{
	terminate();
	}

global ready_for_data: event();

event ready_for_data()
	{
	if ( Cluster::node == "worker-1" )
		{
		SumStats::observe("test", [$host=1.2.3.4], [$num=34]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=30]);
		SumStats::observe("test", [$host=6.5.4.3], [$num=1]);
		SumStats::observe("test", [$host=7.2.1.5], [$num=54]);
		}
	if ( Cluster::node == "worker-2" )
		{
		SumStats::observe("test", [$host=1.2.3.4], [$num=75]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=30]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=3]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=57]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=52]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=61]);
		SumStats::observe("test", [$host=1.2.3.4], [$num=95]);
		SumStats::observe("test", [$host=6.5.4.3], [$num=5]);
		SumStats::observe("test", [$host=7.2.1.5], [$num=91]);
		SumStats::observe("test", [$host=10.10.10.10], [$num=5]);
		}
	}

@if ( Cluster::local_node_type() == Cluster::MANAGER )

event zeek_init() &priority=100
	{
	Broker::auto_publish(Cluster::worker_topic, ready_for_data);
	}

global peer_count = 0;

event Broker::peer_added(endpoint: Broker::EndpointInfo, msg: string)
	{
	++peer_count;

	if ( peer_count == 2 )
		event ready_for_data();
	}

@endif
