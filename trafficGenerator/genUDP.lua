require "Pktgen"

-- Setup values at your convenience
local port = "0";
local dstip = "10.42.0.1";
local srcip = "10.42.0.2";
local dstmac = "08:c0:eb:bf:ee:c2";
local srcmac = "08:c0:eb:bf:ef:8e";

-- Clear everything
pktgen.clr();


pktgen.range.dst_mac("all", "start", dstmac);
pktgen.range.src_mac("all", "start", srcmac);

pktgen.range.src_ip("all", "start", srcip);

-- For some reason, the simple start statement does not work
-- I had to set min, max to the same value and increment to 0
pktgen.range.dst_ip("all", "start", dstip);
pktgen.range.dst_ip("all", "min", dstip);
pktgen.range.dst_ip("all", "inc", 0);
pktgen.range.dst_ip("all", "max", dstip);


-- pktgen.set_ipaddr("all", "dst", dstip);
-- pktgen.set_ipaddr("all", "src", srcip);

pktgen.range.src_port("all", "start", 1024);
pktgen.range.src_port("all", "inc", 1);
pktgen.range.src_port("all", "min", 1024);
pktgen.range.src_port("all", "max", 1034);

-- For some reason, the simple start statement does not work
-- I had to set min, max to the same value and increment to 0
pktgen.range.dst_port("all", "start", 80);
pktgen.range.dst_port("all", "inc", 0);
pktgen.range.dst_port("all", "min", 80);
pktgen.range.dst_port("all", "max", 80);

pktgen.range.pkt_size("all", "start", 128);
pktgen.range.pkt_size("all", "inc", 2);
pktgen.range.pkt_size("all", "min", 64);
pktgen.range.pkt_size("all", "max", 1518);

pktgen.set_range("all", "on");
pktgen.range.ip_proto(port,"udp");
pktgen.set("all","rate",100);
-- pktgen.range.tcp_flags(port,"0x2");
-- pktgen.range.pad_fpath("all","/root/pktgen-3.1.2_work/scripts/payload.txt");

pktgen.start("all");