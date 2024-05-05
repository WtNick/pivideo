
#include <span>
#include <algorithm>
#include <iostream>
#include <memory>
#include <queue>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>

#include <functional>

#include <linux/media.h>
#include <linux/videodev2.h>

#include "bcm2835-isp.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

class UDPSender
{
	public:
		UDPSender(std::string hostname, int portnr)
		{
			struct addrinfo hint = {};
    		hint.ai_family = AF_INET;
    		hint.ai_socktype = SOCK_DGRAM;
    		hint.ai_protocol = IPPROTO_UDP;

			struct addrinfo *addrs = NULL;
			
			int ret = getaddrinfo(hostname.c_str(), NULL, &hint, &addrs);

			if (ret){
				std::cout << "could not resolve host name" << std::endl;
			} else {
				server_addr = *addrs->ai_addr;
				((sockaddr_in*)&server_addr)->sin_port = htons(portnr);
				freeaddrinfo(addrs);

				socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			}
		}

		void Write(const void* mem ,size_t len)
		{
			//std::cout << "dgram size: "<<len<<std::endl;

			int maxpacket = 1500;//16*1024;

			int bytesleft = len;
			uint8_t* buf = (uint8_t*)mem;
			while(bytesleft){
				size_t chunk = bytesleft;
				if (chunk>maxpacket){
					chunk = maxpacket;
				}
				int ret = sendto(socketfd, buf, chunk, 0, &server_addr, sizeof(server_addr));
				if (ret<0){
					std::cout << "failed : "<<ret<<std::endl;
				}

				bytesleft -= chunk;
				buf+=chunk;
			}
		}
	private:
		//struct sockaddr_in server_addr;
		struct sockaddr server_addr;
		int socketfd;
};

std::unique_ptr<UDPSender> udpsender;

#define tcase(a) \
	case a:      \
		return #a


struct line
{
	const int count;const char ch;
	line(int count, char ch = '*'):count(count),ch(ch){}
};

static std::ostream& operator<<(std::ostream& os, const line& v)
{
	for(int i = 0;i<v.count;i++){
		os << v.ch;
	}
	return os;
}	

struct V4L2Capability final : v4l2_capability {
	const char *driver() const
	{
		return reinterpret_cast<const char *>(v4l2_capability::driver);
	}
	const char *card() const
	{
		return reinterpret_cast<const char *>(v4l2_capability::card);
	}
	const char *bus_info() const
	{
		return reinterpret_cast<const char *>(v4l2_capability::bus_info);
	}
	unsigned int device_caps() const
	{
		return capabilities & V4L2_CAP_DEVICE_CAPS
				    ? v4l2_capability::device_caps
				    : v4l2_capability::capabilities;
	}
	bool isMultiplanar() const
	{
		return device_caps() & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
					V4L2_CAP_VIDEO_OUTPUT_MPLANE |
					V4L2_CAP_VIDEO_M2M_MPLANE);
	}
	bool isCapture() const
	{
		return device_caps() & (V4L2_CAP_VIDEO_CAPTURE |
					V4L2_CAP_VIDEO_CAPTURE_MPLANE |
					V4L2_CAP_META_CAPTURE);
	}
	bool isOutput() const
	{
		return device_caps() & (V4L2_CAP_VIDEO_OUTPUT |
					V4L2_CAP_VIDEO_OUTPUT_MPLANE |
					V4L2_CAP_META_OUTPUT);
	}
	bool isVideo() const
	{
		return device_caps() & (V4L2_CAP_VIDEO_CAPTURE |
					V4L2_CAP_VIDEO_CAPTURE_MPLANE |
					V4L2_CAP_VIDEO_OUTPUT |
					V4L2_CAP_VIDEO_OUTPUT_MPLANE);
	}
	bool isM2M() const
	{
		return device_caps() & (V4L2_CAP_VIDEO_M2M |
					V4L2_CAP_VIDEO_M2M_MPLANE);
	}
	bool isMeta() const
	{
		return device_caps() & (V4L2_CAP_META_CAPTURE |
					V4L2_CAP_META_OUTPUT);
	}
	bool isVideoCapture() const
	{
		return isVideo() && isCapture();
	}
	bool isVideoOutput() const
	{
		return isVideo() && isOutput();
	}
	bool isMetaCapture() const
	{
		return isMeta() && isCapture();
	}
	bool isMetaOutput() const
	{
		return isMeta() && isOutput();
	}
	bool hasStreaming() const
	{
		return device_caps() & V4L2_CAP_STREAMING;
	}
	bool hasMediaController() const
	{
		return device_caps() & V4L2_CAP_IO_MC;
	}
};

static std::string getctrltype(int type)
{
	switch(type)
	{
		tcase (V4L2_CTRL_TYPE_INTEGER);
		tcase (V4L2_CTRL_TYPE_BOOLEAN);
		tcase (V4L2_CTRL_TYPE_MENU);
		tcase (V4L2_CTRL_TYPE_BUTTON);
		tcase (V4L2_CTRL_TYPE_INTEGER64);
		tcase (V4L2_CTRL_TYPE_CTRL_CLASS);
		tcase (V4L2_CTRL_TYPE_STRING);
		tcase (V4L2_CTRL_TYPE_BITMASK);
		tcase (V4L2_CTRL_TYPE_INTEGER_MENU);
		tcase (V4L2_CTRL_TYPE_U8);
		tcase (V4L2_CTRL_TYPE_U16);
		tcase (V4L2_CTRL_TYPE_U32);
//		tcase (V4L2_CTRL_TYPE_AREA);
//		tcase (V4L2_CTRL_TYPE_HDR10_CLL_INFO);
//		tcase (V4L2_CTRL_TYPE_HDR10_MASTERING_DISPLAY);
//		tcase (V4L2_CTRL_TYPE_H264_SPS);
//		tcase (V4L2_CTRL_TYPE_H264_PPS);
//		tcase (V4L2_CTRL_TYPE_H264_SCALING_MATRIX);
//		tcase (V4L2_CTRL_TYPE_H264_SLICE_PARAMS);
//		tcase (V4L2_CTRL_TYPE_H264_DECODE_PARAMS);
//		tcase (V4L2_CTRL_TYPE_H264_PRED_WEIGHTS);
//		tcase (V4L2_CTRL_TYPE_FWHT_PARAMS);
//		tcase (V4L2_CTRL_TYPE_VP8_FRAME);
//		tcase (V4L2_CTRL_TYPE_MPEG2_QUANTISATION);
//		tcase (V4L2_CTRL_TYPE_MPEG2_SEQUENCE);
//		tcase (V4L2_CTRL_TYPE_MPEG2_PICTURE);
//		tcase (V4L2_CTRL_TYPE_VP9_COMPRESSED_HDR);
//		tcase (V4L2_CTRL_TYPE_VP9_FRAME);
		default:
			return "???";
	}
}

static int xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static std::string getfunction(int fn)
{
	switch (fn)
	{
		tcase(MEDIA_ENT_F_UNKNOWN);
		tcase(MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN);
		tcase(MEDIA_ENT_F_DTV_DEMOD);
		tcase(MEDIA_ENT_F_TS_DEMUX);
		tcase(MEDIA_ENT_F_DTV_CA);
		tcase(MEDIA_ENT_F_DTV_NET_DECAP);
		tcase(MEDIA_ENT_F_IO_V4L);
		tcase(MEDIA_ENT_F_IO_DTV);
		tcase(MEDIA_ENT_F_IO_VBI);
		tcase(MEDIA_ENT_F_IO_SWRADIO);
		tcase(MEDIA_ENT_F_CAM_SENSOR);
		tcase(MEDIA_ENT_F_FLASH);
		tcase(MEDIA_ENT_F_LENS);
		tcase(MEDIA_ENT_F_TUNER);
		tcase(MEDIA_ENT_F_IF_VID_DECODER);
		tcase(MEDIA_ENT_F_IF_AUD_DECODER);
		tcase(MEDIA_ENT_F_AUDIO_CAPTURE);
		tcase(MEDIA_ENT_F_AUDIO_PLAYBACK);
		tcase(MEDIA_ENT_F_AUDIO_MIXER);
		tcase(MEDIA_ENT_F_PROC_VIDEO_COMPOSER);
		tcase(MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER);
		tcase(MEDIA_ENT_F_PROC_VIDEO_PIXEL_ENC_CONV);
		tcase(MEDIA_ENT_F_PROC_VIDEO_LUT);
		tcase(MEDIA_ENT_F_PROC_VIDEO_SCALER);
		tcase(MEDIA_ENT_F_PROC_VIDEO_STATISTICS);
		tcase(MEDIA_ENT_F_PROC_VIDEO_ENCODER);
		tcase(MEDIA_ENT_F_PROC_VIDEO_DECODER);
		tcase(MEDIA_ENT_F_VID_MUX);
		tcase(MEDIA_ENT_F_VID_IF_BRIDGE);
		tcase(MEDIA_ENT_F_ATV_DECODER);
		tcase(MEDIA_ENT_F_DV_DECODER);
		tcase(MEDIA_ENT_F_DV_ENCODER);
	default:
		return "unknown";
	}
}
static std::string entitiyflags(int flags)
{
	std::string result;
	if (flags & MEDIA_ENT_FL_DEFAULT)
		result += "default ";
	if (flags & MEDIA_ENT_FL_CONNECTOR)
		result += "connector ";
	return result;
}

std::string getdevice(int major, int minor)
{
	std::ifstream ueventFile("/sys/dev/char/" + std::to_string(major) + ":" + std::to_string(minor) + "/uevent");

	// ueventFile.open();
	if (!ueventFile)
		return std::string();

	std::string line;
	while (ueventFile >> line)
	{
		if (line.find("DEVNAME=") == 0)
		{
			auto str = line.substr(8);
			return str;
		}
	}
	return "<unknown>";
}

static bool populateEntities(const struct media_v2_topology &topology)
{
	auto mediaEntities = (media_v2_entity *)topology.ptr_entities;
	// struct media_v2_entity *mediaEntities = reinterpret_cast<struct media_v2_entity *>(topology.ptr_entities);

	for (unsigned int i = 0; i < topology.num_entities; ++i)
	{
		struct media_v2_entity *ent = &mediaEntities[i];

		std::cout << "\t\tentity[" << ent->id << "] " << ent->name << " " << getfunction(ent->function) << " " << entitiyflags(ent->flags) << std::endl;
		/*
		 * Find the interface linked to this entity to get the device
		 * node major and minor numbers.
		 */
		// struct media_v2_interface *iface = findInterface(topology, ent->id);
		// MediaEntity *entity = new MediaEntity(this, ent, iface);

		// if (!addObject(entity)) {
		// delete entity;
		// return false;
		// }

		// entities_.push_back(entity);
	}

	return true;
}

static std::string padflags(int padflag)
{
	std::string result = "";
	if (padflag & MEDIA_PAD_FL_SINK)
	{
		result += "sink ";
	}
	if (padflag & MEDIA_PAD_FL_SOURCE)
	{
		result += "source ";
	}
	if (padflag & MEDIA_PAD_FL_MUST_CONNECT)
	{
		result += "mustconnect ";
	}
	return result;
}

static std::string getinterfacetype(int intf_type)
{
	switch (intf_type)
	{
		tcase(MEDIA_INTF_T_DVB_FE);
		tcase(MEDIA_INTF_T_DVB_DEMUX);
		tcase(MEDIA_INTF_T_DVB_DVR);
		tcase(MEDIA_INTF_T_DVB_CA);
		tcase(MEDIA_INTF_T_DVB_NET);
		tcase(MEDIA_INTF_T_V4L_VIDEO);
		tcase(MEDIA_INTF_T_V4L_VBI);
		tcase(MEDIA_INTF_T_V4L_RADIO);
		tcase(MEDIA_INTF_T_V4L_SUBDEV);
		tcase(MEDIA_INTF_T_V4L_SWRADIO);
		tcase(MEDIA_INTF_T_V4L_TOUCH);
		tcase(MEDIA_INTF_T_ALSA_PCM_CAPTURE);
		tcase(MEDIA_INTF_T_ALSA_PCM_PLAYBACK);
		tcase(MEDIA_INTF_T_ALSA_CONTROL);
	default:
		return "???";
	}
}

static int populateInterfaces(const struct media_v2_topology &topology)
{
	auto mediaif = (media_v2_interface *)topology.ptr_interfaces;

	for (int i = 0; i < topology.num_interfaces; i++)
	{
		auto mi = &mediaif[i];
		std::cout << "\t\tinterface: /dev/" << getdevice(mi->devnode.major, mi->devnode.minor) << " " << getinterfacetype(mi->intf_type) << " flags:" << mi->flags << std::endl;
	}
	return 1;
}

static int populatePads(const struct media_v2_topology &topology)
{
	struct media_v2_pad *mediaPads = (media_v2_pad *)topology.ptr_pads;

	for (unsigned int i = 0; i < topology.num_pads; ++i)
	{
		unsigned int entity_id = mediaPads[i].entity_id;
		std::cout << "\t\tpad[index:" << mediaPads[i].index << "]"
				  << " id:" << mediaPads[i].id
				  << " entity_id:" << mediaPads[i].entity_id
				  << " [" << padflags(mediaPads[i].flags) << "]"
				  << std::endl;

	
		/* Store a reference to this MediaPad in entity. */
		// MediaEntity *mediaEntity = dynamic_cast<MediaEntity *>
		// (object(entity_id));
		// if (!mediaEntity) {
		// LOG(MediaDevice, Error)
		// << "Failed to find entity with id: "
		// << entity_id;
		// return false;
		// }

		// MediaPad *pad = new MediaPad(&mediaPads[i], mediaEntity);
		// if (!addObject(pad)) {
		// delete pad;
		// return false;
		// }

		// mediaEntity->addPad(pad);
	}

	return true;
}

static int populateLinks(const struct media_v2_topology &topology)
{
	struct media_v2_link *mediaLinks = reinterpret_cast<struct media_v2_link *>(topology.ptr_links);

	for (unsigned int i = 0; i < topology.num_links; ++i)
	{

		auto ml = &mediaLinks[i];
		std::cout << "medialink[" << ml->id << "]"
				  << " flags:" << ml->flags
				  << " source_id:"
				  << ml->source_id
				  << " sink_id:" << ml->sink_id
				  << std::endl;

		if ((mediaLinks[i].flags & MEDIA_LNK_FL_LINK_TYPE) ==
			MEDIA_LNK_FL_INTERFACE_LINK)
			continue;

		/* Look up the source and sink objects. */
		// unsigned int source_id = mediaLinks[i].source_id;
		// MediaObject *source = object(source_id);
		// if (!source) {
		// LOG(MediaDevice, Error)
		// << "Failed to find MediaObject with id "
		// << source_id;
		// return false;
		// }

		// unsigned int sink_id = mediaLinks[i].sink_id;
		// MediaObject *sink = object(sink_id);
		// if (!sink) {
		// LOG(MediaDevice, Error)
		// << "Failed to find MediaObject with id "
		// << sink_id;
		// return false;
		// }

		// switch (mediaLinks[i].flags & MEDIA_LNK_FL_LINK_TYPE) {
		// case MEDIA_LNK_FL_DATA_LINK: {
		// MediaPad *sourcePad = dynamic_cast<MediaPad *>(source);
		// MediaPad *sinkPad = dynamic_cast<MediaPad *>(sink);
		// if (!source || !sink) {
		// LOG(MediaDevice, Error)
		// << "Source or sink is not a pad";
		// return false;
		// }

		// MediaLink *link = new MediaLink(&mediaLinks[i],
		// sourcePad, sinkPad);
		// if (!addObject(link)) {
		// delete link;
		// return false;
		// }

		// link->source()->addLink(link);
		// link->sink()->addLink(link);

		// break;
		// }

		// case MEDIA_LNK_FL_ANCILLARY_LINK: {
		// MediaEntity *primary = dynamic_cast<MediaEntity *>(source);
		// MediaEntity *ancillary = dynamic_cast<MediaEntity *>(sink);
		// if (!primary || !ancillary) {
		// LOG(MediaDevice, Error)
		// << "Source or sink is not an entity";
		// return false;
		// }

		// primary->addAncillaryEntity(ancillary);

		// break;
		// }

		// default:
		// LOG(MediaDevice, Warning)
		// << "Unknown media link type";

		// break;
		// }
	}

	return true;
}

static int populate(std::string name)
{

	std::cout << "populate " << name << "." << std::endl;

	bool valid_ = false;

	struct media_v2_topology topology = {};
	struct media_v2_entity *ents = nullptr;
	struct media_v2_interface *interfaces = nullptr;
	struct media_v2_link *links = nullptr;
	struct media_v2_pad *pads = nullptr;
	__u64 version = -1;
	int ret;

	// clear();

	int fd = open(name.c_str(), O_RDWR);
	if (fd <= 0)
		return fd;

	struct media_device_info info = {};
	ret = xioctl(fd, MEDIA_IOC_DEVICE_INFO, &info);
	if (ret)
	{
		ret = -errno;
		std::cout << "Failed to get media device info " << strerror(-ret);
		goto done;
	}

	std::cout << "\tbus : " << info.bus_info << std::endl;
	std::cout << "\tdriver : " << info.driver << std::endl;
	std::cout << "\tdriver version: " << info.driver_version << std::endl;
	std::cout << "\thw_revision : " << info.hw_revision << std::endl;
	std::cout << "\tmedia_version : " << info.media_version << std::endl;
	std::cout << "\tmodel : " << info.model << std::endl;
	std::cout << "\tserial : " << info.serial << std::endl;

	// auto driver_ = info.driver;
	// auto model_ = info.model;
	// auto version_ = info.media_version;
	// auto hwRevision_ = info.hw_revision;

	/*
	 * Keep calling G_TOPOLOGY until the version number stays stable.
	 */
	while (true)
	{
		topology.topology_version = 0;
		topology.ptr_entities = reinterpret_cast<uintptr_t>(ents);
		topology.ptr_interfaces = reinterpret_cast<uintptr_t>(interfaces);
		topology.ptr_links = reinterpret_cast<uintptr_t>(links);
		topology.ptr_pads = reinterpret_cast<uintptr_t>(pads);

		ret = xioctl(fd, MEDIA_IOC_G_TOPOLOGY, &topology);
		if (ret < 0)
		{
			ret = -errno;
			std::cout
				<< "Failed to enumerate topology: "
				<< strerror(-ret);
			goto done;
		}

		if (version == topology.topology_version)
			break;

		delete[] ents;
		delete[] interfaces;
		delete[] pads;
		delete[] links;

		ents = new struct media_v2_entity[topology.num_entities]();
		interfaces = new struct media_v2_interface[topology.num_interfaces]();
		links = new struct media_v2_link[topology.num_links]();
		pads = new struct media_v2_pad[topology.num_pads]();

		version = topology.topology_version;
	}

	/* Populate entities, pads and links. */
	if (populateEntities(topology) &&
		populatePads(topology) &&
		populateInterfaces(topology) &&
		populateLinks(topology))
	{
		valid_ = true;
	}

	ret = 0;
done:
	// close();

	delete[] ents;
	delete[] interfaces;
	delete[] pads;
	delete[] links;

	return ret;
}

static int enumerate()
{
	struct dirent *ent;
	DIR *dir;

	static const char *const sysfs_dirs[] = {
		"/sys/subsystem/media/devices",
		"/sys/bus/media/devices",
		"/sys/class/media/devices",
	};

	for (const char *dirname : sysfs_dirs)
	{
		dir = opendir(dirname);
		if (dir)
			break;
	}

	if (!dir)
	{
		std::cout << "No valid sysfs media device directory";
		return -ENODEV;
	}

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strncmp(ent->d_name, "media", 5))
			continue;

		char *end;
		unsigned int idx = strtoul(ent->d_name + 5, &end, 10);
		if (*end != '\0')
			continue;

		std::string devnode = "/dev/media" + std::to_string(idx);

		/* Verify that the device node exists. */
		struct stat devstat;
		if (stat(devnode.c_str(), &devstat) < 0)
		{
			std::cout << "Device node /dev/media" << idx << " should exist but doesn't";
			continue;
		}

		std::cout << "device found:" << devnode << std::endl;
		populate(devnode);
	}

	closedir(dir);

	return 0;
}

#define tcm(c, ident)                   \
	do                                  \
	{                                   \
		if (c & ident)                  \
		{                               \
			std::cout << #ident << " "; \
		}                               \
	} while (0)

void decode(uint32_t caps)
{
	tcm(caps, V4L2_CAP_VIDEO_CAPTURE);
	tcm(caps, V4L2_CAP_VIDEO_OUTPUT);
	tcm(caps, V4L2_CAP_VIDEO_OVERLAY);
	tcm(caps, V4L2_CAP_VBI_CAPTURE);
	tcm(caps, V4L2_CAP_VBI_OUTPUT);
	tcm(caps, V4L2_CAP_SLICED_VBI_CAPTURE);
	tcm(caps, V4L2_CAP_SLICED_VBI_OUTPUT);
	tcm(caps, V4L2_CAP_RDS_CAPTURE);
	tcm(caps, V4L2_CAP_VIDEO_OUTPUT_OVERLAY);
	tcm(caps, V4L2_CAP_HW_FREQ_SEEK);
	tcm(caps, V4L2_CAP_RDS_OUTPUT);
	tcm(caps, V4L2_CAP_VIDEO_CAPTURE_MPLANE);
	tcm(caps, V4L2_CAP_VIDEO_OUTPUT_MPLANE);
	tcm(caps, V4L2_CAP_VIDEO_M2M_MPLANE);
	tcm(caps, V4L2_CAP_VIDEO_M2M);
	tcm(caps, V4L2_CAP_TUNER);
	tcm(caps, V4L2_CAP_AUDIO);
	tcm(caps, V4L2_CAP_RADIO);
	tcm(caps, V4L2_CAP_MODULATOR);
	tcm(caps, V4L2_CAP_SDR_CAPTURE);
	tcm(caps, V4L2_CAP_EXT_PIX_FORMAT);
	tcm(caps, V4L2_CAP_SDR_OUTPUT);
	tcm(caps, V4L2_CAP_META_CAPTURE);
	tcm(caps, V4L2_CAP_READWRITE);
	tcm(caps, V4L2_CAP_ASYNCIO);
	tcm(caps, V4L2_CAP_STREAMING);
	tcm(caps, V4L2_CAP_META_OUTPUT);
	tcm(caps, V4L2_CAP_TOUCH);
	tcm(caps, V4L2_CAP_IO_MC);
	tcm(caps, V4L2_CAP_DEVICE_CAPS);
}

struct slice
{
	const int start;
	const int stop;
	const int step;
	slice(int start, int stop, int step=1)
	:start(start),stop(stop),step(step){
	}
};

std::ostream& operator<<(std::ostream& os, const slice& v)
{
	if (v.step!=1){
    	os << '[' << v.start << ":" << v.stop << ":" << v.step << ']';
	} else {
		os << '[' << v.start << ":" << v.stop << ']';
	}
    return os;
}

static std::ostream& operator<<(std::ostream& os, const v4l2_colorspace& v)
{
	switch(v){
		case V4L2_COLORSPACE_DEFAULT       : os << "V4L2_COLORSPACE_DEFAULT";break;
		case V4L2_COLORSPACE_SMPTE170M     : os << "V4L2_COLORSPACE_SMPTE170M";break;
		case V4L2_COLORSPACE_SMPTE240M     : os << "V4L2_COLORSPACE_SMPTE240M";break;
		case V4L2_COLORSPACE_REC709        : os << "V4L2_COLORSPACE_REC709";break;
		case V4L2_COLORSPACE_BT878         : os << "V4L2_COLORSPACE_BT878";break;
		case V4L2_COLORSPACE_470_SYSTEM_M  : os << "V4L2_COLORSPACE_470_SYSTEM_M";break;
		case V4L2_COLORSPACE_470_SYSTEM_BG : os << "V4L2_COLORSPACE_470_SYSTEM_BG";break;
		case V4L2_COLORSPACE_JPEG          : os << "V4L2_COLORSPACE_JPEG";break;
		case V4L2_COLORSPACE_SRGB          : os << "V4L2_COLORSPACE_SRGB";break;
		case V4L2_COLORSPACE_OPRGB         : os << "V4L2_COLORSPACE_OPRGB";break;
		case V4L2_COLORSPACE_BT2020        : os << "V4L2_COLORSPACE_BT2020";break;
		case V4L2_COLORSPACE_RAW           : os << "V4L2_COLORSPACE_RAW";break;
		case V4L2_COLORSPACE_DCI_P3        : os << "V4L2_COLORSPACE_DCI_P3";break;
		default:
			break;
	}
    return os;
}

void read_VIDIOC_ENUM_FRAMEINTERVALS(int fd, unsigned int pixelformat, unsigned int framewidth, unsigned int frameheight)
{
	struct v4l2_frmivalenum data;
	memset(&data,0,sizeof(data));
	data.type = V4L2_FRMIVAL_TYPE_DISCRETE;
	data.pixel_format = pixelformat;
	data.width = framewidth;
	data.height = frameheight;

	while (xioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS, &data) == 0)
	{    
		std::cout << "\t\tframeinterval: " << data.discrete.numerator << "/" << data.discrete.denominator<<std::endl;
		data.index++;
	}
}

void read_VIDIOC_ENUM_FRAMESIZES(int fd, unsigned int pixelformat)
{
	struct v4l2_frmsizeenum data;
	memset(&data,0,sizeof(data));
	data.type = V4L2_FRMSIZE_TYPE_DISCRETE;
	data.pixel_format = pixelformat;
	
	while (xioctl(fd,VIDIOC_ENUM_FRAMESIZES, &data) == 0)
    {   
		switch(data.type){
			case V4L2_FRMSIZE_TYPE_DISCRETE:
			std::cout << "\tframe size : "<< data.type<< " " << data.discrete.width << " x " <<data.discrete.height<< std::endl;
			read_VIDIOC_ENUM_FRAMEINTERVALS(fd, pixelformat, data.discrete.width, data.discrete.height);
			break;
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			case V4L2_FRMSIZE_TYPE_STEPWISE:
			std::cout << "\tframe size : " << slice(data.stepwise.min_width,data.stepwise.max_width,data.stepwise.step_width)  << " x " << slice(data.stepwise.min_height,data.stepwise.max_height,data.stepwise.step_height) << std::endl;
			break;
		}
		data.pixel_format = pixelformat;
		data.type = V4L2_FRMSIZE_TYPE_DISCRETE;
		data.index++;
	}
}

struct v4l2_pixelformat
{
	uint32_t fourcc;
	v4l2_pixelformat(v4l2_pixelformat &v) = default;
	v4l2_pixelformat& operator=(const v4l2_pixelformat& other) = default;
	v4l2_pixelformat(uint32_t fourcc):fourcc(fourcc){}
};

static std::ostream& operator<<(std::ostream& os, const v4l2_pixelformat& v)
{
	char* t = (char*)&v;
	os << "'" << t[0]<<t[1]<<t[2]<<t[3]<<"'";
	return os;
}

void read_VIDIOC_ENUM_FMT(int fd)
{
		struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc,0,sizeof(fmtdesc));
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (xioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) == 0)
        {    
			std::cout << v4l2_pixelformat(fmtdesc.pixelformat)<< "  "<< fmtdesc.description <<std::endl;
            
			read_VIDIOC_ENUM_FRAMESIZES(fd, fmtdesc.pixelformat);
			fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            fmtdesc.index++;
        }
}


void querycaps(std::string device)
{
	int fd = open(device.c_str(), O_RDWR);
	if (fd <= 0)
	{
		std::cout << "unknown device" << std::endl;
		return;
	}

	struct v4l2_capability cap;

	int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0)
	{
		std::cout << "ioctl error: " << ret << std::endl;
		return;
	}

	decode(cap.device_caps);
	std::cout << std::endl;

	//read_VIDIOC_G_FMT(fd);
	//read_VIDIOC_ENUM_FMT(fd);

	close(fd);
}

//V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE

#include <sys/mman.h>
#include <map>
#include <vector>

std::map<int, void*> mappeddata;

int query_mmapbuffer(int fd, int index)
{
	struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    int res = xioctl(fd, VIDIOC_QUERYBUF, &buf);
	
    if(res == -1) {
        perror("Could not query buffer");
        return 2;
    }
    auto bufferptr = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
	mappeddata[buf.index] = bufferptr;

    return buf.length;
}


#include <linux/dma-buf.h>
#include <linux/dma-heap.h>

struct dmabuf
{
		int fd;
		size_t length;
		size_t bytesused;
};

void alloc_dma(size_t bufsize, dmabuf* bufinfo)
{
	int fd = open("/dev/dma_heap/linux,cma", O_RDWR, 0);
	if ( fd<0){
		perror("failed to open dma heap.");
		exit(1);
		return;
	}

	// alloc
	struct dma_heap_allocation_data alloc = {};

		alloc.len = bufsize;
		alloc.fd_flags = O_CLOEXEC | O_RDWR;

		int ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &alloc);
		if (ret < 0) {
			perror("dmaHeap allocation failure for ");
			exit(1);
			return;
		}
	
	auto bufferptr = mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_SHARED, alloc.fd, 0);
	mappeddata[alloc.fd] = bufferptr;

	close(fd);
	bufinfo->fd = alloc.fd;
	bufinfo->length = bufsize;
	bufinfo->bytesused = 0;
}

class vdev
{
	int _fd;
	std::string name;
	public:
	const int& fd() const {return _fd;}

	class stream
	{
		
		const int buf_type;
		int mem_type;
		std::queue<int> freebufs;
		std::map<int, void*> mmapbuffers;

		public:
		vdev* dev;
		stream(vdev* dev, int buf_type)
				:dev(dev)
				,buf_type(buf_type)
				,pixelformat(0)

		{}

		void Reset()
		{
			//request_buffer(0, mem_type);
		}

		void* query_buffer_mmap(int index)
		{
			v4l2_plane planes[VIDEO_MAX_PLANES];
			v4l2_buffer buffer = {};
			buffer.type = buf_type;
			buffer.memory = V4L2_MEMORY_MMAP;
			buffer.index = index;
			buffer.length = 1;
			buffer.m.planes = planes;
			if (dev->cioctl(VIDIOC_QUERYBUF, &buffer) < 0) {
				throw std::runtime_error("failed to capture query buffer " + std::to_string(index));
			}
			
			auto bufferptr = mmap (NULL, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd(), buffer.m.planes[0].m.mem_offset);
			if (bufferptr == MAP_FAILED){
				throw std::runtime_error("failed to mmap capture buffer " + std::to_string(index));
			}

			// Whilst we're going through all the capture buffers, we may as well queue
			// them ready for the encoder to write into.
			if (dev->cioctl(VIDIOC_QBUF, &buffer) < 0) {
				throw std::runtime_error("failed to queue capture buffer " + std::to_string(index));
			}
			return bufferptr;
		}

		/// @brief Fill the stream queue with dma buffers
		void FillQueueDMA()
		{
			size_t bufsize = getformat();
			while(!freebufs.empty()){
				dmabuf bufinfo;
				alloc_dma(bufsize, &bufinfo);
				queuedma(bufinfo);
			}
		}

		void FillQueueMMAP()
		{
			while(!freebufs.empty()){

				v4l2_plane planes[VIDEO_MAX_PLANES];
				v4l2_buffer buffer = {};
				buffer.type = buf_type;
				buffer.memory = V4L2_MEMORY_MMAP;
				buffer.index = freebufs.front();
				freebufs.pop();
				buffer.length = 1;
				buffer.m.planes = planes;
				if (dev->cioctl(VIDIOC_QUERYBUF, &buffer) < 0) {
					throw std::runtime_error("failed to capture query buffer " + std::to_string(buffer.index));
				}

				auto bufferptr = mmap (NULL, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd(), buffer.m.planes[0].m.mem_offset);
				if (bufferptr == MAP_FAILED){
					throw std::runtime_error("failed to mmap capture buffer " + std::to_string(buffer.index));
				}
				mmapbuffers[buffer.index] = bufferptr;

				// Whilst we're going through all the capture buffers, we may as well queue
				// them ready for the encoder to write into.
				if (dev->cioctl(VIDIOC_QBUF, &buffer) < 0) {
					throw std::runtime_error("failed to queue capture buffer " + std::to_string(buffer.index));
				}
			}
		}

		int RequestBuffers(int count, int mem_type)
		{
			this->mem_type = mem_type;
			struct v4l2_requestbuffers req = {0};
    		req.count = count;
    		req.type = buf_type;
    		req.memory = mem_type;
    		dev->ioctl(VIDIOC_REQBUFS, &req);

			for(int i = 0;i<req.count;i++){
				freebufs.push(i);
			}
			
			return req.count;
		}

		void start_streaming()
		{
			//std::cout << "start streaming" << std::endl;
			int type = buf_type;
			dev->ioctl(VIDIOC_STREAMON, &type);
    	}

		void stop_streaming()
		{
			//std::cout << "stop  streaming" << std::endl;
			int type = buf_type;
			dev->ioctl(VIDIOC_STREAMOFF, &type);
    	}

		void queuedma(dmabuf& b)
		{
			if (freebufs.empty()){
				std::cout << "freebufs empty "<< dev->name << " "<< buf_type << std::endl;
				return;
			}

			v4l2_buffer vb = {};
			vb.index = freebufs.front();
			freebufs.pop();
			vb.type = buf_type;
			vb.memory = V4L2_MEMORY_DMABUF;
			vb.m.fd = b.fd;
			vb.bytesused = b.length;
			vb.length = b.length;
			dev->ioctl(VIDIOC_QBUF, &vb);
		}

		void queuedma_plane(dmabuf& b)
		{
			if (freebufs.empty()){
				std::cout << "freebufs empty "<< dev->name << " "<< buf_type << std::endl;
				return;
			}

			v4l2_buffer vb = {};
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			
			vb.index = freebufs.front();
			freebufs.pop();
			vb.type = buf_type; // buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			vb.memory = V4L2_MEMORY_DMABUF;

			vb.field = V4L2_FIELD_NONE;	
			vb.length = 1;
			vb.timestamp.tv_sec = 0 / 1000000;
			vb.timestamp.tv_usec = 0 % 1000000;
			vb.m.planes = planes;
			vb.m.planes[0].m.fd = b.fd;
			vb.m.planes[0].bytesused = b.bytesused;
			vb.m.planes[0].length = b.length;
			//std::cout << "encoder output queued "<< vb.index <<", bytesused: " << b.bytesused <<" length "<< b.length<<std::endl;
			dev->ioctl(VIDIOC_QBUF, &vb);
		}

		void dequeuedma_plane(dmabuf* b)
		{
			v4l2_buffer buf = {};
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			buf.type = buf_type;
			buf.memory = V4L2_MEMORY_DMABUF;
			buf.length = 1;
			buf.m.planes = planes;
			dev->ioctl(VIDIOC_DQBUF, &buf);
			
			b->fd = buf.m.planes[0].m.fd;
			b->bytesused = buf.m.planes[0].bytesused;
			b->length = buf.m.planes[0].length;
			freebufs.push(buf.index);
		}

		bool trydequeuedma_plane(dmabuf* b)
		{
			v4l2_buffer buf = {};
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			buf.type = buf_type;
			buf.memory = V4L2_MEMORY_DMABUF;
			buf.length = 1;
			buf.m.planes = planes;
			if (dev->cioctl(VIDIOC_DQBUF, &buf))
			{
				b->fd = buf.m.planes[0].m.fd;
				b->bytesused = buf.m.planes[0].bytesused;
				b->length = buf.m.planes[0].length;
				freebufs.push(buf.index);
				//std::cout << "encoder output dequeued "<< buf.index <<" fd:" << b->fd << " , bytesused: " << b->bytesused <<" length "<< b->length<<std::endl;
				return true;
			}
			return false;
		}

		bool trydequeue_mmap_plane(int* bufindex, std::function<void(std::span<uint8_t> data)> ondata)
		{
			v4l2_buffer buf = {};
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			buf.type = buf_type;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.length = 1;
			buf.m.planes = planes;
			if (dev->cioctl(VIDIOC_DQBUF, &buf)) 
			{
				auto ptr = mmapbuffers[buf.index];
				auto len = buf.m.planes[0].bytesused;

					// if (buf.flags & V4L2_BUF_FLAG_MAPPED			) std::cout << " mapped";
					// if (buf.flags & V4L2_BUF_FLAG_QUEUED			) std::cout << " queued";
					// if (buf.flags & V4L2_BUF_FLAG_DONE			) std::cout << " done";
					// if (buf.flags & V4L2_BUF_FLAG_KEYFRAME			) std::cout << " keyframe";
					// if (buf.flags & V4L2_BUF_FLAG_PFRAME			) std::cout << " pframe";
					// if (buf.flags & V4L2_BUF_FLAG_BFRAME			) std::cout << " bframe";
					// if (buf.flags & V4L2_BUF_FLAG_ERROR			) std::cout << " error";
					// if (buf.flags & V4L2_BUF_FLAG_IN_REQUEST		) std::cout << " in_request";
					// if (buf.flags & V4L2_BUF_FLAG_TIMECODE			) std::cout << " timecode";
					// if (buf.flags & V4L2_BUF_FLAG_M2M_HOLD_CAPTURE_BUF	) std::cout << " M2M_hold_capture_buf";
					// if (buf.flags & V4L2_BUF_FLAG_PREPARED			) std::cout << " prepared";
					// if (buf.flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE	) std::cout << " no_cache_inv";
					// if (buf.flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN		) std::cout << " no_cache_clean";
					// if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK		) std::cout << " timestamp_mask";
					// if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN		) std::cout << " timestamp_unknown";
					// if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC	) std::cout << " timestamp_monotonic";
					// if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_COPY		) std::cout << " timestamp_copy";
					// if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK		) std::cout << " tstamp_src_mask";
					// if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_EOF		) std::cout << " tstamp_src_eof";
					// if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_SOE		) std::cout << " tstamp_src_soe";

				ondata({(uint8_t*)ptr, len});
				
				buf.m.planes[0].bytesused = buf.m.planes[0].length;

				// requeue
				dev->ioctl(VIDIOC_QBUF, &buf);
				return true;
			}
			return false;
		}

		void dequeuedma(dmabuf* b){
			v4l2_buffer vb;
			vb.type = buf_type;
    		vb.memory = V4L2_MEMORY_DMABUF;
    		dev->ioctl(VIDIOC_DQBUF, &vb);
			freebufs.push(vb.index);
			b->bytesused = vb.bytesused;
			b->length = vb.length;
			b->fd = vb.m.fd;
		}

		bool trydequeuedma(int events, dmabuf* b){
			if (dev->Poll(events)){
				dequeuedma(b);
				return true;
			}
			return false;
		}
		
		int width;
		int height;
		int bytesperline;
		v4l2_pixelformat pixelformat;

		void setformat(int width, int height, const v4l2_pixelformat& pixelformat)
		{
			struct v4l2_format fm;

			fm.type = buf_type;
			dev->ioctl(VIDIOC_G_FMT, &fm);

			// have v4l2 driver come up with best bytesperline/sizeimage 
			fm.fmt.pix.bytesperline = 0;
			fm.fmt.pix.sizeimage = 0;

			fm.fmt.pix.width = width;
			fm.fmt.pix.height = height;
			fm.fmt.pix.pixelformat = pixelformat.fourcc;

			std::cout << dev->name << " set format"<<std::endl;
			dev->ioctl(VIDIOC_S_FMT, &fm);
		
			this->bytesperline = fm.fmt.pix.bytesperline;
			this->width = fm.fmt.pix.width;
			this->height = fm.fmt.pix.height;
			this->pixelformat = pixelformat;
		}

		void setformat(const stream& stream){
			setformat(stream.width, stream.height, stream.pixelformat);
		}

		size_t getformat()
		{
			struct v4l2_format data = {0};
			data.type = buf_type;

        	dev->ioctl(VIDIOC_G_FMT, &data);
        
			auto colorspace = static_cast<v4l2_colorspace>(data.fmt.pix.colorspace);
			std::cout << dev->name << " v4l2_buf_type:" << data.type << ' ' << colorspace <<' ' << v4l2_pixelformat(data.fmt.pix.pixelformat) <<' ' << data.fmt.pix.width << "x"<<data.fmt.pix.height<< " size image:" << data.fmt.pix.sizeimage << std::endl;
			switch(data.type){
				case V4L2_BUF_TYPE_VIDEO_CAPTURE:
				case V4L2_BUF_TYPE_VIDEO_OUTPUT:
					return data.fmt.pix.sizeimage;
				case V4L2_BUF_TYPE_META_CAPTURE:
				case V4L2_BUF_TYPE_META_OUTPUT:
					return data.fmt.meta.buffersize;
			}	
			return 0;
		}

		void confirmformat()
		{
			struct v4l2_format fm = {0};

			fm.type = buf_type;
			dev->ioctl(VIDIOC_G_FMT, &fm);
			fm.type = buf_type;
			dev->ioctl(VIDIOC_S_FMT, &fm);
		}
	};
	
	vdev(std::string videonode)
		:_fd(open(videonode.c_str(), O_RDWR | O_NONBLOCK))
		,name(videonode)
	{
		if (_fd <= 0) {
			perror("unknown device");
			return;
		}
	}

	std::unique_ptr<stream> getstream(int buf_type){
		return std::make_unique<stream>(this, buf_type);
	}

	void ioctl(int request, void* arg)
	{
		int ret = xioctl(_fd, request, arg);
		if(ret < 0)
    	{	
			std::cerr << "ERROR  "<< name <<" ----------------- ioctl error : " <<ret<<std::endl;
        	perror("ioctl");
			exit(1);
        	return;
    	}
	}
	bool cioctl(int request, void* arg)
	{
		int ret = xioctl(_fd, request, arg);
		if(ret < 0)
    	{	
			//std::cout << "ioctl ret: "<<ret<<std::endl;
			return false;
    	}
		return true;
	}

	int Poll(int events = POLLIN | POLLOUT | POLLPRI)
	{
		pollfd pfd;
		pfd.fd = _fd;
		pfd.events = events;
		int pr = poll(&pfd, 1, 0);
		//std::cout << name << " poll ret: "<<pr<<", events: "<<pfd.revents<<std::endl;
		return pr;
	}

	void wait_done()
	{
    	fd_set fds;
    	FD_ZERO(&fds);
    	FD_SET(_fd, &fds);
    	struct timeval tv = {0};
    	tv.tv_sec = 2; //set timeout to 2 second
    	int r = select(_fd+1, &fds, NULL, NULL, &tv);
    	if(-1 == r) {
    	    perror("Error waiting for Frame");
    	    exit(1);
			return;
    	}
		if (r==0){
			std::cout << "timeout."<<std::endl;
			return;
		}
	}



	void listcontrols()
	{
		struct v4l2_query_ext_ctrl ctrl = {};
		
		while(true){
			ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
			if (!cioctl(VIDIOC_QUERY_EXT_CTRL, &ctrl)) {
				break;
			}
			
			if (ctrl.type == V4L2_CTRL_TYPE_CTRL_CLASS) 
			{	
				std::cout << ctrl.name ;
			}else{
				std::cout << "   "<< ctrl.name<<" " << getctrltype(ctrl.type) << " ";
				std:: cout << "min: "<<ctrl.minimum<<",max: "<<ctrl.maximum<<", step:" << ctrl.step << ", default:" << ctrl.default_value;

				if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED) std::cout<<" Disabled ";
				if (ctrl.flags & V4L2_CTRL_FLAG_GRABBED) std::cout<<" Grabbed ";
				if (ctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) std::cout<<" ReadOnly ";
				if (ctrl.flags & V4L2_CTRL_FLAG_UPDATE) std::cout<<" Update ";
				if (ctrl.flags & V4L2_CTRL_FLAG_INACTIVE) std::cout<<" Inactive ";
				if (ctrl.flags & V4L2_CTRL_FLAG_SLIDER) std::cout<<" Slider ";
				if (ctrl.flags & V4L2_CTRL_FLAG_WRITE_ONLY) std::cout<<" WriteOnly ";
				if (ctrl.flags & V4L2_CTRL_FLAG_VOLATILE) std::cout<<"Volatile ";
				if (ctrl.flags & V4L2_CTRL_FLAG_HAS_PAYLOAD) std::cout<<" Has Payload ";
				if (ctrl.flags & V4L2_CTRL_FLAG_EXECUTE_ON_WRITE) std::cout<<" ExecuteOnWrite ";
				if (ctrl.flags & V4L2_CTRL_FLAG_MODIFY_LAYOUT) std::cout<<" ModifyLayout ";

			
				
				std::vector<v4l2_ext_control> v4l2Ctrls;

				v4l2Ctrls.push_back({0});
				v4l2Ctrls[0].id = ctrl.id;

				struct v4l2_ext_controls ext;
				ext.which = V4L2_CTRL_WHICH_CUR_VAL;
				ext.controls = v4l2Ctrls.data();
				ext.count = 1;
				ioctl(VIDIOC_G_EXT_CTRLS, &ext);
				if (ext.error_idx){
					std::cout << "value error" << std::endl;
				}

				std::cout << " value = "<<v4l2Ctrls[0].value;
			}
			std::cout << std::endl;
		}
	}

	~vdev()
	{
		close(_fd);
	}
};

class h264encoder
	:public vdev
{
	public:
	vdev::stream output;
	vdev::stream capture;
		h264encoder(std::string videonode)
		:vdev(videonode)
		,output(this, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		,capture(this, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		{
		}

		void setbitrate(int bitrate)
		{
			v4l2_control ctrl = {};
			ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
			ctrl.value=bitrate;
			ioctl(VIDIOC_S_CTRL, &ctrl);
		}

		void init(const vdev::stream& stream)
		{
			init(stream.width,stream.height,stream.bytesperline);
		}
		void init(int width, int height, int inputstride_bytes)
		{

std::cout << "encoder init: "<<width<<"x"<<height<<std::endl;

			const int DEFAULT_FRAMERATE = 30;

			// Set the output and capture formats. We know exactly what they will be.
			v4l2_format fmt = {};
			fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			fmt.fmt.pix_mp.width = width;
			fmt.fmt.pix_mp.height = height;
			// We assume YUV420 here, but it would be nice if we could do something
			// like info.pixel_format.toV4L2Fourcc();
			fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
			fmt.fmt.pix_mp.plane_fmt[0].bytesperline = inputstride_bytes;
			fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
			//fmt.fmt.pix_mp.colorspace = get_v4l2_colorspace(info.colour_space);
			fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_REC709;//get_v4l2_colorspace(info.colour_space);
			fmt.fmt.pix_mp.num_planes = 1;
			if (cioctl(VIDIOC_S_FMT, &fmt) < 0)
				throw std::runtime_error("failed to set output format");

			fmt = {};
			fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			fmt.fmt.pix_mp.width = width;
			fmt.fmt.pix_mp.height = height;
			fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
			fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
			fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
			fmt.fmt.pix_mp.num_planes = 1;
			fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
			fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 512 << 10;
			if (cioctl(VIDIOC_S_FMT, &fmt) < 0)
				throw std::runtime_error("failed to set capture format");

			struct v4l2_streamparm parm = {};
			parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			parm.parm.output.timeperframe.numerator = 1000 / DEFAULT_FRAMERATE;
			parm.parm.output.timeperframe.denominator = 1000;
			if (cioctl(VIDIOC_S_PARM, &parm) < 0)
				throw std::runtime_error("failed to set streamparm");

		}	


		// int request_buffers(int count, int streamtype, int buftype){	
		// 	struct v4l2_requestbuffers req = {0};
    	// 	req.count = count;
    	// 	req.type = streamtype;
    	// 	req.memory = buftype;
    		
		// 	if (cioctl(VIDIOC_REQBUFS, &req)<0) {
		// 		throw std::runtime_error("failed to request buffers");
		// 	}

		// 	return req.count;
		// }

		void queueframe_output(int fd, int index, size_t bytesused, size_t length, const timeval& tv)
		{
			//std::cout << "queueframe_output: index: " <<index<< " size: "<<bytesused<<"/"<< length<<std::endl;
			v4l2_buffer buf = {};
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			buf.index = index;
			buf.field = V4L2_FIELD_NONE;
			buf.memory = V4L2_MEMORY_DMABUF;
			buf.length = 1;
			buf.timestamp = tv;
			buf.m.planes = planes;
			buf.m.planes[0].m.fd = fd;
			buf.m.planes[0].bytesused = bytesused;
			buf.m.planes[0].length = length;
			if (cioctl(VIDIOC_QBUF, &buf) < 0) {
				throw std::runtime_error("failed to queue input to codec");
			}
		}

		bool trydequeue(v4l2_buffer* b)
		{
			bool outputdequeued = false;
			if (Poll(POLLIN)){
				v4l2_buffer buf = {};
				v4l2_plane planes[VIDEO_MAX_PLANES] = {};
				buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
				buf.memory = V4L2_MEMORY_DMABUF;
				buf.length = 1;
				buf.m.planes = planes;
				if (cioctl(VIDIOC_DQBUF, &buf) == 0)
				{
					outputdequeued = true;
					b->m.fd = buf.m.planes[0].m.fd;
					b->bytesused = buf.m.planes[0].bytesused;
					b->length = buf.m.planes[0].length;
					std::cout << "encoder dequeued, buf length: " << b->bytesused <<std::endl;
				}

				buf = {};
				memset(planes, 0, sizeof(planes));
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.length = 1;
				buf.m.planes = planes;
				if (cioctl(VIDIOC_DQBUF, &buf) == 0) {

					auto ptr = nullptr;// capturebuffers[buf.index];
					auto len = buf.m.planes[0].bytesused;
					//std::cout << ptr<<std::endl;
					//outputfile.write((char*)ptr, (size_t)len);

					//udpsender->Write(ptr,len);
					std::cout << "capture dequeued, size: " << buf.m.planes[0].bytesused << "/" << buf.m.planes[0].length;
					

					std::cout << '[';
					if (buf.flags & V4L2_BUF_FLAG_MAPPED			) std::cout << " mapped";
					if (buf.flags & V4L2_BUF_FLAG_QUEUED			) std::cout << " queued";
					if (buf.flags & V4L2_BUF_FLAG_DONE			) std::cout << " done";
					if (buf.flags & V4L2_BUF_FLAG_KEYFRAME			) std::cout << " keyframe";
					if (buf.flags & V4L2_BUF_FLAG_PFRAME			) std::cout << " pframe";
					if (buf.flags & V4L2_BUF_FLAG_BFRAME			) std::cout << " bframe";
					if (buf.flags & V4L2_BUF_FLAG_ERROR			) std::cout << " error";
					if (buf.flags & V4L2_BUF_FLAG_IN_REQUEST		) std::cout << " in_request";
					if (buf.flags & V4L2_BUF_FLAG_TIMECODE			) std::cout << " timecode";
					if (buf.flags & V4L2_BUF_FLAG_M2M_HOLD_CAPTURE_BUF	) std::cout << " M2M_hold_capture_buf";
					if (buf.flags & V4L2_BUF_FLAG_PREPARED			) std::cout << " prepared";
					if (buf.flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE	) std::cout << " no_cache_inv";
					if (buf.flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN		) std::cout << " no_cache_clean";
					if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK		) std::cout << " timestamp_mask";
					if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN		) std::cout << " timestamp_unknown";
					if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC	) std::cout << " timestamp_monotonic";
					if (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_COPY		) std::cout << " timestamp_copy";
					if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK		) std::cout << " tstamp_src_mask";
					if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_EOF		) std::cout << " tstamp_src_eof";
					if (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_SOE		) std::cout << " tstamp_src_soe";

					std::cout << ']' <<std::endl;

					buf.m.planes[0].bytesused = buf.m.planes[0].length;

					// requeue
					if (cioctl(VIDIOC_QBUF, &buf) < 0) {
						throw std::runtime_error("encoder failed to queue capture buffer");
					}
					//int64_t timestamp_us = (buf.timestamp.tv_sec * (int64_t)1000000) + buf.timestamp.tv_usec;
					//OutputItem item = { buffers_[buf.index].mem,
					//					buf.m.planes[0].bytesused,
					//					buf.m.planes[0].length,
					//					buf.index,
					//					!!(buf.flags & V4L2_BUF_FLAG_KEYFRAME),
					//					timestamp_us };
				}
			}
			return outputdequeued;
		}

};

static void printhistogram(std::span<uint32_t> hist, int linecount = 30)
{
	//std::span b_hist(stats->hist->b_hist, bincount);
	int laa = hist.size() * linecount;
	std::cout << "\e[" << linecount << "A";
	int b=0;
	int l=0;
	
	auto maxvalue = *std::max_element(hist.begin(), hist.end());
	
	for(int i = 0;i<linecount;i++)
	{
		int acc = 0;
		while(l < hist.size()) {
			acc += hist[b++];
			l += linecount;
		}
		l -= hist.size();
		std::cout << acc <<  line(acc*20/maxvalue) << "\e[K" <<std::endl;
	}
}

static char fchars[256] = {
	/* 0 */ ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
	/* 1 */ ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
	/* 2 */ '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	/* 3 */ '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	/* 4 */ '^','^','^','^','^','^','^','^','^','^','^','^','^','^','^','^',
	/* 5 */ '^','^','^','^','^','^','^','^','^','^','^','^','^','^','^','^',
	/* 6 */ 'o','o','o','o','o','o','o','o','o','o','o','o','o','o','o','o',
	/* 7 */ 'o','o','o','o','o','o','o','o','o','o','o','o','o','o','o','o',
	/* 8 */ 'O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O',
	/* 9 */ 'O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O',
	/* A */ 'Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q',
	/* B */ 'Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q','Q',
	/* C */ 'W','W','W','W','W','W','W','W','W','W','W','W','W','W','W','W',
	/* D */ 'W','W','W','W','W','W','W','W','W','W','W','W','W','W','W','W',
	/* E */ '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
	/* F */ '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@'
};

static void printframe(uint8_t* frame, int width, int height, int stride)
{
	int vstep = 6;
	int hstep = 10;
	char line[256];
	auto lineptr = frame;
	int linecount = 0;

	for(int h = 0;h<height;h+=hstep){
		auto lp = lineptr;
		int k = 0;
		for(int v = 0;v<width;v+=vstep){
			line[k++]=fchars[lp[v*3]];
		}
		line[k]=0;
		std::cout << line<<std::endl;
		linecount++;
		lineptr+=stride*hstep;
	}
	std::cout << "\e[" << linecount << "A";
}

class eventmanager
{
		struct callbackinfo
		{
			short event;
			std::function<void()> callback;
		};
		std::vector<pollfd> pfd;
		std::vector<callbackinfo> callbacks;

	public:
		void registercallback(int fd, short pollevent, std::function<void()> fn)
		{
			pfd.push_back(pollfd{fd, pollevent, 0});
			callbacks.push_back(callbackinfo{pollevent, fn});
		}

		void Run()
		{
			int pr = poll(pfd.data(), pfd.size(), 300);
			if (pr>0) {
				for(int i = 0;i<pfd.size();i++)
				{
					if (pfd[i].revents & callbacks[i].event){
						callbacks[i].callback();
					}
				}
			} 
		}
};

void captureframe()
{
	vdev vid0("/dev/video0");
	vdev vid_isp_out("/dev/video13");
	vdev vid_isp_capture1("/dev/video14");
	vdev vid_isp_capture2("/dev/video15");
	vdev vid_isp_stat("/dev/video16");


	auto vid = vid0.getstream(V4L2_BUF_TYPE_VIDEO_CAPTURE);
	auto isp_out = vid_isp_out.getstream(V4L2_BUF_TYPE_VIDEO_OUTPUT);
	auto isp_cap1 = vid_isp_capture1.getstream(V4L2_BUF_TYPE_VIDEO_CAPTURE);
	auto isp_cap2 = vid_isp_capture2.getstream(V4L2_BUF_TYPE_VIDEO_CAPTURE);
	auto isp_stats = vid_isp_stat.getstream(V4L2_BUF_TYPE_META_CAPTURE);

	//vdev vid_cam("/dev/v4l-subdev0");
	//vid_cam.listcontrols();

	//vid->stop_streaming();
	isp_out->stop_streaming();
	isp_cap1->stop_streaming();
	isp_cap2->stop_streaming();
	isp_stats->stop_streaming();

	vid->Reset();
	isp_out->Reset();
	isp_cap1->Reset();
	isp_cap2->Reset();
	isp_stats->Reset();

	int width = 640;
	int height = 480;
	

//auto format = V4L2_PIX_FMT_SBGGR8;// 10 bit Bayer packed
auto format = V4L2_PIX_FMT_SGBRG10P;

	vid->setformat(width, height, format); 
	isp_out->setformat(width, height, format);

	//vid->setformat(width, height, V4L2_PIX_FMT_YUYV);
	//isp_out->setformat(width, height, V4L2_PIX_FMT_YUYV);

	isp_cap1->setformat(width, height, V4L2_PIX_FMT_YUV420);
	isp_cap2->setformat(width/2, height/2, V4L2_PIX_FMT_YUV420);
	isp_stats->confirmformat();
	
	h264encoder encoder("/dev/video11");
	encoder.output.stop_streaming();
	encoder.capture.stop_streaming();
	encoder.init(*isp_cap1);
	encoder.setbitrate(1000000);

	eventmanager em;
	em.registercallback(vid0.fd(), POLLIN, [&vid, &isp_out](){
		dmabuf vbuf;
		vid->dequeuedma(&vbuf);
		isp_out->queuedma(vbuf);
	});

	em.registercallback(vid_isp_out.fd(), POLLOUT, [&isp_out,&vid](){
		dmabuf vbuf;
		isp_out->dequeuedma(&vbuf);
		vid->queuedma(vbuf);
	});

	em.registercallback(vid_isp_capture1.fd(), POLLIN, [&isp_cap1,&encoder](){
		dmabuf vbuf;
		isp_cap1->dequeuedma(&vbuf);
		encoder.output.queuedma_plane(vbuf);
	});

	em.registercallback(vid_isp_capture2.fd(), POLLIN, [&isp_cap2](){
		dmabuf vbuf;
		isp_cap2->dequeuedma(&vbuf);
		isp_cap2->queuedma(vbuf);
		
	});
	em.registercallback(vid_isp_stat.fd(), POLLIN, [&isp_stats](){
		dmabuf vbuf;
		isp_stats->dequeuedma(&vbuf);
		isp_stats->queuedma(vbuf);
	});
	em.registercallback(encoder.fd(), POLLIN, [&encoder, &isp_cap1](){
		dmabuf vbuf;
		encoder.output.trydequeuedma_plane(&vbuf);
		isp_cap1->queuedma(vbuf);
		
		int bufindex;
		encoder.capture.trydequeue_mmap_plane(&bufindex,[](auto data){
			if (udpsender){
				udpsender->Write(data.data(), data.size_bytes());
			}
		});
	});

	encoder.output.RequestBuffers(12, V4L2_MEMORY_DMABUF);		
	encoder.capture.RequestBuffers(12, V4L2_MEMORY_MMAP);
	encoder.capture.FillQueueMMAP();

	vid->RequestBuffers(4, V4L2_MEMORY_DMABUF);
	vid->FillQueueDMA();

	isp_out->getformat();
	isp_out->RequestBuffers(4, V4L2_MEMORY_DMABUF);
	
	isp_cap1->RequestBuffers(4, V4L2_MEMORY_DMABUF);
	isp_cap1->FillQueueDMA();

	isp_cap2->RequestBuffers(4, V4L2_MEMORY_DMABUF);
	isp_cap2->FillQueueDMA();

	isp_stats->RequestBuffers(4, V4L2_MEMORY_DMABUF);
	isp_stats->FillQueueDMA();

	vid->start_streaming();
	isp_out->start_streaming();
	isp_cap1->start_streaming();
	isp_cap2->start_streaming();
	isp_stats->start_streaming();
	
	encoder.capture.start_streaming();
	encoder.output.start_streaming();
	
	std::cout << "go.. "<<std::endl;
	long prev = 0;
	for(;;) {
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		dmabuf vbuf;
		
		if (vid->trydequeuedma(POLLIN, &vbuf))
		{
			isp_out->queuedma(vbuf);
		}

		if (isp_out->trydequeuedma(POLLOUT, &vbuf)) {
			vid->queuedma(vbuf);
		}

		if (isp_cap1->trydequeuedma(POLLIN, &vbuf))
		{
			encoder.output.queuedma_plane(vbuf);
		}
		
		if (encoder.Poll(POLLIN)){
			if (encoder.output.trydequeuedma_plane(&vbuf)){
				isp_cap1->queuedma(vbuf);
			}
			int bufindex;
			encoder.capture.trydequeue_mmap_plane(&bufindex,[](auto data){
				udpsender->Write(data.data(), data.size_bytes());
			});
		}
		
		if (isp_cap2->trydequeuedma(POLLIN, &vbuf)){
			isp_cap2->queuedma(vbuf);
		}
		
		if (isp_stats->trydequeuedma(POLLIN, &vbuf)) {
			auto ptr = (uint8_t*)mappeddata[vbuf.fd];
			if (ptr){
				//auto stats = (bcm2835_isp_stats*)ptr;
				//printhistogram(std::span(stats->hist->b_hist, NUM_HISTOGRAM_BINS));
			}
			isp_stats->queuedma(vbuf);
		}
	}

	std::cout << "isp_capture stream off"<<std::endl;
	isp_cap1->stop_streaming();
	isp_cap2->stop_streaming();
	isp_stats->stop_streaming();

	std::cout << "isp_output stream off"<<std::endl;
	isp_out->stop_streaming();

	vid->stop_streaming();

	//close(fd);
}

#include <regex.h>
int main(int argc, const char *argv[])
{
	
	int portnr = 8805;
	std::string host = "192.168.1.181";
	if  (argc>1){
			std::string str = argv[1];
			auto pos = (int)str.find_first_of(":");
			host = str;
			if (pos>0){
				host = str.substr(0,pos);
				portnr = std::stoi(str.substr(pos+1));
				if (portnr<0) {
					std::cerr<<"bad port nr"<<std::endl;
				}
			}else{
				host=str;
			}
	}

	std::cout << "host: "<<host <<", port: "<<portnr<<std::endl;

	
	udpsender = std::make_unique<UDPSender>(host, portnr);

	captureframe();

	return 0;
}
