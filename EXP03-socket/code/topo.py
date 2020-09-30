from mininet.net import Mininet
from mininet.cli import CLI
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.node import OVSBridge

class MyTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        s1 = self.addSwitch('s1')

        self.addLink(h1, s1, bw=10, delay='10ms')
        self.addLink(h2, s1)

topo = MyTopo()
net = Mininet(topo = topo, switch = OVSBridge, link = TCLink, controller = None)
net.start()
CLI(net)
net.stop()
