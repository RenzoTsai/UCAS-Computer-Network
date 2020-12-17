# 路由器转发实验

2020年11月8日

[蔡润泽](https://github.com/RenzoTsai)

[本实验 `Github` 地址](https://github.com/RenzoTsai/UCAS-Computer-Network/tree/master/EXP07-Router)

## 实验内容

### 实验内容一

- 运行给定网络拓扑(router_topo.py)

- 在r1上执行路由器程序./router，进行数据包的处理

- 在h1上进行ping实验

    * Ping 10.0.1.1 (r1)，能够ping通

    * Ping 10.0.2.22 (h2)，能够ping通

    * Ping 10.0.3.33 (h3)，能够ping通

    * Ping 10.0.3.11，返回ICMP Destination Host Unreachable

    * Ping 10.0.4.1，返回ICMP Destination Net Unreachable

### 实验内容二

- 构造一个包含多个路由器节点组成的网络

    * 手动配置每个路由器节点的路由表

    * 有两个终端节点，通过路由器节点相连，两节点之间的跳数不少于3跳，手动配置其默认路由表

- 连通性测试

    * 终端节点ping每个路由器节点的入端口IP地址，能够ping通

- 路径测试

    * 在一个终端节点上traceroute另一节点，能够正确输出路径上每个节点的IP信息

## 设计思路

### ARP数据包处理

#### arp.c 中 `handle_arp_packet` 函数

当每一个端口收到数据包后，数据链路层若查询到这是一个ARP的包，则处理函数跳转到`handle_arp_packet` 函数。

`handle_arp_packet` 函数需要比对端口IP。若该端口IP与数据包的目的IP相同，则进一步处理：

- 若该包是ARP应答包，则说明该端口需要将源IP和源MAC地址的映射插入到ARP Cache中，即调用`arpcache_insert`处理。

- 若该包是ARP请求包，则向发送请求的端口回复ARP包，传递该端口IP和MAC地址的解析信息，即调用`arp_send_reply` 函数进行ARP应答。

#### arp.c 中 `arp_send_reply` 函数

当ARP请求的目的IP对应的收到ARP请求包，则向发送请求的端口回复ARP包，传递该端口IP和MAC地址的解析信息。

`arp_send_reply` 函数需要根据`ARP协议`和接收到的`ARP请求`进行组包应答，具体实现函数如下：

```c
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
    char * packet = (char *) malloc(ETHER_HDR_SIZE + sizeof(struct ether_arp));
    struct ether_header * ether_hdr = (struct ether_header *)packet;
    struct ether_arp * ether_arp_pkt = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
    ether_hdr->ether_type = htons(ETH_P_ARP);
    memcpy(ether_hdr->ether_shost, iface->mac, ETH_ALEN);
    memcpy(ether_hdr->ether_dhost, req_hdr->arp_sha, ETH_ALEN);
    ether_arp_pkt->arp_hrd = htons(ARPHRD_ETHER);
    ether_arp_pkt->arp_pro = htons(ETH_P_IP);
    ether_arp_pkt->arp_hln = (u8)ETH_ALEN;
    ether_arp_pkt->arp_pln = (u8)4;
    ether_arp_pkt->arp_op = htons(ARPOP_REPLY);
    memcpy(ether_arp_pkt->arp_sha, iface->mac, ETH_ALEN);
    ether_arp_pkt->arp_spa = htonl(iface->ip);
    memcpy(ether_arp_pkt->arp_tha, req_hdr->arp_sha, ETH_ALEN);
    ether_arp_pkt->arp_tpa = req_hdr->arp_spa;
    iface_send_packet(iface, packet, ETHER_HDR_SIZE + sizeof(struct ether_arp));
}
```

#### arp.c 中 `arp_send_request` 函数

被插入到ARP Cache中被缓存的条目若找不到这个IP的条目，说明还没有发过ARP请求，此时需要调用`arp_send_reply` 函数发送`ARP请求`。

`arp_send_reply` 函数需要根据`ARP协议`进行组包发送`ARP请求`，具体实现函数如下：

```c
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
    char * packet = (char *) malloc(ETHER_HDR_SIZE + sizeof(struct ether_arp));
    struct ether_header * ether_hdr = (struct ether_header *)packet;
    struct ether_arp * ether_arp_pkt = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
    ether_hdr->ether_type = htons(ETH_P_ARP);
    memcpy(ether_hdr->ether_shost, iface->mac, ETH_ALEN);
    memset(ether_hdr->ether_dhost, (u8)0xff, ETH_ALEN);
    ether_arp_pkt->arp_hrd = htons(ARPHRD_ETHER);
    ether_arp_pkt->arp_pro = htons(ETH_P_IP);
    ether_arp_pkt->arp_hln = (u8)ETH_ALEN;
    ether_arp_pkt->arp_pln = (u8)4;
    ether_arp_pkt->arp_op = htons(ARPOP_REQUEST);
    memcpy(ether_arp_pkt->arp_sha, iface->mac, ETH_ALEN);
    ether_arp_pkt->arp_spa = htonl(iface->ip);
    memset(ether_arp_pkt->arp_tha, 0, ETH_ALEN);
    ether_arp_pkt->arp_tpa = htonl(dst_ip);
    iface_send_packet(iface, packet, ETHER_HDR_SIZE + sizeof(struct ether_arp));
}
```

#### arpcache.c 中 `arpcache_insert` 函数

若端口收到ARP应答包，则说明该端口需要将源IP和源MAC地址的映射插入到ARP Cache中，即调用`arpcache_insert`处理。

`arpcache_insert` 函数需要将IP和MAC的映射写入到ARP Cache中，如果缓存已满（MAX NUM = 32），则随机替换掉其中一个。

然后将在缓存中等待该映射的数据包，依次填写目的MAC地址，调用 arp.c 中的`iface_send_packet_by_arp`函数转发出去，并删除掉相应缓存数据包。

#### arpcache.c 中 `arpcache_append_packet` 函数

arp.c 中的`iface_send_packet_by_arp`函数被调用时首先查找IP和MAC映射在不在ARP Cache中。若在ARP Cache中，则填充数据包的目的MAC地址，并转发该数据包。若不在ARP Cache中，则需要调用`arpcache_append_packet`函数，将需要发送的包加入到缓存队列。

`arpcache_append_packet`函数需要先查找需要发送的目的IP地址在不在缓存队列中。如果不在，则分配一个空间用来记录需要请求的目的IP地址条目，并配置好相关参数。

`arpcache_append_packet`再次发送ARP请求，来获得目的IP对应的MAC地址。

#### arpcache.c 中 `arpcache_sweep` 函数

每隔1秒钟，进程都需要运行一次arpcache_sweep操作：

- 如果一个缓存条目在缓存中已存在超过了15秒，将该条目清除。

- 如果一个IP对应的ARP请求发出去已经超过了1秒，重新发送ARP请求。

- 如果发送超过5次仍未收到ARP应答，则对该队列下的数据包依次回复ICMP（Destination Host Unreachable）消息，并删除等待的数据包。

### ICMP数据包处理

#### ip.c 中 `handle_ip_packet` 函数

当每一个端口收到数据包后，数据链路层若查询到这是一个IP包，则处理函数跳转到`handle_ip_packet` 函数。

`handle_ip_packet` 函数若检测到这个IP包是一个Ping命令请求，且该端口的IP为Ping请求的目的IP,则调用`icmp_send_packet`函数发送ICMP_ECHOREPLY包。

否则，`handle_ip_packet` 函数转发该数据包。

#### ip.c 中 `ip_forward_package` 函数

如果需要转发的的IP包TTL超时即小于等于0，则调用`icmp_send_packet`函数发送ICMP_EXC_TTL包。

如果需要转发的IP包无法找到目的IP的最长前缀匹配，则调用`icmp_send_packet`函数发送ICMP_NET_UNREACH包。

如果上述问题均未出现，则调用`ip_send_packet`来转发IP包。

#### ip_base.c 中 `longest_prefix_match` 函数

该函数用来寻找IP地址和掩码的最长前缀匹配，若找到则返回路由表中的相应条目。具体代码实现如下：

```c
rt_entry_t *longest_prefix_match(u32 dst)
{
    rt_entry_t * rt_entry = NULL;
    rt_entry_t * rt_longest_mask_entry = NULL;
    list_for_each_entry(rt_entry, &rtable, list) {
        if ((rt_entry->dest & rt_entry->mask) == (dst & rt_entry->mask)) {
            if (rt_longest_mask_entry == NULL) {
                rt_longest_mask_entry = rt_entry;
            } else if (rt_longest_mask_entry->mask < rt_entry->mask) {
                    rt_longest_mask_entry = rt_entry;
            }
        }
    }
    return rt_longest_mask_entry;
}
```

#### ip_base.c 中 `ip_send_packet` 函数

该函数用来转发IP包。

其根据最长前缀匹配找到的路由表中的条目中的下一跳地址，调用`iface_send_packet_by_arp`函数将IP包转发出去。

#### icmp.c 中 `icmp_send_packet` 函数

该函数需要根据遇到的错误和ICMP协议，将对应的ICMP包发出去。具体的实现函数如下：

```c
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
    int packet_len = 0;
    struct iphdr *in_ip_hdr = packet_to_ip_hdr(in_pkt);
    if (type == ICMP_ECHOREPLY) {
        packet_len = len;
    } else {
    packet_len = ETHER_HDR_SIZE + ICMP_HDR_SIZE + IP_BASE_HDR_SIZE + IP_HDR_SIZE(in_ip_hdr) + 8;
    }
    char *packet = (char *)malloc(packet_len);
    struct ether_header *eh = (struct ether_header *)packet;
    eh->ether_type = htons(ETH_P_IP);
    struct iphdr *out_ip_hdr = packet_to_ip_hdr(packet);
    rt_entry_t *rt_entry = longest_prefix_match(ntohl(in_ip_hdr->saddr));
    ip_init_hdr(out_ip_hdr,
                rt_entry->iface->ip,
                ntohl(in_ip_hdr->saddr),
                packet_len - ETHER_HDR_SIZE,
                IPPROTO_ICMP);
    struct icmphdr * icmp_hdr = (struct icmphdr *)(packet + ETHER_HDR_SIZE + IP_HDR_SIZE(in_ip_hdr));
    icmp_hdr->type = type;
    icmp_hdr->code = code;
    if (type != ICMP_ECHOREPLY) {
        memset((char*)icmp_hdr + ICMP_HDR_SIZE - 4, 0, 4);
        memcpy((char*)icmp_hdr + ICMP_HDR_SIZE, in_ip_hdr, IP_HDR_SIZE(in_ip_hdr) + 8);
    } else {
        memcpy((char*)icmp_hdr + ICMP_HDR_SIZE - 4,
        (char*)in_ip_hdr + IP_HDR_SIZE(in_ip_hdr) + 4,
        len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_ip_hdr) - 4);
    }
    icmp_hdr->checksum = icmp_checksum(icmp_hdr, packet_len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_ip_hdr));
    ip_send_packet(packet, packet_len);
}
```

## 结果验证

### 实验一结果验证（在h1上进行ping实验）

#### Ping 10.0.1.1 (r1)，能够ping通

实验结果如下：

![ping-route](/EXP07-Router/assets/ping-route.jpg)

上述结果符合预期，即该实验成功。

#### Ping 10.0.2.22 (h2)，能够ping通

实验结果如下：

![ping-2](/EXP07-Router/assets/ping-2.jpg)

上述结果符合预期，即该实验成功。

#### Ping 10.0.3.33 (h3)，能够ping通

实验结果如下：

![ping-3](/EXP07-Router/assets/ping-3.jpg)

上述结果符合预期，即该实验成功。

#### Ping 10.0.3.11，返回ICMP Destination Host Unreachable

实验结果如下：

![host-unreachable](/EXP07-Router/assets/host-unreachable.jpg)

上述结果符合预期，即该实验成功。

#### Ping 10.0.4.1，返回ICMP Destination Net Unreachable

实验结果如下：

![net-unreachable](/EXP07-Router/assets/net_unreachable.jpg)

上述结果符合预期，即该实验成功。

### 实验二结果验证

本实验构造了一个含有两个路由器，两个主机的网络，路由器R1和R2相连，主机H1与R1相连H2与R2相连。Python 配置如下：

```py
    h1.cmd('ifconfig h1-eth0 10.0.1.11/24')
    h2.cmd('ifconfig h2-eth0 10.0.2.22/24')

    h1.cmd('route add default gw 10.0.1.1')
    h2.cmd('route add default gw 10.0.2.1')

    r1.cmd('ifconfig r1-eth0 10.0.1.1/24')
    r1.cmd('ifconfig r1-eth1 10.0.3.1/24')
    r1.cmd('route add -net 10.0.2.0 netmask 255.255.255.0 gw 10.0.3.2 dev r1-eth1')

    r2.cmd('ifconfig r2-eth0 10.0.2.1/24')
    r2.cmd('ifconfig r2-eth1 10.0.3.2/24')
    r2.cmd('route add -net 10.0.1.0 netmask 255.255.255.0 gw 10.0.3.1 dev r2-eth1')
```

#### 连通性测试

主机H1和H2相互Ping对方，结果如下：

![2-ping](/EXP07-Router/assets/2-ping.jpg)

由上图可知，两个节点可以互相Ping通，因此两主机节点连通。

#### 路径测试

在一个终端节点上traceroute另一节点，能够正确输出路径上每个节点的IP信息。

![2-traceroute](/EXP07-Router/assets/2-traceroute.jpg)

由上图可知，两个节点可以traceroute符合路由表预期。