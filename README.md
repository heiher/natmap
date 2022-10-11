# NATMap

[![status](https://gitlab.com/hev/natmap/badges/master/pipeline.svg)](https://gitlab.com/hev/natmap/commits/master)

This project is used to establish a TCP port mapping from ISP NAT public address
to local private address. If all layers of NAT are full cones (NAT-1), any host
can access internal services through the mapped public address. In bind mode,
all traffic does not go through this program.

## How to Build

```bash
git clone --recursive git://github.com/heiher/natmap
cd natmap
make

# statically link
make ENABLE_STATIC=1

# openwrt
make CROSS_PREFIX=${openwrt-toolchain}/bin/x86_64-openwrt-linux-
```

## How to Use

### Usage

```
Usage:
 natmap [options]

Options:
 -4                use IPv4
 -6                use IPv6
 -d                run as daemon
 -i <interface>    network interface
 -k <interval>     seconds between each keep-alive
 -s <address>      domain name or address to STUN server
 -h <address>      domain name or address to HTTP server
 -e <path>         script path for notify mapped address

Bind options:
 -b <port>         port number for binding

Forward options:
 -t <address>      domain name or address to forward target
 -p <port>         port number to forward target
```

### Bind mode

This program will establishs a TCP port mapping in two steps:

1. Establish a connection with the HTTP server from the specified bind port and
keep-alive.
2. Establish a connection with the STUN server from the same port and obtain the
public address.

And this program will call the script specified by the argument to inform the
public address after the TCP port mapping is established. and the script can
update it to the DNS record for external access.

Please note that you need to open the firewall to allow access to the bind port.

#### OpenWrt

Goto Network -> Firewall -> Traffic Rules

Add a traffic rule:

* Protocol: TCP
* Source zone: wan
* Destination zone: Device (input)
* Destination port: [bind port]
* Action: accept
* Others: keep default values

```bash
natmap -s stun.stunprotocol.org -h qq.com -b 80 -e /bin/echo
```

If the port binding fails, because it is already in use. This program will try
to find out which local service process takes up the port and enable reuse port
remotely. This works in Linux kernel 5.6 and later, and needs to run as root.

### Forward mode

Similar to bind mode, this program will listening on bound port and accepts the
incoming connections and forward to target address.

```bash
natmap -s stun.stunprotocol.org -h qq.com -b 80 -t 10.0.0.2 -p 80 -e /bin/echo
```

Another way is to use firewall's DNAT to forward, and this way should uses bind
mode.

#### OpenWrt

Goto Network -> Firewall -> Port Forwards

Add a port forward rule:

* Protocol: TCP
* Source zone: wan
* External port: [bind port]
* Destination zone: lan
* Internal IP address: 10.0.0.2
* Internal port: 80
* Others: keep default values

### Script arguments

* argv[0]: Script path
* argv[1]: IPv4 (142.163.54.79)
* argv[2]: Port (53906)
* argv[3]: IP4P (2001::92d2:8ea3:364f)

### IP4P address

The IP4P address format uses IPv6 special addresses to encode IPv4 addresses and
ports for easy distribution through DNS AAAA records.

```
2001::{port}:{ipv4-hi16}:{ipv4-lo16}
```

## Contributors
* **hev** - https://hev.cc
* **mike wang** - https://github.com/mikewang000000

## License
MIT
