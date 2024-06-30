# NATMap

[![status](https://github.com/heiher/natmap/actions/workflows/build.yaml/badge.svg?branch=master&event=push)](https://github.com/heiher/natmap)
[![chat](https://github.com/heiher/natmap/raw/master/.github/badges/telegram.svg)](https://t.me/hellonatter)

This project is used to establish a TCP/UDP port mapping from ISP NAT public
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
 -s <addr>[:port]    domain name or address of STUN server
 -h <addr>[:port]    domain name or address of HTTP server
 -e <path>           script path for notify mapped address
 -f <mark>           fwmark value (hex: 0x1, dec: 1, oct: 01)

Bind options:
 -b <port>           port number for binding

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
keep-alive.
2. Establish a connection with the STUN server from the same port and obtain the
public address.

And this program will call the script specified by the argument to inform the
public address after the port mapping is established. and the script can update
to the DNS record for external access.

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

If the port binding fails, because it is already in use. This program will try
to find out which local service process takes up the port and enable reuse port
remotely. This works in Linux kernel 5.6 and later, and needs to run as root.

### Forward mode

```bash
# TCP
natmap -s turn.cloudflare.com -h example.com -b 80 -t 10.0.0.2 -p 80

# UDP
natmap -u -s turn.cloudflare.com -b 443 -t 10.0.0.2 -p 443
```

Similar to bind mode, this program will listening on bound port and accepts the
incoming connections and forward to target address.

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

## Contributors
* **abgelehnt** - https://github.com/abgelehnt
* **hev** - https://hev.cc
* **mike wang** - https://github.com/mikewang000000
* **muink** - https://github.com/muink
* **tianling shen** - https://github.com/1715173329
* **xhe** - https://github.com/xhebox

## License
MIT
