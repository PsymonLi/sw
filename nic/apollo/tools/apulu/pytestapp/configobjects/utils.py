import enum
import socket

def getnum2mac(mac):
    return ':'.join(format(s, '02x') for s in bytes.fromhex(hex(mac)[2:].zfill(12)))

def getmac2num(mac,reorder=False):
    by8 = mac.split(":")
    if len(by8) != 6:
       by8 = mac.split("-")
    if reorder:
       return (int(by8[5], 16) << 40) +\
           (int(by8[4], 16) << 32) +\
           (int(by8[3], 16) << 24) +\
           (int(by8[2], 16) << 16) +\
           (int(by8[1], 16) << 8) +\
           (int(by8[0], 16))
    else:
       return (int(by8[0], 16) << 40) +\
           (int(by8[1], 16) << 32) +\
           (int(by8[2], 16) << 24) +\
           (int(by8[3], 16) << 16) +\
           (int(by8[4], 16) << 8) +\
           (int(by8[5], 16))

PENSANDO_NIC_MAC = 0x022222111111
PDS_UUID_MAGIC_BYTE_VAL = 0x4242
PDS_UUID_BYTE_ORDER = "little"
PDS_NODE_UUID_BYTE_ORDER = "big"

PDS_UUID_LEN = 16
PDS_UUID_ID_LEN = 4
PDS_UUID_OBJTYPE_LEN = 2
PDS_UUID_RESERVED_LEN = 2
PDS_UUID_MAGIC_BYTE_LEN = 2
PDS_UUID_SYSTEM_MAC_LEN = 6

PDS_UUID_ID_OFFSET_START = 0
PDS_UUID_ID_OFFSET_END = PDS_UUID_ID_OFFSET_START + PDS_UUID_ID_LEN

PDS_UUID_OBJTYPE_OFFSET_START = PDS_UUID_ID_OFFSET_END
PDS_UUID_OBJTYPE_OFFSET_END = PDS_UUID_OBJTYPE_OFFSET_START + PDS_UUID_OBJTYPE_LEN

PDS_UUID_RESERVED_OFFSET_START = PDS_UUID_OBJTYPE_OFFSET_END
PDS_UUID_RESERVED_OFFSET_END = PDS_UUID_RESERVED_OFFSET_START + PDS_UUID_RESERVED_LEN

PDS_UUID_MAGIC_BYTE_OFFSET_START = PDS_UUID_RESERVED_OFFSET_END
PDS_UUID_MAGIC_BYTE_OFFSET_END = PDS_UUID_MAGIC_BYTE_OFFSET_START + PDS_UUID_MAGIC_BYTE_LEN

PDS_UUID_SYSTEM_MAC_OFFSET_START = PDS_UUID_MAGIC_BYTE_OFFSET_END

PDS_UUID_MAGIC_BYTE = PDS_UUID_MAGIC_BYTE_VAL.to_bytes(PDS_UUID_MAGIC_BYTE_LEN, PDS_UUID_BYTE_ORDER)
# only for SIM
PDS_UUID_SYSTEM_MAC = PENSANDO_NIC_MAC.to_bytes(PDS_UUID_SYSTEM_MAC_LEN, PDS_NODE_UUID_BYTE_ORDER)

class PdsUuid:

    def __init__(self, value, node_uuid=None, objtype=None):

        if node_uuid:
            node_uuid = int(node_uuid, 16)
            node_uuid = node_uuid.to_bytes(6, "big")

        if isinstance(value, int):
            self.Id = value
            self.Type = objtype
            self.Uuid = PdsUuid.GetUUIDfromId(self.Id, node_uuid, self.Type)
        elif isinstance(value, bytes):
            self.Uuid = value
            self.Id = PdsUuid.GetIdfromUUID(self.Uuid)
        elif isinstance(value, list):
            self.Uuid = bytes(value)
            self.Id = PdsUuid.GetIdfromUUID(self.Uuid)
        else:
            logger.error(f"{type(value)} is NOT supported for PdsUuid class")
            assert(0)
        self.UuidStr = PdsUuid.GetUuidString(self.Uuid)


    def __str__(self):
        return f"ID:{self.Id} UUID:{self.UuidStr}"

    def GetUuid(self):
        return self.Uuid

    def GetId(self):
        return self.Id

    @staticmethod
    def GetUuidString(uuid):
        uuid_id = PdsUuid.GetIdfromUUID(uuid)
        uuid_magic = int.from_bytes(uuid[PDS_UUID_MAGIC_BYTE_OFFSET_START:PDS_UUID_MAGIC_BYTE_OFFSET_END], PDS_UUID_BYTE_ORDER)
        uuid_mac = int.from_bytes(uuid[PDS_UUID_SYSTEM_MAC_OFFSET_START:], "big")
        uuidstr = hex(uuid_id) + "-" + hex(uuid_magic) + "-" + hex(uuid_mac)
        return uuidstr

    def GetUuidMacString(uuid):
        uuid_id = PdsUuid.GetIdfromUUID(uuid)
        uuid_mac = int.from_bytes(uuid[PDS_UUID_SYSTEM_MAC_OFFSET_START:], "big")
        uuid_mac_str = hex(uuid_mac)
        return uuid_mac_str

    @staticmethod
    def GetIdfromUUID(uuid):
        return int.from_bytes(uuid[PDS_UUID_ID_OFFSET_START:PDS_UUID_ID_OFFSET_END], PDS_UUID_BYTE_ORDER)

    @staticmethod
    def GetUUIDfromId(id, node_uuid=None, objtype=None):
        if not node_uuid:
            node_uuid = PDS_UUID_SYSTEM_MAC
        # uuid is of 16 bytes
        uuid = bytearray(PDS_UUID_LEN)
        # first 4 bytes ==> id
        uuid[PDS_UUID_ID_OFFSET_START:PDS_UUID_ID_OFFSET_END] = id.to_bytes(PDS_UUID_ID_LEN, PDS_UUID_BYTE_ORDER)
        # next 2 bytes ==> object type (except HAL created objects like lifs & ports)
        if objtype:
            uuid[PDS_UUID_OBJTYPE_OFFSET_START:PDS_UUID_OBJTYPE_OFFSET_END] = objtype.to_bytes(PDS_UUID_OBJTYPE_LEN, PDS_UUID_BYTE_ORDER)
        # next 2 bytes ==> reserved (0x0000)
        # next 2 bytes ==> magic byte (0x4242)
        uuid[PDS_UUID_MAGIC_BYTE_OFFSET_START:PDS_UUID_MAGIC_BYTE_OFFSET_END] = PDS_UUID_MAGIC_BYTE
        # next 6 bytes ==> system mac (0x022222111111)
        uuid[PDS_UUID_SYSTEM_MAC_OFFSET_START:] = node_uuid
        return bytes(uuid)

class InterfaceTypes(enum.IntEnum):
    NONE = 0
    ETH = 1
    ETH_PC = 2
    TUNNEL = 3
    UPLINK = 4
    UPLINKPC = 5
    L3 = 6
    LIF = 7
    HOST = 10

def PortToEthIfIdx(port):
    ifidx = InterfaceTypes.ETH << 28
    ifidx = ifidx | (1 << 24)
    ifidx = ifidx | (port << 16)
    ifidx = ifidx | 1
    return ifidx

def GetIPProtoByName(protoname):
    return socket.getprotobyname(protoname)

def LifIfIdx2HostIfIdx(ifidx):
    idx = ifidx & ~(0xf << 28)
    host_ifidx = InterfaceTypes.HOST << 28
    host_ifidx = host_ifidx | idx
    return host_ifidx
