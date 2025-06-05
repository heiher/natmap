# NATMap

[![status](https://github.com/heiher/natmap/actions/workflows/build.yaml/badge.svg?branch=master&event=push)](https://github.com/heiher/natmap)
[![chat](https://github.com/heiher/natmap/raw/master/.github/badges/telegram.svg)](https://t.me/hellonatter)

This project is used to establish a TCP/UDP port mapping from an ISP NAT public
address to local private address. If all layers of NAT are full cones (NAT-1),
any host can access internal services through the mapped public address. In bind
mode, all traffic does not go through this program.

[中文文档](https://github.com/heiher/natmap/wiki)

## How to Build

```bash
git clone --recursive https://github.com/heiher/natmap.git
cd natmap
make

# statically link
make ENABLE_STATIC=1

# cross compile
make CROSS_PREFIX=${cross-toolchain}/bin/x86_64-unknown-linux-

# android
mkdir natmap
cd natmap
git clone --recursive https://github.com/heiher/natmap.git jni
ndk-build

# windows (msys2)
export MSYS=winsymlinks:native
git clone --recursive https://github.com/heiher/natmap.git
cd natmap
make LFLAGS="-lmsys-2.0 -lws2_32"
```

## How to Use

### Usage

```
Usage:
 natmap [options]

Options:
 -4                  use IPv4
 -6                  use IPv6
 -u                  UDP mode
 -d                  run as daemon
 -i <interface>      network interface or IP address
 -k <interval>       seconds between each keep-alive
 -c <count>          UDP STUN check cycle (every <count> intervals)
 -s <addr>[:port]    domain name or address of STUN server
 -h <addr>[:port]    domain name or address of HTTP server
 -e <path>           script path for notify mapped address
 -f <mark>           fwmark value (hex: 0x1, dec: 1, oct: 01)

Bind options:
 -b <port>[-port]    port number range for binding
                     - <0>: random allocation
                     - <port>: specified
                     - <port>-<port>: sequential allocation within the range

Forward options:
 -T <timeout>        port forwarding timeout in seconds
 -t <address>        domain name or address of forward target
 -p <port>           port number of forward target (0: use public port)
```

### Bind mode

```bash
# TCP
natmap -s turn.cloudflare.com -h example.com -b 80

# UDP
natmap -u -s turn.cloudflare.com -b 443
```

In TCP mode, this program will establishs a TCP port mapping in two steps:

1. Establish a connection with the HTTP server from the specified bind port and
keep it alive.
2. Establish a connection with the STUN server from the same port and obtain the
public address.

This program will then call the script specified by the argument to inform the
public address after the port mapping is established. The script can update
the DNS record for external access.

In TCP mode on Windows, make sure your application server is bound to the local
network IP, not to `0.0.0.0` and `::`.

In UDP mode on Windows, make sure your application server is bound to `0.0.0.0`
or `::`, not to the local network IP.

Please note that you need to open the firewall to allow access to the bind port.

#### OpenWrt

Goto Network -> Firewall -> Traffic Rules

Add a traffic rule:

* Protocol: TCP/UDP
* Source zone: wan
* Destination zone: Device (input)
* Destination port: [bind port]
* Action: accept
* Others: keep default values

If the port binding fails because it is already in use, this program will try
to find out which local service process occupies the port and enable port reuse
remotely. This works in Linux kernel 5.6 and later, and needs to run as root.

### Forward mode

```bash
# TCP
natmap -s turn.cloudflare.com -h example.com -b 80 -t 10.0.0.2 -p 80

# UDP
natmap -u -s turn.cloudflare.com -b 443 -t 10.0.0.2 -p 443
```

Similar to bind mode, this program will listen on bound port, accepts incoming
connections, and forward them to target address.

Another way is to use firewall's DNAT to forward, and this way should uses bind
mode.

#### OpenWrt

Goto Network -> Firewall -> Port Forwards

Add a port forward rule:

* Protocol: TCP/UDP
* Source zone: wan
* External port: [bind port]
* Destination zone: lan
* Internal IP address: 10.0.0.2
* Internal port: 80
* Others: keep default values

### Script arguments

```
{public-addr} {public-port} {ip4p} {private-port} {protocol} {private-addr}
```

* argv[0]: Script path
* argv[1]: Public address (IPv4/IPv6)
* argv[2]: Public port
* argv[3]: IP4P
* argv[4]: Bind port (private port)
* argv[5]: Protocol (TCP/UDP)
* argv[6]: Private address (IPv4/IPv6)

### IP4P address

The IP4P address format uses IPv6 special addresses to encode IPv4 addresses and
ports for easy distribution through DNS AAAA records.

```
2001::{port}:{ipv4-hi16}:{ipv4-lo16}
```

### Docker

* Support **amd64** and **arm64**.
* image tags: latest-amd64, latest-arm64 or [release tags]-amd64 like 20250512-amd64
* Change command to what you want.
* volumes script path, and make sure have permission to run.

docker-compose.yml
```docker-compse
services:
  natmap:
    container_name: natmap
    image: heiher/natmap:latest-amd64
    command: -u -s stun.qq.com -b 30101 -t 127.0.0.1 -p 51820 -e /opt/cf_ip4p.sh -k 60
    volumes:
      - ./natmap/cf_ip4p.sh:/opt/cf_ip4p.sh
    cap_add:
      - NET_ADMIN
    privileged: true
    environment:
      - TZ=Asia/Shanghai
    network_mode: host
    restart: always
```

## Contributors
* **abgelehnt** - https://github.com/abgelehnt
* **hev** - https://hev.cc
* **mike wang** - https://github.com/mikewang000000
* **muink** - https://github.com/muink
* **tianling shen** - https://github.com/1715173329
* **xhe** - https://github.com/xhebox

## License
MIT
