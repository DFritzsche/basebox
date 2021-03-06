/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CPACKETPOOL_H_
#define CPACKETPOOL_H_ 1

#include <exception>
#include <vector>
#include <deque>

#include <rofl/common/cpacket.h>
#include <rofl/common/locking.hpp>

namespace rofcore {

class ePacketPoolBase : public std::runtime_error {
public:
  ePacketPoolBase(const std::string &__arg) : std::runtime_error(__arg){};
};
class ePacketPoolExhausted : public ePacketPoolBase {
public:
  ePacketPoolExhausted(const std::string &__arg) : ePacketPoolBase(__arg){};
};

class cpacketpool {
  static cpacketpool *packetpool;
  cpacketpool(unsigned int n_pkts = 256, unsigned int pkt_size = 1518);
  cpacketpool(cpacketpool const &packetpool);
  ~cpacketpool();

  std::vector<rofl::cpacket *> pktpool;
  std::deque<rofl::cpacket *> idlepool;

  // lock for peer controllers
  rofl::crwlock pool_rwlock;

public:
  static cpacketpool &get_instance();

  rofl::cpacket *acquire_pkt();

  void release_pkt(rofl::cpacket *pkt);
};

}; // end of namespace vmcore

#endif /* CPACKETPOOL_H_ */
