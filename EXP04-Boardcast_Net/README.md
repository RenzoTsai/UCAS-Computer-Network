# 广播网络实验

2020年10月12日

[蔡润泽](https://github.com/RenzoTsai)

[本实验 `Github` 地址](https://github.com/RenzoTsai/UCAS-Computer-Network/tree/master/EXP04-Boardcast_Net)

## 实验内容

### 1、实现节点广播的broadcast_packet函数

### 2、验证广播网络能够正常运行

- 从一个端节点ping另一个端节点
    
### 3、验证广播网络的效率

- 在three_nodes_bw.py进行iperf测量

- 两种场景：

  H1: iperf client; H2, H3: servers （h1同时向h2和h3测量）

  H1: iperf server; H2, H3: clients （h2和h3同时向h1测量）

### 3、自己动手构建环形拓扑，验证该拓扑下节点广播会产生数据包环路

## 设计思路及结果验证

### 实现节点广播

#### 设计思路

本实验需要在已有代码框架的基础上实现 `main.c` 中的 `broadcast_packet` 函数。

注意到，如要实现广播网路，则非接受节点若收到数据包，需要将数据包从其他端口转发出去。因此，该函数的伪代码如下：

    foreach iface in iface_list:
        if iface != rx_iface:
            iface_send_packet(iface, packet, len);

（注：`iface_list` 保存所有端口的信息）

在该实验中，该函数的具体实现如下:

```c
void broadcast_packet(iface_info_t *iface, const char *packet, int len)
{
	// TODO: broadcast packet 
	fprintf(stdout, "TODO: broadcast packet here.\n");
	iface_info_t * iface_entry = NULL;
	list_for_each_entry(iface_entry, &instance->iface_list, list) {
		if (iface_entry -> fd != iface -> fd) {
			iface_send_packet(iface_entry, packet, len);
		}
	}
}
```

其中，`instance_iface` 保存所有端口的信息，iface_list中的表项利用fd来进行区分。

#### 结果验证

##### 利用 `three_nodes_bw.py` 拓扑文件打开 `Mininet`，互相 `ping` 三个节点使之能通，测试结果如下：

###### `h1`节点 ping `h2` 节点和 `h3` 节点：

![h1_exp04_1_ping](/EXP04-Boardcast_Net/assets/h1_exp04_1_ping.jpg)

上图中 `10.0.0.2` 和 `10.0.0.3` 分别表示 `h2` 和 `h3`节点。如图，`h1` 节点可以`ping`通`h2` 节点和 `h3` 节点。

###### `h2`节点 ping `h1` 节点和 `h3` 节点：

![h2_exp04_1_ping](/EXP04-Boardcast_Net/assets/h2_exp04_1_ping.jpg)

上图中 `10.0.0.1` 和 `10.0.0.3` 分别表示 `h1` 和 `h3`节点。如图，`h2` 节点可以`ping`通`h1` 节点和 `h3` 节点。

###### `h3`节点 ping `h1` 节点和 `h2` 节点：

![h3_exp04_1_ping](/EXP04-Boardcast_Net/assets/h3_exp04_1_ping.jpg)

上图中 `10.0.0.1` 和 `10.0.0.2` 分别表示 `h1` 和 `h2`节点。如图，`h3` 节点可以`ping`通`h1` 节点和 `h2` 节点。

##### 进行iperf测试，验证广播网络的链路利用效率，测试结果如下：

###### `h1`: `iperf client`; `h2`,`h3`: `iperf servers`：

![h1_exp04_1_client_iperf](/EXP04-Boardcast_Net/assets/h1_exp04_1_client_iperf.jpg)

上图中 `h1` 节点同时向`h2` 节点和 `h3`节点测量，可以看出`h1` 节点时向`h2` 节点和 `h3` 节点的发送带宽分别为`2.92Mbps`和`3.89Mbps`。`h2` 节点和 `h3` 节点的接收带宽分别为`2.89Mbps`和`3.88Mbps`。

而在拓扑文件中，`h1 -> b1`的带宽为`20Mbps`，`b1 -> h2` 的带宽为`10Mbps`,`b1 -> h3` 的带宽为`10Mbps`。因此带宽的利用率为`34.05%`。

###### `h1`: `iperf server`; `h2`,`h3`: `iperf client`:

![h1_exp04_1_server_iperf](/EXP04-Boardcast_Net/assets/h1_exp04_1_server_iperf.jpg)

上图中 `h2` 节点和 `h3`节点同时向 `h1` 节点测量，可以看出`h1` 节点时接收`h2` 节点和 `h3` 节点的接收带宽分别为`4.14Mbps`和`3.76Mbps`。`h2` 节点和 `h3` 节点的发送带宽分别为`4.31Mbps`和`3.82Mbps`。

同样的，在拓扑文件中，`h1 -> b1`的带宽为`20Mbps`，`b1 -> h2` 的带宽为`10Mbps`,`b1 -> h3` 的带宽为`10Mbps`。因此带宽的利用率为`39.5%`。

### 构建环形拓扑网络

#### 设计思路

本实验的代码在 `three_nodes_bw.py` 代码的基础上进行改写，实现在`circle_topo.py` 中，构建了一个各节点间带宽为20Mbps的环形拓扑。关键代码实现如下：

```python
class BroadcastTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        b1 = self.addHost('b1')
        b2 = self.addHost('b2')
        b3 = self.addHost('b3')

        self.addLink(h1, b1, bw=20)
        self.addLink(h2, b2, bw=20)
        self.addLink(b1, b2, bw=20)
        self.addLink(b1, b3, bw=20)
        self.addLink(b2, b3, bw=20)

if __name__ == '__main__':
    check_scripts()

    topo = BroadcastTopo()
    net = Mininet(topo = topo, link = TCLink, controller = None) 

    h1, h2, b1, b2, b3 = net.get('h1', 'h2', 'b1', 'b2', 'b3')
    h1.cmd('ifconfig h1-eth0 10.0.0.1/8')
    h2.cmd('ifconfig h2-eth0 10.0.0.2/8')
    clearIP(b1)
    clearIP(b2)
    clearIP(b3)

    for h in [ h1, h2, b1, b2, b3 ]:
        h.cmd('./scripts/disable_offloading.sh')
        h.cmd('./scripts/disable_ipv6.sh')

    net.start()
    CLI(net)
    net.stop()  
```

#### 结果验证

通过 `h1# ping -c 1 10.0.0.2` 指令，抓包看到一个数据包不断被转发，测试结果如下图：

`h1` 显示结果：

![h1_exp04_2_circle_topo](/EXP04-Boardcast_Net/assets/h1_exp04_2_circle_topo.jpg)

`h2` 利用`wireshark`抓包结果：

![h2_exp04_2_wireshark_1](/EXP04-Boardcast_Net/assets/h2_exp04_2_wireshark_1.jpg)

![h2_exp04_2_wireshark_2](/EXP04-Boardcast_Net/assets/h2_exp04_2_wireshark_2.jpg)

由上图可知，数据包在该环形拓扑中被不断转发。
