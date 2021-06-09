#include <common/gtest.h>
#include "tunnels/gtp_man.h"
#include "pal/linux/mbuf.h"
#include "44bsd/tcp_dpdk.h"
#include "utl_mbuf.h"
#include "bp_sim.h"
#include "arpa/inet.h"


class gt_tunnel  : public testing::Test {

protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
public:
};


void create_pcap_and_compare(rte_mbuf_t* m, std::string pcap_file) {
    CCapPktRaw m_raw;
    fill_pkt(&m_raw, m);
    CFileWriterBase * lpWriter=CCapWriterFactory::CreateWriter(LIBPCAP, (char *)("generated/" + pcap_file).c_str());
    assert(lpWriter->write_packet(&m_raw));
    delete lpWriter;
    CErfCmp cmp;
    cmp.dump = 1;
    bool res = true;
    res = cmp.compare((char *)("exp/" + pcap_file).c_str(), (char *)("generated/" + pcap_file).c_str());
    EXPECT_EQ(1, res?1:0);
}


void prepend_ipv4_and_compare(char *buf, uint16_t len, uint32_t teid, std::string src_ipv4_str, std::string dst_ipv4_str, std::string pcap_file) {
    ipv4_addr_t src_ip, dst_ip;
    inet_pton(AF_INET, src_ipv4_str.c_str(), &src_ip);
    inet_pton(AF_INET, dst_ipv4_str.c_str(), &dst_ip);
    rte_mbuf_t* m = utl_rte_pktmbuf_mem_to_pkt(buf, len, 1024, tcp_pktmbuf_get_pool(0,1024));
    CGtpuCtx context(teid, src_ip, dst_ip);
    m->dynfield_ptr = &context;
    CGtpuMan gtpu(TUNNEL_MODE_TX);
    gtpu.on_tx(0, m);
    create_pcap_and_compare(m, pcap_file);
}


void prepend_ipv6_and_compare(char *buf, uint16_t len, uint32_t teid, std::string src_ipv6_str, std::string dst_ipv6_str, std::string pcap_file) {
    ipv6_addr_t src, dst;
    inet_pton(AF_INET6, src_ipv6_str.c_str(), src.addr);
    inet_pton(AF_INET6, dst_ipv6_str.c_str(), dst.addr);
    rte_mbuf_t* m = utl_rte_pktmbuf_mem_to_pkt(buf, len, 1024, tcp_pktmbuf_get_pool(0,1024));
    CGtpuCtx context(teid, &src, &dst);
    m->dynfield_ptr = &context;
    CGtpuMan gtpu(TUNNEL_MODE_TX);
    gtpu.on_tx(0, m);
    create_pcap_and_compare(m, pcap_file);
}


void adjust_and_compare(char *buf, uint16_t len, std::string pcap_file) {
    rte_mbuf_t* m = utl_rte_pktmbuf_mem_to_pkt(buf, len, 1024, tcp_pktmbuf_get_pool(0,1024));
    CGtpuMan gtpu(TUNNEL_MODE_RX);
    gtpu.on_rx(0, m);
    create_pcap_and_compare(m, pcap_file);
}


TEST_F(gt_tunnel, tst1) {
    unsigned char buf[74] = {0x24, 0x8a, 0x07, 0xa2, 0xf3, 0x81, 0x24, 0x8a, 0x07, 0xa2, 0xf3, 0x80, 0x08,
                             0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x40, 0x00, 0x7F, 0x06, 0x00, 0x00,
                             0x0B, 0x0B, 0x00, 0x02, 0x30, 0x00, 0x00, 0x01, 0xE6, 0xCA, 0x00, 0x50, 0x32,
                             0xA8, 0x69, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x5B, 0x53,
                             0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08,
                             0x0A, 0x6B, 0x8B, 0x45, 0x92, 0x00, 0x00, 0x00, 0x00};

    prepend_ipv4_and_compare((char *)buf, sizeof(buf), 33554432, "16.11.0.1", "52.0.0.0", "tunnel_prepend.pcap");
}


TEST_F(gt_tunnel, tst2) {
    unsigned char buf[110] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x08, 0x00,
                              0x45, 0x00, 0x00, 0x60, 0xFF, 0x7E, 0x00, 0x00, 0x40, 0x11, 0x37, 0x03, 0x10, 0x0B,
                              0x00, 0x01, 0x34, 0x00, 0x00, 0x00, 0xE6, 0xCA, 0x08, 0x68, 0x00, 0x4C, 0x00, 0x00,
                              0x30, 0xFF, 0x00, 0x3C, 0x02, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00,
                              0x40, 0x00, 0x7F, 0x06, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x02, 0x30, 0x00, 0x00, 0x01,
                              0xE6, 0xCA, 0x00, 0x50, 0x32, 0xA8, 0x69, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02,
                              0x80, 0x00, 0x5B, 0x53, 0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00,
                              0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x45, 0x92, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust.pcap");
}


TEST_F(gt_tunnel, tst3) {
   unsigned char buf[94] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x86, 0xDD,
                            0x60, 0x00, 0x00, 0x00, 0x00, 0x28, 0x06, 0x40, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x06, 0xFF, 0x03, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x2D, 0xF6, 0xB9,
                            0x00, 0x50, 0x33, 0x58, 0xBD, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00,
                            0xF7, 0xE7, 0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01,
                            0x08, 0x0A, 0x6B, 0x8B, 0x46, 0x27, 0x00, 0x00, 0x00, 0x00};
    prepend_ipv6_and_compare((char *)buf, sizeof(buf), 33554432, "ff05::b0b:9", "ff04::3000:12", "tunnel_prepend_ipv6.pcap");
}


TEST_F(gt_tunnel, tst4) {
   unsigned char buf[150] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x86, 0xDD,
                             0x60, 0x00, 0x00, 0x00, 0x00, 0x60, 0x11, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x09, 0xFF, 0x04, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x12, 0xF6, 0xB9,
                             0x08, 0x68, 0x00, 0x60, 0x00, 0x00, 0x30, 0xFF, 0x00, 0x50, 0x02, 0x00, 0x00, 0x00,
                             0x60, 0x00, 0x00, 0x00, 0x00, 0x28, 0x06, 0x40, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x06, 0xFF, 0x03, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x2D, 0xF6, 0xB9,
                             0x00, 0x50, 0x33, 0x58, 0xBD, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00,
                             0xF7, 0xE7, 0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01,
                             0x08, 0x0A, 0x6B, 0x8B, 0x46, 0x27, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv6.pcap");
}


TEST_F(gt_tunnel, tst5) {
   unsigned char buf[78] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00,
                            0x0A, 0x08, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x40, 0x00, 0x7F, 0x06, 0x00, 0x00,
                            0x0B, 0x0B, 0x00, 0x0A, 0x30, 0x00, 0x00, 0x31, 0x06, 0xAD, 0x00, 0x50, 0x33, 0x52, 0x11,
                            0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x92, 0x2F, 0x00, 0x00, 0x02, 0x04,
                            0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x45, 0xF2, 0x00,
                            0x00, 0x00, 0x00};
    prepend_ipv4_and_compare((char *)buf, sizeof(buf), 167772160, "11.11.0.1", "1.1.1.11", "tunnel_prepend_with_vlan.pcap");
}


TEST_F(gt_tunnel, tst6) {
   unsigned char buf[114] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00,
                             0x0A, 0x08, 0x00, 0x45, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x00, 0x00,
                             0x0B, 0x0B, 0x00, 0x01, 0x01, 0x01, 0x01, 0x0B, 0x06, 0xAD, 0x08, 0x68, 0x00, 0x4C, 0x00,
                             0x00, 0x30, 0xFF, 0x00, 0x3C, 0x0A, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00,
                             0x40, 0x00, 0x7F, 0x06, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x0A, 0x30, 0x00, 0x00, 0x31, 0x06,
                             0xAD, 0x00, 0x50, 0x33, 0x52, 0x11, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00,
                             0x92, 0x2F, 0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08,
                             0x0A, 0x6B, 0x8B, 0x45, 0xF2, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_with_vlan.pcap");
}


TEST_F(gt_tunnel, tst7) {
   unsigned char buf[98] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00, 0x0A,
                            0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x28, 0x06, 0x40, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x0A, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x13, 0x06, 0xAA, 0x00, 0x50, 0x36, 0xFF,
                            0x47, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x57, 0x55, 0x00, 0x00, 0x02, 0x04,
                            0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x49, 0x39, 0x00, 0x00,
                            0x00, 0x00};
    prepend_ipv6_and_compare((char *)buf, sizeof(buf), 33554432, "ff05::b0b:9", "ff04::3000:12", "tunnel_prepend_ipv6_with_vlan.pcap");
}


TEST_F(gt_tunnel, tst8) {
   unsigned char buf[154] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00, 0x0A,
                             0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x60, 0x11, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x09, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x12, 0x0B, 0x0B, 0x08, 0x68, 0x00, 0x60,
                             0x00, 0x00, 0x30, 0xFF, 0x00, 0x50, 0x02, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x28,
                             0x06, 0x40, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B,
                             0x00, 0x0A, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00,
                             0x00, 0x13, 0x06, 0xAA, 0x00, 0x50, 0x36, 0xFF, 0x47, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02,
                             0x80, 0x00, 0x57, 0x55, 0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01,
                             0x08, 0x0A, 0x6B, 0x8B, 0x49, 0x39, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv6_with_vlan.pcap");
}


TEST_F(gt_tunnel, tst9) {
   unsigned char buf[86] = {0x00, 0x05, 0x73, 0xA0, 0x07, 0xD1, 0x68, 0xA3, 0xC4, 0xF9, 0x49, 0xF6, 0x86, 0xDD, 0x60,
                            0x00, 0x00, 0x00, 0x00, 0x20, 0x06, 0x40, 0x20, 0x01, 0x04, 0x70, 0xE5, 0xBF, 0xDE, 0xAD,
                            0x49, 0x57, 0x21, 0x74, 0xE8, 0x2C, 0x48, 0x87, 0x26, 0x07, 0xF8, 0xB0, 0x40, 0x0C, 0x0C,
                            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0xF9, 0xC7, 0x00, 0x19, 0x03, 0xA0,
                            0x88, 0x30, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x20, 0x00, 0xDA, 0x47, 0x00, 0x00, 0x02,
                            0x04, 0x05, 0x8C, 0x01, 0x03, 0x03, 0x08, 0x01, 0x01, 0x04, 0x02};
    prepend_ipv4_and_compare((char *)buf, sizeof(buf), 167772160, "11.11.0.1", "1.1.1.11", "tunnel_prepend_ipv4_with_ipv6.pcap");
}


TEST_F(gt_tunnel, tst10) {
   unsigned char buf[122] = {0x00, 0x05, 0x73, 0xA0, 0x07, 0xD1, 0x68, 0xA3, 0xC4, 0xF9, 0x49, 0xF6, 0x08, 0x00, 0x45,
                             0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x01,
                             0x01, 0x01, 0x01, 0x0B, 0xF9, 0xC7, 0x08, 0x68, 0x00, 0x58, 0x4E, 0x75, 0x30, 0xFF, 0x00,
                             0x48, 0x0A, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x06, 0x40, 0x20, 0x01,
                             0x04, 0x70, 0xE5, 0xBF, 0xDE, 0xAD, 0x49, 0x57, 0x21, 0x74, 0xE8, 0x2C, 0x48, 0x87, 0x26,
                             0x07, 0xF8, 0xB0, 0x40, 0x0C, 0x0C, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A,
                             0xF9, 0xC7, 0x00, 0x19, 0x03, 0xA0, 0x88, 0x30, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x20,
                             0x00, 0xDA, 0x47, 0x00, 0x00, 0x02, 0x04, 0x05, 0x8C, 0x01, 0x03, 0x03, 0x08, 0x01, 0x01,
                             0x04, 0x02};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv4_with_ipv6.pcap");
}


TEST_F(gt_tunnel, tst11) {
   unsigned char buf[98] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00, 0x0A,
                            0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x28, 0x06, 0x40, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x0A, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x13, 0x06, 0xAA, 0x00, 0x50, 0x36, 0xFF,
                            0x47, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x57, 0x55, 0x00, 0x00, 0x02, 0x04,
                            0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x49, 0x39, 0x00, 0x00,
                            0x00, 0x00};
    prepend_ipv4_and_compare((char *)buf, sizeof(buf), 167772160, "11.11.0.1", "1.1.1.11", "tunnel_prepend_ipv4_with_vlan_ipv6.pcap");
}


TEST_F(gt_tunnel, tst12) {
   unsigned char buf[134] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00, 0x0A,
                             0x08, 0x00, 0x45, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x00, 0x00, 0x0B, 0x0B,
                             0x00, 0x01, 0x01, 0x01, 0x01, 0x0B, 0x06, 0xAA, 0x08, 0x68, 0x00, 0x60, 0x41, 0x7B, 0x30, 0xFF,
                             0x00, 0x50, 0x0A, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x28, 0x06, 0x40, 0xFF, 0x02,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x0A, 0xFF, 0x03,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x13, 0x06, 0xAA,
                             0x00, 0x50, 0x36, 0xFF, 0x47, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x57, 0x55,
                             0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B,
                             0x49, 0x39, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv4_with_vlan_ipv6.pcap");
}


TEST_F(gt_tunnel, tst13) {
    unsigned char buf[74] = {0x24, 0x8a, 0x07, 0xa2, 0xf3, 0x81, 0x24, 0x8a, 0x07, 0xa2, 0xf3, 0x80, 0x08,
                             0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x40, 0x00, 0x7F, 0x06, 0x00, 0x00,
                             0x0B, 0x0B, 0x00, 0x02, 0x30, 0x00, 0x00, 0x01, 0xE6, 0xCA, 0x00, 0x50, 0x32,
                             0xA8, 0x69, 0xC6, 0x00, 0x00, 0x00, 0x00,0xA0, 0x02, 0x80, 0x00, 0x5B, 0x53,
                             0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08,
                             0x0A, 0x6B, 0x8B, 0x45, 0x92, 0x00, 0x00, 0x00, 0x00};

    prepend_ipv6_and_compare((char *)buf, sizeof(buf), 33554432,  "ff05::b0b:9", "ff04::3000:12", "tunnel_prepend_ipv6_with_ipv4.pcap");
}


TEST_F(gt_tunnel, tst14) {
   unsigned char buf[134] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x86, 0xDD, 0x60, 0x00,
                             0x00, 0x00, 0x00, 0x4C, 0x11, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x0B, 0x0B, 0x00, 0x09, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x30, 0x00, 0x00, 0x12, 0xE6, 0xCA, 0x08, 0x68, 0x00, 0x4C, 0x9F, 0xA2, 0x30, 0xFF,
                             0x00, 0x3C, 0x02, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x40, 0x00, 0x7F, 0x06,
                             0x00, 0x00, 0x0B, 0x0B, 0x00, 0x02, 0x30, 0x00, 0x00, 0x01, 0xE6, 0xCA, 0x00, 0x50, 0x32, 0xA8,
                             0x69, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x5B, 0x53, 0x00, 0x00, 0x02, 0x04,
                             0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x45, 0x92, 0x00, 0x00,
                             0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv6_with_ipv4.pcap");
}


TEST_F(gt_tunnel, tst15) {
    unsigned char buf[78] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00,
                             0x0A, 0x08, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x40, 0x00, 0x7F, 0x06, 0x00, 0x00,
                             0x0B, 0x0B, 0x00, 0x0A, 0x30, 0x00, 0x00, 0x31, 0x06, 0xAD, 0x00, 0x50, 0x33, 0x52, 0x11,
                             0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x92, 0x2F, 0x00, 0x00, 0x02, 0x04,
                             0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B, 0x45, 0xF2, 0x00,
                             0x00, 0x00, 0x00};
    prepend_ipv6_and_compare((char *)buf, sizeof(buf), 33554432, "ff05::b0b:9", "ff04::3000:12", "tunnel_prepend_ipv6_with_vlan_ipv4.pcap");
}


TEST_F(gt_tunnel, tst16) {
   unsigned char buf[134] = {0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x81, 0x24, 0x8A, 0x07, 0xA2, 0xF3, 0x80, 0x81, 0x00, 0x00, 0x0A,
                             0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x11, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x09, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x12, 0x06, 0xAD, 0x08, 0x68, 0x00, 0x4C,
                             0x7F, 0xC0, 0x30, 0xFF, 0x00, 0x3C, 0x02, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x3C, 0x00, 0x00,
                             0x40, 0x00, 0x7F, 0x06, 0x00, 0x00, 0x0B, 0x0B, 0x00, 0x0A, 0x30, 0x00, 0x00, 0x31, 0x06, 0xAD,
                             0x00, 0x50, 0x33, 0x52, 0x11, 0xC6, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x02, 0x80, 0x00, 0x92, 0x2F,
                             0x00, 0x00, 0x02, 0x04, 0x05, 0xB4, 0x01, 0x03, 0x03, 0x00, 0x01, 0x01, 0x08, 0x0A, 0x6B, 0x8B,
                             0x45, 0xF2, 0x00, 0x00, 0x00, 0x00};
    adjust_and_compare((char *)buf, sizeof(buf), "tunnel_adjust_ipv6_with_vlan_ipv4.pcap");
}