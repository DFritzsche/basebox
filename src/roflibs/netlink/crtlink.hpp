/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CRTLINK_H_
#define CRTLINK_H_ 1

#include <algorithm>
#include <cassert>
#include <exception>
#include <map>
#include <ostream>
#include <set>
#include <string>

#include <rofl/common/caddress.h>
#include <rofl/common/cmemory.h>

#include <netlink/object.h>
#include <netlink/route/link.h>
#include <netlink/route/link/bridge.h>
#include <inttypes.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include "roflibs/netlink/crtneighs.hpp"

namespace rofcore {

class crtlink {
public:
  class eRtLinkBase : public std::runtime_error {
  public:
    eRtLinkBase(const std::string &__arg) : std::runtime_error(__arg) {}
  };
  class eRtLinkNotFound : public eRtLinkBase {
  public:
    eRtLinkNotFound(const std::string &__arg) : eRtLinkBase(__arg) {}
  };
  class eRtLinkExists : public eRtLinkBase {
  public:
    eRtLinkExists(const std::string &__arg) : eRtLinkBase(__arg) {}
  };

public:
  /**
   *
   */
  crtlink()
      : flags(0), operstate(0), af(0), arptype(0), ifindex(0), mtu(0),
        master(0) {
    memset(&br_vlan, 0, sizeof(struct rtnl_link_bridge_vlan));
  }

  /**
   *
   */
  crtlink(struct rtnl_link *link)
      : flags(0), af(0), arptype(0), ifindex(0), mtu(0), master(0) {
    char s_buf[128];
    memset(s_buf, 0, sizeof(s_buf));

    nl_object_get(
        (struct nl_object *)link); // increment reference counter by one

    devname.assign(rtnl_link_get_name(link));
    maddr = rofl::cmacaddr(
        nl_addr2str(rtnl_link_get_addr(link), s_buf, sizeof(s_buf)));
    bcast = rofl::cmacaddr(
        nl_addr2str(rtnl_link_get_broadcast(link), s_buf, sizeof(s_buf)));
    flags = rtnl_link_get_flags(link);
    operstate = rtnl_link_get_operstate(link);
    af = rtnl_link_get_family(link);
    arptype = rtnl_link_get_arptype(link);
    ifindex = rtnl_link_get_ifindex(link);
    mtu = rtnl_link_get_mtu(link);
    master = rtnl_link_get_master(link);

    if (AF_BRIDGE == af) {
      struct rtnl_link_bridge_vlan *vlans =
          rtnl_link_bridge_get_port_vlan(link);
      if (vlans) {
        memcpy(&br_vlan, vlans, sizeof(struct rtnl_link_bridge_vlan));
      } else {
        memset(&br_vlan, 0, sizeof(struct rtnl_link_bridge_vlan));
      }
    } else {
      memset(&br_vlan, 0, sizeof(struct rtnl_link_bridge_vlan));
    }

    nl_object_put(
        (struct nl_object *)link); // decrement reference counter by one
  }

  /**
   *
   */
  virtual ~crtlink() {}

  /**
   *
   */
  crtlink(const crtlink &rtlink) { *this = rtlink; }

  /**
   *
   */
  crtlink &operator=(const crtlink &rtlink) {
    if (this == &rtlink)
      return *this;

    devname = rtlink.devname;
    maddr = rtlink.maddr;
    bcast = rtlink.bcast;
    flags = rtlink.flags;
    operstate = rtlink.operstate;
    af = rtlink.af;
    arptype = rtlink.arptype;
    ifindex = rtlink.ifindex;
    mtu = rtlink.mtu;
    master = rtlink.master;

    memcpy(&br_vlan, &rtlink.br_vlan, sizeof(rtnl_link_bridge_vlan));

    return *this;
  }

  /**
   *
   */
  bool operator==(const crtlink &rtlink) {
    return ((devname == rtlink.devname) && (ifindex == rtlink.ifindex) &&
            (maddr == rtlink.maddr));
  }

public:
  /**
   *
   */
  const std::string &get_devname() const { return devname; }

  /**
   *
   */
  const rofl::cmacaddr &get_hwaddr() const { return maddr; }

  /**
   *
   */
  const rofl::cmacaddr &get_broadcast() const { return bcast; }

  /**
   *
   */
  unsigned int get_flags() const { return flags; }

  unsigned int get_operstate() const { return operstate; }

  /**
   *
   */
  int get_family() const { return af; }

  /**
   *
   */
  unsigned int get_arptype() const { return arptype; }

  /**
   *
   */
  int get_ifindex() const { return ifindex; }

  /**
   *
   */
  unsigned int get_mtu() const { return mtu; }

  int get_master() const { return master; }

  uint16_t get_pvid() const { return br_vlan.pvid; }

  const struct rtnl_link_bridge_vlan *get_br_vlan() const { return &br_vlan; }

  static bool are_br_vlan_equal(const rtnl_link_bridge_vlan *lhs,
                                const rtnl_link_bridge_vlan *rhs) {
    assert(lhs);
    assert(rhs);
    if (lhs->pvid != rhs->pvid) {
      return false;
    }

    if (memcmp(lhs->vlan_bitmap, rhs->vlan_bitmap, sizeof(rhs->vlan_bitmap))) {
      return false;
    }

    if (memcmp(lhs->untagged_bitmap, rhs->untagged_bitmap,
               sizeof(rhs->untagged_bitmap))) {
      return false;
    }

    return true;
  }

private:
  static int find_next_bit(int i, uint32_t x) {
    int j;

    if (i >= 32)
      return -1;

    /* find first bit */
    if (i < 0)
      return __builtin_ffs(x);

    /* mask off prior finds to get next */
    j = __builtin_ffs(x >> i);
    return j ? j + i : 0;
  }

  static std::string dump_bitmap(const uint32_t *b) {
    int i = -1, j, k;
    int start = -1, prev = -1;
    int done, found = 0;

    std::stringstream ss;

    for (k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
      int base_bit;
      uint32_t a = b[k];

      base_bit = k * 32;
      i = -1;
      done = 0;
      while (!done) {
        j = find_next_bit(i, a);
        if (j > 0) {
          /* first hit of any bit */
          if (start < 0 && prev < 0) {
            start = prev = j - 1 + base_bit;
            goto next;
          }
          /* this bit is a continuation of prior bits */
          if (j - 2 + base_bit == prev) {
            prev++;
            goto next;
          }
        } else
          done = 1;

        if (start >= 0) {
          found++;
          if (done && k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN - 1)
            break;

          ss << " " << start;
          if (start != prev)
            ss << "-" << prev;

          if (done)
            break;
        }
        if (j > 0)
          start = prev = j - 1 + base_bit;
      next:
        i = j;
      }
    }
    if (!found)
      ss << " <none>";

    return ss.str();
  }

public:
  /**
   *
   */
  friend std::ostream &operator<<(std::ostream &os, crtlink const &rtlink) {
    os << "<crtlink: >" << std::endl;
    os << "<devname: " << rtlink.devname << " >" << std::endl;
    os << "<hwaddr: >" << std::endl;
    os << rtlink.maddr;
    os << "<bcast: >" << std::endl;
    os << rtlink.bcast;
    os << "<flags: " << (std::hex) << rtlink.flags << (std::dec) << " >"
       << std::endl;
    os << "<operstate: " << std::hex << (unsigned)rtlink.operstate << " >"
       << std::endl;
    os << "<af: " << rtlink.af << " >" << std::endl;
    os << "<arptype: " << rtlink.arptype << " >" << std::endl;
    os << "<ifindex: " << rtlink.ifindex << " >" << std::endl;
    os << "<mtu: " << rtlink.mtu << " >" << std::endl;
    os << "<master: " << rtlink.master << " >" << std::endl;

    if (AF_BRIDGE == rtlink.af) {
      os << "<pvid: " << rtlink.br_vlan.pvid << " >" << std::endl;
      os << "<vlans (all): " << dump_bitmap(rtlink.br_vlan.vlan_bitmap) << " >"
         << std::endl;
      os << "<vlans (untagged): " << dump_bitmap(rtlink.br_vlan.untagged_bitmap)
         << " >" << std::endl;
    }

    return os;
  }

  /**
   *
   */
  std::string str() const {
    /*
     * 2: mgt0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast
*state UP mode DEFAULT qlen 1000
*	   link/ether 52:54:00:b5:b4:e2 brd ff:ff:ff:ff:ff:ff
     */
    std::stringstream ss;
    ss << "@" << ifindex << ": " << devname << ": <";
    // flags
    if (flags & IFF_UP)
      ss << "UP,";
    if (flags & IFF_BROADCAST)
      ss << "BROADCAST,";
    if (flags & IFF_POINTOPOINT)
      ss << "POINTOPOINT,";
    if (flags & IFF_RUNNING)
      ss << "RUNNING,";
    if (flags & IFF_NOARP)
      ss << "NOARP,";
    if (flags & IFF_PROMISC)
      ss << "PROMISC,";
    if (flags & IFF_MASTER)
      ss << "MASTER,";
    if (flags & IFF_SLAVE)
      ss << "SLAVE,";
    if (flags & IFF_MULTICAST)
      ss << "MULTICAST,";
    if (flags & IFF_LOWER_UP)
      ss << "LOWER_UP,";

    ss << "> ";
    if (IF_OPER_UNKNOWN != operstate) {
      char buf[64];
      ss << "state " << rtnl_link_operstate2str(operstate, buf, sizeof(buf))
         << " ";
    }

    ss << "mtu: " << mtu << std::endl;
    ss << "link/";
    if (arptype == ARPHRD_ETHER)
      ss << "ether ";
    else
      ss << "unknown ";
    ss << maddr.str() << " brd " << bcast.str() << std::endl;

    if (AF_BRIDGE == af) {
      ss << "pvid: " << br_vlan.pvid << std::endl;
      ss << "vlans (all): " << dump_bitmap(br_vlan.vlan_bitmap) << std::endl;
      ss << "vlans (untagged): " << dump_bitmap(br_vlan.untagged_bitmap)
         << std::endl;
    }

    return ss.str();
  }

  /**
   *
   */
  class crtlink_find_by_ifindex : public std::unary_function<crtlink, bool> {
    int ifindex;

  public:
    crtlink_find_by_ifindex(unsigned int ifindex) : ifindex(ifindex) {}
    bool operator()(crtlink const &rtl) { return (ifindex == rtl.ifindex); }
    bool operator()(std::pair<unsigned int, crtlink> const &p) {
      return (ifindex == p.second.ifindex);
    }
    bool operator()(std::pair<unsigned int, crtlink *> const &p) {
      return (ifindex == p.second->ifindex);
    }
  };

  /**
   *
   */
  class crtlink_find_by_devname : public std::unary_function<crtlink, bool> {
    std::string devname;

  public:
    crtlink_find_by_devname(std::string const &devname) : devname(devname) {}
    bool operator()(crtlink const &rtl) { return (devname == rtl.devname); }
    bool operator()(std::pair<unsigned int, crtlink> const &p) {
      return (devname == p.second.devname);
    }
  };

private:
  std::string devname;  // device name (e.g. eth0)
  rofl::cmacaddr maddr; // mac address
  rofl::cmacaddr bcast; // broadcast address
  unsigned int flags;   // link flags
  uint8_t operstate;    // link operational status
  int af;               // address family (AF_INET, AF_UNSPEC, ...)
  unsigned int arptype; // ARP type (e.g. ARPHDR_ETHER)
  int ifindex;          // interface index
  unsigned int mtu;     // maximum transfer unit
  int master;           // ifindex of master interface

  struct rtnl_link_bridge_vlan br_vlan;
};

} // end of namespace dptmap

#endif /* CRTLINK_H_ */
