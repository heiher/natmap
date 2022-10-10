# NATMap

[![status](https://gitlab.com/hev/natmap/badges/master/pipeline.svg)](https://gitlab.com/hev/natmap/commits/master)

The NATMap is used to establish a TCP mapping of public address to private
address. On a full cone NAT(NAT-1), this allows any host to access internal
services through public address. The transmission does not go through the
NATMap on binding mode.

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

The NATMap binds to TCP port 80 as the source, establishes a TCP connection to
the qq.com for keep-alive and anoter to the stun.stunprotocol.org to get the
public source address. The script (/bin/echo) will be execute with arguments
after the mapped address is returned by the STUN server. The argument of the
script is the public source address/port and IP4P address format for IPv4.

Typically, the script is used to update the IP4P address to the DNS record of a
domain name for accessing the mapped services.

```bash
natmap -s stun.stunprotocol.org -h qq.com -b 80 -e /bin/echo
```

When binding an address that is already bound and not set reuseport, the NATMap
will try to set the reuseport of the socket in the service process. That works
from Linux kernel 5.6 and needs run as root.

### Forward mode

Similar to bind mode, the difference is the transmission is go through the
NATMap.

```bash
natmap -s stun.stunprotocol.org -h qq.com -b 80 -t 127.0.0.1 -p 80 -e /bin/echo
```

### Script arguments

* argv[0]: Script path
* argv[1]: IPv4 (142.163.54.79)
* argv[2]: Port (53906)
* argv[3]: IP4P (2001::92d2:8ea3:364f)

### IP4P address

The IP4P address format uses IPv6 reserved addresses to encode IPv4 addresses
and ports for easy distribution thorugh DNS AAAA records.

```
2001::{port}:{ipv4-hi16}:{ipv4-lo16}
```

## Contributors
* **hev** - https://hev.cc

## License
MIT
