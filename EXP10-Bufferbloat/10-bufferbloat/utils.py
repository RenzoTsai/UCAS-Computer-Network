import os
import sys
import re

from time import sleep, time
from subprocess import Popen, PIPE
from multiprocessing import Process

def config_ip(net):
    h1, h2, r1 = net.get('h1', 'h2', 'r1')
    h1.cmd('ifconfig h1-eth0 10.0.1.11/24')
    h1.cmd('route add default gw 10.0.1.1')

    h2.cmd('ifconfig h2-eth0 10.0.2.22/24')
    h2.cmd('route add default gw 10.0.2.1')

    r1.cmd('ifconfig r1-eth0 10.0.1.1/24')
    r1.cmd('ifconfig r1-eth1 10.0.2.1/24')
    r1.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')

    for n in [ h1, h2, r1 ]:
        for intf in n.intfList():
            intf.updateAddr()

def cwnd_monitor(net, fname):
    h1, h2 = net.get('h1', 'h2')
    cmd = 'ss -i | grep %s:5001 -A 1 | grep cwnd' % (h2.IP())
    with open(fname, 'w') as ofile:
        while 1:
            t = time()
            p = h1.popen(cmd, shell=True, stdout=PIPE)
            output = p.stdout.read()
            if output != '':
                ofile.write('%f, %s' % (t, output))
            sleep(0.01)

def start_cwnd_monitor(net, fname):
    print 'Start cwnd monitor ...'
    monitor = Process(target=cwnd_monitor, args=(net, fname))
    monitor.start()
    return monitor

def stop_cwnd_monitor(monitor):
    print 'Stop cwnd monitor ...'
    monitor.terminate()

def qlen_monitor(net, fname):
    r1 = net.get('r1')
    pat = re.compile(r'backlog\s[^\s]+\s([\d]+)p')
    cmd = 'tc -s qdisc show dev r1-eth1'
    with open(fname, 'w') as ofile:
        while 1:
            t = time()
            p = r1.popen(cmd, shell=True, stdout=PIPE)
            output = p.stdout.read()
            matches = pat.findall(output)
            if matches and len(matches) > 1:
                ofile.write('%f, %s\n' % (t, matches[1]))
            sleep(0.01)

def start_qlen_monitor(net, fname):
    print 'Start queue monitor ...'
    monitor = Process(target=qlen_monitor, args=(net, fname))
    monitor.start()
    return monitor

def stop_qlen_monitor(monitor):
    print 'Stop qlen monitor ...'
    monitor.terminate()

def rtt_monitor(net, fname):
    h1, h2 = net.get('h1', 'h2')
    cmd = 'ping -c 1 %s | grep ttl' % (h2.IP())
    with open(fname, 'w') as ofile:
        while 1:
            t = time()
            p = h1.popen(cmd, shell=True, stdout=PIPE)
            output = p.stdout.read()
            if output != '':
                ofile.write('%f, %s' % (t, output))
            sleep(0.1)

def start_rtt_monitor(net, fname):
    print 'Start rtt monitor ...'
    monitor = Process(target=rtt_monitor, args=(net, fname))
    monitor.start()
    return monitor

def stop_rtt_monitor(monitor):
    print 'Stop rtt monitor ...'
    monitor.terminate()

def start_iperf(net, duration):
    h1, h2 = net.get('h1', 'h2')
    print 'Start iperf ...'
    server = h2.popen('iperf -s -w 16m')
    # client = h1.popen('iperf -c %s -t %d' % (h2.IP(), duration+ 5))
    client = h1.cmd('iperf -c %s -t %d -i 0.1 | tee iperf_result.txt &' % (h2.IP(), duration+ 5))

def stop_iperf():
    print 'Kill iperf ...'
    Popen('pgrep -f iperf | xargs kill -9', shell=True).wait()

def set_qdisc_algo(net, algo):
    algo_func_dict = {
            'taildrop': [],
            'red': ['tc qdisc add dev r1-eth1 parent 5:1 handle 6: red limit 1000000 avpkt 1000'],
            'codel': ['tc qdisc add dev r1-eth1 parent 5:1 handle 6: codel limit 1000']
            }
    if algo not in algo_func_dict.keys():
        print '%s is not supported.' % (algo)
        sys.exit(1)

    r1 = net.get('r1')
    for func in algo_func_dict[algo]:
        r1.cmd(func)

def dynamic_bw(net, tot_time):
    h2, r1 = net.get('h2', 'r1')

    start_time = time()
    bandwidth = [100,10,1,50,1,100]
    count = 1
    while True:                                              
        sleep(tot_time/6)
        now = time()
        delta = now - start_time
        if delta > tot_time or count >= 6:
            break
        print '%.1fs left...' % (tot_time - delta)
        h2.cmd('tc class change dev h2-eth0 parent 5:0 classid 5:1 htb rate %fMbit burst 15k' % bandwidth[count] )
        r1.cmd('tc class change dev r1-eth1 parent 5:0 classid 5:1 htb rate %fMbit burst 15k' % bandwidth[count] )
        count += 1
    return
