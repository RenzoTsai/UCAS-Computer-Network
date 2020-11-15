#!/usr/bin/python

from mininet.topo import Topo
from mininet.node import Host 
from mininet.link import TCLink
from mininet.net import Mininet
from mininet.cli import CLI

from argparse import ArgumentParser
from utils import *

import os

# available algo: taildrop, red, codel
parser = ArgumentParser(description='Args for mitigating Bufferbloat')
parser.add_argument('--algo', '-a', help='Queue discipline algorithm ', required=True)
args = parser.parse_args()

class BBTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        r1 = self.addHost('r1')
        self.addLink(h1, r1, bw=100, max_queue_size=1000)
        self.addLink(h2, r1, bw=100)

def mitigate_bufferbloat(net, duration=60):
    set_qdisc_algo(net, args.algo)

    dname = args.algo
    if not os.path.exists(dname):
        os.makedirs(dname)

    start_iperf(net, duration)
    rmon = start_rtt_monitor(net, '%s/rtt.txt' % (dname))

    dynamic_bw(net, duration)

    stop_rtt_monitor(rmon)
    stop_iperf()

if __name__ == '__main__':
    os.system('sysctl -w net.ipv4.tcp_congestion_control=reno')
    topo = BBTopo()
    net = Mininet(topo=topo, link=TCLink, controller=None)
    config_ip(net)

    net.start()

    # CLI(net)
    mitigate_bufferbloat(net, 60)

    net.stop()
