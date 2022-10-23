#/bin/sh
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi
echo 12582912  > /proc/sys/net/core/wmem_max
echo 12582912  > /proc/sys/net/core/rmem_max
echo "10240 87380 12582912" > /proc/sys/net/ipv4/tcp_rmem
echo "10240 87380 12582912" > /proc/sys/net/ipv4/tcp_wmem
echo "12582912 12582912 12582912" > /proc/sys/net/ipv4/tcp_mem
echo 12582912 > /proc/sys/net/ipv4/tcp_limit_output_bytes
#echo 1048576 > /proc/sys/net/ipv4/tcp_limit_output_bytes
echo 1 > /proc/sys/net/ipv4/route/flush
