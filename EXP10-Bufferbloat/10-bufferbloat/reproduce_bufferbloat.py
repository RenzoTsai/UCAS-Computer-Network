#!/usr/bin/python

from mininet.topo import Topo
from mininet.node import Host
from mininet.link import TCLink
from mininet.net import Mininet
from mininet.cli import CLI

from time import sleep
from argparse import ArgumentParser
from utils import *

import os

parser = ArgumentParser(description='Args for reproducing Bufferbloat')

parser.add_argument('--maxq', '-q', type=int, help='Max buffer size of network interface in packets', required=True)

args = parser.parse_args()

class BBTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        r1 = self.addHost('r1')
        self.addLink(h1, r1, delay='5ms')
        self.addLink(h2, r1, bw=10, delay='5ms', max_queue_size=args.maxq)

def reproduce_bufferbloat(net, duration=60):
    dname = 'qlen-%d' % (args.maxq)
    if not os.path.exists(dname):
        os.makedirs(dname)

    # monitoring the cwnd size of h1
    cmon = start_cwnd_monitor(net, '%s/cwnd.txt' % (dname))

    # monitoring the queue sizes of r1's interface (connected with h2)
    qmon = start_qlen_monitor(net, '%s/qlen.txt' % (dname))

    # monitoring the rtt between h1 and h2
    rmon = start_rtt_monitor(net, '%s/rtt.txt' % (dname))

    start_iperf(net, duration)
    sleep(duration)

    stop_cwnd_monitor(cmon)
    stop_qlen_monitor(qmon)
    stop_rtt_monitor(rmon)

    stop_iperf()

if __name__ == '__main__':
    # Linux uses CUBIC by default which doesn't have the sawtooth behaviour
    os.system('sysctl -w net.ipv4.tcp_congestion_control=reno')
    topo = BBTopo()
    net = Mininet(topo=topo, link=TCLink, controller=None)
    config_ip(net)

    net.start()

    # CLI(net)
    reproduce_bufferbloat(net, 60)

    net.stop()
