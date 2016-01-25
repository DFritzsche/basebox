/*
 * ctapdev.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CTAPDEV_H_
#define CTAPDEV_H_ 1


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#ifdef __cplusplus
}
#endif

#include <list>

#include <rofl/common/cthread.hpp>
#include <rofl/common/cdptid.h>
#include <roflibs/netlink/cnetdev.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class eTapDevBase 			: public eNetDevBase {
public:
	eTapDevBase(const std::string& __arg) : eNetDevBase(__arg) {};
};
class eTapDevSysCallFailed 	: public eTapDevBase {
public:
	eTapDevSysCallFailed(const std::string& __arg) : eTapDevBase(__arg) {};
};
class eTapDevOpenFailed		: public eTapDevSysCallFailed {
public:
	eTapDevOpenFailed(const std::string& __arg) : eTapDevSysCallFailed(__arg) {};
};
class eTapDevIoctlFailed	: public eTapDevSysCallFailed {
public:
	eTapDevIoctlFailed(const std::string& __arg) : eTapDevSysCallFailed(__arg) {};
};
class eTapDevNotFound		: public eTapDevBase {
public:
	eTapDevNotFound(const std::string& __arg) : eTapDevBase(__arg) {};
};



class ctapdev : public cnetdev, rofl::cthread_env {

	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets
	rofl::cdptid					dptid;
	std::string						devname;
	uint16_t						pvid;
	rofl::cmacaddr					hwaddr;
	rofl::cthread thread;

	enum ctapdev_timer_t {
		CTAPDEV_TIMER_OPEN_PORT = 1,
	};

public:


	/**
	 *
	 * @param netdev_owner
	 * @param devname
	 * @param hwaddr
	 */
	ctapdev(
			cnetdev_owner *netdev_owner,
			const rofl::cdptid& dptid,
			std::string const& devname,
			uint16_t pvid,
			rofl::cmacaddr const& hwaddr,
			pthread_t tid = 0);


	/**
	 *
	 */
	virtual ~ctapdev();


	/**
	 * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
	 *
	 * rofl::cpacket instance must have been allocated on heap and must
	 * be removed
	 */
	virtual void enqueue(rofl::cpacket *pkt);

	virtual void enqueue(std::vector<rofl::cpacket*> pkts);

	/**
	 *
	 */
	rofl::cdptid
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	uint16_t
	get_pvid() const { return pvid; };

protected:

	void
	tx();

	/**
	 * @brief	open tapX device
	 */
	void
	tap_open(std::string const& devname, rofl::cmacaddr const& hwaddr);


	/**
	 * @brief	close tapX device
	 *
	 */
	void
	tap_close();


	/**
	 * @brief	handle read events on file descriptor
	 */
	virtual void
	handle_read_event(rofl::cthread& thread, int fd);


	/**
	 * @brief	handle write events on file descriptor
	 */
	virtual void
	handle_write_event(rofl::cthread& thread, int fd);

	virtual void
	handle_wakeup(rofl::cthread& thread)
	{
		tx();
	}

	/**
	 * @brief	reschedule opening of port in case of failure
	 */
	virtual void
	handle_timeout(rofl::cthread& thread, uint32_t timer_id,
			const std::list<unsigned int>& ttypes);

public:

	class ctapdev_find_by_devname {
		rofl::cdptid dptid;
		std::string devname;
	public:
		ctapdev_find_by_devname(const rofl::cdptid& dptid, const std::string& devname) :
			dptid(dptid), devname(devname) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dptid == dptid) && (p.second->devname == devname));
		};
	};

	class ctapdev_find_by_hwaddr {
		rofl::cdptid dptid;
		rofl::caddress_ll hwaddr;
	public:
		ctapdev_find_by_hwaddr(const rofl::cdptid& dptid, const rofl::caddress_ll& hwaddr) :
			dptid(dptid), hwaddr(hwaddr) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dptid == dptid) && (p.second->hwaddr == hwaddr));
		};
	};

	class ctapdev_find_by_pvid {
		rofl::cdptid dptid;
		uint16_t pvid;
	public:
		ctapdev_find_by_pvid(const rofl::cdptid& dptid, uint16_t pvid) :
			dptid(dptid), pvid(pvid) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dptid == dptid) && (p.second->pvid == pvid));
		};
	};

};

}; // end of namespace rofcore

#endif /* CTAPDEV_H_ */
